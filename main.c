#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <wchar.h>
#include "go.h"
#include "utils.h"

// Have bot play one move given current state
play_result randy_play(state* st, move* mv) {
	move move_list[COUNT+1];
	return go_move_play_random(st, mv, move_list);
}

/*
legal_moves = all possible moves
repeat N times:
	state2 = make a copy of current state
	pick one random next move
	play random moves until game over
	if game over:
		count score according to Chinese rules
		if I win:
			count as win
		else:
			count as lose
calculate probabilities of winning for each possible next move
find move with highest probability
play that move
*/
play_result karl_play(state* st, move* mv, int N) {
	move legal_moves[COUNT+1];
	int num_moves = go_get_legal_plays(st, legal_moves);

	if (num_moves == 1) {
		*mv = legal_moves[0];
		return go_move_play(st, mv);
	}

	color me = st->nextPlayer;
	color notme = (me == BLACK) ? WHITE : BLACK;

	int win[COUNT];
	int lose[COUNT];
	int draw[COUNT];
	double pwin[COUNT];

	int i;
	for (i = 0; i < num_moves; ++i) {
		win[i] = lose[i] = draw[i] = 0;
		pwin[i] = 0.0;
	}

	state test_st;

	for (int i = 0; i < N; ++i) {
		state_copy(st, &test_st);

		int test_idx = RANDI(0, num_moves);
		move test_mv = legal_moves[test_idx];
		go_move_play(&test_st, &test_mv);

		move test_mv2;
		move test_move_list[COUNT+1];
		int t = 1;
		while (!go_is_game_over(&test_st)) {
			go_move_play_random(&test_st, &test_mv2, test_move_list);
			++t;
		}

		float test_score[3] = {0.0, 0.0, 0.0};
		state_score(&test_st, test_score, false);
		if (test_score[me] > test_score[notme]) {
			++win[test_idx];
		} else {
			++lose[test_idx];
		}
	}

	double best_pwin = 0;
	move best_move;
	for (i = 0; i < num_moves; ++i) {
		pwin[i] = (double) win[i] / (win[i] + lose[i] + 1);
if (win[i] + lose[i] == 0) {
			pwin[i] = 0;
		} else {
			pwin[i] = (double) win[i] / (win[i] + lose[i]);
		}

		wprintf(L"Move ");
		move_print(&legal_moves[i]);
		wprintf(L": +%d, -%d => %.1f%%\n", win[i], lose[i], pwin[i]*100);

		if (pwin[i] > best_pwin) {
			best_pwin = pwin[i];
			best_move = legal_moves[i];
		};
	}
	wprintf(L"Going with ");
	move_print(&best_move);
	wprintf(L" at %.1f%%\n", best_pwin*100);

	*mv = best_move;
	return go_move_play(st, mv);
}

int main(/*int argc, char* argv[]*/) {

	setlocale(LC_ALL, "");
	seed_rand_once();

	state* st = state_create();
	if (!st) return -1;

	move* mv = move_create();
	if (!mv) return -1;

	char mv_in[2];

	color human_player = WHITE;

	bool is_legal;
	bool is_parse_valid;
	play_result result;
	long double t0;
	long double dt;
	long double t_think;
	while (1) {
		if (st->nextPlayer == human_player) {
			state_print(st);
			if (go_is_game_over(st)) {
				break;
			}

			wprintf(L"Your move: %lc ", color_char(st->nextPlayer));

			t0 = timer_now();
			scanf("%s", mv_in);
			is_parse_valid = move_parse(mv, mv_in);
			dt = timer_now() - t0;
			t_think = dt;

			if (!is_parse_valid) {
				wprintf(L"Invalid input\n");
				continue;
			}

			t0 = timer_now();
			is_legal = go_move_legal(st, mv);
			dt = timer_now() - t0;
			wprintf(L"Move is %s [%.3Lf us]\n", (is_legal ? "legal" : "illegal"), dt*1e6);

			t0 = timer_now();
			result = go_move_play(st, mv);
			dt = timer_now() - t0;

			if (result != SUCCESS) {
				wprintf(L"Invalid move: ");
				switch (result) {
					case FAIL_GAME_ENDED:
						wprintf(L"Game ended\n");
						break;
					case FAIL_BOUNDS:
						wprintf(L"Out of bounds\n");
						break;
					case FAIL_OCCUPIED:
						wprintf(L"Occupied\n");
						break;
					case FAIL_KO:
						wprintf(L"Ko\n");
						break;
					case FAIL_SUICIDE:
						wprintf(L"Suicide\n");
						break;
					case FAIL_OTHER:
					default:
						wprintf(L"Other\n");
						break;
				}
				continue;
			}

			wprintf(L"%lc (You) played ", color_char((st->nextPlayer == BLACK) ? WHITE : BLACK));
			move_print(mv);
			wprintf(L" (%.2Lf s thinking, %.2Lf us playing)\n", t_think, dt*1e6);
		} else {
			state_print(st);
			if (go_is_game_over(st)) {
				break;
			}

			t0 = timer_now();
			result = karl_play(st, mv, 200000);
			dt = timer_now() - t0;

			wprintf(L"Karl's move: %lc ", color_char(st->nextPlayer));
			move_print(mv);
			wprintf(L"\n");

			if (result != SUCCESS) {
				wprintf(L"Karl has no more moves [%.2Lf us]\n", dt*1e6);
				return 0;
			}

			wprintf(L"%lc (Karl) played ", color_char((st->nextPlayer == BLACK) ? WHITE : BLACK));
			move_print(mv);
			wprintf(L" [%.0Lf ms]\n", dt*1e3);

			state_print(st);
			return 42;
		}
	}

	float final_score[3];
	state_score(st, final_score, false);
	color winner = (final_score[BLACK] > final_score[WHITE]) ? BLACK : WHITE;
	color loser = (winner == BLACK) ? WHITE : BLACK;
	wprintf(L"Game over: %lc wins by %.1f points.\n", color_char(winner), final_score[winner] - final_score[loser]);

	return 0;
}
