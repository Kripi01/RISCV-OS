// DOCS:
// scause :
// https://five-embeddev.com/riscv-priv-isa-manual/Priv-v1.12/supervisor.html#sec:scause

// mcause :
// https://five-embeddev.com/riscv-priv-isa-manual/Priv-v1.12/machine.html#sec:mcause

// ecall :
// https://www.cs.cornell.edu/courses/cs3410/2019sp/schedule/slides/14-ecf-pre.pdf
// https://five-embeddev.com/riscv-user-isa-manual/latest-adoc/rv32.html#_environment_call_and_breakpoints
// Section 3.3.1:
// https://courses.grainger.illinois.edu/ece391/sp2026/docs/priv-isa-20240411.pdf

// CLINT:
// https://static.dev.sifive.com/FE310-G000.pdf (Chapitre 11 et plus
// particulièrement, 11.3)

// PLIC:
// https://courses.grainger.illinois.edu/ECE391/fa2025/docs/riscv-plic-1.0.0.pdf

#include "interrupt.h"
#include "cpu.h"
#include "keyboard.h"
#include "platform.h"
#include "process.h"
#include "ramfs.h"
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
// du CLINT. Cette fonction doit être exécutée en mode S
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
      // On arrive ici suite à une interruption transmise par le mode M
      update_time();

      // Pour modifier CLINT_TIMER_CMP, il faut passer en mode M => ecall.
      // L'acquittement de l'IRQ est faite par l'exception handler du mode M
      __asm__("ecall"); // trap vers mtvec car medeleg (cf. enter_smode)

      ordonnance(); // on change de processus à chaque interruption timer
      break;
    }
    case IRQ_S_EXT: { // interruptions du PLIC (cf. mideleg de enter_smode)
      // On claim l'interruption PLIC en lisant le registre claim/complete.
      uint32_t interrupt_id = *(volatile uint32_t *)PLIC_IRQ_CLAIM(1);
      if (interrupt_id == 10) { // interruption de l'UART
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
    } else if (scause == EXC_S_ENV_CALL_FROM_U) { // syscall user
      uint64_t code = tc->a7;

      switch (code) {
      case CODE_UPUTC: {
        char c = (char)tc->a0;
        printf("%c", (char)c);
        break;
      }
      case CODE_UPUTS: {
        char *str = (char *)tc->a0;
        // Le mode S doit accèder à une adresse virtuelle user => on active SUM
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
      case CODE_UFRANCE: {
        printf("\f");
        display_french_flag();
        break;
      }
      case CODE_UCREE_PROCESSUS: {
        int (*ps_code_va)(int, char **) = (int (*)(int, char **))tc->a0;
        int argc = (int)tc->a1;
        char **argv = (char **)tc->a2;

        // ucree_processus étant appelé par un processus user, l'adresse de la
        // fonction fournie est une adresse virtuelle, pareil pour nom. Donc SUM
        enable_sum();
        int64_t pid = cree_processus(ps_code_va, argc, argv);
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
        fin_processus();
        // On renvoie l'exit code, donc tc->a0 ne change pas
        break;
      }
      case CODE_UPS: {
        tc->a0 = ps();
        break;
      }
      case CODE_UDORS: {
        dors(tc->a0);
        break;
      }
      case CODE_UNBR_SECONDES: {
        tc->a0 = nbr_secondes();
        break;
      }
      case CODE_UMON_NOM: {
        tc->a0 = (uint64_t)mon_nom();
        break;
      }
      case CODE_UMON_PID: {
        tc->a0 = mon_pid();
        break;
      }
      case CODE_UGET_RAW_ARGV: {
        char *raw_argv = (char *)tc->a0;
        tc->a0 = (uint64_t)get_raw_argv(raw_argv);
        break;
      }
      case CODE_UEXEC_COMMAND: {
        // NOTE: argv est un tableau de pointeurs (=char *[]=char **)
        char **argv = (char **)tc->a0;
        char *target_cmd = (char *)tc->a1;
        uint64_t argc = tc->a2;
        int (*target_fct)(int, char **) = (int (*)(int, char **))tc->a3;

        // on a potentiellement des adresses virtuelles donc bit SUM
        enable_sum();
        tc->a0 = exec_command(argv, target_cmd, argc, target_fct);
        disable_sum();
        break;
      }
      case CODE_UMKDIR: {
        char *name = (char *)tc->a0;
        enable_sum(); // car name est potentiellement une va
        // WARNING: mkdir renvoie une pa donc non utilisable par un proc user
        tc->a0 = (uint64_t)mkdir(name);
        disable_sum();
        break;
      }
      case CODE_ULS: {
        ls();
        break;
      }
      case CODE_UPWD: {
        pwd();
        break;
      }
      case CODE_URM: {
        char *name = (char *)tc->a0;
        enable_sum();
        tc->a0 = rm(name);
        disable_sum();
        break;
      }
      case CODE_UCD: {
        char *path = (char *)tc->a0;
        enable_sum();
        tc->a0 = cd(path);
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

// Gère (ou masque) les interruptions/exceptions du mode M. Cette fonction doit
// être exécutée en mode M.
void m_trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip) {
  int64_t is_interrupt = (int64_t)mcause < 0; // MSB à 1 ?

  if (is_interrupt) {
    if ((mie & mip) == 0) { // Interruption masquée
      return;
    }

    uint64_t masked_mcause = mcause & 0x7FFFFFFFFFFFFFFF;
    if (masked_mcause == IRQ_M_TMR) {
      // Les interruptions levées par le CLINT sont pour le mode M (mip.MTIP =
      // 1). On les transmet simplement au mode S en activant le bit mip.STIP
      // sip est en lecture seule (sauf sip.SSIP/USIP), donc on écrit dans mip
      __asm__("csrs mip, %0" ::"r"(STIP));
      // NOTE: scause est modifié par le hardware -> scause = 0x80000005

      // On masque les interruptions machines timer (mie.MTIE) jusqu'à l'ack
      // du mode S car sinon, on repartirait direct en interruption machine à la
      // fin du traitement de l'IRQ car elle n'est pas ack dans cette fonction
      __asm__("csrc mie, %0" ::"r"(MTIE));
    }
    // Les autres interruptions ont été délguées au mode S (cf enter_smode.S)
  }

  else { // Exceptions
    if (mcause == EXC_M_ENV_CALL_FROM_S) {
      // Pour l'instant, le seul endroit où on utilise ecall en mode M est pour
      // claim une IRQ timer (donc on ne checke pas les arguments de ecall).

      // NOTE: mip.MTIP est remis automatiquement à 0 lors de l'update de
      // CLINT_TIMER_CMP
      *(volatile uint64_t *)(CLINT_TIMER_CMP) =
          *(volatile uint64_t *)CLINT_TIMER + IT_TICS_REMAINING;

      __asm__("csrc mip, %0" ::"r"(STIP)); // on acquitte l'IRQ superviseur
      __asm__("csrs mie, %0" ::"r"(MTIE)); // on réautorise les IRQ M timer
      return_ecall_m();
    }
    // Les page faults ont été délguées au mode S (cf enter_smode.S)
  }
}

// Initialise le vecteur d'interruption du mode S.
void init_traitant(void traitant()) {
  __asm__("csrw stvec, %0" ::"r"(traitant));
}

// Autorise les interruptions du timer du mode S
void enable_timer() {
  // NOTE: Les interruptions globales sont encore masquées lors du setup (pour
  // éviter de partir en interruption à un moment critique) mais on les active
  // dès qu'on entre dans idle.
  __asm__("csrs sie, %0" ::"r"(1 << IRQ_S_TMR));
}
