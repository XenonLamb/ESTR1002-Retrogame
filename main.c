#include"structs.h"
#include <stdlib.h> 
#include <time.h>
#include <math.h>

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 50

//testing 
gameobj tes;
int playerhp = 8;
int plhplim = 15;
int playermoney = 0;
int playerkey = 0;
int playerbomb = 1;
char* pl_onetime;
char* pl_multime;
FILE *MAP;
int mapheight, mapwidth;
int currentmap[40][80];

struct SMALL_RECT {
	SHORT Left;
	SHORT Top;
	SHORT Right;
	SHORT Bottom;
};

void setupDisplay() {
#if defined(_WIN32) || defined(_WIN64)
	system("chcp 437");		// Windows-only
#endif
}

#if defined(_WIN32) || defined(_WIN64)
void sleep(unsigned ms) {
	Sleep(ms);
}
#else
void sleep(unsigned ms) {
	usleep(ms * 1000);		// supposedly this should work on Mac, but I don't have a Mac, so I am not sure
}
#endif

void showcon();
void init_main();
void drawmap(int px,int py);

void adjustWindowSize()
{
	struct SMALL_RECT test;

	HANDLE hStdout;
	COORD coord;
	BOOL ok;

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	coord.X = SCREEN_HEIGHT;
	coord.Y = SCREEN_WIDTH;
	ok = SetConsoleScreenBufferSize(hStdout, coord);

	test.Left = 0;
	test.Top = 0;
	test.Right = coord.X - 1;
	test.Bottom = coord.Y - 1;

	SetConsoleWindowInfo(hStdout, ok, &test);

}

void clearScreen() {
	// this creates less flicker than standard "clear()"
	move(0, 0);
	for (int i = 0; i < SCREEN_HEIGHT;i++)
		for (int j=0;j<SCREEN_WIDTH;j++)
		printw(" ");
	
}

void printInMiddle(int y, int colorPair, char *string) {
	int length = strlen(string);
	
	int x = (SCREEN_WIDTH - length)/2;
	attron(COLOR_PAIR(colorPair));
	mvprintw(y, x, "%s",string);
	attroff(COLOR_PAIR(colorPair));
}

int doMenu() {
	int selectedItem = 0;

	while (1) {
		// 1. get buffered user input
		int ch = getch();
		if (ch == KEY_UP )
			selectedItem = (selectedItem+2) %3;
		else if(ch==KEY_DOWN)
			selectedItem = (selectedItem + 1) % 3;
		else if (ch == ' ') {
			if (selectedItem == 1) showcon();
			else return selectedItem;
		}

		// 2. render the display this turn
		clear();		// clear what's on screen last time
		//box(win, 10, 10);
		printInMiddle(17, 0, "ESTR1002B Game Project Sample");
		/* if (selectedItem % 2 == 0) {
			printInMiddle(9, 1, " Start Game ");
			printInMiddle(10, 0, " Exit ");
		} else {
			printInMiddle(9, 0, " Start Game ");
			printInMiddle(10, 1, " Exit ");
		}*/
		if(selectedItem%3==0) printInMiddle(19, 1, " Start Game ");
		             else printInMiddle(19, 0, " Start Game ");
		if (selectedItem % 3 == 1) printInMiddle(20, 1, " Control Settings ");
					 else printInMiddle(20, 0, " Control Settings ");
		if (selectedItem % 3 == 2) printInMiddle(11, 1, " Exit ");
					 else printInMiddle(21, 0, " Exit ");
		printInMiddle(32, 0, "Press space to select ... ");
		
		refresh();		// update the display in one go, very important

		// 3. stop running for some time to prevent using up all CPU power;
		sleep(10);			// want to sleep for roughly 10ms
	}
}

void drawBackground() {
	clearScreen();// just call clear screen now...
	move(8, 0);
	for (int i = 0;i < 24;i++)
		printw("-");
	for (int i = 0;i <= 8;i++)
	{
		move(i, 24);
		printw("|");
	}
	displayhp();
	drawmap(10,0);
}

typedef enum {NOTHING = 0, PLAYER, BULLET, BOMB} ObjectType;
typedef enum { NORTH, EAST, SOUTH, WEST } Direction;
double DIRECTION2X[4] = { 0, 1, 0, -1 };
double DIRECTION2Y[4] = { -1, 0, 1, 0 };
double DIAGONALX[4] = { 1, 1, -1, -1 };
double DIAGONALY[4] = { 1, -1, 1, -1 };

typedef struct {
	double x, y;				// current position
	double destX, destY;		// destination
	double speed;
	ObjectType type;
	int turnsAlive;
	bool destroyOnDest;			// whether this object gets destroyed when reaching destination
} GameObject;

