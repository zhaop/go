#include <stdio.h>

#include "go.h"

int main(int argc, char* argv[]) {

	state* st = state_create();
	if (!st) return -1;

	move* mv = move_create();
	if (!mv) return -1;

	char mv_in[2];
	
	while (1) {
		state_print(st);
		printf("Your move (%c plays): \n> ", st->nextPlayer == BLACK ? 'x' : 'o');
		scanf("%s", mv_in);
		if (!move_parse(mv, mv_in)) {
			printf("Invalid input\n");
			continue;
		}

		if (!go_move_play(st, mv)) {
			printf("Invalid move\n");
			continue;
		}
		printf("%c plays ", st->nextPlayer == BLACK ? 'x' : 'o');
		move_print(mv);
		printf("\n");
	}

	return 0;
}
