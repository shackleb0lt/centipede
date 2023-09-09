
/***************************************************************
 *  Header file that defines all macros, user struct and
 *  declares functions to be used to run the program
 *  Refer to example.c for detailed use of code
****************************************************************/
#ifndef EXAMPLE_H
#define EXAMPLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>

// Key Mapping for game actions
#define MOVE_LEFT 'a'
#define MOVE_RIGHT 'd'
#define MOVE_UP 'w'
#define MOVE_DOWN 's'
#define SHOOT ' '
#define QUIT 'q'

// Dimensions of player 
#define P_HEIGHT 3
#define P_ANIMS 8
#define P_LENGTH 3

// Dimension of enemy
#define E_HEIGHT 2
#define E_ANIMS 4
#define E_LENGTH 33

// Default start position of player
#define P_START_ROW 20
#define P_START_COL 40

// Game Board Size
#define GAME_ROWS 24
#define GAME_COLS 80

// Ticks for thread loops
#define SCORE_UPDATE_TICKS 25
#define SCREEN_REFRESH_TICKS 2
#define PLAYER_ANIM_TICKS 40
#define ENEMY_GEN_TICKS 500
#define UPKEEP_INT_TICKS 300
#define BULLET_MOV_TICKS 15
#define ENEMY_MOV_TICKS 30

// enumertion to store the movement direction
enum Direction
{
    UP,
    RIGHT,
    LEFT,
    DOWN
};

// Enumeration to store the game status
enum GAME_STATUS
{
    Running,
    Quit,
    Lost,
    Won,
    Error
};

// Struct to store player info
struct Player
{
    int pos_r;                      // Coordinates of 
    int pos_c;                      // upper left corner
    
    unsigned int lives;             // Number of lives remaining
    unsigned int score;             
    unsigned int anim_count;        // Animation counter
    pthread_mutex_t player_lock;    // Mutex lock of player
    pthread_t anim_thread;          // Thread that handles player animation
};

// Struct to store bullet info
struct Bullet
{
    int pos_r;                  // Coordinates of 
    int pos_c;                  // bullet

    char *anim[1];              // 2D representation of bullet
    bool is_live;               // Whether the bullet is live or dead due to being out of bounds
    enum Direction direct;      // Direction in which the bullet is heading
    pthread_t bullet_thread;    // Thread that simulates bullet movement
    struct Bullet *next;        // Pointer to next bullet to use as a linked list
};

// Struct to store caterpillar/enemy info 
struct Enemy
{
    int pos_r;                  // Coordinates of upper left corner 
    int pos_c;                  // of caterpillar

    unsigned int anim_count;    // Animation counter of caterpillar
    enum Direction direct;      // Direction in which caterpillar is moving

    pthread_t enemy_thread;     // Thread to animate  caterpillar movement
    struct Enemy *next;         // Pointer to next caterpillar to use as a linked list
};

// Driver function
void exampleRun();

// Thread functions that simulate 
void *keyboardThreadFun();
void *enemyGenThreadFun();
void *bulletUpkeepThreadFun();
void *playerAnimationThreadFun();
void *updateScoreScreenThreadFun();
void *bulletAnimationThreadFun(void *arg);
void *enemyAnimationThreadFun(void *arg);

// Helper functions to breakup large pieces of code 
void initLocks();
void initPlayer();
void destroyLocks();
void printGameExit();
void deleteAllEnemy();
void deleteAllBullets();
void movePlayer(int old_row, int old_col);
void updateEnemyPos(struct Enemy * e, int * r, int * c);
void createInsertBullet(enum Direction d, int r, int c);

#endif
