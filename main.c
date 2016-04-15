/**
 * @file        main.c
 * @brief       Sometris main
 * @author      Copyright (C) Peter Ivanov, 2013, 2014
 *
 * Created      2013-12-30 11:48:53
 * Last modify: 2016-04-14 14:57:44 ivanovp {Time-stamp}
 * Licence:     GPL
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_timer.h>
#include <SDL/SDL_ttf.h>

#include "game_common.h"
#include "game_gfx.h"

#define BLOCKS_SPRITE_FILENAME  "stblocks.spt"
#define CONFIG_FILENAME         "stconfig.bin"
#define GAME_FILENAME           "stgame.bin"    /**< Saved game */
#define PLAYLIST_FILENAME       "playlist.txt"  /**< Path of your music collection (MOD/S3M/XM) */
#define PLAYLIST_MEM_LIMIT      (32768)         /**< 32 KiB */
#define DEFAULT_MUSIC_FILENAME  "music.mod"
#define SOUND_FREQ_HZ           44100
#define SOUND_CHANNELS          1       /* 1: mono, 2: stereo, FIXME stereo decoding not OK! */
#define OS_DIR_SEP_CHR          '\\'
#define OS_DIR_SEP_STR          "\\"

typedef struct linkedListElement_tag
{
    void* item;                            /* Data to store in the element */
    struct linkedListElement_tag* next;    /* Next element, NULL if it is the last one. */
    struct linkedListElement_tag* prev;    /* Previous element, NULL if it is the first one. */
    uint16_t pos;
} linkedListElement_t;

const char* info[] =
{
    "This is sometris, a tetris-like game.",
    "Copyright (C) Peter Ivanov <ivanovp@gmail.com>, 2013",
    "This program comes with ABSOLUTELY NO WARRANTY;",
    "for details see COPYING.",
    "This is free software, and you are welcome to",
    "redistribute it under certain conditions; see COPYING",
    "for details.",
    "During play you can use these buttons:",
    "START: Pause game",
    "SELECT+START: Exit game",
    "Left/right/down: Move",
    "A: Rotate",
    "Now you can select number of block types with up/down.",
    "Press 'A' to start game."
};

/* Game related */
bool_t       gameRunning   = TRUE;
main_state_machine_t main_state_machine = STATE_undefined;

/* Random related */
uint8_t      key_delta[KEY_DELTA_SIZE];
uint16_t     key_delta_widx = 0;
uint16_t     key_delta_ridx = 0;

bool_t       can_load_game = FALSE;

/* Music playing */
bool_t       music_initted = FALSE;
bool_t       playlist_loaded = FALSE;
linkedListElement_t* playlist_first = NULL;     /**< First MOD file name */
linkedListElement_t* playlist_last = NULL;      /**< Last MOD file name */
linkedListElement_t* playlist_actual = NULL;    /**< Actual file name in playlist which is played */
linkedListElement_t** playlist_it = NULL;       /**< Playlist iterator. Used to build playlist. */
linkedListElement_t* playlist_it_prev = NULL;   /**< Playlist previous element. Used to build playlist. */
uint32_t     playlist_file_cntr = 0;            /**< Number of files in the playlist */
uint32_t     playlist_file_pos = 0;             /**< Current file's number. */
size_t       playlist_mem_size = 0; /**< Allocated memory for playlist. */
char         musicFileName[16];
char         musicFilePath[FSYS_FILENAME_MAX];
bool_t       playEnd = FALSE;

/* Default configuration, could be overwritten by loadConfig() */
config_t config =
{
    .version = 6,
    .volume = (VOLUME_MAX - VOLUME_MIN) / 2,
    .music_paused = FALSE,
    .game_counter = 0,
    .player_idx = 0,
    .music_file_path = { 0 }
};

SDL_Event event;

/**
 * Set up default configuration and load if configuration file exists.
 */
void loadConfig (void)
{
    uint16_t i, j;
    FILE* configFile;
    config_t configTemp;

    drawInfoScreen ("Loading configuration...");

    /* Fill records with default data */
    for (j = 0; j < RECORD_TYPES; j++)
    {
        for (i = 0; i < MAX_RECORD_NUM; i++)
        {
            strcpy (config.records[j][i].player_name, "Valaki");
            config.records[j][i].score = MAX_RECORD_NUM - i;
        }
    }
    /* All user names are empty by default */
    for (i = 0; i < MAX_PLAYERS; i++)
    {
        config.player_names[i][0] = 0;
    }
    configFile = fopen (CONFIG_FILENAME, "rb");
    if (configFile)
    {
        size_t size;
        size = fread (&configTemp, 1, sizeof (configTemp), configFile);
        if (size == sizeof (configTemp) && configTemp.version == config.version)
        {
            /* Configuration is OK, copy it */
            memcpy (&config, &configTemp, sizeof (config));
        }
        fclose (configFile);
    }
}

/**
 * Save current configuration.
 */
bool_t saveConfig (void)
{
    bool_t ok = FALSE;
    FILE* configFile;

    drawInfoScreen ("Saving configuration...");

    strncpy (config.music_file_path, musicFilePath, sizeof (config.music_file_path));
    configFile = fopen (CONFIG_FILENAME, "wb");
    if (configFile)
    {
        size_t size;
        size = fwrite (&config, 1, sizeof (config), configFile);
        if (size == sizeof (config))
        {
            ok = TRUE;
        }
        fclose (configFile);
    }

    return ok;
}

