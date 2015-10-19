#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
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

char color_char(color player) {
	if (player == BLACK) {
		return 'X';
	} else if (player == WHITE) {
		return 'O';
	} else {
		return '.';
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

// row i, col j
#define BOARD(i,j) board[(i)*SIZE+(j)]

void state_print(state* st) {
	int i;
	int j;
	color nextPlayer = st->nextPlayer;
	int* prisoners = st->prisoners;
	dot* board = st->board;

	printf("   ");
	for (i = 0; i < SIZE; ++i) {
		printf("%c ", int_char(i));
	}
	printf("\n   ");
	for (i = 0; i < SIZE; ++i) {
		printf("  ");
	}
	printf("(%d%c%c  %d%c%c)",
		prisoners[BLACK], color_char(BLACK), (nextPlayer != BLACK ? '*' : ' '),
		prisoners[WHITE], color_char(WHITE), (nextPlayer != WHITE ? '*' : ' '));
	for (i = 0; i < SIZE; ++i) {
		printf("\n%c  ", int_char(i));
		for (j = 0; j < SIZE; ++j) {
			printf("%c ", color_char(BOARD(i, j).player));
		}
	}
	printf("\n");
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
	} while (stone->next != b0);

	a0->prev->next = b0->prev;
	b0->prev->prev = a0->prev;
	a0->prev = b0;
	b0->next = a0;

	a->length += b->length;
	a->freedoms += b->freedoms;

	return a;
}

#define UP_OK    (i >= 1)
#define LEFT_OK  (j >= 1)
#define RIGHT_OK (j <= SIZE-2)
#define DOWN_OK  (i <= SIZE-2)

// Change freedoms for neighboring groups of given color
void change_neighbors_freedoms_if_specific_color(dot* board, color player, int i, int j, int delta) {

	if (UP_OK    && BOARD(i-1, j).player == player) BOARD(i-1, j).group->freedoms += delta;
	if (LEFT_OK  && BOARD(i, j-1).player == player) BOARD(i, j-1).group->freedoms += delta;
	if (RIGHT_OK && BOARD(i, j+1).player == player) BOARD(i, j+1).group->freedoms += delta;
	if (DOWN_OK  && BOARD(i+1, j).player == player) BOARD(i+1, j).group->freedoms += delta;

	return;
}

// Removes all of a group's stones from the board
// Caller must destroy gp afterwards
void group_kill_stones(dot* board, group* gp) {
	int i;
	int j;
	dot* gp0 = gp->anchor;
	color enemy = (gp0->player == BLACK) ? WHITE : BLACK;
	dot* stone = gp0;
	dot* tmp_next;
	do {
		tmp_next = stone->next;

		i = stone->index / SIZE;
		j = stone->index % SIZE;
		change_neighbors_freedoms_if_specific_color(board, enemy, i, j, +1);

		stone->player = EMPTY;
		stone->group = NULL;
		stone->prev = NULL;
		stone->next = NULL;

		stone = tmp_next;
	} while (stone != gp0);
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

void move_print(move* mv) {
	int i = *mv/SIZE;
	int j = *mv%SIZE;
	printf("%c%c", int_char(i), int_char(j));
}

void go_move_random(move* mv, state* st) {
	move tmp;
	do {
		tmp = randi(0, COUNT);
	} while (st->board[tmp].player != EMPTY);
	*mv = tmp;
}

bool go_move_legal(state* st, move* mv) {
	dot* b = st->board;

	if (*mv > COUNT) {
		return false;
	}

	if (b[*mv].player == EMPTY) {
		return true;
	}
	return false;
}

// Destroys enemy group at (i, j) if dead
bool remove_dead_neighbor_enemy(dot* board, color enemy, int i, int j) {
	if ( (BOARD(i, j).player == enemy) && (BOARD(i, j).group->freedoms == 0)) {
		group* gp = BOARD(i, j).group;
		group_kill_stones(board, gp);
		group_destroy(gp);
		return true;
	}
	return false;
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

/*
merge_with_every_friendly(board, i, j):
		bool part_of_group = false
		group gp0
		for each friendly neighbor:
			if (part_of_group)
				group_merge(gp0, neighbor.group)
			else
				gp0 = neighbor.group
				group_add_stone(gp0, BOARD(i, j), liberties)
*/
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

bool go_move_play(state* st, move* mv_ptr) {
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

	// Check neighbors for killed enemies
	change_neighbors_freedoms_if_specific_color(board, enemy, i, j, -1);

	// If dead enemy, kill group
	// TODO Count how many are killed (count prisoners, and also ko!)
	if (UP_OK)    remove_dead_neighbor_enemy(board, enemy, i-1, j);
	if (LEFT_OK)  remove_dead_neighbor_enemy(board, enemy, i, j-1);
	if (RIGHT_OK) remove_dead_neighbor_enemy(board, enemy, i, j+1);
	if (DOWN_OK)  remove_dead_neighbor_enemy(board, enemy, i+1, j);

	// Count own liberties
	int liberties = 0;
	if (UP_OK    && BOARD(i-1, j).player == EMPTY) ++liberties;
	if (LEFT_OK  && BOARD(i, j-1).player == EMPTY) ++liberties;
	if (RIGHT_OK && BOARD(i, j+1).player == EMPTY) ++liberties;
	if (DOWN_OK  && BOARD(i+1, j).player == EMPTY) ++liberties;

	/*
	int liberties = count_liberties()
	decrement_all_friendlies_freedoms()
	if (!search_for_living_friendlies())	// Look for illegal or lone-stone cases
		if (!liberties)
			increment_all_neighbors_freedoms()
			return false	// Illegal
		else if (!search_for_dying_friendlies())
			make_lone_group(liberties)
			return true
	merge_with_every_friendly()
	*/
	// Look for dead friendly neighbors
	change_neighbors_freedoms_if_specific_color(board, friendly, i, j, -1);

	// Look for illegal move or lone-stone cases
	bool merge_with_friendlies = true;
	if (!has_living_friendlies(board, friendly, i, j)) {
		if (liberties == 0) {
			change_neighbors_freedoms_if_specific_color(board, friendly, i, j, +1);
			change_neighbors_freedoms_if_specific_color(board, enemy, i, j, +1);
			return false;	// Illegal
		} else if (!has_dying_friendlies(board, friendly, i, j)) {
			create_lone_group(&BOARD(i, j), friendly, liberties);
			merge_with_friendlies = false;
		}
	}

	if (merge_with_friendlies) {
		merge_with_every_friendly(board, friendly, i, j, liberties);
	}

	st->nextPlayer = enemy;

	return true;
}
