#ifndef GO_H
#define GO_H

#include <stdbool.h>

#define SIZE 9
#define COUNT SIZE*SIZE

#define KOMI 6.5

#define NO_POSSIBLE_KO -1
#define MOVE_PASS -1

typedef enum { EMPTY, BLACK, WHITE, NEUTRAL } color;

typedef enum { SUCCESS, FAIL_GAME_ENDED, FAIL_BOUNDS, FAIL_OCCUPIED, FAIL_KO, FAIL_SUICIDE, FAIL_OTHER } play_result;

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
	int passes;		// Consecutive passes (when 2, game is over)
	dot board[COUNT];
} state;

typedef struct {
	color player;
	int area;
} territory;

typedef int move;


wchar_t color_char(color);


state* state_create();

void state_copy(state*, state*);

void state_destroy_children(state*);

void state_destroy(state*);

void state_print(state*);

void state_score(state*, float score[3], bool);


group* group_create(dot*, int);

void group_destroy(group*);


move* move_create();

void move_destroy(move*);

bool move_parse(move*, char str[2]);

void move_sprint(wchar_t str[3], move*);

void move_print(move*);


bool go_move_legal(state*, move*);

int go_get_legal_plays(state*, move move_list[COUNT+1]);

play_result go_move_play(state*, move*);

play_result go_move_play_random(state*, move*, move*);

bool go_is_game_over(state*);

#endif