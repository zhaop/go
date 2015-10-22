#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <wchar.h>
#include "go.h"
#include "utils.h"

// Have bot play one move given current state
bool bot_play(state* st, move* mv) {
	move move_list[COUNT];
	return go_move_play_random(st, mv, move_list);
}

int main(/*int argc, char* argv[]*/) {

	setlocale(LC_ALL, "");
	seed_rand_once();

	state* st = state_create();
	if (!st) return -1;

	move* mv = move_create();
	if (!mv) return -1;

	char mv_in[2];

	color human_player = BLACK;

	bool is_legal;
	bool result;
	long double t0;
	long double dt;
	long double t_think;
	while (1) {
		if (st->nextPlayer == human_player) {
			state_print(st);
			wprintf(L"Your move: %lc ", color_char(st->nextPlayer));

			t0 = timer_now();
			scanf("%s", mv_in);
			result = move_parse(mv, mv_in);
			dt = timer_now() - t0;
			t_think = dt;

			if (!result) {
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

			if (!result) {
				wprintf(L"Invalid move\n");
				continue;
			}

			wprintf(L"%lc (You) played ", color_char((st->nextPlayer == BLACK) ? WHITE : BLACK));
			move_print(mv);
			wprintf(L" (%.2Lf s thinking, %.2Lf us playing)\n", t_think, dt*1e6);
		} else {
			state_print(st);
			wprintf(L"Randy's move: %lc ", color_char(st->nextPlayer));

			t0 = timer_now();
			result = bot_play(st, mv);
			dt = timer_now() - t0;

			move_print(mv);
			wprintf(L"\n");

			if (!result) {
				wprintf(L"Randy has no more moves [%.2Lf us]\n", dt*1e6);
				return 0;
			}

			wprintf(L"%lc (Randy) played ", color_char((st->nextPlayer == BLACK) ? WHITE : BLACK));
			move_print(mv);
			wprintf(L" [%.2Lf us]\n", dt*1e6);
		}
	}

	return 0;
}
