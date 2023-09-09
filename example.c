
#include "console.h"
#include "example.h"


// Global variables 
struct Player player;			// Holds player info
struct Bullet *bhead;			// Linked list head of all bullets 
struct Enemy *ehead;			// Linked list head of all enemy/caterpillar
enum GAME_STATUS game_status;	// Variable to store game status

// Variables storing threads
pthread_t stat_thread;			// Thread to print score and lives info	
pthread_t keyboard_thread;		// Thread to handle keypress
pthread_t upkeep_thread;		// Thread that rountinely deletes dead bullets and their thread  
pthread_t enemy_gen_thread;		// Thread that generates enemy/caterpillar

// Global mutex locks
pthread_mutex_t bullet_list_lock;	// Lock to be acquired for modifying bullet linked list
pthread_mutex_t game_board_lock;	// Lock to be acquired for modifying enemy linked list
pthread_mutex_t enemy_list_lock;	// Lock to be acquired for updating game board

// Initial game board Look
char *GAME_BOARD[] = {
	"                Score:                                    Lives:               ",
	"=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-centipiede!=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"",
	"",
	"",
	"",
	"",
	"",
	"",
	""};

// 3D array that holds all player animations
char *PLAYER_ANIMATIONS[P_ANIMS][P_HEIGHT] =
	{
		{"(!)",
		 "(0)",
		 "/|\\"},
		{"(!)",
		 "(O)",
		 "/|\\"},
		{"(*)",
		 "(0)",
		 "/|\\"},
		{"(*)",
		 "(O)",
		 "/|\\"},
		{"(+)",
		 "(0)",
		 "/|\\"},
		{"(+)",
		 "(O)",
		 "/|\\"},
		{"(^)",
		 "(0)",
		 "/|\\"},
		{"(^)",
		 "(O)",
		 "/|\\"}};
	
// 3D array that holds all enemy animation while moving towards left
char *ENEMY_BODY_LEFT[E_ANIMS][E_HEIGHT] =
	{
		{"X|||^|||^|||^|||^|||^|||^|||^|||^",
		 "=;;;,;;;,;;;,;;;,;;;,;;;,;;;,;;;,"},
		{"C||^|||^|||^|||^|||^|||^|||^|||^|",
		 "=;;,;;;,;;;,;;;,;;;,;;;,;;;,;;;,;"},
		{"X|^|||^|||^|||^|||^|||^|||^|||^||",
		 "=;,;;;,;;;,;;;,;;;,;;;,;;;,;;;,;;"},
		{"C^|||^|||^|||^|||^|||^|||^|||^|||",
		 "=,;;;,;;;,;;;,;;;,;;;,;;;,;;;,;;;"}};

// 3D array that holds all enemy animation while moving toward right
char *ENEMY_BODY_RIGHT[E_ANIMS][E_HEIGHT] =
	{
		{"^|||^|||^|||^|||^|||^|||^|||^|||X",
		 ",;;;,;;;,;;;,;;;,;;;,;;;,;;;,;;;="},
		{"|^|||^|||^|||^|||^|||^|||^|||^||C",
		 ";,;;;,;;;,;;;,;;;,;;;,;;;,;;;,;;="},
		{"||^|||^|||^|||^|||^|||^|||^|||^|X",
		 ";;,;;;,;;;,;;;,;;;,;;;,;;;,;;;,;;="},
		{"|||^|||^|||^|||^|||^|||^|||^|||^C",
		 ";;;,;;;,;;;,;;;,;;;,;;;,;;;,;;;,="}};

/**
 * Driver function that does the following
 *  Initialize the console game board with specified dimension
 *  Initialize global variables, mutex locks and player info
 * 	Start the game threads with corresponnding function
 *  Wait for threads to finish
 *  Destory all locks and release dynamically alloted memory
 *  Print exit message and wait for final key press
 */
