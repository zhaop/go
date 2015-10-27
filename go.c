#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "go.h"
#include "utils.h"

// Malloc & init a state
state* state_create() {
	void* p;
	if (!(p = malloc(sizeof(state)))) {
		return NULL;
	}
	state* st = (state*) p;
	st->nextPlayer = BLACK;
	st->prisoners[BLACK] = 0.0;
	st->prisoners[WHITE] = 0.0;
	st->possibleKo = NO_POSSIBLE_KO;
	st->passes = 0;

	dot* board = st->board;
	for (int i = 0; i < COUNT; ++i) {
		board[i].index = i;
		board[i].player = EMPTY;
		board[i].group = NULL;
		board[i].prev = NULL;
		board[i].next = NULL;
	}
	return st;
}

// Deep copy st0 --> st1
void state_copy(state* st0, state* st1) {
	st1->nextPlayer = st0->nextPlayer;
	st1->possibleKo = st0->possibleKo;
	st1->passes = st0->passes;

	st1->prisoners[1] = st0->prisoners[1];
	st1->prisoners[2] = st0->prisoners[2];

	dot* b0 = st0->board;
	dot* b1 = st1->board;

	int i;
	for (i = 0; i < COUNT; ++i) {
		b1[i] = b0[i];
		if (b0[i].player != EMPTY) {
			b1[i].prev = &b1[b0[i].prev->index];
			b1[i].next = &b1[b0[i].next->index];
		}
	}
	for (i = 0; i < COUNT; ++i) {
		if (b0[i].group && (&b0[i] == b0[i].group->anchor)) {
			b1[i].group = group_create(&b1[i], b0[i].group->freedoms);
			if (b1[i].group == NULL) {
				// TODO Manage edge case
			}

			group* gp = b1[i].group;
			dot* anchor = gp->anchor;
			dot* stone = anchor;
			do {
				stone->group = gp;
				stone = stone->next;
			} while (stone != anchor);
		}
	}
}

void state_destroy_children(state* st) {
	dot* board = st->board;
	group* group_list[COUNT/2];
	int group_count = 0;
	int i;
	for (i = 0; i < COUNT; ++i) {
		if (board[i].player != EMPTY) {
			if (board[i].group->anchor == &board[i]) {
				group_list[group_count] = board[i].group;
				++group_count;
			}
		}
	}

	for (i = 0; i < group_count; ++i) {
		group_destroy(group_list[i]);
	}
}

void state_destroy(state* st) {
	state_destroy_children(st);
	free(st);
}

#define ALREADY_COUNTED(i, j) already_counted[(i)*SIZE+(j)]
#define BOARD(i,j) board[(i)*SIZE+(j)]
#define GROUP_AT(i,j) board[(i)*SIZE+(j)].group
#define UP_OK    (i >= 1)
#define LEFT_OK  (j >= 1)
#define RIGHT_OK (j <= SIZE-2)
#define DOWN_OK  (i <= SIZE-2)

// Recursively update the territory struct until all accounted for
void count_territory(dot* board, bool* already_counted, int i, int j, territory* tr) {
	color neighbor_player;

	ALREADY_COUNTED(i, j) = true;
	++tr->area;

	if (UP_OK && !ALREADY_COUNTED(i-1, j)) {
		neighbor_player = BOARD(i-1, j).player;
		if (neighbor_player == EMPTY) {
			count_territory(board, already_counted, i-1, j, tr);
		} else if (tr->player == EMPTY) {
			tr->player = neighbor_player;
		} else if (tr->player == NEUTRAL || tr->player != neighbor_player) {
			tr->player = NEUTRAL;
		}
	}

	if (LEFT_OK && !ALREADY_COUNTED(i, j-1)) {
		neighbor_player = BOARD(i, j-1).player;
		if (neighbor_player == EMPTY) {
			count_territory(board, already_counted, i, j-1, tr);
		} else if (tr->player == EMPTY) {
			tr->player = neighbor_player;
		} else if (tr->player == NEUTRAL || tr->player != neighbor_player) {
			tr->player = NEUTRAL;
		}
	}

	if (RIGHT_OK && !ALREADY_COUNTED(i, j+1)) {
		neighbor_player = BOARD(i, j+1).player;
		if (neighbor_player == EMPTY) {
			count_territory(board, already_counted, i, j+1, tr);
		} else if (tr->player == EMPTY) {
			tr->player = neighbor_player;
		} else if (tr->player == NEUTRAL || tr->player != neighbor_player) {
			tr->player = NEUTRAL;
		}
	}

	if (DOWN_OK && !ALREADY_COUNTED(i+1, j)) {
		neighbor_player = BOARD(i+1, j).player;
		if (neighbor_player == EMPTY) {
			count_territory(board, already_counted, i+1, j, tr);
		} else if (tr->player == EMPTY) {
			tr->player = neighbor_player;
		} else if (tr->player == NEUTRAL || tr->player != neighbor_player) {
			tr->player = NEUTRAL;
		}
	}
}