/**
 * Check whether valid saved game exists.
 */
bool_t canLoadGame (void)
{
    FILE* gameFile;
    bool_t canLoad = FALSE;
    uint8_t version;

    gameFile = fopen (GAME_FILENAME, "rb");
    if (gameFile)
    {
        size_t size;
        size = fread (&version, 1, sizeof (version), gameFile);
        if (size == sizeof (version) && version == game.version)
        {
            canLoad = TRUE;
        }
        fclose (gameFile);
    }

    return canLoad;
}

/**
 * Load saved game.
 */
void loadGame (void)
{
    FILE* gameFile;
    game_t gameTemp;

    drawInfoScreen ("Loading game...");

    gameFile = fopen (GAME_FILENAME, "rb");
    if (gameFile)
    {
        size_t size;
        size = fread (&gameTemp, 1, sizeof (gameTemp), gameFile);
        if (size == sizeof (gameTemp) && gameTemp.version == game.version)
        {
            /* Configuration is OK, copy it */
            memcpy (&game, &gameTemp, sizeof (game));
        }
        fclose (gameFile);
    }
}

/**
 * Save actual game.
 */
bool_t saveGame (void)
{
    bool_t ok = FALSE;
    FILE* gameFile;

    drawInfoScreen ("Saving game...");

    gameFile = fopen (GAME_FILENAME, "wb");
    if (gameFile)
    {
        size_t size;
        size = fwrite (&game, 1, sizeof (game), gameFile);
        if (size == sizeof (game))
        {
            ok = TRUE;
        }
        fclose (gameFile);
    }

    return ok;
}

/**
 * Delete saved game.
 */
void deleteGame (void)
{
    remove (GAME_FILENAME);
    can_load_game = FALSE;
}

/**
 * Check if current score should be recorded.
 *
 * @return 0xFF is current score is not to be recorded. Otherwise: record position.
 */
uint8_t getNewRecordPos (uint8_t aBlockType, uint8_t aScore)
{
    uint8_t i;
    uint8_t pos = 0xFF;

    for (i = 0; i < MAX_RECORD_NUM; i++)
    {
        if (aScore > config.records[RECORD_TYPE(aBlockType)][i].score)
        {
            pos = i;
            break;
        }
    }

    return pos;
}

/**
 * Put record into right position. Configuration shall be saved.
 * @see saveConfig()
 */
void insertRecord (uint8_t aBlockType, uint8_t aScore)
{
    uint8_t i;
    uint8_t pos;

    pos = getNewRecordPos (aBlockType, aScore);

    if (pos != 0xFF)
    {
        /* Shift down */
        for (i = MAX_RECORD_NUM - 1; i > pos; i--)
        {
            memcpy (&config.records[RECORD_TYPE(aBlockType)][i],
                    &config.records[RECORD_TYPE(aBlockType)][i - 1], sizeof (record_t));
        }
        /* Insert new record */
        strncpy (config.records[RECORD_TYPE(aBlockType)][pos].player_name,
                 config.player_names[config.player_idx], PLAYER_NAME_LENGTH);
        config.records[RECORD_TYPE(aBlockType)][pos].level = game.level;
        config.records[RECORD_TYPE(aBlockType)][pos].score = game.score;
    }
}

#if 0
/**
 * @brief getFileName
 * Return filename from full path.
 * @param[out]  aFileName       Filename from full path. Example: "DATA.DAT"
 * @param[in]   aPath           Full path. Example: "A:\\GAME\\\DATA.DAT"
 * @param[in]   aFileNameLength Size of aFileName buffer.
 * @return
 */
char* getFileName (char* aFileName, const char* aPath, size_t aFileNameLength)
{
    char *pos;

    pos = strrchr (aPath, OS_DIR_SEP_CHR);
    if (pos)
    {
        ++pos;
        strncpy (aFileName, pos, aFileNameLength);
    }
    else
    {
        /* Directory separator (\) not found */
        strncpy (aFileName, aPath, aFileNameLength);
    }

    return aFileName;
}

/**
 * Allocate memory for a new element of linked list.
 *
 * @param[in] aItem Linked list item to store.
 * @param[in] aPrev Previous linked list item.
 * @param[in] aPos Number of linked list item.
 */
linkedListElement_t* addElement (void* aItem, linkedListElement_t* aPrev, uint16_t aPos)
{
    linkedListElement_t* linkedList = NULL;

    linkedList = malloc (sizeof (linkedListElement_t));
    if (linkedList)
    {
        linkedList->item = aItem;
        linkedList->prev = aPrev;
        linkedList->pos = aPos;
    }

    return linkedList;
}

/**
 * @brief freeElement
 * Releases memory of a linked list's item and linked list element too.
 *
 * @param aLinkedList
 * @return
 */
linkedListElement_t* freeElement (linkedListElement_t* aLinkedList)
{
    linkedListElement_t* next = NULL;

    if (aLinkedList)
    {
        next = aLinkedList->next;
        if (aLinkedList->item)
        {
            free (aLinkedList->item);
        }
        free (aLinkedList);
    }

    return next;
}