void exampleRun()
{
	if (consoleInit(GAME_ROWS, GAME_COLS, GAME_BOARD))
	{ 
		srand(time(NULL));		// Seed the pseudo randomizer
		initPlayer();			// Initialize player info
		initLocks();			// Initialize all mutex locks

		bhead = NULL;			// Initally no bullets exist
		ehead = NULL;			// Initally no enemy exist
		game_status = Running;	// Change game status to running

		// Intialize threads refer to each function defintion for their purpose
		pthread_create(&stat_thread, NULL, updateScoreScreenThreadFun, NULL);
		pthread_create(&keyboard_thread, NULL, keyboardThreadFun, NULL);
		pthread_create(&player.anim_thread, NULL, playerAnimationThreadFun, NULL);
		pthread_create(&upkeep_thread, NULL, bulletUpkeepThreadFun, NULL);
		pthread_create(&enemy_gen_thread, NULL, enemyGenThreadFun, NULL);

		// Join all threads
		pthread_join(keyboard_thread, NULL);
		pthread_cancel(upkeep_thread); 			// Cancelling these two thread due to their
		pthread_cancel(enemy_gen_thread);		// long sleep times delay exit from program
		pthread_join(stat_thread, NULL);
		pthread_join(player.anim_thread, NULL);
		pthread_join(upkeep_thread, NULL);
		pthread_join(enemy_gen_thread, NULL);

		// Destroy Locks and release memory 
		destroyLocks();
		deleteAllBullets();
		deleteAllEnemy();
		
		// Print Exit message
		printGameExit();
		finalKeypress(); /* wait for final key before killing curses and game */
	}
	consoleFinish();

}

/**
 * Function that initializes global mutex locks
 */
void initLocks()
{
	pthread_mutex_init(&game_board_lock, NULL);
	pthread_mutex_init(&player.player_lock, NULL);
	pthread_mutex_init(&bullet_list_lock, NULL);
	pthread_mutex_init(&enemy_list_lock, NULL);
}

/**
 * Function that destroys global mutex lockss
 */
void destroyLocks()
{
	pthread_mutex_destroy(&game_board_lock);
	pthread_mutex_destroy(&player.player_lock);
	pthread_mutex_destroy(&bullet_list_lock);
	pthread_mutex_destroy(&enemy_list_lock);
}

/**
 * Initialize player info
 */
void initPlayer()
{
	player.score = 0;
	player.lives = 3;
	player.anim_count = 0;
	player.pos_c = P_START_COL;
	player.pos_r = P_START_ROW;
}

/**
 * Function that prints how the game came to an end
 */
void printGameExit()
{
	if (game_status == Quit)
		putBanner("Quitting Game!");
	else if (game_status == Lost)
		putBanner("You Lost. Better Luck Next Time");
	else if (game_status == Won)
		putBanner("Congrats You Won!!");
	else if (game_status == Error)
		putBanner("Error occured while running!!");

}

/**
 * Function that spawns enemy at random but regular intervals
 */
void *enemyGenThreadFun()
{
	// Initialize a timer to count interval between enemy spawns
	unsigned int t = 1;

	while (game_status == Running)
	{
		t--;
		if (t > 0)
		{
			sleepTicks(ENEMY_GEN_TICKS);
			continue;
		}
		// Generate new enemy only when timer hits zero
		// Then re initialize the timer
		t = 3 + rand()%7;

		// Acquire the lock to prevent modification by another thread
		pthread_mutex_lock(&enemy_list_lock);

		// Allocate memory for new enemy
		struct Enemy *temp = (struct Enemy *) malloc(sizeof(struct Enemy));
		
		if(temp == NULL)
		{
			game_status = Error;
			return NULL;
		}

		// Intialize data for new enemy 
		temp->pos_c = GAME_COLS - 1;
		temp->pos_r = 2;
		temp->anim_count = 0;
		temp->next = ehead;
		temp->direct = LEFT;
		ehead = temp;
		
		// Release the lock
		pthread_mutex_unlock(&enemy_list_lock);

		// Create new thread that simulates the enemy movement and animation
		pthread_create(&temp->enemy_thread, NULL, enemyAnimationThreadFun, (void *)temp);
		sleepTicks(ENEMY_GEN_TICKS);
	}
	return NULL;
}

/**
 * Function that releases memory and threads of dead bullets
 * at a regular interval
 */
