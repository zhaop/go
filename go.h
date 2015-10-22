#ifndef GO_H
#define GO_H

#include <stdbool.h>

#define SIZE 9
#define COUNT SIZE*SIZE

#define KOMI 6.5

#define NO_POSSIBLE_KO -1

/*
group { dot* anchor , int length, int freedoms }
dot   { color player , group* group, dot* prev, dot* next }
state { color nextPlayer , dot[] board }
*/

typedef enum { EMPTY, BLACK, WHITE, NEUTRAL } color;

struct group;
struct dot;

typedef struct group {
	struct dot* anchor;
	int length;
	int freedoms;
} group;

// Dots are sometimes called "stone"s when they're not empty
typedef struct dot {
	int index;		// Location
	color player;
	struct group* group;
	struct dot* prev;	// Prev in group
	struct dot* next;	// Next in group
} dot;

typedef struct {
	color nextPlayer;
	int prisoners[3];
	int possibleKo;		// Board index or NO_POSSIBLE_KO
	dot board[COUNT];
} state;

typedef struct {
	color player;
	int area;
} territory;

typedef int move;

state* state_create();

void state_copy(state*, state*);

void state_destroy(state*);

wchar_t color_char(color);

void state_print(state*);

move* move_create();

void move_destroy(move*);

bool move_parse(move*, char str[2]);

void move_sprint(wchar_t str[3], move*);

void move_print(move*);

bool go_move_legal(state*, move*);

bool go_move_play(state*, move*);

bool go_move_play_random(state*, move*, move*);

#endif
