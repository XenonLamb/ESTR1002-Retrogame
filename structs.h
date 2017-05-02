#if defined(_WIN32) || defined(_WIN64)
#include "curses.h"
#include <Windows.h>
#include <stdlib.h> 
#else
#include "ncurses.h"		// please download yourself on Mac...
#include <unistd.h>
#endif

struct gameObj
{
	int objtype;
	int hostile;
	int destroyable;
	int penetration;
	int hurt;
	int color;
	int hp;
	double speed;
	int atkspeed;
	int damage;
	double x, y;
	double desx, desy;
	int attackcd;
	
};

struct DefObj
{
	int destroyable;
	int penetration;
	int color;
	int hp;
	double speed;
	int atkspeed;
	int damage;
	int height, width;
	int icon[10][10];
    int attackmode;
	int atktype;
	int hostile;
	int movemode;
	int destroydes;
};
typedef struct gameObj gameobj;
typedef struct DefObj defobj;
void renderIcon(int x,int y, defobj obje, int color) {
	attron(COLOR_PAIR(color));
	for (int i = 0;i < obje.height;i++) {
		for (int j = 0; j < obje.width; j++) {
			move(x + i+10, y + j);
			if (obje.icon[i][j] != ' ') addch(obje.icon[i][j]);
		}
	}
	attroff(COLOR_PAIR(color));
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
	init_pair(6, COLOR_RED, COLOR_WHITE);
	return;
}

void init_objlib(defobj lib[]) {               // order of input:  1. number of objs
	FILE *OBJ;                                // 2. destroyable, penetration, color, hp, speed, atkspeed, damage, attackspeed
	int n;                                     // 3. height, width
	OBJ = fopen("objlib.in", "r");             // array icon
	fscanf(OBJ, "%d", &n);
	for (int i = 0;i < n;i++) {
		fscanf(OBJ, "%d", &(lib[i].destroyable));
		fscanf(OBJ, "%d", &(lib[i].penetration));
		fscanf(OBJ, "%d", &(lib[i].color));
		fscanf(OBJ, "%d", &(lib[i].hp));
		fscanf(OBJ, "%lf", &(lib[i].speed));
		fscanf(OBJ, "%d", &(lib[i].atkspeed));
		fscanf(OBJ, "%d", &(lib[i].damage));
		fscanf(OBJ, "%d", &(lib[i].attackmode));
		fscanf(OBJ, "%d", &(lib[i].hostile));
		fscanf(OBJ, "%d", &(lib[i].movemode));
		fscanf(OBJ, "%d", &(lib[i].destroydes));
		fscanf(OBJ, "%d", &(lib[i].atktype));
		fscanf(OBJ, "%d", &(lib[i].height));
		fscanf(OBJ, "%d", &(lib[i].width));
		for(int j=0;j<lib[i].height;j++)
			for(int k=0;k<lib[i].width;k++)
				fscanf(OBJ, "%d", &(lib[i].icon[j][k]));
	}
	fclose(OBJ);
}

void godie() {
	clear();
	move(20, 40);
	attron(COLOR_PAIR(6));
	printw("You Died");
	attron(COLOR_PAIR(6));
	refresh();
}

