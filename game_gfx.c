/**
 * @file        game_gfx.c
 * @brief       Tetris-like game's common functions
 * @author      (C) Peter Ivanov, 2013, 2014
 * 
 * Created:     2013-12-23 11:29:32
 * Last modify: 2014-01-15 08:42:10 ivanovp {Time-stamp}
 * Licence:     GPL
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_gfxPrimitives.h>

#include "game_common.h"
#include "game_gfx.h"

/* Block sprites */
SDL_Surface * blocks[MAX_BLOCK_TYPES];

SDL_Surface * loadImage(const char* filename)
{
  SDL_Surface* loadedImage = NULL;
  SDL_Surface* optimizedImage = NULL;

  printf("Loading %s...", filename);
  loadedImage = IMG_Load (filename);

  // Check if image loaded
  if (loadedImage != NULL)
  {
      // Create an optimized image
      optimizedImage = SDL_DisplayFormat (loadedImage);

      // Free the old image
      SDL_FreeSurface (loadedImage);

      if (optimizedImage != NULL)
      {
          // Map the color key
          Uint32 colorkey = SDL_MapRGB (optimizedImage->format, 0xFF, 0, 0xFF);

          // Set all pixels of color R 0xFF, G 0, B 0xFF to be transparent
          SDL_SetColorKey (optimizedImage, SDL_SRCCOLORKEY, colorkey);

          printf("Done\n");
      }
      else
      {
        printf("Error: cannot optimize image!\n");
      }
  }
  else
  {
    printf("Error: cannot load image!\n");
  }

  // Return the optimized image
  return optimizedImage;
}

void loadBlocks()
{
  int i;
  char filename[64];

  for ( i = 0; i < MAX_BLOCK_TYPES; i++)
  {
    snprintf(filename, sizeof(filename), BLOCK_PNG, i);
    blocks[i] = loadImage(filename);
  }
}

void freeBlocks()
{
  int i;

  for ( i = 0; i < MAX_BLOCK_TYPES; i++)
  {
    printf("Releasing block %i\r\n", i);
    SDL_FreeSurface(blocks[i]);
  }
}

void drawBlock (uint8_t x, uint8_t y, uint8_t shape)
{
  SDL_Rect offset;

  offset.x = x * BLOCK_SIZE_X_PX;
  offset.y = y * BLOCK_SIZE_Y_PX;

  // Blit the surface
  SDL_BlitSurface( blocks[shape], NULL, screen, &offset );
}

