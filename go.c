#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
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
	st->possibleKo = NO_POSSIBLE_KO;

	int i;
	dot* b = st->board;
	for (i = 0; i < COUNT; ++i) {
		b[i].index = i;
		b[i].player = EMPTY;
		b[i].group = NULL;
		b[i].prev = NULL;
		b[i].next = NULL;
	}
	return st;
}

// Copy st0 --> st1
void state_copy(state* st0, state* st1) {
	st1->nextPlayer = st0->nextPlayer;
	st1->possibleKo = st0->possibleKo;

	st1->prisoners[1] = st0->prisoners[1];
	st1->prisoners[2] = st0->prisoners[2];

	dot* b0 = st0->board;
	dot* b1 = st1->board;

	int i;
	for (i = 0; i < COUNT; ++i) {
		b1[i] = b0[i];
	}
}

void state_destroy(state* st) {
	free(st);
}

#define ALREADY_COUNTED(i, j) already_counted[(i)*SIZE+(j)]
#define BOARD(i,j) board[(i)*SIZE+(j)]
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
void state_score(state* st, float* score) {
	dot* board = st->board;

	score[BLACK] = st->prisoners[BLACK];
	score[WHITE] = st->prisoners[WHITE] + KOMI;

	bool already_counted[COUNT];
	int i;
	int j;
	for (i = 0; i < SIZE; ++i) {
		for (j = 0; j < SIZE; ++j) {
			if (!already_counted[i*SIZE+j] && BOARD(i, j).player == EMPTY) {
				territory tr = {EMPTY, 0};
				count_territory(board, already_counted, i, j, &tr);
				if (tr.player == BLACK || tr.player == WHITE) {
					score[tr.player] += tr.area;
				}
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
	for (i = 0; i < SIZE; ++i) {
		wprintf(L"\n%c  ", int_char(i));
		for (j = 0; j < SIZE; ++j) {
			wprintf(L"%lc ", dot_char(i, j, BOARD(i, j).player));
		}
	}

	float score[3];

	double t0 = timer_now();
	state_score(st, score);
	double dt = timer_now() - t0;

	wprintf(L"\n\nScore: (%lc %.1f  %lc %.1f) [%.3f us]\n", color_char(BLACK), score[BLACK], color_char(WHITE), score[WHITE], dt*1e6);

	// Debug info about groups & ko
	/*if (st->possibleKo != NO_POSSIBLE_KO) {
		wprintf(L"Possible ko if ");
		move_print(&st->possibleKo);
		wprintf(L" captured\n");
	} else {
		wprintf(L"No possible ko\n");
	}

	dot* stone;
	t0 = timer_now();
	for (i = 0; i < COUNT; ++i) {
		stone = &board[i];
		if (stone->group && stone->group->anchor == stone) {
			group_print(stone->group);
		}
	}
	dt = timer_now() - t0;
	wprintf(L"Dumping groups took [%.3f us]\n", dt*1e6);*/
}

// Stone must already exist on board
// Assumes linked list never empty
void group_add_stone(group* gp, dot* stone, int liberties) {
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

	// a0->prev->next = b0->prev;
	// b0->prev->prev = a0->prev;
	// a0->prev = b0;
	// b0->next = a0;

	dot* at = a0->prev;
	dot* bt = b0->prev;
	a0->prev = bt;
	at->next = b0;
	b0->prev = at;
	bt->next = a0;

	a->length += b->length;
	a->freedoms += b->freedoms;

	return a;
}

// Change freedoms for neighboring groups of given color
void change_neighbors_freedoms_if_specific_color(dot* board, color player, int i, int j, int delta) {

	if (UP_OK    && BOARD(i-1, j).player == player) BOARD(i-1, j).group->freedoms += delta;
	if (LEFT_OK  && BOARD(i, j-1).player == player) BOARD(i, j-1).group->freedoms += delta;
	if (RIGHT_OK && BOARD(i, j+1).player == player) BOARD(i, j+1).group->freedoms += delta;
	if (DOWN_OK  && BOARD(i+1, j).player == player) BOARD(i+1, j).group->freedoms += delta;

	return;
}

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
		change_neighbors_freedoms_if_specific_color(board, enemy, i, j, +1);

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

// Returns whether move correctly parsed
bool move_parse(move* mv, char str[2]) {
	int i = -1;
	int j = -1;
	if (str[0] >= 'a' && str[0] <= 'z') {
		i = str[0] - 'a' + 10;
	} else if (str[0] >= 'A' && str[0] <= 'z') {
		i = str[0] - 'A' + 36;
	} else if (str[0] >= '0' && str[0] <= '9') {
		i = str[0] - '0';
	}
	if (str[1] >= 'a' && str[1] <= 'z') {
		j = str[1] - 'a' + 10;
	} else if (str[1] >= 'A' && str[1] <= 'Z') {
		j = str[1] - 'A' + 36;
	} else if (str[1] >= '0' && str[1] <= '9') {
		j = str[1] - '0';
	}

	if (i < 0 || i >= SIZE || j < 0 || j >= SIZE) {
		return false;
	}

	*mv = i * SIZE + j;
	return true;
}

// str must be a wchar_t[3]
void move_sprint(wchar_t str[3], move* mv) {
	int i = *mv/SIZE;
	int j = *mv%SIZE;
	swprintf(str, 3, L"%c%c", int_char(i), int_char(j));
}

void move_print(move* mv) {
	wchar_t str[3];
	move_sprint(str, mv);
	wprintf(str);
}

// Param move_list must be move[COUNT]
// Returns number of possible moves
int go_get_legal_moves(state* st, move* move_list) {
	int num = 0;
	move mv;
	int i;
	for (i = 0; i < COUNT; ++i) {
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
		tmp = randi(0, COUNT);
		rand_searches++;
	} while ((rand_searches < timeout) && (go_move_play(st, &tmp) != SUCCESS));

	wprintf(L"Tried %d random moves\n", rand_searches);

	if (rand_searches < timeout) {
		*mv = tmp;
		return SUCCESS;
	}

	// Few moves remaining; look for them
	wprintf(L"Random play timed out; going systematic\n");
	int move_count = go_get_legal_moves(st, move_list);
	if (move_count > 0) {
		tmp = move_list[randi(0, move_count)];
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

bool is_neighbor_enemy_dead(dot* board, color enemy, int i, int j) {
	return (BOARD(i, j).player == enemy) && (BOARD(i, j).group->freedoms == 0);
}

// Destroys enemy group at (i, j) if dead, return number captured
int remove_dead_neighbor_enemy(dot* board, color enemy, int i, int j) {
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

bool has_living_friendlies(dot* board, color friendly, int i, int j) {
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

void merge_with_every_friendly(dot* board, color friendly, int i, int j, int liberties) {
	group* gp0 = create_lone_group(&BOARD(i, j), friendly, liberties);

	if (UP_OK    && BOARD(i-1, j).player == friendly) gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i-1, j).group);
	if (LEFT_OK  && BOARD(i, j-1).player == friendly) gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i, j-1).group);
	if (RIGHT_OK && BOARD(i, j+1).player == friendly) gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i, j+1).group);
	if (DOWN_OK  && BOARD(i+1, j).player == friendly) gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i+1, j).group);

	/*// Maybe better performance? But more spaghetti
	bool part_of_group = false;
	group* gp0 = NULL;
	if (UP_OK && BOARD(i-1, j).player == friendly) {
		if (!part_of_group) {
			gp0 = BOARD(i-1, j).group;
			group_add_stone(gp0, &BOARD(i, j), liberties);
		} else {
			gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i-1, j).group);
		}
	}
	if (LEFT_OK && BOARD(i, j-1).player == friendly) {
		if (!part_of_group) {
			gp0 = BOARD(i, j-1).group;
			group_add_stone(gp0, &BOARD(i, j), liberties);
		} else {
			gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i, j-1).group);
		}
	}
	if (RIGHT_OK && BOARD(i, j+1).player == friendly) {
		if (!part_of_group) {
			gp0 = BOARD(i, j+1).group;
			group_add_stone(gp0, &BOARD(i, j), liberties);
		} else {
			gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i, j+1).group);
		}
	}
	if (DOWN_OK && BOARD(i+1, j).player == friendly) {
		if (!part_of_group) {
			gp0 = BOARD(i+1, j).group;
			group_add_stone(gp0, &BOARD(i, j), liberties);
		} else {
			gp0 = group_merge_and_destroy_smaller(gp0, BOARD(i+1, j).group);
		}
	}*/
}