/**
 * @brief freeLinkedList
 * @param aFirstLinkedList
 */
void freeLinkedList (linkedListElement_t* aFirstLinkedList)
{
    linkedListElement_t* linkedList = aFirstLinkedList;
    bool_t end = FALSE;
    bool_t first = TRUE;

    while (linkedList && !end)
    {
        if (!first && linkedList == aFirstLinkedList)
        {
            end = TRUE;
        }
        linkedList = freeElement (linkedList);
        first = FALSE;
    }
}

/**
 * @brief getPrevMusicFile
 * @param[out] aTurnOver The playlist's beginning was reached.
 * @return Previous music file's name.
 */
char* getPrevMusicFile (bool_t* aTurnOver)
{
    if (aTurnOver)
    {
        *aTurnOver = FALSE;
    }
    if (playlist_loaded)
    {
        if (playlist_actual && playlist_actual->prev)
        {
            if (playlist_actual->prev == playlist_last && aTurnOver)
            {
                *aTurnOver = TRUE;
            }
            playlist_actual = playlist_actual->prev;
            if (playlist_actual->item)
            {
                playlist_file_pos = playlist_actual->pos;
                strncpy (musicFilePath, (char*) playlist_actual->item, sizeof (musicFilePath));
            }
        }
    }
    else
    {
        strncpy (musicFilePath, DEFAULT_MUSIC_FILENAME, sizeof (musicFilePath));
    }
    /* Get filename */
    getFileName (musicFileName, musicFilePath, sizeof (musicFileName));

    return musicFilePath;
}

/**
 * @brief getNextMusicFile
 * @param[out] aTurnOver The playlist's end was reached.
 * @return Next music file's name.
 */
char* getNextMusicFile (bool_t* aTurnOver)
{
    if (aTurnOver)
    {
        *aTurnOver = FALSE;
    }
    if (playlist_loaded)
    {
        if (playlist_actual && playlist_actual->next)
        {
            if (playlist_actual->next == playlist_first && aTurnOver)
            {
                *aTurnOver = TRUE;
            }
            playlist_actual = playlist_actual->next;
            if (playlist_actual->item)
            {
                playlist_file_pos = playlist_actual->pos;
                strncpy (musicFilePath, (char*) playlist_actual->item, sizeof (musicFilePath));
            }
        }
    }
    else
    {
        strncpy (musicFilePath, DEFAULT_MUSIC_FILENAME, sizeof (musicFilePath));
    }
    /* Get filename */
    getFileName (musicFileName, musicFilePath, sizeof (musicFileName));

    return musicFilePath;
}


/**
 * @brief searchFiles
 * Search files in directory and add to playlist.
 *
 * @param aPath         Path to search. Last character must be '\\'!
 * @param aExtension    Extensions to search. NULL means search all.
 * @param aRecursive    TRUE: search in subdirectories too!
 *
 * @return Number of files found.
 */
uint16_t searchFiles (const char* aPath, const char* aExtension, bool_t aRecursive)
{
    int ret = 0;
    uint16_t file_cntr = 0;
    char path[FSYS_FILENAME_MAX];
    fsys_file_info_t file_info __attribute__ ((aligned (4))); /* FIXME why aligning is needed? */
    bool_t mem_alloc_ok = TRUE;

    if (aRecursive)
    {
        /* First search directories and enter them */
        strncpy (path, aPath, sizeof (path));
        strncat (path, "*", sizeof (path));
        ret = fsys_findfirst (path, FSYS_ATTR_DIR, &file_info);
        if (!ret)
        {
            do
            {
                strncpy (path, aPath, sizeof (path));
                strncat (path, file_info.name, sizeof (path));
                strncat (path, OS_DIR_SEP_STR, sizeof (path));
                file_cntr += searchFiles (path, aExtension, aRecursive);
            } while ((fsys_findnext (&file_info) == 0));
            fsys_findclose (&file_info);
        }
    }

    /* Search files */
    strncpy (path, aPath, sizeof (path));
    if (!aExtension)
    {
        strncat (path, "*", sizeof (path));
    }
    else
    {
        strncat (path, aExtension, sizeof (path));
    }

    ret = fsys_findfirst (path, FSYS_ATTR_FILE, &file_info);
    if (!ret)
    {
        do
        {
            char* filename;
            size_t filename_size = strlen (aPath) + strlen (file_info.name) + 1;
            if (playlist_mem_size + filename_size < PLAYLIST_MEM_LIMIT)
            {
                filename = malloc (filename_size);
                if (filename)
                {
                    playlist_mem_size += filename_size;
                    strncpy (filename, aPath, filename_size);
                    strncat (filename, file_info.name, filename_size);
                    (*playlist_it) = addElement (filename, playlist_it_prev, playlist_file_cntr + 1);
                    if (*playlist_it)
                    {
                        playlist_mem_size += sizeof (linkedListElement_t);
                        playlist_last = (*playlist_it);
                        playlist_it_prev = *playlist_it;
                        playlist_it = &((*playlist_it)->next);
                        file_cntr++;
                        playlist_file_cntr++;
                        if ((playlist_file_cntr % 50) == 0)
                        {
                            char s[32];
                            gfx_render_target_clear (DEFAULT_BG_COLOR);
                            gfx_font_print_center (TEXT_YN(5), gameFontNormal, "Scanning:");
                            gfx_font_print_center (TEXT_YN(6), gameFontNormal, aPath);
                            snprintf (s, sizeof (s), "%i files found.", playlist_file_cntr);
                            gfx_font_print_center (TEXT_YN(7), gameFontNormal, s);
                            display_flip (gameDisplay);
                        }
                    }
                    else
                    {
                        mem_alloc_ok = FALSE;
                    }
                }
                else
                {
                    mem_alloc_ok = FALSE;
                }
            }
            else
            {
                mem_alloc_ok = FALSE;
            }
        } while ((fsys_findnext (&file_info) == 0) && mem_alloc_ok);
        fsys_findclose (&file_info);
    }
    else
    {
        /* No files found */
    }

    return file_cntr;
}