// Score must be a float array[3]
void state_score(state* st, float* score, bool chinese_rules) {
	dot* board = st->board;

	score[BLACK] = st->prisoners[BLACK];
	score[WHITE] = st->prisoners[WHITE] + KOMI;

	bool already_counted[COUNT];
	int i;
	int j;
	memset(already_counted, (int) false, sizeof(bool) * COUNT);

	for (i = 0; i < SIZE; ++i) {
		for (j = 0; j < SIZE; ++j) {
			if (!already_counted[i*SIZE+j] && BOARD(i, j).player == EMPTY) {
				territory tr = {EMPTY, 0};
				count_territory(board, already_counted, i, j, &tr);
				if (tr.player == BLACK || tr.player == WHITE) {
					score[tr.player] += tr.area;
				}
			} else if (chinese_rules && BOARD(i, j).player != EMPTY) {
				++score[BOARD(i, j).player];
			}
		}
	}
}

bool is_star_point(int i, int j) {
	switch (SIZE) {
		case 9 :
			return ((i == 2) || (i == 4) || (i == 6)) && ((j == 2) || (j == 4) || (j == 6));
			break;
		case 13 :
			return ((i == 3) || (i == 6) || (i == 9)) && ((j == 3) || (j == 6) || (j == 9));
			break;
		case 19 :
			return ((i == 3) || (i == 9) || (i == 15)) && ((j == 3) || (j == 9) || (j == 15));
			break;
	}
	return false;
}

wchar_t color_char(color player) {
	if (player == BLACK) {
		return L'🌑';
	} else if (player == WHITE) {
		return L'⭘';
	} else {
		return L'·';
	}
}

wchar_t dot_char(int i, int j, color player) {
	if ((player == EMPTY) && (is_star_point(i, j))) {
		return L'•';
	} else {
		return color_char(player);
	}
}

char int_char(int n) {
	if (n < 10) {
		return n + '0';
	} else if (n < 36) {
		return (n - 10) + 'a';
	} else {
		return (n - 36) + 'A';
	}
}

group* group_create(dot* anchor, int freedoms) {
	void* p;
	if (!(p = malloc(sizeof(group)))) {
		return NULL;
	}
	group* gp = (group*) p;
	gp->anchor = anchor;
	gp->length = 1;
	gp->freedoms = freedoms;
	return gp;
}

void group_destroy(group* gp) {
	free(gp);
}

void group_print(group* gp) {
	wchar_t str[3];
	dot* anchor = gp->anchor;
	move_sprint(str, &(anchor->index));
	wprintf(L"Group %lc {anchor: %ls, length: %d, freedoms: %d, list: ", color_char(anchor->player), str, gp->length, gp->freedoms);

	dot* stone = anchor;
	do {
		move_print(&(stone->index));
		wprintf(L"->");
		stone = stone->next;
	} while (stone != anchor);

	wprintf(L"}\n");
}

