#include <locale.h>
#include <stdio.h>
#include <unistd.h>
#include <wchar.h>
#include "go.h"
#include "utils.h"

int main(/*int argc, char* argv[]*/) {

	setlocale(LC_ALL, "");
	seed_rand_once();

	state* st = state_create();
	if (!st) return -1;

	move* mv = move_create();
	if (!mv) return -1;

	char mv_in[2];

	bool is_legal;
	bool result;
	long double t0;
	long double dt;
	long double t_think;
	while (1) {
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

		wprintf(L"%lc plays ", color_char((st->nextPlayer == BLACK) ? WHITE : BLACK));
		move_print(mv);
		wprintf(L" (%.2Lf s thinking, %.2Lf us playing)\n", t_think, dt*1e6);
	}

	return 0;
}
