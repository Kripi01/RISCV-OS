// TODO: Ajouter des volatiles

#include "font.h"
#include "platform.h"
#include <cpu.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ======================================================= */
/* =========== AFFICHAGE SUR LA CONSOLE (UART) =========== */
/* ======================================================= */

void init_uart() {
  // NOTE: UART_BAUD_RATE et UART_CLOCK_FREQ sont déjà correctement configurés
  // dans platform.h
  uint16_t div_freq = UART_CLOCK_FREQ / (16 * UART_BAUD_RATE);

  *(uint8_t *)(UART_BASE + UART_LCR) |= 0x80;

  *(uint8_t *)(UART_BASE + UART_DLL) = div_freq;      // Poids faible
  *(uint8_t *)(UART_BASE + UART_DLH) = div_freq >> 8; // Poids fort

  // On active les files d'attentes pour la réception et l'émission
  *(uint8_t *)(UART_BASE + UART_FCR) |= 0x1;

  // On sélectionne 8 bits pour la transmission
  *(uint8_t *)(UART_BASE + UART_LCR) |= 0x3;
  // On active le bit de parité
  *(uint8_t *)(UART_BASE + UART_LCR) |= 0x8;

  // On ne sélectionne plus le mode de configuration de la vitesse pour pouvoir
  // accèder au registre Transmitter Holding Register par la suite
  *(uint8_t *)(UART_BASE + UART_LCR) &= 0x7F;
}

void traite_car_uart(char c) {
  uint8_t *addr = (uint8_t *)(UART_BASE + UART_THR);
  *addr = c;
}

/* ======================================================= */
/* ================= AFFICHAGE À L'ÉCRAN ================= */
/* ======================================================= */

static volatile uint32_t (*const display_base)[DISPLAY_HEIGHT][DISPLAY_WIDTH] =
    (volatile uint32_t (*const)[DISPLAY_HEIGHT][DISPLAY_WIDTH])
        BOCHS_DISPLAY_BASE_ADDRESS;

// Couleurs
const uint32_t red = 0x00FF0000;
const uint32_t green = 0x0000FF00;
const uint32_t blue = 0x000000FF;
const uint32_t white = red | green | blue;
const uint32_t black = 0x00000000;

// Curseur (intialisé par défaut en haut à gauche de l'écran)
uint32_t cursor_lig = 1;
uint32_t cursor_col = 0;

// Écran
#define DISPLAY_NB_COL DISPLAY_WIDTH / 8
#define DISPLAY_NB_LIG DISPLAY_HEIGHT / 8

