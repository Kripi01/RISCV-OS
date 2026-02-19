// DOCS:
// scause :
// https://five-embeddev.com/riscv-priv-isa-manual/Priv-v1.12/supervisor.html#sec:scause

// mcause :
// https://five-embeddev.com/riscv-priv-isa-manual/Priv-v1.12/machine.html#sec:mcause

// ecall :
// https://www.cs.cornell.edu/courses/cs3410/2019sp/schedule/slides/14-ecf-pre.pdf
// https://five-embeddev.com/riscv-user-isa-manual/latest-adoc/rv32.html#_environment_call_and_breakpoints

// CLINT:
// https://static.dev.sifive.com/FE310-G000.pdf (Chapitre 11 et plus
// particulièrement, 11.3)

// PLIC:
// https://courses.grainger.illinois.edu/ECE391/fa2025/docs/riscv-plic-1.0.0.pdf

// TODO: réduire les commentaires

#include "interrupt.h"
#include "cpu.h"
#include "keyboard.h"
#include "platform.h"
#include "process.h"
#include "screen.h"
#include "shell.h"
#include <stdint.h>
#include <stdio.h>

static volatile uint32_t time = 0;

// Renvoie le nombre de secondes depuis le lancement du programme.
inline uint32_t nbr_secondes() { return time / IT_FREQ; }

// Update et affiche le temps en haut à droite de l'écran au format [HH:MM:SS]
static void update_time() {
  time++;

  // On affiche le temps depuis le démarrage en haut à droite
  uint32_t n = nbr_secondes();
  char time_str[18];
  sprintf(time_str, "[%02u:%02u:%02u]", (n / 3600) % 100, (n / 60) % 60,
          n % 60);
  display_top_right(time_str, 10);
}

// Gère les interruptions du mode S. Fait un syscall pour ack l'IRQ
// du CLINT
// TODO: faire 2 fonctions pour le traitement des IRQ et des exceptions.
void s_trap_handler(uint64_t scause, uint64_t sie, uint64_t sip,
                    trap_ctxt_t *tc) {
  int64_t is_interrupt = (int64_t)scause < 0; // MSB à 1 ?
  if (is_interrupt) {
    if ((sie & sip) == 0) { // IRQ masquée (les exceptions ne lèvent pas sip)
      return;
    }

    uint64_t masked_scause = scause & 0x7FFFFFFFFFFFFFFF;
    switch (masked_scause) {
    case IRQ_S_SFT: {
      // TODO:
      break;
    }
    case IRQ_S_TMR: {
      // On arrive ici suite à une interrupt transmise par le mode M
      update_time();

      // Pour modifier CLINT_TIMER_CMP, il faut passer en mode M. On fait donc
      // un syscall avec ecall qui lance une exception. L'acquittement de l'IRQ
      // est faite par l'exception handler du mode M
      __asm__("ecall"); // trap vers mtvec i.e. exception.S (c'est un syscall)

      // On change de processus à chaque interruption timer.
      ordonnance();
      break;
    }
    case IRQ_S_EXT: {
      // Géré directement par le mode S (donc context 1) grâce au mideleg (cf
      // enter_smode.S)

      // On claim l'interruption PLIC en lisant le registre claim/complete.
      uint32_t interrupt_id = *(volatile uint32_t *)PLIC_IRQ_CLAIM(1);
      if (interrupt_id == 10) {
        // On gère l'interruption de l'UART.
        char c = getchar_uart();
        printf("%c", c);

        // On complète l'interruption en écrivant l'ID de l'interrupt dans le
        // regsitre claim/complete.
        *(volatile uint32_t *)PLIC_IRQ_CLAIM(1) = interrupt_id;
      } else {
        // Pour l'instant, la seule interruption du PLIC est l'UART
      }
      break;
    } break;
    }
    // TODO: The interrupt handler can check the local meip/seip/ueip bits
    // before exiting the handler, to allow more efficient service of other
    // interrupts without first restoring the interrupted context and taking
    // another interrupt trap
  }

  else { // Exceptions
    if (EXC_IS_PF(scause)) {
      printf("Segmentation fault (core not dumped)\n");
      fin_processus(); // Une segfault kill le processus actif
    } else if (scause == EXC_S_ENV_CALL_FROM_U) { // syscall
      uint64_t code = tc->a7;
      switch (code) {
      case CODE_UPUTC: {
        char c = (char)tc->a0;
        printf("%c", (char)c);
        break;
      }
      case CODE_UPUTS: {
        char *str = (char *)tc->a0;

        // Lors du traitement d'une interruption/exception, le mode S utilise la
        // page table du mode U. Cependant, il ne peut pas accèder aux données
        // user à cause de PTE_U. Pour le uputs, on active temporairement le bit
        // SUM pour lire correctement l'adresse vrituelle du str et l'afficher.
        enable_sum();
        printf("%s", str);
        disable_sum();
        break;
      }
      case CODE_UPRINTF: {
        char *fmt = (char *)tc->a0;
        enable_sum();
        // Si il y a moins que 7 arguments, alors les arguments superflus sont
        // ignorés par printf (c'est un warning mais pas une erreur).
        printf(fmt, tc->a1, tc->a2, tc->a3, tc->a4, tc->a5, tc->a6);
        disable_sum();
        break;
      }
      case CODE_UCREE_PROCESSUS: {
        // ucree_processus étant appelé par un processus user, l'adresse de la
        // fonction fournie est une adresse virtuelle (dans son adressage)
        int (*ps_code_va)() = (int (*)())tc->a0;
        char *ps_nom = (char *)tc->a1;

        // On doit autoriser le SUM pour lire correctement l'adresse virtuelle
        // du code du processus et du nom en mode S
        enable_sum();
        int64_t pid = cree_processus(ps_code_va, ps_nom);
        disable_sum();

        tc->a0 = pid; // on retourne le pid du processus créé
        break;
      }
      case CODE_UWAITPID: {
        int64_t pid = tc->a0;
        waitpid(pid);
        break;
      }
      case CODE_UEXIT: {
        fin_processus(); // On renvoie a0 qui est l'exit code (très souvent 0)
        break;
      }
      case CODE_UPS: {
        tc->a0 = ps();
        break;
      }
      case CODE_UGET_COMMAND: {
        char *va = (char *)tc->a0;
        char *c = get_command(va);
        tc->a0 = (uint64_t)c;
        break;
      }
      case CODE_UEXEC_COMMAND: {
        char *cmd_str = (char *)tc->a0;
        char *target_cmd_str = (char *)tc->a1;
        int (*target_fct)() = (int (*)())tc->a2;

        // on a potentiellement des adresses virtuelles donc bit SUM
        enable_sum();
        tc->a0 = exec_command(cmd_str, target_cmd_str, target_fct);
        disable_sum();
        break;
      }
      default:
        break;
      }

      return_ecall_s();
    }
  }
}

