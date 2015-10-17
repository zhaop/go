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
#define BOARD(i,j) board[i*SIZE+j]

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

// Does not do freedom calculations
// Only adds stone before anchor in circular doubly linked list
// Assumes linked list never empty
void group_add_stone(group* gp, dot* stone) {
	dot* anchor = gp->anchor;
	stone->prev = anchor->prev;
	stone->next = anchor;
	anchor->prev->next = stone;
	anchor->prev = stone;
	++gp->length;
}

// Merges b into a
// Caller must destroy b afterwards
void group_merge(group* a, group* b) {
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

// Assumes (i, j) are valid coordinates
void decrement_neighbor_enemy_freedom(dot* board, color enemy, int i, int j) {
	if (BOARD(i, j).player == enemy) {
		--BOARD(i, j).group->freedoms;
	}
}

void check_neighbor_enemy_dead(dot* board, color enemy, int i, int j) {
	if ( (BOARD(i, j).player == enemy) && (BOARD(i, j).group->freedoms == 0)) {
		// Kill group
	}
}

#define UP_OK i >= 1
#define LEFT_OK j >= 1
#define RIGHT_OK j <= SIZE-2
#define DOWN_OK i <= SIZE-2

bool go_move_play(state* st, move* mv_ptr) {
	move mv = *mv_ptr;
	dot* b = st->board;
	color friendly = st->nextPlayer;
	color enemy = (friendly == BLACK) ? WHITE : BLACK;

	int i = mv / SIZE;
	int j = mv % SIZE;

	// Check neighbors for killed enemies
	// Decrease every enemy neighbor's group's freedom by 1 (may decrement more than once)
	// Check each enemy neighbor for deadness
	// If no dead, continue
	if (UP_OK)    decrement_neighbor_enemy_freedom(b, enemy, i-1, j);
	if (LEFT_OK)  decrement_neighbor_enemy_freedom(b, enemy, i, j-1);
	if (RIGHT_OK) decrement_neighbor_enemy_freedom(b, enemy, i, j+1);
	if (DOWN_OK)  decrement_neighbor_enemy_freedom(b, enemy, i+1, j);

	if (UP_OK)    check_neighbor_enemy_dead(b, enemy, i-1, j);
	if (LEFT_OK)  check_neighbor_enemy_dead(b, enemy, i, j-1);
	if (RIGHT_OK) check_neighbor_enemy_dead(b, enemy, i, j+1);
	if (DOWN_OK)  check_neighbor_enemy_dead(b, enemy, i+1, j);

	b[mv].player = st->nextPlayer;
	st->nextPlayer = enemy;

	return true;
}
