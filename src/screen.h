#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "platform.h"
#include <stdint.h>

// Caractéristiques de l'écran
#define DISPLAY_NB_COL (DISPLAY_WIDTH / 8)
#define DISPLAY_NB_LIG (DISPLAY_HEIGHT / 8)
#define BYTES_PER_PIXEL 4
#define FONT_HEIGHT 8
#define FONT_WIDTH 8
#define BYTES_PER_LINE (DISPLAY_WIDTH * BYTES_PER_PIXEL * FONT_HEIGHT)

// Couleurs
#define RED 0x00ff0000
#define GREEN 0x0000ff00
#define BLUE 0x000000ff
#define WHITE 0x00ffffff
#define BLACK 0x00000000

void set_validated();
uint64_t init_ecran();
void pixel(uint32_t x, uint32_t y, uint32_t couleur);
void ecrit_car(uint32_t lig, uint32_t col, char c, uint32_t char_color,
               uint32_t bg_color);
int has_car(uint32_t lig, uint32_t col, uint32_t bg_color);
void supprime_car(uint32_t bg_color);
void _supprime_curseur();
void _place_curseur(uint32_t lig, uint32_t col);
void place_curseur(uint32_t lig, uint32_t col);
void efface_ecran();
void defilement();
void traite_car(char c);
void display_top_right(const char *s, int len);
void display_french_flag();

#endif // __SCREEN_H__
