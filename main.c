
#include <stdio.h>
#include "example.h" 

/**
 * Things Implemented :-
 *  Basic Player that animates, moves in accordance with wasd key press and fires bullets
 *  Basic Caterpillar that animates moves and wrap aorunds and shoots bullet at random times
 *  Bulllets that move appropriately and die when out of bounds
 *  End conditions - quit and lose implemented 
 *  Joins all threads and releases all dynamic memory,
 * 	The only memory leaks shown in valgrind are due to ncurses library
 * 
 * Failed to implement advanced player and advanced caterpillar that are 
 * capable of interacting with bulletsS
*/
int main(int argc, char**argv) 
{
	// Running the game
	exampleRun();
	// Print "done!" after successful exit from game
	printf("done!\n");
}
