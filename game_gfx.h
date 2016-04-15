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

#define DEFAULT_BG_COLOR        gfx_color_rgb (0x00, 0x00, 0x00)    /* Black */
#define TRANSPARENT_BG_COLOR    gfx_color_rgb (0xFF, 0x00, 0xFF)    /* Magenta */

#define MAP_SIZE_X_PX           (MAP_SIZE_X * BLOCK_SIZE_X_PX)
#define MAP_SIZE_Y_PX           (MAP_SIZE_Y * BLOCK_SIZE_Y_PX)

#define BLOCK_SIZE_X_PX         16
#define BLOCK_SIZE_Y_PX         16

#if MAP_SIZE_X_PX > 320
#error MAP_SIZE_X_PX greater than width of LCD!
#endif
#if MAP_SIZE_Y_PX > 240
#error MAP_SIZE_Y_PX greater than height of LCD!
#endif

#define TEXT_X_0        (MAP_SIZE_X_PX + 5)
#define TEXT_Y_0        (0)
#define TEXT_X(x)       (TEXT_X_0 + gfx_font_width (gameFontSmall, "X") * (x))
#define TEXT_Y(y)       (TEXT_Y_0 + (gfx_font_height (gameFontSmall) + 4) * (y))
#define TEXT_XN(x)      (TEXT_X_0 + gfx_font_width (gameFontNormal, "X") * (x))
#define TEXT_YN(y)      (TEXT_Y_0 + (gfx_font_height (gameFontNormal)) * (y))

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
