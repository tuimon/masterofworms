/* Master of Worms
   When Pacman meets Snake
   TU 2021                                         */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>
#include <math.h>
#include "maze.h"

#define WORM_MAX_LENGTH 100
#define SLEEP_LENGTH 100000
#define FOOD_AMOUNT 3
#define GHOST_SPEED 2

// globs are bad, mmmkay
int row, col;
int xacc=0, yacc=0, score=0;
int Ax=40, Ay=11;
int prevAx = 40, prevAy=11;
int timeleft = 1200;

struct Worm {
	int x[WORM_MAX_LENGTH];
	int y[WORM_MAX_LENGTH];
	int curr_length;
	int prevx;
	int prevy;
};

// TODO: convert to pointer within main
struct Worm worm = {
		{10,10,10,10,10,10,10,10,10,10},
		{5,5,5,5,5,5,5,5,5,5},
		1, 10, 5
};

// update other game info
void printScreen() {
	move(24,0); clrtoeol();
	mvprintw(24,0,"Worm length: %i", worm.curr_length );
	mvprintw(24, 30, "MASTER OF WORMS");
	mvprintw(24, 64, "Time left: %is", timeleft/10);
}

// print the bad guys
void printGhosts() {	
	// clear old and print new
	mvaddch( prevAy, prevAx, EMPTY_CHAR );
	mvaddch( Ay, Ax, GHOST_A_CHAR );
}

// print the worm to both logical and physical screen buffer
void printWorm() {
	int l;
	attron(COLOR_PAIR(2));
	// clear old worm
	for (l=0; l<worm.curr_length+1 &&l <WORM_MAX_LENGTH; l++) {
		lbuffer[worm.y[l]*WIDTH+worm.x[l]] = EMPTY_CHAR;
		mvaddch( worm.y[l], worm.x[l], EMPTY_CHAR );

	}
	// draw new worm
	for (l=1; l<worm.curr_length;l++) {
		lbuffer[worm.y[l]*WIDTH+worm.x[l]] = WORM_CHAR;
		mvaddch( worm.y[l], worm.x[l], WORM_CHAR );
	}
	lbuffer[worm.y[0]*WIDTH+worm.x[0]] = WORM_HEAD_CHAR;
	mvaddch( worm.y[0], worm.x[0], WORM_HEAD_CHAR );
}

/* return true if coords are a given obstacle */
bool collision(int x, int y, char obs) {
	char c = lbuffer[WIDTH*y+x];
	return c == obs;
}

/* Print food into suitable random place */
bool printFood(int x, int y) {
	// randomizer hit the maze, no can do	
	if (!collision(x,y, EMPTY_CHAR)) {
		return false;
	}
	else {
		lbuffer[y*WIDTH+x] = FOOD_CHAR;
		mvaddch(y, x, FOOD_CHAR);
		return true;
	}
}

// move worm, if possible, based on acceleration
void moveWorm(int xacc, int yacc) {
    int l;
	// check if the new position ends up with collision
	if (collision(worm.x[0]+xacc, worm.y[0]+yacc, WALL_CHAR)) {
		return;
	}

	// first move the tail positions
	for (l=WORM_MAX_LENGTH-2; l>=0; l--) {
		worm.x[l+1] = worm.x[l];
		worm.y[l+1] = worm.y[l];
	}
	
	// then move the head according to acceleration
	worm.y[0] += yacc;
	worm.x[0] += xacc;
}

// move ghosts
void moveGhosts() {
	int diffy = (worm.y[0])-Ay;
	int diffx = (worm.x[0])-Ax;
	prevAx = Ax; prevAy = Ay;

	if (abs(diffy) > abs(diffx)) {
		if (diffy < 0) Ay--;
		else Ay++;
	}
	else {
		if (diffx < 0) Ax--;
		else Ax++;
	}
	while (collision(Ax,Ay, WALL_CHAR)) {
		Ax = prevAx;
		Ay = prevAy;
		Ax += ((rand() % 3)-1);
		if (Ax == prevAx) Ay += ((rand() % 3)-1);
	}
}

// print the actual maze to physical screen
void printMaze() {
	attron(COLOR_PAIR(1));
	//  loop to get linefeed - this helps with > 80 col terminals
	int k=0, x=0, y=0;
	for (k=0; k<HEIGHT*WIDTH; k++) {
		mvaddch(y, x++, lbuffer[k]);
		if (!(x % WIDTH)) { 
			x=0; y++;
		}
	}
}

// init ncurses and setup colors
void init() {
	initscr();
	noecho();
	curs_set(false);
	nodelay(stdscr,true);
	getmaxyx(stdscr, row, col);
	start_color();
	init_pair(1, COLOR_CYAN, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	printMaze();
}

// Main. Print the maze only once since it's intact. Loop the movements only.
int main() {

	int ch, eaten = FOOD_AMOUNT;
	srand((unsigned) time(NULL));
	init();

	// game main loop
	while(timeleft) {
		printWorm();
		if (timeleft % GHOST_SPEED) moveGhosts(); // limit the ghost speed
		printGhosts();
		printScreen();
		while (eaten) {
			while (!printFood(rand() % WIDTH, rand() % HEIGHT));
			eaten--;
		}
		
		// handle keyboard 
		ch = getch();
		switch(ch) {
			case 'a': 
				xacc = -1; yacc = 0;
				break;
			case 'd':
				xacc = 1; yacc = 0;
				break;
			case 'w':
				yacc = -1; xacc = 0;
				break;
			case 's':
				yacc = 1; xacc = 0;
				break;
		}
		// check if ghost collision!
		if (collision(Ax, Ay, WORM_CHAR) || collision(Ax, Ay, WORM_HEAD_CHAR)) {
			if (worm.curr_length > 1) worm.curr_length--;
			prevAy = Ay; prevAx = Ax;
			Ay=11; Ax=40;
			beep();
			printGhosts();
			printWorm();
		}

		moveWorm(xacc, yacc);

		// check if dinner time
		if (collision(worm.x[0], worm.y[0], FOOD_CHAR)) {
			beep(); eaten++; score++; flash();
			if (worm.curr_length < WORM_MAX_LENGTH) {
				worm.curr_length++;
			}
		}

		// all is done, refresh, sleep, and then all over again
		refresh();
		timeleft--;
		usleep(SLEEP_LENGTH);
	}
	while(1);
	endwin();
	return 0;
}
