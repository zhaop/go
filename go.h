#ifndef GO_H
#define GO_H

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#define SIZE 5
#define COUNT (SIZE*SIZE)

#define NGROUPS (COUNT-1)
#define NMOVES (COUNT+1)

#define KOMI (6.5)

#define NO_POSSIBLE_KO -1
#define MOVE_PASS -1
#define MOVE_RESIGN -2

#define ADDR_NULL -1

typedef uint8_t color;
#define EMPTY 0
#define BLACK 1
#define WHITE 2
#define NEUTRAL 3

typedef enum { SUCCESS, FAIL_GAME_ENDED, FAIL_BOUNDS, FAIL_OCCUPIED, FAIL_KO, FAIL_SUICIDE, FAIL_OTHER } move_result;

typedef int16_t addr;

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
	addr possibleKo;		// Board index or NO_POSSIBLE_KO
	int passes;		// Consecutive passes (when 2, game is over)
	int prisoners[3];
	float komi;
	struct dot board[COUNT];
	struct group_pool groups;
} state;

typedef struct {
	color player;
	int area;
} territory;

typedef int16_t move;

typedef struct {
	color winner;
	// int t;
	// float score[3];
} playout_result;


wchar_t color_char(color);

color color_opponent(color);


move* move_create();

void move_destroy(move*);

bool move_parse(move*, char str[2]);

void move_sprint(wchar_t str[3], move*);

void move_print(move*);


state* state_create();

void state_copy(state*, state*);

void state_destroy(state*);

void state_print(state*);

void state_dump(state*);

void state_score(state*, float score[3], bool);

color state_winner(state*);


bool fills_in_friendly_eye(dot*, color, int, int);

bool go_is_game_over(state*);

bool go_is_move_legal(state*, move*);

int go_get_legal_moves(state*, move move_list[NMOVES]);

int go_get_reasonable_moves(state*, move move_list[NMOVES]);

move_result go_play_move(state*, move*);

move_result go_play_random_move(state*, move*, move*);

void go_play_out(state*, playout_result*);


void go_print_heatmap(state*, move*, double*, int);

void go_print_move_result(move_result);

#endif