void printCommon (void)
{
    char s[64];

    gfx_line_draw (MAP_SIZE_X_PX + 1, 0,
                   MAP_SIZE_X_PX + 1, MAP_SIZE_Y_PX,
                   gfx_color_rgb (0xFF, 0xFF, 0xFF));
    sprintf (s, "Score: %i", game.score);
    gfx_font_print(TEXT_X(0), TEXT_YN(0), gameFontNormal, s);
    sprintf (s, "Level: %i", game.level);
    gfx_font_print(TEXT_X(0), TEXT_YN(1), gameFontNormal, s);
#if 0
    if (music_initted)
    {
        gfx_line_draw (MAP_SIZE_X_PX + 1, TEXT_YN(2),
                       screen->w - 1, TEXT_YN(2),
                       gfx_color_rgb (0xFF, 0xFF, 0xFF));
        if (!config.music_paused)
        {
            sprintf (s, "Volume: %i", config.volume);
        }
        else
        {
            sprintf (s, "** MUTED **");
        }
        gfx_font_print(TEXT_X(0), TEXT_YN(2), gameFontNormal, s);
        if (playlist_loaded)
        {
            gfx_font_print(TEXT_X(0), TEXT_YN(3), gameFontNormal, musicFileName);
//            strncpy (s, mod.songname, 20);
            s[20] = 0;
            gfx_font_print(TEXT_X(0), TEXT_YN(4) + 2, gameFontSmall, s);
//            sprintf (s, "Pos: %i/%i", mod.play_position + 1, mod.songlength);
            gfx_font_print(TEXT_X(0), TEXT_YN(5), gameFontNormal, s);
            sprintf (s, "File: %i/%i", playlist_file_pos, playlist_file_cntr);
            gfx_font_print(TEXT_X(0), TEXT_YN(6), gameFontNormal, s);
        }
    }
#endif
    if (GAME_IS_OVER())
    {
        gfx_font_print (TEXT_X_0, TEXT_YN(8), gameFontNormal, "** GAME **");
        gfx_font_print (TEXT_X_0, TEXT_YN(9), gameFontNormal, "** OVER **");
        gfx_font_print (TEXT_X_0, TEXT_YN(10), gameFontNormal, "Press Enter");
        gfx_font_print (TEXT_X_0, TEXT_YN(11), gameFontNormal, "to replay,");
        gfx_font_print (TEXT_X_0, TEXT_YN(12), gameFontNormal, "Escape to quit...");
    }
    else if (GAME_IS_PAUSED())
    {
        gfx_font_print(TEXT_X_0, TEXT_YN(8), gameFontNormal, "** PAUSED **");
        snprintf (s, sizeof (s), "Sometris v%i.%i.%i", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
        gfx_font_print(TEXT_X_0, TEXT_Y(11), gameFontSmall, s);
        gfx_font_print(TEXT_X_0, TEXT_Y(12), gameFontSmall, "Copyright (C)");
        gfx_font_print(TEXT_X_0, TEXT_Y(13), gameFontSmall, "Peter Ivanov,");
        gfx_font_print(TEXT_X_0, TEXT_Y(14), gameFontSmall, "2013-2016");
        gfx_font_print(TEXT_X_0, TEXT_Y(15), gameFontSmall, "ivanovp@gmail.com");
        gfx_font_print(TEXT_X_0, TEXT_Y(16), gameFontSmall, "http://dev.ivanov.eu");
//        gfx_font_print(TEXT_X_0, TEXT_Y(14), gameFontSmall, "Licence: GPLv2");
//        gfx_font_print(TEXT_X_0, TEXT_Y(15), gameFontSmall, "ABSOLUTELY");
//        gfx_font_print(TEXT_X_0, TEXT_Y(16), gameFontSmall, "NO WARRANTY!");
    }
    else
    {
#ifdef TEST_RANDOM
        /* DEBUG */
        uint8_t i, y = 4;

        for (i = 0; i < KEY_DELTA_SIZE; i += 4)
        {
            snprintf (s, sizeof (s), "%3i %3i %3i %3i",
                      key_delta[i], key_delta[i + 1], key_delta[i + 2],
                    key_delta[i + 3]);
            gfx_font_print (TEXT_X_0, TEXT_Y(y), gameFontSmall, s);
            y++;
        }
        snprintf (s, sizeof (s), "w:%i r:%i", key_delta_widx, key_delta_ridx);
        gfx_font_print (TEXT_X_0, TEXT_Y(y), gameFontSmall, s);
#endif
    }
    /* Small debug */
    snprintf (s, sizeof (s), "C:%i", game.figure_counter);
    gfx_font_print (TEXT_X_0,
                    (screen->h - FONT_SMALL_SIZE_Y_PX - 4),
                    gameFontSmall, s);
    snprintf (s, sizeof (s), "v%lu.%lu.%lu", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
    gfx_font_print_fromright ((screen->w - 4),
                              (screen->h - FONT_SMALL_SIZE_Y_PX - 4), gameFontSmall, s);
}

static void drawRecord (uint8_t aBlockType)
{
    uint8_t i;
    char s[32];

    gfx_font_print (0, TEXT_YN(0), gameFontNormal,
            "Player   Level Score");
    gfx_font_print (0, TEXT_YN(1), gameFontNormal,
            "--------------------");
    for (i = 0; i < MAX_RECORD_NUM; i++)
    {
        snprintf (s, sizeof (s), "%-8s %i   %7i",
                config.records[RECORD_TYPE(aBlockType)][i].player_name,
                config.records[RECORD_TYPE(aBlockType)][i].level,
                config.records[RECORD_TYPE(aBlockType)][i].score
                );
        gfx_font_print (0, TEXT_YN(i + 2), gameFontNormal, s);
    }
}

void drawGameScreen (void)
{
    // Restore background
    SDL_BlitSurface( background, NULL, screen, NULL );

    printCommon ();
    if (GAME_IS_PAUSED() || GAME_IS_OVER())
    {
        drawRecord (game.block_types);
    }
    else
    {
        drawMap ();
        drawFigure ();
    }

    SDL_Flip( screen );
}

/**
 * @brief blinkMap
 * FIXME A non-blocking method should be implemented!
 *
 * @param blinkNum Number of blinks.
 */
void blinkMap (uint8_t blinkNum)
{
    uint8_t i;
    uint8_t x, y;
    SDL_Event event;

    /* Blink same blocks */
    for (i = 0; i < blinkNum * 2 && gameRunning; i++)
    {
        key_task();
        if (gameRunning)
        {
          SDL_BlitSurface( background, NULL, screen, NULL );
          printCommon ();

          for (x = 0; x < MAP_SIZE_X; x++)
          {
            for (y = 0; y < MAP_SIZE_Y; y++)
            {
              if (!MAP_IS_SELECTED(x, y) || (i & 1))
              {
                drawBlock (x, y, MAP(x,y));
              }
            }
          }
          SDL_Flip( screen );
          SDL_Delay( 200 ); /* 200 ms */
        }
    }
}

/**
 * @brief drawMap
 * Draw game's map.
 */
void drawMap (void)
{
    uint8_t x, y;

    for (x = 0; x < MAP_SIZE_X; x++)
    {
        for (y = 0; y < MAP_SIZE_Y; y++)
        {
            drawBlock (x, y, MAP(x,y));
        }
    }
}

/**
 * @brief drawFigure
 * Draw the falling figure into map.
 */
void drawFigure (void)
{
    uint8_t i;

    if (game.figure_is_vertical)
    {
        /* VERTICAL, Fuggoleges */
        for (i = 0; i < FIGURE_SIZE; i++)
        {
            drawBlock (game.figure_x, game.figure_y + i, game.figure[i]);
        }
    }
    else
    {
        /* HORIZONTAL, Vizszintes */
        for (i = 0; i < FIGURE_SIZE; i++)
        {
            drawBlock (game.figure_x + i, game.figure_y, game.figure[i]);
        }
    }
}

/**
 * @brief clearFigure
 * Clear the figure from the map.
 * This is only useful if redrawing of screen is costly.
 */
void clearFigure (void)
{
    uint8_t i;

    if (game.figure_is_vertical)
    {
        /* VERTICAL, Fuggoleges */
        for (i = 0; i < FIGURE_SIZE; i++)
        {
            drawBlock (game.figure_x, game.figure_y + i, 0);
        }
    }
    else
    {
        /* HORIZONTAL, Vizszintes */
        for (i = 0; i < FIGURE_SIZE; i++)
        {
            drawBlock (game.figure_x + i, game.figure_y, 0);
        }
    }
}

/**
 * @brief shiftDownColumn
 * Remove one block from map and move down the remaining blocks of column.
 *
 * @param x0 Coordinate X of block that shall be removed.
 * @param y0 Coordinate Y of block that shall be removed.
 */
void shiftDownColumn (uint8_t x0, uint8_t y0)
{
    uint8_t y;

    for (y = y0; y > 0; y--)
    {
        uint8_t block = MAP(x0, y - 1);
        MAPW(x0, y) = block;
    }
    MAPW(x0, 0) = 0;
}

/**
 * Prints message in the center of screen.
 * Note: gameDisplay shall be initialized to use this function!
 *
 * @param aInfo Text to print.
 */
void drawInfoScreen (const char* aInfo)
{
    SDL_BlitSurface( background, NULL, screen, NULL );
    gfx_font_print_center (screen->h / 2, gameFontNormal, aInfo);
    SDL_Flip( screen );
}
