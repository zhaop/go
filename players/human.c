#include <stdio.h>

#include "players/human.h"

// params is ignored for human play
move_result human_play(player* self, state* st, move* mv) {
	self = self;	// @gcc pls dont warn kthx
	while (1) {
		wprintf(L"Your move: %lc ", color_char(st->nextPlayer));
		
		char mv_in[5];	// TODO DANGER BUFFER OVURFLURW
		scanf("%5c", mv_in);
		fpurge(stdin);

		wprintf(L"Got this: %c%c%c%c%c\n", mv_in[0], mv_in[1], mv_in[2], mv_in[3], mv_in[4]);

		if (!move_parse(mv, mv_in)) {
			wprintf(L"Invalid input\n");
		} else if (!chess_is_move_legal(st, mv)) {
			wprintf(L"Move (%d) is illegal\n", *mv);
		} else {
			move_result result = chess_play_move(st, mv);

			if (result != SUCCESS) {
				wprintf(L"Unsuccessful move: ");
				chess_print_move_result(result);
			} else {
				return result;
			}
		}
	}
}

