#ifndef GO_H
#define GO_H

#include <stdbool.h>

#define SIZE 9
#define COUNT (SIZE*SIZE)

#define NGROUPS (COUNT-1)

#define KOMI 6.5

#define NO_POSSIBLE_KO -1
#define MOVE_PASS -1

#define ADDR_NULL -1

typedef enum { EMPTY, BLACK, WHITE, NEUTRAL } color;

typedef enum { SUCCESS, FAIL_GAME_ENDED, FAIL_BOUNDS, FAIL_OCCUPIED, FAIL_KO, FAIL_SUICIDE, FAIL_OTHER } play_result;

typedef int addr;

struct group;
struct dot;

typedef struct group {
	struct group* prev;
	struct group* next;

	struct dot* head;
	int length;
	int freedoms;
} group;

typedef struct group_pool {
	struct group mem[NGROUPS];	// Must come first
	struct group* free;
	struct group* used;
} group_pool;

// Dots are sometimes called "stone"s when they're not empty
typedef struct dot {
	addr i;		// Location; should be constant
	color player;
	struct group* group;
	struct dot* prev;	// Prev in group
	struct dot* next;	// Next in group
} dot;

typedef struct {
	color nextPlayer;
	int possibleKo;		// Board index or NO_POSSIBLE_KO
	int passes;		// Consecutive passes (when 2, game is over)
	int prisoners[3];
	struct dot board[COUNT];
	struct group_pool groups;
} state;

typedef struct {
	color player;
	int area;
} territory;

typedef int move;


wchar_t color_char(color);

color color_opponent(color);


state* state_create();

void state_copy(state*, state*);

void state_destroy_children(state*);

void state_destroy(state*);

void state_print(state*);

void state_heatmap_print(state*, move*, double*, int);

void state_score(state*, float score[3], bool);


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