void state_print(state* st) {
	int i;
	int j;
	color nextPlayer = st->nextPlayer;
	color enemy = (st->nextPlayer == BLACK) ? WHITE : BLACK;
	int* prisoners = st->prisoners;
	dot* board = st->board;

	wprintf(L"   ");
	for (i = 0; i < SIZE; ++i) {
		wprintf(L"%c ", int_char(i));
	}
	wprintf(L"\n   ");
	for (i = 0; i < SIZE; ++i) {
		wprintf(L"  ");
	}
	wprintf(L"(%lc %d%c  %lc %d%c)",
		color_char(BLACK), prisoners[BLACK], (nextPlayer == BLACK ? '*' : ' '),
		color_char(WHITE), prisoners[WHITE], (nextPlayer == WHITE ? '*' : ' '));
	if (st->passes == 1) {
		wprintf(L" (%lc passed)", color_char(enemy));
	} else if (st->passes == 2) {
		wprintf(L" (game ended)");
	}
	for (i = 0; i < SIZE; ++i) {
		wprintf(L"\n%c  ", int_char(i));
		for (j = 0; j < SIZE; ++j) {
			wprintf(L"%lc ", dot_char(i, j, BOARD(i, j).player));
		}
	}

	float score[3] = {0.0, 0.0, 0.0};

	double t0 = timer_now();
	state_score(st, score, false);
	double dt = timer_now() - t0;

	wprintf(L"\n\nScore: (%lc %.1f  %lc %.1f) [%.3f us]\n", color_char(BLACK), score[BLACK], color_char(WHITE), score[WHITE], dt*1e6);
}

// Debug info about groups & ko
void state_print_debug(state* st) {
	dot* board = st->board;
	if (st->possibleKo != NO_POSSIBLE_KO) {
		wprintf(L"Possible ko if ");
		move_print(&st->possibleKo);
		wprintf(L" captured\n");
	} else {
		wprintf(L"No possible ko\n");
	}

	dot* stone;
	double t0 = timer_now();
	for (int i = 0; i < COUNT; ++i) {
		stone = &board[i];
		if (stone->group && stone->group->anchor == stone) {
			group_print(stone->group);
		}
	}
	double dt = timer_now() - t0;
	wprintf(L"Dumping groups took [%.3f us]\n", dt*1e6);
}

static inline void board_set_stone(dot* stone, color player, group* gp) {
	stone->player = player;
	stone->group = gp;
}

// Stone must already be set on board
// Assumes linked list never empty
static inline void group_add_stone(group* gp, dot* stone, int liberties) {
	dot* anchor = gp->anchor;
	stone->prev = anchor->prev;
	stone->next = anchor;
	anchor->prev->next = stone;
	anchor->prev = stone;
	++gp->length;
	gp->freedoms += liberties;
}

// Merges smaller group into bigger, and returns the latter
// DO NOT have any references to these groups prior to call (one is destroyed)
group* group_merge_and_destroy_smaller(group* gp1, group* gp2) {
	group* a;
	group* b;

	if (gp1 == gp2) {
		return gp1;
	}

	if (gp1->length >= gp2->length) {
		a = gp1;
		b = gp2;
	} else {
		a = gp2;
		b = gp1;
	}

	dot* a0 = a->anchor;
	dot* b0 = b->anchor;

	dot* stone = b0;
	do {
		stone->group = a;
		stone = stone->next;
	} while (stone != b0);

	dot* at = a0->prev;
	dot* bt = b0->prev;
	a0->prev = bt;
	at->next = b0;
	b0->prev = at;
	bt->next = a0;

	a->length += b->length;
	a->freedoms += b->freedoms;

	free(b);

	return a;
}

// Change freedoms for neighboring groups of given color
MANUAL_INLINE /*void change_neighbors_freedoms(dot* board, group* mem, int i, int j, int delta) {
	if (UP_OK    && BOARD(i-1, j).player != EMPTY) GROUP_AT(i-1, j)->freedoms += delta;
	if (LEFT_OK  && BOARD(i, j-1).player != EMPTY) GROUP_AT(i, j-1)->freedoms += delta;
	if (RIGHT_OK && BOARD(i, j+1).player != EMPTY) GROUP_AT(i, j+1)->freedoms += delta;
	if (DOWN_OK  && BOARD(i+1, j).player != EMPTY) GROUP_AT(i+1, j)->freedoms += delta;
}*/

// Change freedoms for neighboring groups of given color
MANUAL_INLINE /*void change_neighbors_freedoms_if_specific_color(dot* board, group* mem, color player, int i, int j, int delta) {
	if (UP_OK    && BOARD(i-1, j).player == player) GROUP_AT(i-1, j)->freedoms += delta;
	if (LEFT_OK  && BOARD(i, j-1).player == player) GROUP_AT(i, j-1)->freedoms += delta;
	if (RIGHT_OK && BOARD(i, j+1).player == player) GROUP_AT(i, j+1)->freedoms += delta;
	if (DOWN_OK  && BOARD(i+1, j).player == player) GROUP_AT(i+1, j)->freedoms += delta;
}*/

