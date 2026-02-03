#include "screen.h"
#include "font.h"
#include <string.h>

static volatile uint32_t (*const display_base)[DISPLAY_HEIGHT][DISPLAY_WIDTH] =
    (volatile uint32_t (*const)[DISPLAY_HEIGHT][DISPLAY_WIDTH])
        BOCHS_DISPLAY_BASE_ADDRESS;

// Curseur (intialisé par défaut en haut à gauche de l'écran)
static uint32_t cursor_lig = 1;
static uint32_t cursor_col = 0;

// Configure le système pour que la carte graphique soit opérationnelle (PCIe et
// écran configurés). Retourne 0 si tout c'est bien passé.
uint64_t init_ecran() {
  // Configuration du PCIe

  // On essaie tous les devices de 0 à 31 et on prend celui qui a le Device ID à
  // 0x1111 et le Vendor ID à 0x1234
  int found = 0;
  // NOTE: On pointe sur des uint16_t car c'est la plus petite taille que l'on
  // utilise mais il faudrait en théorie pointer sur des uint8_t car certains
  // champs du header sont sur 8 bits
  volatile uint16_t *device_addr;
  for (uint32_t device = 0; device < 32; device++) {
    // NOTE: il faut cast PCI_ECAM car les adresses sont sur 64 bits
    device_addr =
        (volatile uint16_t *)((uint64_t)PCI_ECAM_BASE_ADDRESS + (device << 15));
    volatile uint32_t id = *(device_addr + 1) << 16 | *device_addr;
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
  volatile uint16_t *dispi_addr =
      (volatile uint16_t *)(BOCHS_CONFIG_BASE_ADDRESS + 0x500);
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
// couleurs spécifiées. Ne prend pas en charge les caractères de contrôle et ne
// modifie pas le curseur (contrairement à traite_car)
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

// Supprime le caractère (si présent) aux coordonnées (lignes et colonnes) et
// avec la couleur d'arrière plane spécifiée. Ne prend pas en charge les
// caractères de contrôle et ne supprime pas le curseur. Ne remonte pas les
// lignes.
static void _supprime_car(uint32_t lig, uint32_t col, uint32_t bg_color) {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      pixel(col * 8 + x, lig * 8 + y, bg_color);
    }
  }
}

// Indique si la case aux coordonnées indiquées possède un caractère.
int has_car(uint32_t lig, uint32_t col, uint32_t bg_color) {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if ((*display_base)[y + lig * 8][x + col * 8] != bg_color) {
        return 1;
      }
    }
  }
  return 0;
}

// Supprime le caractère (si présent) juste avant le curseur. Ne supprime pas le
// curseur. Gère la suppression sur plusieurs lignes.
void supprime_car(uint32_t bg_color) {
  if (cursor_col > 0) {
    uint32_t new_col = cursor_col - 1;
    _supprime_car(cursor_lig, new_col, bg_color);
    place_curseur(cursor_lig, new_col);
  } else if (cursor_lig > 1) { // On ne veut pas supprimer la première ligne
    uint32_t new_lig = cursor_lig - 1;
    // On déplace le curseur sur le slot suivant le dernier caractère sur la
    // ligne du dessus. Si aucun caractère n'est présent sur la ligne du dessus
    // alors le curseur va au début de la ligne.
    int x = DISPLAY_NB_COL - 1;
    while (x >= 0) {
      if (has_car(new_lig, x, bg_color) == 1) {
        break;
      }
      x--;
    }
    _supprime_car(new_lig, x + 1, bg_color);
    place_curseur(new_lig, x + 1);
  }
}

// On enlève le curseur de sa position actuelle
static void _supprime_curseur() {
  uint32_t y = cursor_lig * 8 + 7;
  for (int x = 0; x < 8; x++) {
    // On inverse la couleur du curseur (sans toucher aux 2 premiers bits)
    uint32_t bg_color = (*display_base)[y][cursor_col * 8 + x] ^ 0x00FFFFFF;
    pixel(cursor_col * 8 + x, y, bg_color);
  }
}
// Déplace le curseur (sans suppression au préalable)
// Acutalise les variables globales cursor_col et cursor_lig
static void _place_curseur(uint32_t lig, uint32_t col) {
  cursor_lig = lig;
  cursor_col = col;

  for (int x = 0; x < 8; x++) {
    // La couleur du curseur est l'opposé de la couleur du fond
    uint32_t new_x = cursor_col * 8 + x;
    uint32_t new_y = cursor_lig * 8 + 7;
    pixel(new_x, new_y, (*display_base)[new_y][new_x] ^ 0x00FFFFFF);
  }
}

// Déplace proprement le curseur aux coordonnées (lignes et colonnes)
// spécifiées. Acutalise les variables globales cursor_col et cursor_lig
void place_curseur(uint32_t lig, uint32_t col) {
  _supprime_curseur();
  _place_curseur(lig, col);
}

// Efface l'écran en le remplissant de noir.
// Place également le curseur en haut à gauche de l'écran.
void efface_ecran() {
  memset((uint32_t *)BOCHS_DISPLAY_BASE_ADDRESS, BLACK,
         DISPLAY_HEIGHT * DISPLAY_WIDTH * 4);
  // On ne supprime pas le curseur actuel car il l'a déjà été avec le memset
  _place_curseur(1, 0);
}

// Fait remonter d'une ligne l'affichage à l'écran
void defilement() {
  if (cursor_lig > 1) {
    // Attention, on ne déplace pas l'horloge (la ligne 0)
    memmove((uint32_t *)(BOCHS_DISPLAY_BASE_ADDRESS + BYTES_PER_LINE),
            (uint32_t *)(BOCHS_DISPLAY_BASE_ADDRESS + 2 * BYTES_PER_LINE),
            DISPLAY_WIDTH * (DISPLAY_HEIGHT - 2 * FONT_HEIGHT) *
                BYTES_PER_PIXEL);

    // Il faut clear la dernière ligne
    memset((uint32_t *)(BOCHS_DISPLAY_BASE_ADDRESS +
                        (DISPLAY_HEIGHT - FONT_HEIGHT) * DISPLAY_WIDTH *
                            BYTES_PER_PIXEL),
           BLACK, BYTES_PER_LINE);

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
    // NOTE: On est obligé de découper la gestion du curseur pour ne pas
    // écraser le curseur avec le caractère (ce qui pose problème quand on
    // veut supprimer le curseur ensuite -> inversion des couleurs)
    _supprime_curseur();

    uint32_t new_lig = cursor_lig;
    uint32_t new_col = cursor_col + 1;
    // Si on est au bout de la ligne, alors on revient à la ligne.
    if (new_col >= DISPLAY_NB_COL) {
      new_col = 0;
      new_lig++;
    }
    ecrit_car(cursor_lig, cursor_col, uc, WHITE, BLACK);

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
    case 127: // SUPPR supprime le caractère courant (et déplace le curseur d'un
              // cran en arrière)
      supprime_car(BLACK);
      break;
    default: // Pour les autres caractères, on ne fait rien
      break;
    }
  }
}

// Affiche le string s sur la ligne en haut à droite.
// WARNING: len (la taille de s) ne doit pas dépasser la taille d'une ligne
// car on ne gère pas les caractères de contrôle.
void display_top_right(const char *s, int len) {
  for (int i = 0; i < len; i++) {
    ecrit_car(0, DISPLAY_NB_COL - (len - i), s[i], WHITE, BLACK);
  }
}
