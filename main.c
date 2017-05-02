#include"structs.h"
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
int shootrange = 20;
char* pl_onetime;
char* pl_multime;
FILE *MAP;
int mapheight, mapwidth;
int currentmap[40][80];
gameobj objects[500];
defobj objlib[100];
double dirx[8] = { 2,-2,0,0,2,-2,-2,2 };
double diry[8] = { 0,0,2,-2,2,-2,2,-2 };
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

defobj getdef(int x) {  // get the corresponding defobj
	return(objlib[objects[x].objtype]);
}
int cango(defobj temp, int x, int y) {
	for (int i = 0;i < temp.height;i++)
		for (int j = 0;j < temp.width;j++)
			if ((temp.icon[i][j] != ' ') && (currentmap[x + i][y + j] == 1)) return 1;
	return 0;
}

bool overlap(int a, int b) {
	defobj tem1, tem2;
	tem1 = getdef(a); tem2 = getdef(b);
	int px1 = objects[a].x, py1 = objects[a].y, px2 = objects[b].x, py2 = objects[b].y;
	if (px1 > (px2 + tem2.height)) return 0;
	if (px2>(px1 + tem1.height)) return 0;
	if (py1 > (py2 + tem2.width)) return 0;
	if (py2 >(py1 + tem1.width)) return 0;
	for(int i1=0;i1<tem1.height;i1++)
		for(int j1=0;j1<tem1.width;j1++)
			for (int i2 = 0;i2<tem2.height;i2++)
				for (int j2 = 0;j2 < tem2.width;j2++) {
					if (((px1 + i1) == (px2 + i2)) && ((py1 + j1) == (py2 + j2)))
						if ((tem1.icon[i1][j1] != ' ') && (tem2.icon[i2][j2] != ' '))
							return 1;
				}
	return 0;
}

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

void calDamage() {
	for (int i = 0;i < 500;i++) {
		if (objects[i].objtype != -1) {
			int termi = 0;
			for (int j = 0;j < 500;j++) 
				if ((objects[j].objtype != -1)&&(objects[i].hostile!=objects[j].hostile)) {
					if (overlap(i, j) == 1) {
						if (objects[j].damage > 0) {
							if ((objects[i].destroyable == 1) && (objects[i].hurt <= 0)) {
								objects[i].hp -= objects[j].damage;
								if (i == playerId) objects[i].hurt = 15;
								   else objects[i].hurt = 2;
								if (objects[i].hp <= 0) {
									if(i!=playerId) objects[i].objtype = -1; termi = 1;
								}

							}
							if ((getdef(i).destroydes != 1) && (getdef(j).destroydes == 1))
								objects[j].objtype = -1;
						}
						if (objects[i].damage > 0) {
							if ((objects[j].destroyable == 1) && (objects[j].hurt <= 0)) {
								objects[j].hp -= objects[i].damage;
								if (j == playerId) objects[j].hurt = 40;
								else objects[j].hurt = 10;
								if (objects[j].hp <= 0) {
									if (i != playerId) objects[j].objtype = -1;
								}

							}
							if ((getdef(j).destroydes != 1) && (getdef(i).destroydes == 1))
								objects[i].objtype = -1;
						}
					}
					if (termi == 1) break;
				}
		}

	}


}