// Gère (ou masque) les interruptions du mode M.
void m_trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip) {
  if ((mie & mip) == 0) { // Interruption masquée
    return;
  }

  int64_t is_interrupt = (int64_t)mcause < 0; // MSB à 1 ?

  if (is_interrupt) {
    uint64_t masked_mcause = mcause & 0x7FFFFFFFFFFFFFFF;
    if (masked_mcause == IRQ_M_TMR) {
      // Les interruptions levées par le CLINT sont pour le mode machine
      // (mip.MTIP passe à 1). On les transmet simplement au mode superviseur en
      // modifiant le bit mip.STIP (on a le droit car on est en mode machine
      // dans cette fonction)

      // On masque les interruptions machines timer (mie.MTIE) jusqu'à l'ack
      // du mode S. Si on ne masquait pas, on repartirait direct en interruption
      // machine à la fin du traitement de l'IRQ car elle n'est pas ack dans
      // cette fonction
      __asm__("csrc mie, %0" ::"r"(MTIE));

      // WARNING: On ne peut pas écrire dans le registre sip (il est en lecture
      // seule, sauf les bits SSIP et USIP), il faut écrire dans le registre mip
      __asm__("csrs mip, %0" ::"r"(STIP)); // mip.STIP = sip.STIP = 1
      // NOTE: Le registre scause est modifié par le hardware (il écrit
      // Supervisor Timer Interrupt -> interrupt = 1,exception code = 5)
    }
    // Les autres interruptions ont été délguées au mode S (cf enter_smode.S)
  }

  else { // Exception
    if (mcause == EXC_M_ENV_CALL_FROM_S) {
      // Pour l'instant, le seul endroit où on utilise ecall est pour
      // claim une interruption timer.

      // NOTE: mip.MTIP est remis automatiquement à 0 lors de l'update de
      // CLINT_TIMER_CMP
      *(volatile uint64_t *)(CLINT_TIMER_CMP) =
          *(volatile uint64_t *)CLINT_TIMER + IT_TICS_REMAINING;

      // On acquitte l'interruption superviseur (on remet mip.STIP à 0)
      __asm__("csrc mip, %0" ::"r"(STIP));

      // On réautorise les interruptions machines timer (mie.MTIE)
      __asm__("csrs mie, %0" ::"r"(MTIE));

      return_ecall_m();
    }
    // Les page faults ont été délguées au mode S (cf enter_smode.S)
  }
}

// Initialise le vecteur d'interruption du mode S.
void init_traitant(void traitant()) {
  __asm__("csrw stvec, %0" ::"r"(traitant));
}

// Autorise les interruptions du timer (et initialise la première)
void enable_timer() {
  // On démasque l'interruption du timer en mode S
  // NOTE: Les interruptions globales sont encore masquées lors du setup (pour
  // éviter de partir en interruption à un moment critique) mais on les active
  // dès qu'on entre dans idle.
  __asm__("csrs sie, %0" ::"r"(1 << IRQ_S_TMR));
}
