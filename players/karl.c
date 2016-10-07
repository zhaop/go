#include "karl.h"
#include "utils.h"

move_result karl_play(player* self, state* st, move* mv) {
	int N = ((karl_params*) self->params)->N;

	state test_st;

	move reasonable_moves[NMOVES];
	int num_moves = chess_get_reasonable_moves(st, reasonable_moves);

	if (num_moves == 1) {
		*mv = reasonable_moves[0];
		return chess_play_move(st, mv);
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
		move starting_move = reasonable_moves[starting_move_idx];
		chess_play_move(&test_st, &starting_move);

		playout_result result;
		chess_play_out(&test_st, &result);

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
			best_pwin_move = reasonable_moves[i];
		};
	}

	wprintf(L"\nDecision heatmap\n");
	chess_print_heatmap(st, reasonable_moves, pwin, num_moves);

	wprintf(L"Going with ");
	move_print(&best_pwin_move);
	wprintf(L" at %.1f%%\n", best_pwin*100);

	*mv = best_pwin_move;
	return chess_play_move(st, mv);
}