// Configure le système pour que la carte graphique soit opérationnelle (PCIe et
// écran configurés). Retourne 0 si tout c'est bien passé.
uint64_t init_ecran() {
  // Configuration du PCIe

  // On essaie tous les devices de 0 à 31 et on prend celui qui a le Device ID à
  // 0x1111 et le Vendor ID à 0x1234
  int found = 0;
  uint16_t *device_addr; // NOTE: On pointe sur des uint16_t car c'est la plus
                         //       petite taille que l'on utilise mais il
                         //       faudrait en théorie pointer sur des uint8_t
                         //       car certains champs du header sont sur 8 bits
  for (uint32_t device = 0; device < 32; device++) {
    // NOTE: il faut cast PCI_ECAM car les adresses sont sur 64 bits
    device_addr =
        (uint16_t *)((uint64_t)PCI_ECAM_BASE_ADDRESS + (device << 15));
    uint32_t id = *(device_addr + 1) << 16 | *device_addr;
    if (id == DISPLAY_PCI_ID) {
      found = 1;
      break;
    }
  }

  // Si on a pas trouvé la carte graphique
  if (!found) {
    return 1;
  }

  // On autorise les IO, les accès à la mémoire et l'écran à aller chercher ses
  // données en mémoire sans intervention du processeur
  *(device_addr + 2) = 0x3;

  // On spécifie l'adresse de base de la mémoire vidéo
  *(device_addr + 8) = BOCHS_DISPLAY_BASE_ADDRESS & 0xFFFF;
  *(device_addr + 9) = (BOCHS_DISPLAY_BASE_ADDRESS >> 16);
  // On spécifie l'adresse de base de la configuration
  *(device_addr + 12) = BOCHS_CONFIG_BASE_ADDRESS & 0xFFFF;
  *(device_addr + 13) = (BOCHS_CONFIG_BASE_ADDRESS >> 16);

  // Configuration de l'écran
  uint16_t *dispi_addr = (uint16_t *)(BOCHS_CONFIG_BASE_ADDRESS + 0x500);
  // On s'assure que l'écran est du bon type
  if ((*(dispi_addr + VBE_DISPI_INDEX_ID) & 0xFFF0) != VBE_DISPI_ID0) {
    return 2;
  }

  // On déconnecte l'écran pour pouvoir le configurer sans glitch
  *(dispi_addr + VBE_DISPI_INDEX_ENABLE) = 0;
  // Configuration
  *(dispi_addr + VBE_DISPI_INDEX_XRES) = DISPLAY_WIDTH;  // X Resolution (1024)
  *(dispi_addr + VBE_DISPI_INDEX_YRES) = DISPLAY_HEIGHT; // Y Resolution (768)
  *(dispi_addr + VBE_DISPI_INDEX_BPP) = DISPLAY_BPP;     // Bits par pixel (32)
  *(dispi_addr + VBE_DISPI_INDEX_BANK) = 0;              // Bank
  *(dispi_addr + VBE_DISPI_INDEX_VIRT_WIDTH) = DISPLAY_WIDTH;   // VWidth (1024)
  *(dispi_addr + VBE_DISPI_INDEX_VIRT_HEIGHT) = DISPLAY_HEIGHT; // VHeight (768)
  *(dispi_addr + VBE_DISPI_INDEX_X_OFFSET) = 0;                 // X Offset
  *(dispi_addr + VBE_DISPI_INDEX_Y_OFFSET) = 0;                 // Y Offset
  // On réactive l'écran: 0x40 -> frame buffer linéaire et, 1 pour l'activaion
  *(dispi_addr + VBE_DISPI_INDEX_ENABLE) =
      VBE_DISPI_LFB_ENABLED + VBE_DISPI_ENABLED;

  return 0;
}

// Fixe la couleur du pixel à l'écran de coordonnées x, y
// La couleur du pixel est décrite par les 3 octets des 32 bits (R -> 23 à 16, G
// -> 15 à 8, B -> 7 à 0)
void pixel(uint32_t x, uint32_t y, uint32_t couleur) {
  if (x < DISPLAY_WIDTH && y < DISPLAY_HEIGHT) {
    (*display_base)[y][x] = couleur;
  }
}

// Écrit le caractère c aux coordonnées (lignes et colonnes) et avec les
// couleurs spécifiées. Ne prend pas en charge les caractères de contrôle.
void ecrit_car(uint32_t lig, uint32_t col, char c, uint32_t char_color,
               uint32_t bg_color) {
  char *f = font8x8_basic[(uint8_t)c];
  for (int y = 0; y < 8; y++) {
    char line = f[y];
    for (int x = 0; x < 8; x++) {
      pixel(col * 8 + x, lig * 8 + y,
            ((line >> x) & 1) ? char_color : bg_color);
    }
  }
}

// On enlève le curseur de sa position actuelle
void _supprime_curseur() {
  uint32_t y = cursor_lig * 8 + 7;
  for (int x = 0; x < 8; x++) {
    // On inverse la couleur du curseur (sans toucher aux 2 premiers bits)
    uint32_t bg_color = (*display_base)[y][cursor_col * 8 + x] ^ 0x00FFFFFF;
    pixel(cursor_col * 8 + x, y, bg_color);
  }
}
// Déplace le curseur (sans suppression au préalable)
// Acutalise les variables globales cursor_col et cursor_lig
void _place_curseur(uint32_t lig, uint32_t col) {
  cursor_lig = lig;
  cursor_col = col;

  for (int x = 0; x < 8; x++) {
    // La couleur du curseur est l'opposé de la couleur du fond
    uint32_t new_x = cursor_col * 8 + x;
    uint32_t new_y = cursor_lig * 8 + 7;
    pixel(new_x, new_y, (*display_base)[new_y][new_x] ^ 0x00FFFFFF);
  }
}

// Déplace proprement le curseur aux coordonnées (lignes et colonnes) spécifiées
// Acutalise les variables globales cursor_col et cursor_lig
void place_curseur(uint32_t lig, uint32_t col) {
  _supprime_curseur();
  _place_curseur(lig, col);
}

