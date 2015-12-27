#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <wchar.h>
#include "go.h"
#include "utils.h"

typedef struct {
	const char* name;
	move_result (*const play)(state*, move*, void*);
	void* params;
} player;

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
move_result randy_play(state* st, move* mv) {
	move move_list[NMOVES];
	return go_play_random_move(st, mv, move_list);
}

typedef struct {
	int N;
} karl_params;

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

int main() {
	setlocale(LC_ALL, "");
	seed_rand_once();

	long double t0, dt;

	state* st = state_create();
	int t = 0;

	player human = {"You", &human_play, NULL};

	karl_params karlp = {80000};
	player karl = {"Karl", &karl_play, &karlp};

	player* players[3];
	players[BLACK] = &karl;
	players[WHITE] = &human;

	while (1) {
		player* pl = players[st->nextPlayer];
		wprintf(L"%lc %s now playing\n", color_char(st->nextPlayer), pl->name);

		state_print(st);
		if (go_is_game_over(st)) {
			break;
		}

		t0 = timer_now();
		move mv;
		move_result result = pl->play(st, &mv, pl->params);
		dt = timer_now() - t0;

		if (result != SUCCESS) {
			wprintf(L"%s has no more moves [%.2Lf us]\n", pl->name, dt*1e6);
			return 0;
		}

		wprintf(L"%lc %s played ", color_char(color_opponent(st->nextPlayer)), pl->name);
		move_print(&mv);
		wprintf(L" [%.0Lf ms]\n", dt*1e3);

		wprintf(L"\n");
		++t;
	}

	float final_score[3];
	state_score(st, final_score, false);
	color winner = (final_score[BLACK] > final_score[WHITE]) ? BLACK : WHITE;
	color loser = (winner == BLACK) ? WHITE : BLACK;
	wprintf(L"Game over: %lc wins by %.1f points.\n", color_char(winner), final_score[winner] - final_score[loser]);

	return 0;
}