/**
 * @brief initPlaylist
 * Create playlist from playlist.txt and search previously listened music.
 */
void initPlaylist (void)
{
    FILE* playListFile;
    uint16_t file_cntr = 0;
    char line[FSYS_FILENAME_MAX + 1] = { 0 };

    playlist_loaded = FALSE;
    playlist_file_cntr = 0;
    playlist_file_pos = 1;
    playlist_first = 0;

    playlist_it = &playlist_first;
    playlist_it_prev = NULL;

    playListFile = fopen (PLAYLIST_FILENAME, "r");
    if (playListFile)
    {
        playlist_actual = playlist_first;
        drawInfoScreen ("Searching music files...");

        do
        {
            fgets (line, sizeof (line), playListFile);
            /* Remove trailing new lines if needed */
            char* eol = strchr (line, '\n');
            if (eol)
            {
                *eol = 0;
            }
            eol = strchr (line, '\r');
            if (eol)
            {
                *eol = 0;
            }
            if (strlen (line) > 0)
            {
                /* Append trailing backslash if it is missing */
                if (line[strlen (line) - 1] != OS_DIR_SEP_CHR)
                {
                    strncat (line, OS_DIR_SEP_STR, sizeof (line));
                }
                file_cntr += searchFiles (line, "*.MOD", TRUE);
                file_cntr += searchFiles (line, "*.S3M", TRUE);
                file_cntr += searchFiles (line, "*.XM", TRUE);
            }
        } while (!feof (playListFile));

        fclose (playListFile);

        if (file_cntr)
        {
            playlist_loaded = TRUE;
            /* Make playlist endless */
            playlist_it_prev->next = playlist_first;
            playlist_first->prev = playlist_it_prev;
            playlist_actual = playlist_first;
            /* Copy first item of playlist */
            if (playlist_actual->item)
            {
                playlist_file_pos = playlist_actual->pos;
                strncpy (musicFilePath, (char*) playlist_actual->item, sizeof (musicFilePath));
            }
            /* Get filename */
            getFileName (musicFileName, musicFilePath, sizeof (musicFileName));
            if (strlen (config.music_file_path))
            {
                bool_t found = FALSE;
                bool_t turnOver = FALSE;
                do
                {
                    if (!strncmp (config.music_file_path, musicFilePath, sizeof (config.music_file_path)))
                    {
                        found = TRUE;
                    }
                    else
                    {
                        getNextMusicFile (&turnOver);
                    }
                } while (!found && !turnOver);
            }
        }
    }
    else
    {
        /* No playlist, using default filename */
        strncpy (musicFilePath, DEFAULT_MUSIC_FILENAME, sizeof (musicFilePath));
        /* Get filename */
        getFileName (musicFileName, musicFilePath, sizeof (musicFileName));
    }
}

/**
 * @brief donePlaylist
 * Free memory allocated for playlist.
 */
void donePlaylist (void)
{
    if (playlist_first)
    {
        /* Release allocated memory */
        freeLinkedList (playlist_first);
        playlist_mem_size = 0;
    }
}

/**
 * @brief init
 * Initialize game.
 *
 * @return TRUE: if successfully initialized. FALSE: error occurred.
 */