// Efface l'écran en le remplissant de noir.
// Place également le curseur en haut à gauche de l'écran.
void efface_ecran() {
  memset((uint32_t *)BOCHS_DISPLAY_BASE_ADDRESS, black,
         DISPLAY_HEIGHT * DISPLAY_WIDTH * 4);
  // On ne supprime pas le curseur actuel car il l'a déjà été avec le memset
  _place_curseur(1, 0);
}

// Fait remonter d'une ligne l'affichage à l'écran
void defilement() {
  if (cursor_lig > 1) {
    uint64_t bytes_per_pixel = 4;
    uint64_t font_height = 8;
    uint64_t bytes_per_line = DISPLAY_WIDTH * bytes_per_pixel * font_height;

    // Attention, on ne déplace pas l'horloge (la ligne 0)
    memmove((uint32_t *)(BOCHS_DISPLAY_BASE_ADDRESS + bytes_per_line),
            (uint32_t *)(BOCHS_DISPLAY_BASE_ADDRESS + 2 * bytes_per_line),
            DISPLAY_WIDTH * (DISPLAY_HEIGHT - 2 * font_height) *
                bytes_per_pixel);

    // Il faut clear la dernière ligne
    memset((uint32_t *)(BOCHS_DISPLAY_BASE_ADDRESS +
                        (DISPLAY_HEIGHT - font_height) * DISPLAY_WIDTH *
                            bytes_per_pixel),
           black, bytes_per_line);

    // On actualise la position du curseur
    cursor_lig -= 1;
  }
}

// Traite un caractère donné i.e. l'affiche si c'est un caractère
// normal ou qui implante l'effet voulu si c'est un caractère de contrôle.
void traite_car(char c) {
  uint8_t uc = (uint8_t)c;
  // Caractère imprimable
  if (32 <= uc && uc <= 126) {
    // NOTE: On est obligé de découper la gestion du curseur pour ne pas écraser
    // le curseur avec le caractère (ce qui pose problème quand on veut
    // supprimer le curseur ensuite -> inversion des couleurs)
    _supprime_curseur();

    uint32_t new_lig = cursor_lig;
    uint32_t new_col = cursor_col + 1;
    // Si on est au bout de la ligne, alors on revient à la ligne.
    if (new_col >= DISPLAY_NB_COL) {
      new_col = 0;
      new_lig++;
    }
    ecrit_car(cursor_lig, cursor_col, uc, white, black);

    // Si on arrive au bas de l'écran, alors il faut remonter d'une ligne
    if (new_lig >= DISPLAY_NB_LIG) {
      defilement();
      new_lig = DISPLAY_NB_LIG - 1;
    }

    _place_curseur(new_lig, new_col);
  } else {
    switch (uc) {
    case 8: // \b (recule le curseur d'une colonne, si la colonne est > 0)
      if (cursor_col > 0) {
        place_curseur(cursor_lig, cursor_col - 1);
      }
      break;
    case 9: // \t (avance à la prochaine tabulation (colonnes 0, 8, 16, ...),
            // passe à la ligne suivante si nécessaire)
    {
      uint32_t new_lig = cursor_lig;
      uint32_t new_col = (cursor_col / 8) * 8 + 8;
      // Si on est au bout de la ligne, alors on revient à la ligne.
      if (new_col >= DISPLAY_NB_COL) {
        new_col = 0;
        new_lig += 1;
      }
      // Si on arrive au bas de l'écran, alors il faut remonter d'une ligne
      if (new_lig >= DISPLAY_NB_LIG) {
        defilement();
        new_lig = DISPLAY_NB_LIG - 1;
      }
      place_curseur(new_lig, new_col);
    } break;
    case 10: // \n (déplace le curseur sur la ligne suivante, colonne 0)
      // Si on arrive au bas de l'écran, alors il faut remonter d'une ligne
      if (cursor_lig + 1 >= DISPLAY_NB_LIG) {
        defilement();
      }
      place_curseur(cursor_lig + 1, 0);
      break;
    case 12: // \f (efface l'écran et place le curseur sur la colonne 0 de la
             // ligne 1)
      efface_ecran();
      break;
    case 13: // \r (déplace le curseur sur la ligne actuelle, colonne 0)
      place_curseur(cursor_lig, 0);
      break;
    default: // Pour les autres caractères, on ne fait rien
      break;
    }
  }
}