void moveObjects() {
	for (int i = 0; i < 500; i++) {
		if (objects[i].objtype == -1)
			continue;
		defobj temp = getdef(i);
		     //  move(5, 5);
				//addch('a');
				//printw("%.3f", objlib[1].movemode);
		if (objects[i].attackcd != 0) objects[i].attackcd--;
		if (objects[i].hurt > 0) objects[i].hurt--;
		if (temp.movemode != -1) {  //adjust dest
			if (temp.movemode == 0) { //tracking player
				objects[i].desx = objects[playerId].x;
				objects[i].desy = objects[playerId].y;
			}
			
		}

		double dispX = objects[i].desx - objects[i].x;
		double dispY = objects[i].desy - objects[i].y;
		double disp = sqrt(dispX * dispX + dispY * dispY);
		if ((fabs(dispX) > 0.001) || (fabs(dispY) > 0.001)) {
			// if gameobject is not in destination, move it!
			double deltaX, deltaY;
			if (disp < objects[i].speed) {
				// if displacement is less than speed, move to destination directly
				deltaX = dispX;
				deltaY = dispY;
			}
			else {
				deltaX = dispX / disp * (double)objects[i].speed;
				deltaY = dispY / disp * (double)objects[i].speed;
			}
			int px= objects[i].x + deltaX;
			int py= objects[i].y + deltaY;
			if ((cango(temp,px,py) != 1) || (temp.destroydes == 1)) {
				objects[i].x += deltaX;
				objects[i].y += deltaY;
			}
			else if (temp.movemode == 1) {  //random 8 direction
				int tempdir = (rand()) % 8;
				objects[i].desx = objects[i].x + dirx[tempdir];
				objects[i].desy = objects[i].y + diry[tempdir];

			}
			if ((currentmap[(int)objects[i].x][(int)objects[i].y] == 1) && (temp.destroydes == 1)) objects[i].objtype = -1;
		}
		else {
		 if (temp.movemode == 1) {  //random 8 direction
			int tempdir = (rand()) % 8;
			objects[i].desx = objects[i].x + dirx[tempdir];
			objects[i].desy = objects[i].y + diry[tempdir];

		}
			if (temp.destroydes == 1) objects[i].objtype = -1;
		}
		if (i != playerId) {    //creatures attack
            if(temp.attackmode!=-1)
				if (objects[i].attackcd == 0) {
					objects[i].attackcd = temp.atkspeed;
					int xx;
					defobj shot = objlib[temp.attackmode];
					if (temp.atktype == 1) {
						xx = setupobj(temp.attackmode, shot.destroyable, shot.penetration, shot.hp, shot.damage, objects[i].x, objects[i].y, (double)1, objects[i].y, shot.color, shot.speed, shot.atkspeed);
						xx = setupobj(temp.attackmode, shot.destroyable, shot.penetration, shot.hp, shot.damage, objects[i].x, objects[i].y, (double)48, objects[i].y, shot.color, shot.speed, shot.atkspeed);
						xx = setupobj(temp.attackmode, shot.destroyable, shot.penetration, shot.hp, shot.damage, objects[i].x, objects[i].y, objects[i].x, (double)1, shot.color, shot.speed, shot.atkspeed);
						xx = setupobj(temp.attackmode, shot.destroyable, shot.penetration, shot.hp, shot.damage, objects[i].x, objects[i].y, objects[i].x, (double)68, shot.color, shot.speed, shot.atkspeed);
					}
					else if (temp.atktype == 2) {
						xx = setupobj(temp.attackmode, shot.destroyable, shot.penetration, shot.hp, shot.damage, objects[i].x, objects[i].y, objects[playerId].x, objects[playerId].y, shot.color, shot.speed, shot.atkspeed);
					}
			  }

		}
		/* else {
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
		}*/

		//gameObject[i].turnsAlive++;
		}

	calDamage();
}