bool_t init (void)
{
    gfx_texture* blockSpriteBmp;
    uint16_t i;
    char tempString[FILENAME_MAX];
    FILE* spriteFile;

    srand (OSTimeGet());

    /* Fill random buffer with the library's values.
     * (Before generating real random values.)
     */
    for (i = 0; i < KEY_DELTA_SIZE; i++)
    {
        key_delta[i] = rand();
    }

    control_init ();
    gameDisplay = display_create (320, 240, 320, (DISPLAY_FORMAT_RGB565 | DISPLAY_BUFFER_STATIC), NULL, NULL);
    if (gameDisplay == NULL)
    {
        control_term();
        return FALSE;
    }
    gfx_init (gameDisplay);

    control_lock (timer_resolution / 4);

    gameFontSmall = gfx_font_load ("font.tga", COLOR_BLACK);
    gameFontNormal = gfx_font_load ("font13.tga", COLOR_BLACK);

    loadConfig ();

    gfx_render_target_clear (DEFAULT_BG_COLOR);

    spriteFile = fopen (BLOCKS_SPRITE_FILENAME, "rb");
    if (spriteFile == NULL)
    {
        drawInfoScreen ("Generating sprites...");

        bool_t end = FALSE;
        blockSprite = sprite_load_from_tga ("block0.tga", TRANSPARENT_BG_COLOR);
        if (blockSprite == NULL)
        {
            return FALSE;
        }
        for (i = 1; !end; i++)
        {
            sprintf (tempString, "block%i.tga", i);
            blockSpriteBmp = gfx_tex_load_tga (tempString);
            if (blockSpriteBmp)
            {
                blockSprite = sprite_frame_add_bitmap (blockSprite,  blockSpriteBmp->address,
                                                       TRANSPARENT_BG_COLOR);
                gfx_tex_delete (blockSpriteBmp);
            }
            else
            {
                end = TRUE;
            }
        }
        sprite_save (blockSprite, BLOCKS_SPRITE_FILENAME);
    }
    else
    {
        fclose (spriteFile);
        blockSprite = sprite_load (BLOCKS_SPRITE_FILENAME);
    }

    gfxTimer = timer_create();

    /* Virtual keyboard */
    vrtkey_init (VRTKEY_MODE_WITHTEXTFIELD);
    //vrtkey_init (VRTKEY_MODE_BASIC);
    //vrtkey_config (VRTKEY_OPT_ENABLECANCEL, 0);
    vrtkey_config (VRTKEY_OPT_SHOWTEXTFIELD, 1);
    
    can_load_game = canLoadGame ();

    /* Music (MOD/XM/S3M files) */
    initPlaylist ();
    music_initted = audiothread_init (SOUND_FREQ_HZ, SOUND_CHANNELS);
    audiothread_pause (config.music_paused);

    return TRUE;
}

void collectRandomNumbers (void)
{
    uint32_t key_time;
    static uint32_t prev_key_time = 0;

    /* Get time between key presses to get better random numbers */
    if ((control_check(CONTROL_BUTTON_A).changed)
            || (control_check(CONTROL_BUTTON_B).changed)
            || (control_check(CONTROL_BUTTON_X).changed)
            || (control_check(CONTROL_BUTTON_Y).changed)
            || (control_check(CONTROL_DPAD_RIGHT).changed)
            || (control_check(CONTROL_DPAD_LEFT).changed)
            || (control_check(CONTROL_DPAD_UP).changed)
            || (control_check(CONTROL_DPAD_DOWN).changed)
            || (control_check(CONTROL_TRIGGER_LEFT).changed)
            || (control_check(CONTROL_TRIGGER_RIGHT).changed)
            || (control_check(CONTROL_BUTTON_SELECT).changed)
            || (control_check(CONTROL_BUTTON_START).changed))
    {
        key_time = OSTimeGet ();
        key_delta[key_delta_widx & KEY_DELTA_IDX_MASK] = key_time - prev_key_time;
        key_delta_widx++;
        key_delta_widx &= KEY_DELTA_IDX_MASK;
        prev_key_time = key_time;
    }
}

/**
 * @brief handleMovement
 * Handle button presses and move figure according to that.
 */
void handleMovement (void)
{
    static uint8_t divider = 0; /* To slow down movement of figure */

    divider++;
    if (divider == 3)
    {
        divider = 0;
        if (control_check (CONTROL_DPAD_RIGHT).pressed)
        {
            /* Right */
            if (canMoveFigureRight ())
            {
                game.figure_x++;
            }
        }
        else if (control_check (CONTROL_DPAD_LEFT).pressed)
        {
            /* Left */
            if (canMoveFigureLeft ())
            {
                game.figure_x--;
            }
        }
        if (control_check (CONTROL_DPAD_DOWN).pressed)
        {
            /* Down */
            if (canMoveFigureDown ())
            {
                game.figure_y++;
            }
        }
    }
    if (control_check(CONTROL_BUTTON_A).pressed
            && control_check(CONTROL_BUTTON_A).changed)
    {
        uint8_t new_x = 0, new_y = 0;
        /* Rotate */
        if (canRotateFigure (&new_x, &new_y))
        {
            rotateFigure (new_x, new_y);
        }
    }
#ifndef TEST_MOVEMENT
    if (OSTimeGet() >= gameTimer)
    {
        /* Automatic fall */
        int16_t ticks;

        if (canMoveFigureDown ())
        {
            game.figure_y++;
        }
        else
        {
            copyFigureToMap ();
            collapseMap ();
            generateFigure ();
        }

        /* Delay time = 0.8 sec - level * 0.1 sec */
        ticks = OS_TICKS_PER_SEC * 8 / 10 - game.level * OS_TICKS_PER_SEC / 10;
        if (ticks < OS_TICKS_PER_SEC / 10) /* less than 0.1 sec */
        {
            ticks = OS_TICKS_PER_SEC / 10;
        }
        gameTimer = OSTimeGet() + ticks;
    }
#endif
}

/**
 * @brief handle_main_state_machine
 * Check inputs and change state machine if it is necessary.
 * @return TRUE: if user plays game again.
 */
