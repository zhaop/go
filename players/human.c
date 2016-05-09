#include <stdio.h>

#include "players/human.h"

// params is ignored for human play
move_result human_play(player* self, state* st, move* mv) {
	self = self;	// @gcc pls dont warn kthx
	while (1) {
		wprintf(L"Your move: %lc ", color_char(st->nextPlayer));
		
		char mv_in[2];	// TODO DANGER BUFFER OVURFLURW
		scanf("%s", mv_in);

		if (!move_parse(mv, mv_in)) {
			wprintf(L"Invalid input\n");
		} else if (!go_is_move_legal(st, mv)) {
			wprintf(L"Move is illegal\n");
		} else {
			move_result result = go_play_move(st, mv);

			if (result != SUCCESS) {
				wprintf(L"Unsuccessful move: ");
				go_print_move_result(result);
			} else {
				return result;
			}
		}
	}
}