// Removes all of a group's stones from the board, returns number captured
// Caller must destroy gp afterwards
int group_kill_stones(dot* board, group* gp) {
	int i;
	int j;
	int captured = 0;
	dot* gp0 = gp->anchor;
	color enemy = (gp0->player == BLACK) ? WHITE : BLACK;
	dot* stone = gp0;
	dot* tmp_next;
	do {
		tmp_next = stone->next;

		i = stone->index / SIZE;
		j = stone->index % SIZE;
		++captured;

		// change_neighbors_freedoms_if_specific_color(board, enemy, i, j, +1);	// Manually inlined below
		if (UP_OK    && BOARD(i-1, j).player == enemy) GROUP_AT(i-1, j)->freedoms++;
		if (LEFT_OK  && BOARD(i, j-1).player == enemy) GROUP_AT(i, j-1)->freedoms++;
		if (RIGHT_OK && BOARD(i, j+1).player == enemy) GROUP_AT(i, j+1)->freedoms++;
		if (DOWN_OK  && BOARD(i+1, j).player == enemy) GROUP_AT(i+1, j)->freedoms++;

		stone->player = EMPTY;
		stone->group = NULL;
		stone->prev = NULL;
		stone->next = NULL;

		stone = tmp_next;
	} while (stone != gp0);

	return captured;
}

move* move_create() {
	void* p;
	if (!(p = malloc(sizeof(move)))) {
		return NULL;
	}
	return (move*) p;
}

void move_destroy(move* mv) {
	free(mv);
}

// Returns a positive number, or -1 if invalid character
static inline int move_parse_char(char c) {
	if (c >= 'a' && c <= 'z') {
		return (c - 'a' + 10);
	} else if (c >= 'A' && c <= 'z') {
		return (c - 'A' + 36);
	} else if (c >= '0' && c <= '9') {
		return (c - '0');
	} else {
		return -1;
	}
}

// Returns whether move correctly parsed
bool move_parse(move* mv, char str[2]) {
	if (str[0] == '-' && str[1] == '-') {
		*mv = MOVE_PASS;
		return true;
	}

	int i = move_parse_char(str[0]);
	int j = move_parse_char(str[1]);
	if (i < 0 || i >= SIZE || j < 0 || j >= SIZE) {
		return false;
	}

	*mv = i * SIZE + j;
	return true;
}

// str must be a wchar_t[3]
void move_sprint(wchar_t str[3], move* mv) {
	if (*mv == MOVE_PASS) {
		swprintf(str, 3, L"--");
		return;
	}

	int i = *mv/SIZE;
	int j = *mv%SIZE;
	swprintf(str, 3, L"%c%c", int_char(i), int_char(j));
}

void move_print(move* mv) {
	wchar_t str[3];
	move_sprint(str, mv);
	wprintf(str);
}

// Param move_list must be move[COUNT+1]
// Returns number of possible moves
int go_get_legal_plays(state* st, move* move_list) {
	int num = 0;

	if (go_is_game_over(st)) {
		return 0;
	}

	move_list[num] = MOVE_PASS;
	++num;

	move mv;
	for (int i = 0; i < COUNT; ++i) {
		mv = i;
		if (go_move_legal(st, &mv)) {
			move_list[num] = mv;
			++num;
		}
	}
	return num;
}

// Plays a random move & stores it in mv
play_result go_move_play_random(state* st, move* mv, move* move_list) {
	move tmp;
	int timeout = COUNT / 2;	// Heuristics
	int rand_searches = 0;

	do {
		tmp = RANDI(-1, COUNT);
		rand_searches++;
	} while ((rand_searches < timeout) && (go_move_play(st, &tmp) != SUCCESS));

	if (rand_searches < timeout) {
		*mv = tmp;
		return SUCCESS;
	}

	// Few moves remaining; look for them
	int move_count = go_get_legal_plays(st, move_list);
	if (move_count > 0) {
		tmp = move_list[RANDI(0, move_count)];
		return go_move_play(st, &tmp);
	}

	return FAIL_OTHER;
}