bool_t handleMainStateMachine (void)
{
    bool_t replay = FALSE;
    char s[32];
    uint8_t i;
    uint8_t vrtkey_ret;

    switch (main_state_machine)
    {
        case STATE_load_game:
            if (can_load_game)
            {
                if ((control_check (CONTROL_BUTTON_A).pressed)
                        && (control_check (CONTROL_BUTTON_A).changed))
                {
                    loadGame ();
                    deleteGame ();
                    main_state_machine = STATE_running;
                }
                if ((control_check (CONTROL_BUTTON_B).pressed)
                        && (control_check (CONTROL_BUTTON_B).changed))
                {
                    deleteGame ();
                    main_state_machine = STATE_difficulty_selection;
                }
                gfx_render_target_clear (DEFAULT_BG_COLOR);
                gfx_font_print_center (TEXT_YN(5), gameFontNormal, "Automatically saved");
                gfx_font_print_center (TEXT_YN(6), gameFontNormal, "game found!");
                gfx_font_print_center (TEXT_YN(7), gameFontNormal, "A: Load");
                gfx_font_print_center (TEXT_YN(8), gameFontNormal, "B: Abandon");
                display_flip (gameDisplay);
            }
            else
            {
                main_state_machine = STATE_difficulty_selection;
            }
            break;
        case STATE_difficulty_selection:
            if ((control_check (CONTROL_DPAD_DOWN).pressed)
                    && (control_check (CONTROL_DPAD_DOWN).changed)
                    && (game.block_types > MIN_BLOCK_TYPES))
            {
                game.block_types--;
            }
            if ((control_check (CONTROL_DPAD_UP).pressed)
                    && (control_check (CONTROL_DPAD_UP).changed)
                    && (game.block_types < MAX_BLOCK_TYPES))
            {
                game.block_types++;
            }
            if (((control_check (CONTROL_BUTTON_START).pressed) && (control_check (CONTROL_BUTTON_START).changed))
                    || ((control_check (CONTROL_BUTTON_A).pressed) && (control_check (CONTROL_BUTTON_A).changed))
                    || ((control_check (CONTROL_BUTTON_B).pressed) && (control_check (CONTROL_BUTTON_B).changed))
                    )
            {
                main_state_machine = STATE_running;
            }
            gfx_render_target_clear (DEFAULT_BG_COLOR);
            for (i = 0; i < sizeof(info) / sizeof(info[0]); i++)
            {
                gfx_font_print (0, TEXT_Y(i), gameFontSmall, (char*) info[i]);
            }
            i++;
            snprintf (s, sizeof (s), ">>> Difficulty: %i <<<", game.block_types);
            gfx_font_print_center (TEXT_Y(i), gameFontSmall, s);
            display_flip (gameDisplay);
            break;
        case STATE_running:
            handleMovement ();
            if (control_check (CONTROL_BUTTON_START).pressed
                    && control_check (CONTROL_BUTTON_START).changed)
            {
                main_state_machine = STATE_paused;
            }
            if (isGameOver ())
            {
                bool_t new_record;
                new_record = getNewRecordPos (game.block_types, game.score) != 0xFF;
                if (new_record)
                {
                    /* New record! */
                    main_state_machine = STATE_select_name;
                }
                else
                {
                    main_state_machine = STATE_game_over;
                }
            }
            else
            {
                drawGameScreen ();
            }
            break;
        case STATE_paused:
            if (control_check (CONTROL_BUTTON_START).pressed
                    && control_check (CONTROL_BUTTON_START).changed)
            {
                main_state_machine = STATE_running;
            }
            if (music_initted)
            {
                if (control_check (CONTROL_DPAD_DOWN).pressed
                        && control_check (CONTROL_DPAD_DOWN).changed)
                {
                    if (config.volume >= VOLUME_DELTA + VOLUME_MIN)
                    {
                        config.volume -= VOLUME_DELTA;
                    }
                }
                if (control_check (CONTROL_DPAD_UP).pressed
                        && control_check (CONTROL_DPAD_UP).changed)
                {
                    if (config.volume <= VOLUME_MAX - VOLUME_DELTA)
                    {
                        config.volume += VOLUME_DELTA;
                    }
                }
                if (playlist_loaded)
                {
                    if (control_check (CONTROL_DPAD_LEFT).pressed
                            && control_check (CONTROL_DPAD_LEFT).changed)
                    {
                        audiothread_playPrev ();
                    }
                    if (control_check (CONTROL_DPAD_RIGHT).pressed
                            && control_check (CONTROL_DPAD_RIGHT).changed)
                    {
                        audiothread_playNext ();
                    }
                }
            }
            drawGameScreen ();
            break;
        case STATE_game_over:
            if (control_check (CONTROL_BUTTON_START).pressed
                    && control_check(CONTROL_BUTTON_START).changed)
            {
                /* Restart game */
                replay = TRUE;
            }
            if (control_check (CONTROL_BUTTON_X).pressed
                    && control_check(CONTROL_BUTTON_X).changed)
            {
                /* Quit */
                gameRunning = FALSE;
            }
            if (music_initted)
            {
                if (control_check (CONTROL_DPAD_DOWN).pressed
                        && control_check (CONTROL_DPAD_DOWN).changed)
                {
                    if (config.volume >= VOLUME_DELTA + VOLUME_MIN)
                    {
                        config.volume -= VOLUME_DELTA;
                    }
                }
                if (control_check (CONTROL_DPAD_UP).pressed
                        && control_check (CONTROL_DPAD_UP).changed)
                {
                    if (config.volume <= VOLUME_MAX - VOLUME_DELTA)
                    {
                        config.volume += VOLUME_DELTA;
                    }
                }
                if (playlist_loaded)
                {
                    if (control_check (CONTROL_DPAD_LEFT).pressed
                            && control_check (CONTROL_DPAD_LEFT).changed)
                    {
                        audiothread_playPrev ();
                    }
                    if (control_check (CONTROL_DPAD_RIGHT).pressed
                            && control_check (CONTROL_DPAD_RIGHT).changed)
                    {
                        audiothread_playNext ();
                    }
                }
            }
            drawGameScreen ();
            break;
        case STATE_select_name:
            if ((control_check (CONTROL_BUTTON_A).pressed)
                    || (control_check (CONTROL_BUTTON_A).changed))
            {
                if (strlen (config.player_names[config.player_idx]) > 0)
                {
                    insertRecord (game.block_types, game.score);
                    saveConfig ();
                    main_state_machine = STATE_game_over;
                }
                else
                {
                    main_state_machine = STATE_set_name;
                }
            }
            if ((control_check (CONTROL_BUTTON_B).pressed)
                    || (control_check (CONTROL_BUTTON_B).changed))
            {
                main_state_machine = STATE_set_name;
            }
            if ((control_check (CONTROL_DPAD_DOWN).pressed)
                    && (control_check (CONTROL_DPAD_DOWN).changed))
            {
                if (config.player_idx < MAX_PLAYERS - 1)
                {
                    config.player_idx++;
                }
            }
            if ((control_check (CONTROL_DPAD_UP).pressed)
                    && (control_check (CONTROL_DPAD_UP).changed))
            {
                if (config.player_idx > 0)
                {
                    config.player_idx--;
                }
            }
            gfx_render_target_clear (DEFAULT_BG_COLOR);
            printCommon ();
            gfx_font_print (0, TEXT_Y(0), gameFontSmall, "Select your name:");
            for (i = 0; i < MAX_PLAYERS; i++)
            {
                uint8_t c = ' ', c2 = ' ';
                char name[PLAYER_NAME_LENGTH];
                if (strlen (config.player_names[i]) > 0)
                {
                    strncpy (name, config.player_names[i], sizeof (name));
                }
                else
                {
                    strncpy (name, "-UNUSED-", sizeof (name));
                }
                if (i == config.player_idx)
                {
                    c = '>';
                    c2 = '<';
                }
                snprintf (s, sizeof (s), "%c%c%c %-9s %c%c%c", c, c, c, name, c2, c2, c2);
                gfx_font_print (0, TEXT_Y(i + 1), gameFontSmall, s);
            }
            gfx_font_print (0, TEXT_Y(i + 2), gameFontSmall, "A: Select name");
            gfx_font_print (0, TEXT_Y(i + 3), gameFontSmall, "B: Change name");
            display_flip (gameDisplay);
            break;
        case STATE_set_name:
            if (!vrtkey_isopen ())
            {
                vrtkey_open (config.player_names[config.player_idx], PLAYER_NAME_LENGTH, false);
            }
            vrtkey_ret = vrtkey_update ();
            switch (vrtkey_ret)
            {
                case VRTKEY_ACCEPT:
                    insertRecord (game.block_types, game.score);
                    saveConfig ();
                    main_state_machine = STATE_game_over;
                    vrtkey_close (false);
                    break;
                case VRTKEY_CANCEL:
                    main_state_machine = STATE_game_over;
                    vrtkey_close (false);
                    break;
            }
            gfx_render_target_clear (DEFAULT_BG_COLOR);
            gfx_font_print (0, TEXT_Y(0), gameFontNormal, "Set your name:");
            vrtkey_draw();
            display_flip (gameDisplay);
            break;
        default:
        case STATE_undefined:
            /* This should not happen */
            break;
    }

    return replay;
}
#endif

