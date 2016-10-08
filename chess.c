#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "chess.h"
#include "rand.h"
#include "utils.h"

INIT_MAKE_RANDI(42, 43);
#if SIZE >= 12 && SIZE <= 22
	MAKE_RANDI512(move_random, -1, COUNT);
#elif SIZE >= 6 && SIZE <= 11
	MAKE_RANDI128(move_random, -1, COUNT);
#elif SIZE <= 5
	MAKE_RANDI32(move_random, -1, COUNT);
#endif


wchar_t color_char(color player) {
	if (player == BLACK) {
		return L'B';
	} else if (player == WHITE) {
		return L'W';
	} else {
		return L'·';
	}
}

color color_opponent(color player) {
	return (player == BLACK) ? WHITE : (player == WHITE ? BLACK : NOBODY);
}


// Maps ['a' to 'h'] and ['1' to '8'] to the range of [0 to 7], or returns -1
static inline int char_index(char c) {
	if (c >= 'a' && c <= 'h') {
		return (c - 'a');
	} else if (c >= '1' && c <= '8') {
		return (c - '1');
	} else {
		return -1;
	}
}

wchar_t heatmap_char(float val) {
	if (val >= 1.0) {
		return L'+';
	} else if (val >= 0.9) {
		return (int)(val*10) + L'0';
	} else if (val >= 0.0) {
		return (int)(val*10) + L'₀';
	} else {
		return L'₋';
	}
}



move* move_create() {
	move* mv;
	if (!(mv = (move*)malloc(sizeof(move)))) {
		return NULL;
	}
	return mv;
}

void move_destroy(move* mv) {
	free(mv);
}

// Returns whether move correctly parsed
bool move_parse(move* mv, char str[5]) {
	if (str[0] == ':' && str[1] == '/') {
		*mv = MOVE_RESIGN;
		return true;
	}
	
	int fx = char_index(str[0]), fy = char_index(str[1]), tx = char_index(str[3]), ty = char_index(str[4]);
	if (fx < 0 || fy < 0 || tx < 0 || ty < 0 || (fx == tx && fy == ty)) {
		return false;
	}
	
	*mv = (fx << 9) + (fy << 6) + (tx << 3) + ty;
	return true;
}

// str must be a wchar_t[6]
void move_sprint(wchar_t str[6], move* mv) {
	if (*mv == MOVE_RESIGN) {
		swprintf(str, 6, L":/");
		return;
	}

	char fx, fy, tx, ty;
	fx =  *mv >> 9;
	fy = (*mv & (0x07 << 6)) >> 6;
	tx = (*mv & (0x07 << 3)) >> 3;
	ty =  *mv & 0x07;

	swprintf(str, 6, L"%c%c-%c%c", fx + 'a', fy + '1', tx + 'a', ty + '1');
}

void move_print(move* mv) {
	wchar_t str[6];
	move_sprint(str, mv);
	wprintf(str);
}


// Malloc & init a state
state* state_create() {
	state* st;
	if (!(st = (state*)malloc(sizeof(state)))) {
		return NULL;
	}

	st->nextPlayer = WHITE;
	st->status = PLAYING;

	return st;
}

// Deep copy st0 --> st1
void state_copy(state* st0, state* st1) {
	st1->nextPlayer = st0->nextPlayer;
}

void state_destroy(state* st) {
	free(st);
}

void state_print(state* st) {
	color nextPlayer = st->nextPlayer;
	color enemy = (st->nextPlayer == BLACK) ? WHITE : BLACK;
	game_status status = st->status;

	wprintf(L"%s plays\n", (nextPlayer == WHITE) ? L"White" : L"Black");
	if (status != PLAYING) {
		wprintf(L"Game is over: ");
		switch (status) {
			case STALEMATE:
				wprintf(L"Stalemate\n");
				break;
			case CHECKMATE:
				wprintf(L"Checkmate by %s\n", (enemy == WHITE) ? L"White" : L"Black");
				break;
			case RESIGNED:
				wprintf(L"Resignation by %s\n", (nextPlayer == WHITE) ? L"White" : L"Black");
				break;
			default:
				break;
		}
	}
}

// Debug info about groups & ko
void state_dump(state* st) {
	state_print(st);
}

