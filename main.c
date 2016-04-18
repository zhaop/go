#include <locale.h>
#include <wchar.h>
#include "go.h"
#include "players.h"
#include "utils.h"

int main() {
	setlocale(LC_ALL, "");
	seed_rand_once();

	long double t0, dt;

	state* st = state_create();
	int t = 0;

	player human = {"You", &human_play, NULL, NULL};

	// karl_params karlp = {80000};
	// player karl = {"Karl", &karl_play, NULL, &karlp};

	teresa_params teresap = {80000, 0.5, NULL, NULL};
	player teresa = {"Teresa", &teresa_play, &teresa_observe, &teresap};

	teresa_node** r = &(teresap.root);

	teresa_params teresa2p = {80000, 0.5, NULL, NULL};
	player teresa2 = {"Teresa 2", &teresa_play, &teresa_observe, &teresa2p};

	teresa_node** r2 = &(teresa2p.root);

	player* players[3];
	players[BLACK] = &teresa;
	players[WHITE] = &human;

	while (1) {
		if (go_is_game_over(st)) {
			state_print(st);
			break;
		}

		color pl_color = st->nextPlayer;
		player* pl = players[pl_color];
		player* opponent = players[color_opponent(pl_color)];

		wprintf(L"%lc %s now playing\n", color_char(pl_color), pl->name);
		state_print(st);

		t0 = timer_now();
		move mv;
		move_result result = pl->play(pl, st, &mv);
		dt = timer_now() - t0;

		if (result != SUCCESS) {
			wprintf(L"%s has no more moves [%.2Lf us]\n", pl->name, dt*1e6);
			return 0;
		}

		wprintf(L"%lc %s played ", color_char(pl_color), pl->name);
		move_print(&mv);
		wprintf(L" [%.0Lf ms]\n", dt*1e3);

		if (opponent->observe) {
			t0 = timer_now();
			opponent->observe(opponent, st, pl_color, &mv);
			dt = timer_now() - t0;
			wprintf(L"Opponent observed the move [%.0Lf us]\n", dt);
		}

		wprintf(L"\n");
		++t;

		if (*r) g2(*r, "graph2.json", 8, 8);
		if (*r2) g2(*r2, "graph4.json", 8, 8);
	}

	color winner = state_winner(st);
	color loser = color_opponent(winner);
	if (st->passes == 2) {
		float final_score[3];
		state_score(st, final_score, false);
		wprintf(L"Game over: %lc wins by %.1f points.\n", color_char(winner), final_score[winner] - final_score[loser]);
	} else if (st->passes == 3) {
		wprintf(L"Game over: %lc wins by resignation\n", color_char(winner));
	}

	return 0;
}
