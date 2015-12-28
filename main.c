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