// State is unchanged at the end (but it can change during execution)
bool go_move_legal(state* st, move* mv_ptr) {
	move mv = *mv_ptr;
	dot* board = st->board;
	color friendly = st->nextPlayer;
	color enemy = (friendly == BLACK) ? WHITE : BLACK;

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

	// If enemy killed, move is legal
	change_neighbors_freedoms_if_specific_color(board, enemy, i, j, -1);

	bool enemy_died = false;
	if      (UP_OK    && is_neighbor_enemy_dead(board, enemy, i-1, j)) enemy_died = true;
	else if (LEFT_OK  && is_neighbor_enemy_dead(board, enemy, i, j-1)) enemy_died = true;
	else if (RIGHT_OK && is_neighbor_enemy_dead(board, enemy, i, j+1)) enemy_died = true;
	else if (DOWN_OK  && is_neighbor_enemy_dead(board, enemy, i+1, j)) enemy_died = true;

	change_neighbors_freedoms_if_specific_color(board, enemy, i, j, +1);
	if (enemy_died) {
		return true;
	}

	// If has liberty, move is legal
	if (UP_OK    && BOARD(i-1, j).player == EMPTY) return true;
	if (LEFT_OK  && BOARD(i, j-1).player == EMPTY) return true;
	if (RIGHT_OK && BOARD(i, j+1).player == EMPTY) return true;
	if (DOWN_OK  && BOARD(i+1, j).player == EMPTY) return true;

	// Look for living friendly neighbors
	change_neighbors_freedoms_if_specific_color(board, friendly, i, j, -1);
	if (has_living_friendlies(board, friendly, i, j)) {
		change_neighbors_freedoms_if_specific_color(board, friendly, i, j, +1);
		return true;
	}

	// No living friendly neighbors
	change_neighbors_freedoms_if_specific_color(board, friendly, i, j, +1);
	return false;
}

// FIXME A bug is in here somewhere; try a 9x9 game & play on diagonals only
play_result go_move_play(state* st, move* mv_ptr) {
	move mv = *mv_ptr;
	dot* board = st->board;
	color friendly = st->nextPlayer;
	color enemy = (friendly == BLACK) ? WHITE : BLACK;

	if (mv < 0 || mv >= COUNT) {
		return FAIL_BOUNDS;
	}

	int i = mv / SIZE;
	int j = mv % SIZE;

	if (BOARD(i, j).player != EMPTY) {
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

	// Check neighbors for dead enemies
	change_neighbors_freedoms_if_specific_color(board, enemy, i, j, -1);

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

	// Look for dead friendly neighbors
	change_neighbors_freedoms_if_specific_color(board, friendly, i, j, -1);

	// Look for illegal move or lone-stone cases
	bool merge_with_friendlies = true;
	if (!has_living_friendlies(board, friendly, i, j)) {
		if (liberties == 0) {
			change_neighbors_freedoms_if_specific_color(board, friendly, i, j, +1);
			change_neighbors_freedoms_if_specific_color(board, enemy, i, j, +1);
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
