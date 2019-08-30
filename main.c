#include <locale.h>
#include <unistd.h>
#include <wchar.h>
#include "go.h"
#include "players/human.h"
#include "players/teresa.h"
#include "utils.h"

/*
Console mode:
No reply if success; prints error otherwise.

Interpreter errors:
- !command
- !syntax

Commands supported:

- ?
  Print commands.

- c
  Reset game state.

- k %f
  Set komi to arg.
  Errors:
  - !invalid

- p 1|2 %c%c
  Play move %c%c as player 1|2. %c%c parseable by move_parse.
  Errors:
  - !player
  - !move
  - !illegal
  - !play %d
    %d is move_result ("error code")

- g 1|2
  Calculate a move for the next player & print it (%c%c).
  Errors: 
  - !player
  - !play %d
    %d is move_result ("error code")

- p
  Draw the current state.

- q
  Quit
*/
int console_main() {
	wprintf(L"Go console mode\n");

	state* st = state_create();

	teresa_params teresap = {500000, 0.5, 1.1, NULL, NULL};
	player teresa = {"genmove", &teresa_play, &teresa_observe, &teresap};

	while (true) {
		wprintf(L"> ");

		int result;

		char input[256];
		fgets(input, 255, stdin);
		input[255] = '\0';

		char line[256];
		result = sscanf(input, "%[^\n]%*c", line);
		line[255] = '\0';

		char command = line[0];

		switch (command) {
			case '\0':
				continue;
				break;
			case '?':
				wprintf(L"?       Display help\n");
				wprintf(L"c       Reset the game state\n");
				wprintf(L"d       Print a drawing of the current state\n");
				wprintf(L"dd      Print debug infos on the current state\n");
				wprintf(L"k 6.5   Set komi to 6.5\n");
				wprintf(L"p 1 8b  Play move 8b as Black (player 1)\n");
				wprintf(L"g 2     Calculate a move for White (player 2)\n");
				wprintf(L"q       Quit\n");
				break;
			case 'c':
			case 'q':
			case 'v':
				break;
			case 'd':
				if (line[1] != '\0' && line[1] != 'd') {
					wprintf(L"!syntax\n");
					continue;
				}
				break;
			case 'k':
			case 'p':
			case 'g':
				if (line[1] != ' ') {
					wprintf(L"!syntax\n");
					continue;
				}
				break;
			default:
				wprintf(L"!command\n");
				continue;
				break;
		}

		switch (command) {
			case 'c': {
				float komi = st->komi;
				state_destroy(st);

				st = state_create();
				st->komi = komi;
				break;
			}
			case 'd': {
				if (line[1] == '\0') {
					state_print(st);
				} else if (line[1] == 'd') {
					state_dump(st);
				}
				break;
			}
			case 'k': {
				float komi;
				result = sscanf(line + 2, "%f", &komi);

				if (result != 1) {
					wprintf(L"!syntax\n");
					continue;
				}

				st->komi = komi;
				break;
			}
			case 'p': {
				int player_in;
				char mv_in[2];
				char test;
				result = sscanf(line + 2, "%d %c%c%c", &player_in, mv_in, mv_in + 1, &test);

				if (result != 3) {
					wprintf(L"!syntax\n");
					continue;
				}

				if (player_in != 1 && player_in != 2) {
					wprintf(L"!syntax\n");
					continue;
				}

				color player = (player_in == 1) ? BLACK : WHITE;
				if (player != st->nextPlayer) {
					// TODO Violates GTP: color "should" not be constrained
					wprintf(L"!player\n");
					continue;
				}

				move mv;
				if (!move_parse(&mv, mv_in)) {
					wprintf(L"!move\n");
					continue;
				}

				if (!go_is_move_legal(st, &mv)) {
					wprintf(L"!illegal\n");
					continue;
				}

				// play(move)
				move_result mv_result = go_play_move(st, &mv);
				if (mv_result != SUCCESS) {
					wprintf(L"!play %d\n", mv_result);
					continue;
				}

				teresa.observe(&teresa, st, player, &mv);
				break;
			}
			case 'g': {
				int player_in;
				result = sscanf(line + 2, "%d", &player_in);

				if (player_in != 1 && player_in != 2) {
					wprintf(L"!syntax\n");
					continue;
				}

				color player = (player_in == 1) ? BLACK : WHITE;
				if (player != st->nextPlayer) {
					// TODO Violates GTP: color "should" not be constrained
					wprintf(L"!player\n");
					continue;
				}

				move mv;
				move_result mv_result = teresa.play(&teresa, st, &mv);
				if (mv_result != SUCCESS) {
					wprintf(L"!play %d\n", mv_result);
					continue;
				}

				move_print(&mv);
				wprintf(L"\n");
				break;
			}
			case 'q': {
				return 0;
				break;
			}
			default:
				break;
		}
	}
	return 0;
}

int game_main() {
	long double t0, dt;

	state* st = state_create();
	int t = 0;

	player human = {"You", &human_play, NULL, NULL};

	// karl_params karlp = {80000};
	// player karl = {"Karl", &karl_play, NULL, &karlp};

	teresa_params teresap = {20000, 0.5, 1.1, NULL, NULL};
	player teresa = {"Teresa", &teresa_play, &teresa_observe, &teresap};

	// teresa_old_node** r = &(teresap.old_root);

	teresa_params teresa2p = {20000, 0.5, 1.1, NULL, NULL};
	player teresa2 = {"Teresa 2", &teresa_play, &teresa_observe, &teresa2p};

	// teresa_old_node** r2 = &(teresa2p.old_root);

	player* players[3];
	players[BLACK] = &teresa;
	players[WHITE] = &teresa2;

	long double sum_dt = 0;

	while (1) {
		if (go_is_game_over(st)) {
			state_print(st);
			break;
		}

		color pl_color = st->nextPlayer;
		player* pl = players[pl_color];
		player* opponent = players[color_opponent(pl_color)];

		++t;

		wprintf(L"%lc %d %s now playing\n", color_char(pl_color), t, pl->name);
		state_print(st);

		t0 = timer_now();
		move mv;
		move_result result = pl->play(pl, st, &mv);
		dt = timer_now() - t0;

		if (result != SUCCESS) {
			wprintf(L"%s has no more moves [%.2Lf us]\n", pl->name, dt*1e6);
			return 0;
		}

		wprintf(L"%lc %d %s played ", color_char(pl_color), t, pl->name);
		move_print(&mv);
		wprintf(L" [%.0Lf ms]\n", dt*1e3);

		sum_dt += dt;

		if (opponent->observe) {
			t0 = timer_now();
			opponent->observe(opponent, st, pl_color, &mv);
			dt = timer_now() - t0;
			wprintf(L"Opponent observed the move [%.0Lf us]\n", dt);
		}

		wprintf(L"\n");

		// TODO Uncomment once adapted to new node structure
		// if (*r) g2(*r, "graph2.json", 8, 8);
		// if (*r2) g2(*r2, "graph4.json", 8, 8);
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

	wprintf(L"Average thinking time: %.2Lf ms\n", sum_dt/t*1e3);

	return 0;
}

int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "");
	seed_rand_once();

	// Parse command line arguments
	int opt;
	bool console = false;
	while ((opt = getopt(argc, argv, "c")) != -1) {
		switch (opt) {
			case 'c':
				console = true;
				break;
			default:
				fwprintf(stderr, L"Usage: %s [-c]\n", argv[0]);
				return 1;
				break;
		}
	}

	if (console) {
		return console_main();
	} else {
		return game_main();
	}
}
