/**
 * @file        game_gfx.c
 * @brief       Header of tetris-like game's common functions
 * @author      (C) Peter Ivanov, 2013, 2014
 * 
 * Created:     2013-12-23 11:29:32
 * Last modify: 2014-01-15 09:21:58 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#ifndef INCLUDE_GAME_GFX_H
#define INCLUDE_GAME_GFX_H

#include "game_common.h"

#include <SDL/SDL.h>

#define DEFAULT_BG_COLOR        gfx_color_rgb (0x00, 0x00, 0x00)    /* Black */

#define MAP_SIZE_X_PX           (MAP_SIZE_X * BLOCK_SIZE_X_PX)
#define MAP_SIZE_Y_PX           (MAP_SIZE_Y * BLOCK_SIZE_Y_PX)

#define BLOCK_SIZE_X_PX         16
#define BLOCK_SIZE_Y_PX         16

#define FONT_SMALL_SIZE_X_PX    8
#define FONT_SMALL_SIZE_Y_PX    12

#define FONT_NORMAL_SIZE_X_PX   8
#define FONT_NORMAL_SIZE_Y_PX   12

#if MAP_SIZE_X_PX > 320
#error MAP_SIZE_X_PX greater than width of LCD!
#endif
#if MAP_SIZE_Y_PX > 240
#error MAP_SIZE_Y_PX greater than height of LCD!
#endif

#define TEXT_X_0        (MAP_SIZE_X_PX + 5)
#define TEXT_Y_0        (0)
#define TEXT_X(x)       (TEXT_X_0 + FONT_SMALL_SIZE_X_PX * (x))
#define TEXT_Y(y)       (TEXT_Y_0 + (FONT_SMALL_SIZE_Y_PX) * (y))
#define TEXT_XN(x)      (TEXT_X_0 + FONT_NORMAL_SIZE_X_PX * (x))
#define TEXT_YN(y)      (TEXT_Y_0 + (FONT_NORMAL_SIZE_Y_PX) * (y))

#define GFX_DIR                 "gfx/"
#define BACKGROUND_PNG          GFX_DIR "bg.png"
#define BLOCK_PNG               GFX_DIR "block%i.png"

#define gfx_color_rgb(r,g,b)                    ( ( r << 24 ) | ( g << 16 ) | ( b << 8 ) | 0xFF )
#define gfx_line_draw(x1, y1, x2, y2, color)    lineColor(screen, x1, y1, x2, y2, color)
#define gfx_font_print(x,y,font,s)              stringColor(screen, x, y, s, gfx_color_rgb(0xFF, 0xFF, 0xFF));
#define gfx_font_print_fromright(x,y,font,s)    stringColor(screen, x - strlen(s) * FONT_NORMAL_SIZE_X_PX, y, s, gfx_color_rgb(0xFF, 0xFF, 0xFF));
#define gfx_font_print_center(y, font, s)       stringColor(screen, screen->w / 2 - strlen(s) / 2 * FONT_NORMAL_SIZE_X_PX, y, s, gfx_color_rgb(0xFF, 0xFF, 0xFF))

extern SDL_Surface* background;
extern SDL_Surface* screen;

void loadBlocks();
void freeBlocks();
void drawBlock (uint8_t x, uint8_t y, uint8_t shape);
void printCommon (void);
void drawGameScreen (void);
void blinkMap (uint8_t blinkNum);
void drawMap (void);
void drawFigure (void);
void clearFigure (void);
void shiftDownColumn (uint8_t x0, uint8_t y0);
void drawInfoScreen (const char* aInfo);

#endif /* INCLUDE_GAME_GFX_H */