GameObject gameObject[100];		// stores all game object!

int playerId;
Direction playerFacing;

void moveObjects() {
	for (int i = 0; i < 100; i++) {
		if (gameObject[i].type == NOTHING)
			continue;

		double dispX = gameObject[i].destX - gameObject[i].x;
		double dispY = gameObject[i].destY - gameObject[i].y;
		double disp = sqrt(dispX * dispX + dispY * dispY);
		if ((fabs(dispX) > 0.01) || (fabs(dispY) > 0.01)) {
			// if gameobject is not in destination, move it!
			double deltaX, deltaY;
			if (disp < gameObject[i].speed) {
				// if displacement is less than speed, move to destination directly
				deltaX = dispX;
				deltaY = dispY;
			}
			else {
				deltaX = dispX / disp * gameObject[i].speed;
				deltaY = dispY / disp * gameObject[i].speed;
			}
			gameObject[i].x += deltaX;
			gameObject[i].y += deltaY;
		} else {
			// reaches destination...
			if (gameObject[i].destroyOnDest == TRUE && gameObject[i].turnsAlive > 0) {
				if (gameObject[i].type == BOMB) {
					// spawn fragments when bombs are destroyed!
					for (int k = 0; k < 4; k++) {
						int bulletId = setupObject(BULLET, gameObject[i].x, gameObject[i].y, 0.5);
						gameObject[bulletId].destX = gameObject[i].x + DIAGONALX[k] * 10;
						gameObject[bulletId].destY = gameObject[i].y + DIAGONALY[k] * 10;
						gameObject[bulletId].destroyOnDest = TRUE;
					}
				}

				gameObject[i].type = NOTHING;		// destroy it!

			}
		}

		gameObject[i].turnsAlive++;
		


	}
}

void displayObjects() {
	for (int i = 0; i < 100; i++) {
		if (gameObject[i].type == NOTHING)
			continue;

		int screenX = gameObject[i].x;
		int screenY = gameObject[i].y;

		if (move(screenY, screenX) != ERR) {
			if (gameObject[i].type == PLAYER) {
				attron(COLOR_PAIR(2));
				addch(97 | A_ALTCHARSET);
				if (move(screenY - 1, screenX) != ERR) {
					addch('o');
				}
				attroff(COLOR_PAIR(2));
			}
			else if (gameObject[i].type == BULLET) {
				addch(96 | A_ALTCHARSET);
				
			}
			else if (gameObject[i].type == BOMB) {
				addch('@');
			}
		}
	}
}

int setupObject(int type, double startX, double startY, double speed) {
	for (int i = 99; i >= 0; i--) {
		if (gameObject[i].type != NOTHING)
			continue;		// if not empty, try next

		gameObject[i].type = type;
		gameObject[i].x = startX;
		gameObject[i].y = startY;
		gameObject[i].destX = gameObject[i].x;
		gameObject[i].destY = gameObject[i].y;
		gameObject[i].speed = speed;
		gameObject[i].turnsAlive = 0;
		return i;
	}
	

}

int doGameLoop() {
	// Extended characters table: http://melvilletheatre.com/articles/ncurses-extended-characters/index.html
	// e.g. addch(97 | A_ALTCHARSET) will print out a "brick" character
	//      addch(96 | A_ALTCHARSET) will print out a diamond
	
	// setup the level and player!
	clear();
	playerId = setupObject(PLAYER,SCREEN_WIDTH/2,SCREEN_HEIGHT/2,0.1);
	
	tes.destroyable = tes.damage = tes.desx = tes.desy = tes.hp = tes.x = tes.y = 0;
	tes.width = 5;tes.height = 3;
	tes.icon[0][0] = ' ';tes.icon[0][1] = '^';tes.icon[0][2] = '_';tes.icon[0][3] = '^';tes.icon[0][4] = ' ';tes.icon[1][0] = '<';
	tes.icon[1][1] = '-';tes.icon[1][2] = '-';tes.icon[1][3] = '-';tes.icon[1][4] = '-';tes.icon[2][0] = '<';
	tes.icon[2][1] = '-';tes.icon[2][2] = '-';tes.icon[2][3] = '-';tes.icon[2][4] = '-';
	renderIcon(0,10,tes);

	// main game loop...
	while (1) {
		// 1. draw background...
		drawBackground();
		//renderIcon(0, 10, tes);
		// 2. get buffered user input and determine player action
		int ch = getch();
		if (ch == KEY_UP) { gameObject[playerId].destY = gameObject[playerId].y - 1; playerFacing = NORTH; }
		else if (ch == KEY_DOWN) { gameObject[playerId].destY = gameObject[playerId].y + 1; playerFacing = SOUTH; }
		else if (ch == KEY_LEFT) { gameObject[playerId].destX = gameObject[playerId].x - 1; playerFacing = WEST; }
		else if (ch == KEY_RIGHT) { gameObject[playerId].destX = gameObject[playerId].x + 1; playerFacing = EAST; }
		else if (ch == ' ') {
			// shoot!
			int bulletId = setupObject(BULLET, gameObject[playerId].x, gameObject[playerId].y, 0.5);
			gameObject[bulletId].destX = gameObject[playerId].x + DIRECTION2X[playerFacing] * 20;
			gameObject[bulletId].destY = gameObject[playerId].y + DIRECTION2Y[playerFacing] * 20;
			gameObject[bulletId].destroyOnDest = TRUE;
		}

		else if (ch == 'b') {
			int bombId = setupObject(BOMB, gameObject[playerId].x, gameObject[playerId].y, 0.25);
			gameObject[bombId].destX = gameObject[playerId].x + DIRECTION2X[playerFacing] * 10;
			gameObject[bombId].destY = gameObject[playerId].y + DIRECTION2Y[playerFacing] * 10;
			gameObject[bombId].destroyOnDest = TRUE;
		}
		

		// 3. update all game objects positions
		moveObjects();
		
		// 4. render the display this turn
		displayObjects();
		refresh();		// update the display in one go

		// 5. stop running for some time to prevent using up all CPU power;
		// if you want to compensate for computational time and sleep non-fixed amount of time,
		// you will need to get system time like clock() and calculate, but that is not necessary most of the time
		sleep(20);			// want to sleep for a few ms; for Mac, probably have to include another library
	}
}