/* ======================================================= */
/* ==================== CONSOLE_PUTBYTES ================= */
/* ======================================================= */

void console_putbytes(const char *s, int len) {
  for (int i = 0; i < len; i++) {
    traite_car_uart(s[i]);
    traite_car(s[i]);
  }
}

/* ======================================================= */
/* ================= INTERRUPTIONS HORLOGES ============== */
/* ======================================================= */

#define IT_FREQ 20
#define IT_TICS_REMAINING TIMER_FREQ / IT_FREQ

static uint32_t time = 0;
extern void mon_traitant();

// Affiche le string s sur la ligne en haut à droite.
// WARNING: len (la taille de s) ne doit pas dépasser la taille d'une ligne car
// on ne gère pas les caractères de contrôle.
void display_top_right(const char *s, int len) {
  for (int i = 0; i < len; i++) {
    ecrit_car(0, DISPLAY_NB_COL - (len - i), s[i], white, black);
  }
}

// Renvoie le nombre de secondes depuis le lancement du programme.
uint32_t nbr_secondes() {
  // return *(uint64_t *)(CLINT_TIMER) / TIMER_FREQ;
  return time / IT_FREQ;
}

// Gère (ou masque) les interruptions.
void trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip) {
  // BUG: Les tests sur le serveur appellent trap_handler avec mcause=7, ce
  // n'est pas comme sur le diapo, le bit 63 n'est pas à 1.
  if (mcause >> 63 ||
      (mcause == 7)) { // C'est bien une interruption (et pas une exception)
    if ((mie & mip) == 0) { // Interruption masquée
      return;
    }

    uint64_t masked_mcause = mcause & 0x7FFFFFFFFFFFFFFF;
    if (masked_mcause == 3) { // Interruption software
      // TODO: Interruptions softwares
    } else if (masked_mcause == 7) { // Interruption timer
      time++;

      // On affiche le temps depuis le démarrage en haut à droite
      uint32_t n = nbr_secondes();
      char time[18];
      sprintf(time, "[%02u:%02u:%02u]", (n / 3600) % 100, (n / 60) % 60,
              n % 60);
      display_top_right(time, 10);

      // On acquitte l'IRQ et on réenclenche la prochaine interruption du timer.
      uint64_t next_timer_value = *(uint64_t *)CLINT_TIMER + IT_TICS_REMAINING;
      *(uint64_t *)(CLINT_TIMER_CMP) = next_timer_value;
    } else if (masked_mcause == 11) { // Interruption externe
      // TODO: Interruptions externes
    }
  } else {
    // TODO: Exceptions
  }
}

// Initialise le vecteur d'interruption.
void init_traitant(void traitant()) {
  __asm__("csrw mtvec, %0" ::"r"(traitant));
}

// Autorise les interruptions du timer (et initialise la première)
void enable_timer() {
  // On démasque l'interruption du timer (et globale)
  __asm__("csrs mie, %0" ::"r"(1 << IRQ_M_TMR));

  // Envoi de la première interruption par le timer
  uint64_t next_timer_value = *(uint64_t *)CLINT_TIMER + IT_TICS_REMAINING;
  *(uint64_t *)(CLINT_TIMER_CMP) = next_timer_value;
}

/* ======================================================= */
/* ===================== UTILITAIRES ===================== */
/* ======================================================= */

// Affiche le drapeau français sur tout l'écran
void display_french_flag() {
  for (int y = 0; y < DISPLAY_HEIGHT; y++) {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      if (x < DISPLAY_WIDTH / 3) {
        pixel(x, y, blue);
      } else if (x < 2 * DISPLAY_WIDTH / 3) {
        pixel(x, y, white);
      } else {
        pixel(x, y, red);
      }
    }
  }
}

void custom_sleep() {
  for (volatile int i = 0; i < 100000; i++) {
    // timeout
  }
}

void kernel_start() {
  init_uart();
  init_ecran();

  init_traitant(mon_traitant);
  enable_timer();
  enable_it();

  for (int i = 0; i < 100; i++) {
    printf("Bonjour\t");
  }

  /* on ne doit jamais sortir de kernel_start */
  while (1) {
    /* cette fonction arrete le processeur */
    hlt();
  }
}