void *bulletUpkeepThreadFun()
{
	struct Bullet *curr;
	struct Bullet *prev;

	while (game_status == Running)
	{
		// If zero bullets exist go back to sleep
		if (bhead == NULL)
		{
			sleepTicks(UPKEEP_INT_TICKS);
			continue;
		}

		// Acquire linked list lock for bullets to prevent external modification
		pthread_mutex_lock(&bullet_list_lock);

		// Update head until head is live
		while (bhead != NULL && !bhead->is_live)
		{
			curr = bhead;
			bhead = bhead->next;
			// join thread and release thread memory
			pthread_join(curr->bullet_thread, NULL);
			// Free bullet representation string
			free(curr->anim[0]);
			// free bullet memory
			free(curr);
		}

		curr = bhead;
		prev = NULL;

		// Iterate through remaining bullets and do the same
		while (curr != NULL)
		{
			if (!curr->is_live)
			{
				prev->next = curr->next;
				pthread_join(curr->bullet_thread, NULL);
				free(curr->anim[0]);
				free(curr);
			}
			else
				prev = curr;

			curr = prev->next;
		}

		// Release the lock
		pthread_mutex_unlock(&bullet_list_lock);
		sleepTicks(UPKEEP_INT_TICKS);
	}
	return NULL;
}

/**
 * Function to print updated score and lives to the screen
 */
void *updateScoreScreenThreadFun()
{
	// String to hold update score and lives
	unsigned int t = SCORE_UPDATE_TICKS;
	char score_lives[GAME_COLS];
	while (game_status == Running)
	{
		t--;
		if(t != 0)
		{
			consoleRefresh();
			sleepTicks(SCREEN_REFRESH_TICKS);
		}
		t = SCORE_UPDATE_TICKS;

		// Store updated score to string
		snprintf(score_lives, GAME_COLS, "                Score: %-4u                               Lives: %-4u", player.score, player.lives);
		// Print updated string to screen
		pthread_mutex_lock(&game_board_lock);
		putString(score_lives, 0, 0, GAME_COLS);
		consoleRefresh();
		pthread_mutex_unlock(&game_board_lock);
		sleepTicks(SCREEN_REFRESH_TICKS);
	}
	return NULL;
}

/**
 * Function that changes player animation every 40 ticks
*/
void *playerAnimationThreadFun()
{
	char **player_body;

	while (game_status == Running)
	{
		// Acquire player locak to prevent external modification
		pthread_mutex_lock(&player.player_lock);

		player_body = PLAYER_ANIMATIONS[player.anim_count];		// Get player body as 2D array
		player.anim_count = (player.anim_count + 1) % P_ANIMS;	// Update animation counter 

		// Acquire board lock to print to screen
		pthread_mutex_lock(&game_board_lock);

		consoleClearImage(player.pos_r, player.pos_c, P_HEIGHT, P_LENGTH);
		consoleDrawImage(player.pos_r, player.pos_c, player_body, P_HEIGHT);

		// Release locks
		pthread_mutex_unlock(&game_board_lock);
		pthread_mutex_unlock(&player.player_lock);
		
		sleepTicks(PLAYER_ANIM_TICKS);
	}
	return NULL;
}

/**
 * Function that handles bullet simulation
*/
void *bulletAnimationThreadFun(void *arg)
{
	struct Bullet *temp = (struct Bullet *)arg;
	temp->is_live = true;	// Mark the bullet as live as it's fired
	int r;					// To store old row number of bullet

	while (game_status == Running && temp->is_live)
	{
		// store current bullet row
		r = temp->pos_r;

		// Update bullet position according to the direction
		if (temp->direct == UP)		
			temp->pos_r--;
		else
			temp->pos_r++;

		// Check if bullet moves out of bounds if yes exit from the loop
		if (temp->pos_r > 23 || temp->pos_r < 2)
		{
			consoleClearImage(r, temp->pos_c, 1, 1);
			break;
		}
		
		// Update bullet position on screen
		pthread_mutex_lock(&game_board_lock);
		consoleClearImage(r, temp->pos_c, 1, 1);
		consoleDrawImage(temp->pos_r, temp->pos_c, temp->anim, 1);
		pthread_mutex_unlock(&game_board_lock);

		sleepTicks(BULLET_MOV_TICKS);
	}

	temp->is_live = false;

	return NULL;
}