/**
 * @brief myrand
 * The user generates random numbers by pressing buttons: we store the time
 * difference between keystrokes.
 * rand() is not good enough for this game.
 * @return A random number.
 */
uint8_t myrand (void)
{
    uint8_t random;

    random = key_delta[key_delta_ridx];
    key_delta_ridx++;
    key_delta_ridx &= KEY_DELTA_IDX_MASK;
    return random;
}

#if 0
/**
 * @brief run
 * Play game.
 */
void run (void)
{
    uint32_t tempTick    = 0;
    uint32_t tempPauseOS = 0;
    bool_t   do_replay   = FALSE;

replay:
    config.game_counter++;
    main_state_machine = STATE_load_game;
    game.score = 0;
    game.level = 1;
    game.figure_counter = 0;
#ifndef TEST_MAP
    initMap ();
#endif
    generateFigure ();

    while (gameRunning)
    {
        sysref = _sys_judge_event (NULL);
        if (sysref < 0)
        {
            ref = sysref;
            break;
        }

        tempTick += timer_delta (gfxTimer);
        if (tempTick < gameTickRate)
        {
            tempPauseOS = (((gameTickRate - tempTick) * OS_TICKS_PER_SEC) / timer_resolution);
            if (tempPauseOS > 0)
            {
                OSTimeDly (tempPauseOS);
            }
            while (tempTick < gameTickRate)
            {
                tempTick += timer_delta (gfxTimer);
            }
        }


        if (tempTick > (gameTickRate << 2))
        {
            tempTick = 0; // HACK - Fixes timing glitch of unknown origin.
        }
        while (tempTick >= gameTickRate)
        {
            tempTick -= gameTickRate;

            control_poll();
            collectRandomNumbers ();
            /* This buttons are working in every state */
            if (control_check (CONTROL_BUTTON_START).pressed
                    && control_check(CONTROL_BUTTON_SELECT).pressed)
            {
                /* Quit from game */
                gameRunning = FALSE;
                break;
            }
            if (control_check (CONTROL_TRIGGER_LEFT).pressed
                    && control_check (CONTROL_TRIGGER_LEFT).changed)
            {
                if (config.volume >= VOLUME_DELTA + VOLUME_MIN)
                {
                    config.volume -= VOLUME_DELTA;
                }
            }
            if (control_check (CONTROL_TRIGGER_RIGHT).pressed
                    && control_check (CONTROL_TRIGGER_RIGHT).changed)
            {
                if (config.volume <= VOLUME_MAX - VOLUME_DELTA)
                {
                    config.volume += VOLUME_DELTA;
                }
            }
            if (control_check (CONTROL_BUTTON_Y).pressed
                    && control_check (CONTROL_BUTTON_Y).changed)
            {
                config.music_paused = !config.music_paused;
                audiothread_pause (config.music_paused);
            }

            do_replay = handleMainStateMachine ();
            if (do_replay)
            {
                goto replay; /* Shh! Bad thing! */
            }
        }
    }
}