int main()
{
	// NOTE: Official HOWTO for Curses library: http://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
	// NOTE: How to setup PDCurses: https://jdonaldmccarthy.wordpress.com/2014/09/05/how-to-set-up-pdcurses-in-visual-studio-2013-c/

	// Setting up the environment; you don't need to understand these things unless you want something entirely different
	setupDisplay();
    
	initscr();				// start the curses mode	
	raw();					// disable line buffering 
	keypad(stdscr, TRUE);	// to detect UP, DOWN etc	
	timeout(0);				// make getch() non-blocking, i.e. won't stop the program when we ask for user input
	noecho();				// don't print user input on screen while we do getch 
	curs_set(0);						// don't display cursor
	MAP = fopen("map.in", "r");
	fscanf(MAP, "%d %d", &mapheight, &mapwidth);
	for (int i = 0; i < mapheight; i++)
		for (int j = 0; j < mapwidth; j++)
			fscanf(MAP, "%d", &currentmap[i][j]); 
	// Set up colors...colors are always in pairs in a terminal!
	init_Color();
	init_main();
	adjustWindowSize();
	// Game logic!
	while (1) {
		int selectedMenu = doMenu();
		if (selectedMenu == 1) exit(0);
		else doGameLoop();
	}
	

	// Cleaning up...
	endwin();			// end the curses mode
	return 0;
}


void showcon() {
	clear();
	printInMiddle(18, 0, " Arrow up/down/left/right: Move ");
	printInMiddle(19, 0, " W/S/A/D: Shoot ");
	printInMiddle(20, 0, "E: Bomb");
	printInMiddle(21, 0, " Q: One-time Skills ");
	printInMiddle(22, 0, " F: Reusable Skills ");

	printInMiddle(32, 0, " Press space to title ");
	
	for (int i = 0;i < 100;i++) {
		move(0, i);
		//printw("%c", i);
		addch(i | A_ALTCHARSET);
	}
	for (int i = 100;i < 200;i++) {
		move(1, i-100);
		//printw("%c", i);
		addch(i | A_ALTCHARSET);
		//printw("%d", i);
	}
	for (int i = 200;i < 256;i++) {
		move(2, i - 100);
		//printw("%c", i);
		addch(i | A_ALTCHARSET);
	}
	while (1) {
		int ch = getch();
		if (ch == ' ') return;
		sleep(10);
	}

}

void init_main() {
	pl_multime= (char*)malloc(20 * sizeof(char));
	pl_onetime= (char*)malloc(20 * sizeof(char));
	pl_multime = "Roll";
	pl_onetime = "The moon";
	return;
}

void drawmap(int px,int py) {
	for(int i=0;i<mapheight;i++)
		for (int j = 0; j <mapwidth; j++)
		{
			move(px + i, py + j);
			switch (currentmap[i][j])
			{
			case 0: 
				addch(' ');
				break;
			case 1:
				addch(48 | A_ALTCHARSET);
				break;
			case 2:
				addch(110 | A_ALTCHARSET);
				break;
			default:
				break;
			}
		}
	
}