/**
 * Function that simulates enemy 
*/
void *enemyAnimationThreadFun(void *arg)
{
	struct Enemy *temp = (struct Enemy *)arg;

	// Stores time interval between bullet fire by enemy
	int t = 3 + (rand() % 11);

	// Stores row and column for wrap around part of enemy
	int w_r = 2;
	int w_c = 0;

	// To store 2D representation of enemy
	char **curr = NULL;
	char **wrap = NULL;

	while (game_status == Running)
	{
		pthread_mutex_lock(&game_board_lock);

		// If enemy is moving towards left
		if (temp->direct == LEFT)
		{
			// clear current position of caterpillar
			consoleClearImage(temp->pos_r, temp->pos_c, E_HEIGHT, E_LENGTH);
			// clear wrap around part if any
			if ((w_c >= GAME_COLS) && (w_c < (GAME_COLS + E_LENGTH)))
				consoleClearImage(w_r, w_c - E_LENGTH, E_HEIGHT, E_LENGTH);
		}

		// If enemy is moving towards right
		if (temp->direct == RIGHT)
		{
			consoleClearImage(temp->pos_r, temp->pos_c - E_LENGTH, E_HEIGHT, E_LENGTH);
			if ((w_c < 0) && (w_c > (-1*E_LENGTH)))
				consoleClearImage(w_r, w_c, E_HEIGHT, E_LENGTH);
		}

		// Update the enemy position and the wrap around part if any
		updateEnemyPos(temp, &w_r, &w_c);	

		
		if (temp->direct == LEFT)
		{
			// Get new 2D representation of enemy and it's wrap around part
			curr = ENEMY_BODY_LEFT[temp->anim_count];
			wrap = ENEMY_BODY_RIGHT[temp->anim_count];
			// Draw the enemy to screen
			consoleDrawImage(temp->pos_r, temp->pos_c, curr, E_HEIGHT);
			// Draw wrap around part of enemy to screen 
			// Taking advantage of passing negative column which draws only partial image
			if ((w_c >= GAME_COLS) && (w_c < (GAME_COLS + E_LENGTH)))
				consoleDrawImage(w_r, w_c - E_LENGTH, wrap, E_HEIGHT);
		}

		if (temp->direct == RIGHT)
		{
			curr = ENEMY_BODY_RIGHT[temp->anim_count];
			wrap = ENEMY_BODY_LEFT[temp->anim_count];
			consoleDrawImage(temp->pos_r, temp->pos_c - E_LENGTH, curr, E_HEIGHT);
			if ((w_c < 0) && (w_c > (-1 * E_LENGTH)))
				consoleDrawImage(w_r, w_c, wrap, E_HEIGHT);
		}

		pthread_mutex_unlock(&game_board_lock);

		// Decrease the time interval to fire the bullet
		t--;

		// If interval hits zero fire a bullet and re initialize time
		if (t == 0)
		{
			createInsertBullet(DOWN, temp->pos_r + 1, temp->pos_c);
			t = 3 + (rand() % 11);
		}

		// If caterpillar reaches end of screen game is lost
		if (temp->pos_r > 14)
		{
			game_status = Lost;
			break;
		}

		sleepTicks(ENEMY_MOV_TICKS);
	}
	return NULL;
}

/**
 * Function to handle key presses
*/
void *keyboardThreadFun()
{
	// Using code from template
	fd_set set;
	while (game_status == Running)
	{
		// Initialize set to zero
		FD_ZERO(&set);
		// Watch for input on stdin
		FD_SET(STDIN_FILENO, &set);

		struct timespec timeout = getTimeout(1);
		int ret = pselect(FD_SETSIZE, &set, NULL, NULL, &timeout, NULL);
		// ret will be non-zero if 
		if (game_status == Running && ret >= 1)
		{
			char c = getchar();

			// Move player if W, A, S or D is pressed
			if (c == MOVE_LEFT && player.pos_c > 0)
			{
				movePlayer(player.pos_r, player.pos_c--);
			}
			else if (c == MOVE_RIGHT && player.pos_c < GAME_COLS - P_LENGTH)
			{
				movePlayer(player.pos_r, player.pos_c++);
			}
			else if (c == MOVE_DOWN && player.pos_r < GAME_ROWS - P_HEIGHT)
			{
				movePlayer(player.pos_r++, player.pos_c);
			}
			else if (c == MOVE_UP && player.pos_r > 17)
			{
				movePlayer(player.pos_r--, player.pos_c);
			}

			// Fire a plyer bullet is space is pressed
			else if (c == SHOOT)
			{
				player.score++;
				createInsertBullet(UP, player.pos_r - 1, player.pos_c + 1);
			}
			
			// Change the game status to quit if q is pressed
			else if (c == QUIT)
			{
				game_status = Quit;
			}
			sleepTicks(SCREEN_REFRESH_TICKS);
		}
	}
	return NULL;
}

