#include <stdio.h>
#include <wchar.h>

#include "players.h"
#include "utils.h"

// params is ignored for human play
move_result human_play(state* st, move* mv, void* params) {
	params = params;	// @gcc pls dont warn kthx
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

// Have bot play one move given current state
move_result randy_play(state* st, move* mv, void* params) {
	params = params;
	move move_list[NMOVES];
	return go_play_random_move(st, mv, move_list);
}

move_result karl_play(state* st, move* mv, void* params) {
	int N = ((karl_params*) params)->N;

	state test_st;

	move legal_moves[NMOVES];
	int num_moves = go_get_legal_moves(st, legal_moves);

	if (num_moves == 1) {
		*mv = legal_moves[0];
		return go_play_move(st, mv);
	}

	color me = st->nextPlayer;
	color notme = (me == BLACK) ? WHITE : BLACK;

	int win[NMOVES];
	int lose[NMOVES];
	double pwin[NMOVES];

	int i;
	for (i = 0; i < num_moves; ++i) {
		win[i] = lose[i] = 0;
		pwin[i] = 0.0;
	}

	// Do playouts
	for (int i = 0; i < N; ++i) {
		state_copy(st, &test_st);

		int starting_move_idx = RANDI(0, num_moves);
		move starting_move = legal_moves[starting_move_idx];
		go_play_move(&test_st, &starting_move);

		playout_result result;
		go_play_out(&test_st, &result);

		if (result.winner == me) {
			++win[starting_move_idx];
		} else if (result.winner == notme) {
			++lose[starting_move_idx];
		}
	}

	// Calculate chances
	double best_pwin = 0;
	move best_pwin_move;
	for (i = 0; i < num_moves; ++i) {

		if (win[i] + lose[i] == 0) {
			pwin[i] = 0;
		} else {
			pwin[i] = (double) win[i] / (win[i] + lose[i]);
		}

		if (pwin[i] > best_pwin) {
			best_pwin = pwin[i];
			best_pwin_move = legal_moves[i];
		};
	}

	wprintf(L"\nDecision heatmap\n");
	go_print_heatmap(st, legal_moves, pwin, num_moves);

	wprintf(L"Going with ");
	move_print(&best_pwin_move);
	wprintf(L" at %.1f%%\n", best_pwin*100);

	*mv = best_pwin_move;
	return go_play_move(st, mv);
}