color state_winner(state* st) {
	switch (st->status) {
		case CHECKMATE:
		case RESIGNED:
			return color_opponent(st->nextPlayer);
			break;
		case PLAYING:
		case STALEMATE:
		default:
			return NOBODY;
			break;
	}
}


// Move-playing function updates game status
bool chess_is_game_over(state* st) {
	return st->status != PLAYING;
}

// State is unchanged at the end (but it can change during execution)
bool chess_is_move_legal(state* st, move* mv_ptr) {
	return true;
}

// Never resign
bool chess_is_move_reasonable(state* st, move* mv_ptr) {
	move mv = *mv_ptr;
	if (!chess_is_move_legal(st, mv_ptr)) {
		return false;
	}

	color me = st->nextPlayer;
	color notme = color_opponent(me);
	
	if (mv == MOVE_RESIGN) {
		return false;
	}

	// TODO
	return true;
}

// Param move_list must be move[COUNT+1]
// Returns number of legally playable moves
int chess_get_legal_moves(state* st, move* move_list) {
	int num = 0;

	if (chess_is_game_over(st)) {
		return 0;
	}

	move_list[num++] = MOVE_RESIGN;

	move mv;
	// TODO move_list[num++] = ...
	return num;
}

// Like chess_get_legal_moves, but without resignations
int chess_get_reasonable_moves(state* st, move* move_list) {
	int num = 0;

	if (chess_is_game_over(st)) {
		return 0;
	}

	move mv;
	// TODO move_list[num++] = ...

	// Or resign when nowhere to play
	if (!num) {
		move_list[num] = MOVE_RESIGN;
		++num;
	}

	return num;
}

move_result chess_play_move(state* st, move* mv_ptr) {
	move mv = *mv_ptr;
	color friendly = st->nextPlayer;
	color enemy = (friendly == BLACK) ? WHITE : BLACK;

	if (mv == MOVE_RESIGN) {
		st->nextPlayer = enemy;
		return SUCCESS;
	}

	if (mv < 0 || mv >= MOVE_MAX) {
		return FAIL_BOUNDS;
	}

	st->nextPlayer = enemy;

	return SUCCESS;
}

// Plays a "random" move & stores it in mv
move_result chess_play_random_move(state* st, move* mv, move* move_list) {
	move tmp;
	int timeout = COUNT;	// Heuristics
	int rand_searches = 0;

	color me = st->nextPlayer;
	color notme = (me == BLACK) ? WHITE : BLACK;

	do {
		tmp = move_random();	// Random never resigns (mv = -2)
		rand_searches++;

		if (chess_play_move(st, &tmp) == SUCCESS) {
			*mv = tmp;
			return SUCCESS;
		}
	} while (rand_searches < timeout);

	// Few moves remaining; look for them
	int move_count = chess_get_legal_moves(st, move_list);
	if (move_count > 1) {
		tmp = move_list[RANDI(0, move_count)];
		return chess_play_move(st, &tmp);
	} else if (move_count == 1) {
		return chess_play_move(st, &move_list[0]);
	}

	return FAIL_OTHER;
}

// Modifies st; stores result
// Assumes game isn't over
void chess_play_out(state* st, playout_result* result) {
	move mv;
	move mv_list[COUNT+1];
	while (!chess_is_game_over(st)) {
		if (chess_play_random_move(st, &mv, mv_list) != SUCCESS) {
			fwprintf(stderr, L"E: chess_play_out couldn't play any moves\n");
			result->winner = NOBODY;
			return;
		}
	}

	// TODO
	result->winner = NOBODY;
	return;
}


void chess_print_heatmap(state* st, move* moves, double* values, int num_moves) {
	// TODO wprintf(L"Something\n")
}

void chess_print_move_result(move_result result) {
	switch (result) {
		case SUCCESS:
			wprintf(L"Success\n");
			break;
		case FAIL_GAME_ENDED:
			wprintf(L"Game ended\n");
			break;
		case FAIL_BOUNDS:
			wprintf(L"Out of bounds\n");
			break;
		case FAIL_OCCUPIED:
			wprintf(L"Square occupied\n");
			break;
		case FAIL_CHECK:
			wprintf(L"King in check\n");
			break;
		case FAIL_OTHER:
		default:
			wprintf(L"Other\n");
			break;
	}
}