// True if ko rule forbids move
bool check_possible_ko(dot* board, int possibleKo, int i, int j) {
	if (possibleKo == i*SIZE + j) {
		group* gp = BOARD(i, j).group;
		return (gp->length == 1) && (gp->freedoms == 1);
	}
	return false;
}

static inline bool is_neighbor_enemy_dead(dot* board, color enemy, int i, int j) {
	return (BOARD(i, j).player == enemy) && (BOARD(i, j).group->freedoms == 0);
}

// Destroys enemy group at (i, j) if dead, return number captured
static inline int remove_dead_neighbor_enemy(dot* board, color enemy, int i, int j) {
	if (is_neighbor_enemy_dead(board, enemy, i, j)) {
		group* gp = BOARD(i, j).group;
		int captured = group_kill_stones(board, gp);
		group_destroy(gp);
		return captured;
	}
	return 0;
}

int count_liberties(dot* board, int i, int j) {
	int liberties = 0;
	if (UP_OK    && BOARD(i-1, j).player == EMPTY) ++liberties;
	if (LEFT_OK  && BOARD(i, j-1).player == EMPTY) ++liberties;
	if (RIGHT_OK && BOARD(i, j+1).player == EMPTY) ++liberties;
	if (DOWN_OK  && BOARD(i+1, j).player == EMPTY) ++liberties;
	return liberties;
}

static inline bool has_living_friendlies(dot* board, color friendly, int i, int j) {
	if (UP_OK    && BOARD(i-1, j).player == friendly && BOARD(i-1, j).group->freedoms != 0) return true;
	if (LEFT_OK  && BOARD(i, j-1).player == friendly && BOARD(i, j-1).group->freedoms != 0) return true;
	if (RIGHT_OK && BOARD(i, j+1).player == friendly && BOARD(i, j+1).group->freedoms != 0) return true;
	if (DOWN_OK  && BOARD(i+1, j).player == friendly && BOARD(i+1, j).group->freedoms != 0) return true;
	return false;
}

bool has_dying_friendlies(dot* board, color friendly, int i, int j) {
	if (UP_OK    && BOARD(i-1, j).player == friendly && BOARD(i-1, j).group->freedoms == 0) return true;
	if (LEFT_OK  && BOARD(i, j-1).player == friendly && BOARD(i, j-1).group->freedoms == 0) return true;
	if (RIGHT_OK && BOARD(i, j+1).player == friendly && BOARD(i, j+1).group->freedoms == 0) return true;
	if (DOWN_OK  && BOARD(i+1, j).player == friendly && BOARD(i+1, j).group->freedoms == 0) return true;
	return false;
}

group* create_lone_group(dot* stone, color player, int liberties) {
	stone->player = player;
	stone->prev = stone;
	stone->next = stone;
	stone->group = group_create(stone, liberties);
	return stone->group;
}

static inline void merge_with_every_friendly(dot* board, color friendly, int i, int j, int liberties) {
	group* gp0 = NULL;
	if (UP_OK && BOARD(i-1, j).player == friendly) {
		if (!gp0) {
			gp0 = BOARD(i-1, j).group;
			board_set_stone(&BOARD(i, j), friendly, gp0);
			group_add_stone(gp0, &BOARD(i, j), liberties);
		} else {
			gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i-1, j).group);
		}
	}
	if (LEFT_OK && BOARD(i, j-1).player == friendly) {
		if (!gp0) {
			gp0 = BOARD(i, j-1).group;
			board_set_stone(&BOARD(i, j), friendly, gp0);
			group_add_stone(gp0, &BOARD(i, j), liberties);
		} else {
			gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i, j-1).group);
		}
	}
	if (RIGHT_OK && BOARD(i, j+1).player == friendly) {
		if (!gp0) {
			gp0 = BOARD(i, j+1).group;
			board_set_stone(&BOARD(i, j), friendly, gp0);
			group_add_stone(gp0, &BOARD(i, j), liberties);
		} else {
			gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i, j+1).group);
		}
	}
	if (DOWN_OK && BOARD(i+1, j).player == friendly) {
		if (!gp0) {
			gp0 = BOARD(i+1, j).group;
			board_set_stone(&BOARD(i, j), friendly, gp0);
			group_add_stone(gp0, &BOARD(i, j), liberties);
		} else {
			gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i+1, j).group);
		}
	}
}

