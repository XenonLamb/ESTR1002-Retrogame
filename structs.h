#if defined(_WIN32) || defined(_WIN64)
#include "curses.h"
#include <Windows.h>
#else
#include "ncurses.h"		// please download yourself on Mac...
#include <unistd.h>
#endif

struct gameObj
{
	int destroyable;
	int penetration;
	int hurt;
	int color;
	int hp;
	int speed;
	int atkspeed;
	int damage;
	int x, y;
	int desx, desy;
	int height, width;
	int icon[10][10];
	int attackmode;
};
typedef struct gameObj gameobj;

void renderIcon(int x,int y, gameobj obje) {
	attron(COLOR_PAIR(obje.color));
	for (int i = 0;i < obje.height;i++) {
		for (int j = 0; j < obje.width; j++) {
			move(x + i, y + j);
			if (obje.icon[i][j] != ' ') addch(obje.icon[i][j]);
		}
	}
	attroff(COLOR_PAIR(obje.color));
	return;
}

void displayhp() {
	extern playerhp,playerbomb,playerkey,playermoney;
	extern char* pl_onetime;
	extern char* pl_multime;
	move(1, 55);
	printw("HP: ");
	attron(COLOR_PAIR(3));
	for (int i = 0;i < playerhp;i++)
		addch(3 | A_ALTCHARSET);
	attroff(COLOR_PAIR(3));
	move(3, 55);
	attron(COLOR_PAIR(2));
	addch(10 | A_ALTCHARSET);
	attroff(COLOR_PAIR(2));
	printw(": %s", pl_multime);
	move(5, 55);
	attron(COLOR_PAIR(5));
	addch(10 | A_ALTCHARSET);
	attroff(COLOR_PAIR(5));
	printw(": %s", pl_onetime);
	move(1, 40);
	attron(COLOR_PAIR(2));
	printw("$");
	attroff(COLOR_PAIR(2));
	printw(": %d",playermoney);
	move(3, 40);
	addch(15 | A_ALTCHARSET);
	printw(": %d", playerbomb);
	move(5, 40);
	attron(COLOR_PAIR(4));
	addch(11 | A_ALTCHARSET);
	attroff(COLOR_PAIR(4));
	printw(": %d", playerkey);
	return;

}
void init_Color() {

	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);		// 1: inverse
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);	// 2: highlight
	init_pair(3, COLOR_RED, COLOR_BLACK);
	init_pair(4, COLOR_BLUE, COLOR_BLACK);
	init_pair(5, COLOR_GREEN, COLOR_BLACK);
	return;
}