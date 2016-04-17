/**
 * @file        common.h
 * @brief       Common definitions
 * @author      Copyright (C) Peter Ivanov, 2011, 2012, 2013, 2014
 *
 * Created      2011-01-19 11:48:53
 * Last modify: 2016-04-14 14:54:30 ivanovp {Time-stamp}
 * Licence:     GPL
 */

#ifndef _INCLUDE_COMMON_H_
#define _INCLUDE_COMMON_H_

//#define TEST_MAP
//#define TEST_MOVEMENT
//#define TEST_RANDOM

#define VERSION_MAJOR    1
#define VERSION_MINOR    2
#define VERSION_REVISION 1

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

#ifndef FALSE
#define FALSE   (0)
#endif
#ifndef TRUE
#define TRUE    (1)
#endif

#define BIT0    0x01u
#define BIT1    0x02u
#define BIT2    0x04u
#define BIT3    0x08u
#define BIT4    0x10u
#define BIT5    0x20u
#define BIT6    0x40u
#define BIT7    0x80u

#define _BV(x)  (1u << (x))

#define GAME_IS_PAUSED()    (main_state_machine == STATE_paused)
#define GAME_IS_OVER()      ((main_state_machine == STATE_game_over) || (main_state_machine == STATE_select_name) || (main_state_machine == STATE_set_name))

#define KEY_DELTA_SIZE      32  /* Must be power of 2! */
#if KEY_DELTA_SIZE & (KEY_DELTA_SIZE - 1) != 0
#error KEY_DELTA_SIZE must be power of two!
#endif
#define KEY_DELTA_IDX_MASK  (KEY_DELTA_SIZE - 1)

//#define VOLUME_MAX          100 /* For Gemei A330 ??? */
#define VOLUME_MAX          30 /* For Dingoo A320 */
#define VOLUME_MIN          0
#define VOLUME_DELTA        1

#define MAX_RECORD_NUM      10
#define PLAYER_NAME_LENGTH  9
#define MAX_PLAYERS         12

#define FSYS_FILENAME_MAX   255 // FIXME inherited from dingoo
#define OS_TICKS_PER_SEC    1000

typedef char bool_t;

typedef enum
{
    STATE_undefined,            /**< This shall not be used! */
    STATE_load_game,            /**< Load game if it was saved. */
    STATE_difficulty_selection, /**< Game difficulty should be selected first. */
    STATE_running,              /**< Game is played */
    STATE_paused,               /**< Game is paused. It can be played again. */
    STATE_select_name,          /**< Game is over, name should be selected for new record. */
    STATE_set_name,             /**< Game is over, new name is entered for record. */
    STATE_game_over,            /**< Game is over, it can be played again. */
    STATE_size        /**< Not a real state. Only to count number of states. THIS SHOULD BE THE LAST ONE! */
} main_state_machine_t;

typedef struct
{
    char player_name[PLAYER_NAME_LENGTH];
    uint8_t level;
    uint32_t score;
} record_t;

typedef struct
{
    uint8_t version;        /* To prevent loading invalid configuration */
    uint8_t volume;         /* Range: VOLUME_MIN..VOLUME_MAX */
    bool_t music_paused;    /* Range: TRUE/FALSE */
    uint32_t game_counter;  /* To count played games */
    uint8_t player_idx;
    char player_names[MAX_PLAYERS][PLAYER_NAME_LENGTH];
    record_t records[RECORD_TYPES][MAX_RECORD_NUM];
    char music_file_path[FSYS_FILENAME_MAX];
} config_t;

/* Game related */
extern uint32_t     gameTimer;      /* Game timer (automatic shift down of figure) */
extern bool_t       gameRunning;    /* TRUE: game is running, FALSE: game shall exit! */
extern main_state_machine_t main_state_machine; /* Game state machine. @see handleMainStateMachine */

/* Random related */
extern uint8_t      key_delta[KEY_DELTA_SIZE]; /**< Time difference between keystrokes. For random number generating. */
extern uint16_t     key_delta_widx; /**< Write index for key_delta */
extern uint16_t     key_delta_ridx; /**< Read index for key_delta */

/* Music playing */
extern bool_t       music_initted;
extern bool_t       playlist_loaded;
extern uint32_t     playlist_file_cntr;
extern uint32_t     playlist_file_pos;
//extern char         musicFileName[16];
//extern char         musicFilePath[FSYS_FILENAME_MAX];

extern config_t     config; /* Actual configuration. */

uint8_t myrand (void);
bool_t saveConfig (void);
char* getNextMusicFile (bool_t *aTurnOver);
char* getPrevMusicFile (bool_t *aTurnOver);
void key_task();

#endif /* _INCLUDE_COMMON_H_ */