// State is unchanged at the end (but it can change during execution)
bool go_move_legal(state* st, move* mv_ptr) {
	move mv = *mv_ptr;
	dot* board = st->board;
	color friendly = st->nextPlayer;
	color enemy = (friendly == BLACK) ? WHITE : BLACK;

	if (st->passes >= 2) {
		return false;
	}

	if (mv == MOVE_PASS) {
		return true;
	}

	if (mv < 0 || mv >= COUNT) {
		return false;
	}

	int i = mv / SIZE;
	int j = mv % SIZE;

	if (BOARD(i, j).player != EMPTY) {
		return false;
	}

	// Check for simple ko
	if (st->possibleKo != NO_POSSIBLE_KO) {
		bool ko_rule_applies =
			   (UP_OK    && check_possible_ko(board, st->possibleKo, i-1, j))
			|| (LEFT_OK  && check_possible_ko(board, st->possibleKo, i, j-1))
			|| (RIGHT_OK && check_possible_ko(board, st->possibleKo, i, j+1))
			|| (DOWN_OK  && check_possible_ko(board, st->possibleKo, i+1, j));

		if (ko_rule_applies) {
			return false;
		}
	}

	// change_neighbors_freedoms(board, i, j, -1);	// MANUAL_INLINE below
	if (UP_OK    && BOARD(i-1, j).player != EMPTY) GROUP_AT(i-1, j)->freedoms--;
	if (LEFT_OK  && BOARD(i, j-1).player != EMPTY) GROUP_AT(i, j-1)->freedoms--;
	if (RIGHT_OK && BOARD(i, j+1).player != EMPTY) GROUP_AT(i, j+1)->freedoms--;
	if (DOWN_OK  && BOARD(i+1, j).player != EMPTY) GROUP_AT(i+1, j)->freedoms--;

	// If enemy killed, move is legal
	if      (UP_OK    && is_neighbor_enemy_dead(board, enemy, i-1, j)) goto cleanup_legal;
	else if (LEFT_OK  && is_neighbor_enemy_dead(board, enemy, i, j-1)) goto cleanup_legal;
	else if (RIGHT_OK && is_neighbor_enemy_dead(board, enemy, i, j+1)) goto cleanup_legal;
	else if (DOWN_OK  && is_neighbor_enemy_dead(board, enemy, i+1, j)) goto cleanup_legal;

	// If has liberty, move is legal
	if (UP_OK    && BOARD(i-1, j).player == EMPTY) goto cleanup_legal;
	if (LEFT_OK  && BOARD(i, j-1).player == EMPTY) goto cleanup_legal;
	if (RIGHT_OK && BOARD(i, j+1).player == EMPTY) goto cleanup_legal;
	if (DOWN_OK  && BOARD(i+1, j).player == EMPTY) goto cleanup_legal;

	// Look for living friendly neighbors
	if (has_living_friendlies(board, friendly, i, j)) {
		goto cleanup_legal;
	} else {
		goto cleanup_illegal;
	}

	// goto considered harmful?
	cleanup_illegal:
		// change_neighbors_freedoms(board, i, j, +1);	// MANUAL_INLINE below
		if (UP_OK    && BOARD(i-1, j).player != EMPTY) GROUP_AT(i-1, j)->freedoms++;
		if (LEFT_OK  && BOARD(i, j-1).player != EMPTY) GROUP_AT(i, j-1)->freedoms++;
		if (RIGHT_OK && BOARD(i, j+1).player != EMPTY) GROUP_AT(i, j+1)->freedoms++;
		if (DOWN_OK  && BOARD(i+1, j).player != EMPTY) GROUP_AT(i+1, j)->freedoms++;
		return false;

	cleanup_legal:
		// change_neighbors_freedoms(board, i, j, +1);	// MANUAL_INLINE below
		if (UP_OK    && BOARD(i-1, j).player != EMPTY) GROUP_AT(i-1, j)->freedoms++;
		if (LEFT_OK  && BOARD(i, j-1).player != EMPTY) GROUP_AT(i, j-1)->freedoms++;
		if (RIGHT_OK && BOARD(i, j+1).player != EMPTY) GROUP_AT(i, j+1)->freedoms++;
		if (DOWN_OK  && BOARD(i+1, j).player != EMPTY) GROUP_AT(i+1, j)->freedoms++;
		return true;
}