/**
 * @brief done
 * End game. Free resources.
 */
void done (void)
{
    if ((main_state_machine == STATE_running)
        || (main_state_machine == STATE_paused))
    {
        saveGame ();
    }

    vrtkey_term ();

    if (music_initted)
    {
        audiothread_term ();
        donePlaylist ();
    }

    saveConfig ();

    timer_delete (gfxTimer);
    gfx_font_delete (gameFontSmall);

    gfx_term ();
    display_delete (gameDisplay);
    control_term ();
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    if (init ())
    {
        run ();
        done ();
    }
    else
    {
        ref = EXIT_FAILURE;
    }

    return ref;
}
#endif

int main( int argc, char* args[] )
{
    // The images
    SDL_Surface* hello = NULL;
    SDL_Surface* screen = NULL;

    //Start SDL
    SDL_Init( SDL_INIT_EVERYTHING );

    //Set up screen
    screen = SDL_SetVideoMode( 640, 480, 32, SDL_SWSURFACE );

    //Load image
    hello = SDL_LoadBMP( "hello.bmp" );

    //Apply image to screen
    SDL_BlitSurface( hello, NULL, screen, NULL );

    // Initialize SDL_ttf library
    if (TTF_Init() != 0)
    {
        printf("TTF_Init() Failed: %s\n", TTF_GetError());
        SDL_Quit();
        exit(1);
    }

    // Load a font
    TTF_Font *font;
    font = TTF_OpenFont("/usr/share/fonts/TTF/FreeSans.ttf", 24);
    if (font == NULL)
    {
        printf("TTF_OpenFont() Failed: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        exit(1);
    }

    // Write text to surface
    SDL_Surface *text;
    SDL_Color text_color = {0, 0, 0};
    text = TTF_RenderText_Solid(font,
                                "A journey of a thousand miles begins with a single step.",
                                text_color);

    if (text == NULL)
    {
        printf("TTF_RenderText_Solid() Failed: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        exit(1);
    }

    SDL_Rect rect;
    rect.x = 10;
    rect.y = 450;
    rect.w = text->clip_rect.w;
    rect.h = text->clip_rect.h;

    // Apply the text to the display
    if (SDL_BlitSurface(text, NULL, screen, &rect) != 0)
    {
        printf("SDL_BlitSurface() Failed: %s\n", SDL_GetError());
    }

    //Update Screen
    SDL_Flip( screen );

    //Pause
    //SDL_Delay( 2000 );
    while (gameRunning)
    {
        //While there's an event to handle
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                    case SDLK_UP:
                        printf("up\r\n");
                        break;
                    case SDLK_DOWN:
                        printf("down\r\n");
                        break;
                    case SDLK_LEFT:
                        printf("left\r\n");
                        break;
                    case SDLK_RIGHT:
                        printf("right\r\n");
                        break;
                    case SDLK_ESCAPE:
                        gameRunning = FALSE;
                        break;
                }
            }
            else if (event.type == SDL_QUIT) /* If the user has Xed out the window */
            {
                /* Quit the program */
                gameRunning = FALSE;
            }
        }
    }

    //Free the loaded image
    SDL_FreeSurface( hello );

    //Quit SDL
    SDL_Quit();

    return 0;
}
