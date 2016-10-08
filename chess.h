#ifndef CHESS_H
#define CHESS_H

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#define SIZE 8
#define COUNT (SIZE*SIZE)

// 4*8 pawns + 14*2 rooks + 8*2 knights + 13*2 bishops + 27*1 queen + 10*1 king + 1 resignation
#define NMOVES 140

#define MOVE_MAX (COUNT*COUNT)
#define MOVE_NULL -1
#define MOVE_RESIGN -2

typedef enum { NOBODY, WHITE, BLACK } color;

typedef enum { SUCCESS, FAIL_GAME_ENDED, FAIL_BOUNDS, FAIL_OCCUPIED, FAIL_CHECK, FAIL_OTHER } move_result;

typedef enum { PLAYING, STALEMATE, CHECKMATE, RESIGNED } game_status;

#define NPIECES 33
typedef enum { EMPTY, WP1, WP2, WP3, WP4, WP5, WP6, WP7, WP8, WR1, WN1, WB1, WQ, WK, WB2, WN2, WR2, BP1, BP2, BP3, BP4, BP5, BP6, BP7, BP8, BR1, BN1, BB1, BQ, BK, BB2, BN2, BR2 } piece;

typedef struct {
	color nextPlayer;	// Player that has to move next, or if game_status CHECKMATE or RESIGNED, the losing player
	game_status status;
	piece board[COUNT];		// Array of 64 "pieces"
	char pieces[NPIECES];	// Array of locations of all 32 pieces (actually 33, but disregard EMPTY)
} state;

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

bool move_parse(move*, char str[5]);

void move_sprint(wchar_t str[3], move*);

void move_print(move*);


state* state_create();

void state_copy(state*, state*);

void state_destroy(state*);

void state_print(state*);

void state_dump(state*);

color state_winner(state*);


bool chess_is_game_over(state*);

bool chess_is_move_legal(state*, move*);

int chess_get_legal_moves(state*, move* move_list);

int chess_get_reasonable_moves(state*, move* move_list);

move_result chess_play_move(state*, move*);

move_result chess_play_random_move(state*, move*, move*);

void chess_play_out(state*, playout_result*);


void chess_print_heatmap(state*, move*, double*, int);

void chess_print_move_result(move_result);

#endif