play_result go_move_play(state* st, move* mv_ptr) {
	move mv = *mv_ptr;
	dot* board = st->board;
	color friendly = st->nextPlayer;
	color enemy = (friendly == BLACK) ? WHITE : BLACK;

	if (st->passes == 2) {
		return FAIL_GAME_ENDED;
	}

	if (mv == MOVE_PASS) {
		st->nextPlayer = enemy;
		++st->passes;
		return SUCCESS;
	}
	st->passes = 0;

	if (mv < 0 || mv >= COUNT) {
		return FAIL_BOUNDS;
	}

	int i = mv / SIZE;
	int j = mv % SIZE;

	if (board[mv].player != EMPTY) {
		return FAIL_OCCUPIED;
	}

	// Check for simple ko
	if (st->possibleKo != NO_POSSIBLE_KO) {
		bool ko_rule_applies =
			   (UP_OK    && check_possible_ko(board, st->possibleKo, i-1, j))
			|| (LEFT_OK  && check_possible_ko(board, st->possibleKo, i, j-1))
			|| (RIGHT_OK && check_possible_ko(board, st->possibleKo, i, j+1))
			|| (DOWN_OK  && check_possible_ko(board, st->possibleKo, i+1, j));

		if (ko_rule_applies) {
			return FAIL_KO;
		}
	}

	// Check neighbors for dead enemies & dead friendly neighbors
	// change_neighbors_freedoms(board, i, j, -1);
	if (UP_OK    && BOARD(i-1, j).player != EMPTY) BOARD(i-1, j).group->freedoms--;
	if (LEFT_OK  && BOARD(i, j-1).player != EMPTY) BOARD(i, j-1).group->freedoms--;
	if (RIGHT_OK && BOARD(i, j+1).player != EMPTY) BOARD(i, j+1).group->freedoms--;
	if (DOWN_OK  && BOARD(i+1, j).player != EMPTY) BOARD(i+1, j).group->freedoms--;

	// If dead enemy, kill group
	int captured = 0;
	if (UP_OK)    captured += remove_dead_neighbor_enemy(board, enemy, i-1, j);
	if (LEFT_OK)  captured += remove_dead_neighbor_enemy(board, enemy, i, j-1);
	if (RIGHT_OK) captured += remove_dead_neighbor_enemy(board, enemy, i, j+1);
	if (DOWN_OK)  captured += remove_dead_neighbor_enemy(board, enemy, i+1, j);

	// If need, check for ko on next move
	if (captured == 1) {
		st->possibleKo = mv;
	} else {
		st->possibleKo = NO_POSSIBLE_KO;
	}

	// Count own liberties
	int liberties = count_liberties(board, i, j);

	// Look for illegal move or lone-stone cases
	bool merge_with_friendlies = true;
	if (!has_living_friendlies(board, friendly, i, j)) {
		if (liberties == 0) {
			// change_neighbors_freedoms(board, i, j, +1);
			if (UP_OK    && BOARD(i-1, j).player != EMPTY) BOARD(i-1, j).group->freedoms++;
			if (LEFT_OK  && BOARD(i, j-1).player != EMPTY) BOARD(i, j-1).group->freedoms++;
			if (RIGHT_OK && BOARD(i, j+1).player != EMPTY) BOARD(i, j+1).group->freedoms++;
			if (DOWN_OK  && BOARD(i+1, j).player != EMPTY) BOARD(i+1, j).group->freedoms++;
			return FAIL_SUICIDE;	// Illegal
		} else if (!has_dying_friendlies(board, friendly, i, j)) {
			create_lone_group(&BOARD(i, j), friendly, liberties);
			merge_with_friendlies = false;
		}
	}

	if (merge_with_friendlies) {
		merge_with_every_friendly(board, friendly, i, j, liberties);
	}

	st->prisoners[st->nextPlayer] += captured;
	st->nextPlayer = enemy;

	return SUCCESS;
}

bool go_is_game_over(state* st) {
	return (st->passes == 2);
}