void displayObjects() {
	for (int i = 0; i < 500; i++) {
		if (objects[i].objtype == -1)
			continue;

		int screenX = objects[i].x;
		int screenY = objects[i].y;
		defobj temp = getdef(i);
		if ((move(screenX, screenY) != ERR)&&((move(screenX+temp.height-1,screenY+temp.width-1 )!=ERR))) {
			if (objects[i].hurt>0) renderIcon(screenX, screenY, temp, 6);
			else renderIcon(screenX, screenY, temp, objects[i].color);
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

int setupobj(int objtype,int destroyable, int penetration, int hp, int damage, double x, double y, double desx, double desy,  int color, double speed, int atkspeed) {
	for (int i = 0; i < 500; i++) {
		if (objects[i].objtype != -1) continue;
		objects[i].objtype = objtype;
		objects[i].destroyable = destroyable;
		objects[i].penetration = penetration;
		objects[i].hp = hp;
		objects[i].damage = damage;
		objects[i].x = x;
		objects[i].y = y;
		objects[i].desx = desx;
		objects[i].desy = desy;
		objects[i].color = color;
		objects[i].speed = speed;
		objects[i].atkspeed = atkspeed;
		objects[i].attackcd = 0;
		objects[i].hostile = objlib[objtype].hostile;
		return(i);

  }
	return(-1);
}

int doGameLoop() {
	// Extended characters table: http://melvilletheatre.com/articles/ncurses-extended-characters/index.html
	// e.g. addch(97 | A_ALTCHARSET) will print out a "brick" character
	//      addch(96 | A_ALTCHARSET) will print out a diamond
	
	// setup the level and player!
	clear();
	//playerId = setupObject(PLAYER,SCREEN_WIDTH/2,SCREEN_HEIGHT/2,0.5);
	playerId = setupobj(0, objlib[0].destroyable, objlib[0].penetration, playerhp, objlib[0].damage, 5, 5, 5, 5, objlib[0].color, objlib[0].speed, objlib[0].atkspeed);
	int xx;
	xx=setupobj(2, objlib[2].destroyable, objlib[2].penetration, objlib[2].hp, objlib[2].damage, 15, 30, 15, 30, objlib[2].color, objlib[2].speed, objlib[2].atkspeed);
	xx = setupobj(4, objlib[4].destroyable, objlib[4].penetration, objlib[4].hp, objlib[4].damage, 10, 25, 10, 25, objlib[4].color, objlib[4].speed, objlib[4].atkspeed);
	// main game loop...
	while (1) {
		// 1. draw background...
		playerhp = objects[playerId].hp;
		if (playerhp <= 0) {
			godie(); return 1;
		}
		drawBackground();
		
		//renderIcon(objects[playerId].x, objects[playerId].y, objlib[0], objects[0].color);
		//renderIcon(0, 10, tes);
		// 2. get buffered user input and determine player action
		int ch = getch();
		if (ch == KEY_UP) { objects[playerId].desx = objects[playerId].x - 1; playerFacing = NORTH; }
		else if (ch == KEY_DOWN) { objects[playerId].desx = objects[playerId].x + 1; playerFacing = SOUTH; }
		else if (ch == KEY_LEFT) { objects[playerId].desy = objects[playerId].y - 1; playerFacing = WEST; }
		else if (ch == KEY_RIGHT) { objects[playerId].desy = objects[playerId].y + 1; playerFacing = EAST; }
		else if ((ch == 's')&&(objects[playerId].attackcd==0)) {
			                       objects[playerId].attackcd = objects[playerId].atkspeed;
			                      int bb = getdef(playerId).attackmode;
                                  int b_id = setupobj(bb,objlib[bb].destroyable,objlib[bb].penetration,objlib[bb].hp,objlib[bb].damage,objects[playerId].x,objects[playerId].y, objects[playerId].x+shootrange, objects[playerId].y,objlib[bb].color,objlib[bb].speed,objlib[bb].atkspeed); 
		                   }
		else if ((ch == 'w') && (objects[playerId].attackcd == 0)) {
			objects[playerId].attackcd = objects[playerId].atkspeed;
			int bb = getdef(playerId).attackmode;
			int b_id = setupobj(bb, objlib[bb].destroyable, objlib[bb].penetration, objlib[bb].hp, objlib[bb].damage, objects[playerId].x, objects[playerId].y, objects[playerId].x- shootrange, objects[playerId].y , objlib[bb].color, objlib[bb].speed, objlib[bb].atkspeed);
		}
		else if ((ch == 'a') && (objects[playerId].attackcd == 0)) {
			objects[playerId].attackcd = objects[playerId].atkspeed;
			int bb = getdef(playerId).attackmode;
			int b_id = setupobj(bb, objlib[bb].destroyable, objlib[bb].penetration, objlib[bb].hp, objlib[bb].damage, objects[playerId].x, objects[playerId].y, objects[playerId].x, objects[playerId].y-shootrange, objlib[bb].color, objlib[bb].speed, objlib[bb].atkspeed);
		}
		else if ((ch == 'd') && (objects[playerId].attackcd == 0)) {
			objects[playerId].attackcd = objects[playerId].atkspeed;
			int bb = getdef(playerId).attackmode;
			int b_id = setupobj(bb, objlib[bb].destroyable, objlib[bb].penetration, objlib[bb].hp, objlib[bb].damage, objects[playerId].x, objects[playerId].y, objects[playerId].x, objects[playerId].y + shootrange, objlib[bb].color, objlib[bb].speed, objlib[bb].atkspeed);
		}
		/*else if (ch == ' ') {
			// shoot!
			int bulletId = setupObject(BULLET, gameObject[playerId].x, gameObject[playerId].y, 0.5);
			gameObject[bulletId].destX = gameObject[playerId].x + DIRECTION2X[playerFacing] * 20;
			gameObject[bulletId].destY = gameObject[playerId].y + DIRECTION2Y[playerFacing] * 20;
			gameObject[bulletId].destroyOnDest = TRUE;
		}*/

		/*else if (ch == 'b') {
			int bombId = setupObject(BOMB, gameObject[playerId].x, gameObject[playerId].y, 0.25);
			gameObject[bombId].destX = gameObject[playerId].x + DIRECTION2X[playerFacing] * 10;
			gameObject[bombId].destY = gameObject[playerId].y + DIRECTION2Y[playerFacing] * 10;
			gameObject[bombId].destroyOnDest = TRUE;
		}*/
		

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
	
	                        // read map from map.in
	MAP = fopen("map.in", "r");
	fscanf(MAP, "%d %d", &mapheight, &mapwidth);
	for (int i = 0; i < mapheight; i++)
		for (int j = 0; j < mapwidth; j++)
			fscanf(MAP, "%d", &currentmap[i][j]); 
	fclose(MAP);
	init_objlib(objlib);  // initialize types of creatures, bullets...

	// Set up colors...colors are always in pairs in a terminal!
	init_Color();
	init_main();
	adjustWindowSize();
	// Game logic!
	int termi=0;
	while (1) {
		if (termi == 1) sleep(50);
		else {
			int selectedMenu = doMenu();
			if (selectedMenu == 1) exit(0);
			else termi = doGameLoop();
		}
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
	//move(5, 0);
	//addch(97);
	while (1) {
		int ch = getch();
		if (ch == ' ') return;
		sleep(10);
	}

}

void init_main() {
	srand((unsigned)time(NULL));
	pl_multime= (char*)malloc(20 * sizeof(char));
	pl_onetime= (char*)malloc(20 * sizeof(char));
	pl_multime = "Roll";
	pl_onetime = "The moon";
	for (int i = 0; i < 500; i++)
		objects[i].objtype = -1;
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