/**
 * Helper function that calculates new coordinates for enemy
 * and it's wrap wround part is any.
*/
void updateEnemyPos(struct Enemy *e, int *r, int *c)
{
	// Change the animation to next one
	e->anim_count = (e->anim_count + 1) % E_ANIMS;

	// Change col value for enemy and wrap around part if any
	if (e->direct == LEFT)
	{	
		e->pos_c--;
		(*c) += 1;
	}
	else
	{
		e->pos_c++;
		(*c) -= 1;
	}

	// If reached end while going left or right
	// Start moving to the opposite direction on next line
	// Reset the wrap around part accordingly
	if (e->pos_c < 0)
	{
		(*r) = e->pos_r;
		(*c) = e->pos_c;

		e->pos_c = 0;
		e->pos_r += 2;
		e->direct = RIGHT;
	}
	if (e->pos_c >= GAME_COLS)
	{
		(*r) = e->pos_r;
		(*c) = e->pos_c;

		e->pos_c = GAME_COLS - 1;
		e->pos_r += 2;
		e->direct = LEFT;
	}
}

/**
 * Helper function that joins all bullet threads and 
 * releases dynamically allocated memoy for them
*/
void deleteAllBullets()
{
	if (bhead == NULL)
		return;

	//acquire lock
	pthread_mutex_lock(&bullet_list_lock);
	struct Bullet *curr;

	while (bhead != NULL)
	{
		curr = bhead;
		bhead = bhead->next;
		pthread_join(curr->bullet_thread, NULL);
		free(curr->anim[0]); 	// Free bullet 2D representation
		free(curr);
	}
	//release lock
	pthread_mutex_unlock(&bullet_list_lock);
}

/**
 * Helper function that joins all enemy threads and
 * releases the dynamically allocated memory
*/
void deleteAllEnemy()
{
	if (ehead == NULL)
		return;

	pthread_mutex_lock(&enemy_list_lock);
	struct Enemy *curr;

	while (ehead != NULL)
	{
		curr = ehead;
		ehead = ehead->next;
		pthread_join(curr->enemy_thread, NULL);
		free(curr);
	}
	pthread_mutex_unlock(&enemy_list_lock);
}

/**
 * Helper function that changes player position 
 * according to key press
*/
void movePlayer(int old_row, int old_col)
{
	// Acquire player and game board lock
	pthread_mutex_lock(&player.player_lock);
	pthread_mutex_lock(&game_board_lock);
	
	// Get player animation 
	char **player_body = PLAYER_ANIMATIONS[player.anim_count];

	// Clear old player position and redraw at new one
	consoleClearImage(old_row, old_col, P_HEIGHT, P_LENGTH);
	consoleDrawImage(player.pos_r, player.pos_c, player_body, P_HEIGHT);
	
	//Release the locks
	pthread_mutex_unlock(&game_board_lock);
	pthread_mutex_unlock(&player.player_lock);
}

/**
 * Helper function that spawns a new bullet thread
 * in the direction and at position provided
*/
void createInsertBullet(enum Direction d, int r, int c)
{
	// Acquire bulllet list lock
	pthread_mutex_lock(&bullet_list_lock);
	struct Bullet *temp = (struct Bullet *) malloc(sizeof(struct Bullet));

	if(temp == NULL)
	{
		game_status = Error;
		return;
	}

	//Acquire player lock to ensure player does not moves
	temp->pos_c = c;
	temp->pos_r = r;
	temp->direct = d;

	// Generate 2D representation of a bullet 
	if (temp->direct == UP)
		temp->anim[0] = strdup("'");
	else
		temp->anim[0] = strdup("v");

	temp->next = bhead;
	bhead = temp;

	// Release the lock
	pthread_mutex_unlock(&bullet_list_lock);

	// Spawn the bullet thread 
	pthread_create(&temp->bullet_thread, NULL, bulletAnimationThreadFun, (void *)temp);
}
