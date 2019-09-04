#include <locale.h>
#include <stdio.h>
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

- h %d
  Place %d handicap stones & print the list of vertices.
  Errors:
  - !syntax
  - !number
  - !board
  - !result

- k %f
  Set komi to arg.
  Errors:
  - !syntax

- p 1|2 %c%c
  Play move %c%c as player 1|2. %c%c parseable by move_parse.
  Errors:
  - !move
  - !illegal
  - !result

- g 1|2
  Calculate a move for the next player & print it (%c%c).
  Errors: 
  - !result

- p
  Draw the current state.

- q
  Quit
*/
void console_print_help(FILE* stream) {
	fwprintf(stream, L"?       Display help\n");
	fwprintf(stream, L"c       Reset the game state\n");
	fwprintf(stream, L"d       Print a drawing of the current state\n");
	fwprintf(stream, L"dd      Print debug infos on the current state\n");
	fwprintf(stream, L"dg      Print a GTP-compatible drawing of the current state\n");
	fwprintf(stream, L"h 3     Place 3 handicap stones at their predefined locations\n");
	fwprintf(stream, L"k 6.5   Set komi to 6.5\n");
	fwprintf(stream, L"p 1 8b  Play move 8b as Black (player 1)\n");
	fwprintf(stream, L"g 2     Calculate a move for White (player 2)\n");
	fwprintf(stream, L"q       Quit\n");
}

int console_main() {
	// Turn off output buffering so other scripts can interact with this console
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	fwprintf(stderr, L"Go console mode\n");
	console_print_help(stderr);

	state* st = state_create();

	teresa_params teresap = {500000, 0.5, 1.1, NULL, NULL};
	player teresa = {"genmove", &teresa_play, &teresa_observe, &teresap};

	while (true) {
		wprintf(L"> ");
		fflush(stdout);

		int result;

		char input[256];
		if (!fgets(input, 255, stdin)) {
			if (feof(stdin)) {
				wprintf(L"!feof\n");
				return 0;
			} else if (ferror(stdin)) {
				wprintf(L"!ferror\n");
				return 1;
			}
		}
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
			case 'c':
			case 'q':
			case 'v':
				break;
			case 'd':
				// d or dd
				switch (line[1]) {
					case '\0':
					case 'd':
					case 'g':
						break;
					default:
						wprintf(L"!command: d%c is not a command\n", line[1]);
						continue;
						break;
				}
				break;
			case 'h':
			case 'k':
			case 'p':
			case 'g':
				if (line[1] != ' ') {
					wprintf(L"!syntax: Missing 1 space after command\n");
					continue;
				}
				break;
			default:
				wprintf(L"!command: %c is not a command\n", command);
				continue;
				break;
		}

		switch (command) {
			case '?': {
				console_print_help(stdout);
				break;
			}
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
				} else if (line[1] == 'g') {
					state_print_gtp(st);
				}
				break;
			}
			case 'h': {
				int numStones;
				result = sscanf(line + 2, "%d", &numStones);

				if (result != 1) {
					wprintf(L"!syntax: number of stones is not an int\n");
					continue;
				}

				if (numStones < 1 || numStones > 9) {
					wprintf(L"!number: number of stones must be between 1 and 9 inclusive\n");
					continue;
				}

				if (WIDTH != HEIGHT) {
					wprintf(L"!board: not square\n");
					continue;
				}

				dot* board = st->board;
				bool isEmpty = true;
				for (int i = 0; i < HEIGHT && isEmpty; ++i) {
					for (int j = 0; j < WIDTH && isEmpty; ++j) {
						if (board[i*WIDTH+j].player != EMPTY) {
							isEmpty = false;
						}
					}
				}

				if (!isEmpty) {
					wprintf(L"!board: not empty\n");
					continue;
				}

				if (!go_place_fixed_handicap(st, numStones)) {
					wprintf(L"!result: placing stones not all successful\n");
					continue;
				}

				// Print space-delimited list of moves
				for (int i = 0; i < HEIGHT; ++i) {
					for (int j = 0; j < WIDTH; ++j) {
						if (board[i*WIDTH+j].player == BLACK) {
							move mv = i * WIDTH + j;
							move_print(&mv);
							wprintf(L" ");
						}
					}
				}
				wprintf(L"\n");
				break;
			}
			case 'k': {
				float komi;
				result = sscanf(line + 2, "%f", &komi);

				if (result != 1) {
					wprintf(L"!syntax: komi is not a float\n");
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
					wprintf(L"!syntax: only got %d out of 3 in (player, row, col)\n", result);
					continue;
				}

				if (player_in != 1 && player_in != 2) {
					wprintf(L"!syntax: player %d is not 1 or 2\n", player_in);
					continue;
				}

				color player = (player_in == 1) ? BLACK : WHITE;
				if (player != st->nextPlayer) {
					// Playing out-of-turn: change player & clear Teresa game tree
					// (Some engine probably reloading a past game state)
					st->nextPlayer = player;
					teresa_reset(&teresa);
				}

				move mv;
				if (!move_parse(&mv, mv_in)) {
					wprintf(L"!move: %c%c not a valid move in this configuration\n", mv_in[0], mv_in[1]);
					continue;
				}

				if (!go_is_move_legal(st, &mv)) {
					wprintf(L"!illegal\n");
					continue;
				}

				// play(move)
				move_result mv_result = go_play_move(st, &mv);
				if (mv_result != SUCCESS) {
					wprintf(L"!result: move_result is %d = ", mv_result);
					go_print_move_result(mv_result);
					continue;
				}

				teresa.observe(&teresa, st, player, &mv);
				break;
			}
			case 'g': {
				int player_in;
				result = sscanf(line + 2, "%d", &player_in);

				if (result != 1) {
					wprintf(L"!syntax: player is not valid\n");
					continue;
				}

				if (player_in != 1 && player_in != 2) {
					wprintf(L"!syntax: player %d is not 1 or 2\n", player_in);
					continue;
				}

				color player = (player_in == 1) ? BLACK : WHITE;
				if (player != st->nextPlayer) {
					// Playing out-of-turn: change player & clear Teresa game tree
					st->nextPlayer = player;
					teresa_reset(&teresa);
				}

				move mv;
				move_result mv_result = teresa.play(&teresa, st, &mv);
				if (mv_result != SUCCESS) {
					wprintf(L"!result: move_result is %d = ", mv_result);
					go_print_move_result(mv_result);
					continue;
				}

				move_print(&mv);
				break;
			}
			case 'q': {
				return 0;
				break;
			}
			default:
				break;
		}

		wprintf(L"\n");
	}
	return 0;
}

int game_main() {
	long double t0, dt;

	state* st = state_create();
	st->komi = 6.5;

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
			wprintf(L"%s has no more moves [%.2Lf ms]\n", pl->name, dt/1e6);
			return 0;
		}

		wprintf(L"%lc %d %s played ", color_char(pl_color), t, pl->name);
		move_print(&mv);
		wprintf(L" [%.0Lf ms]\n", dt/1e6);

		sum_dt += dt;

		if (opponent->observe) {
			t0 = timer_now();
			opponent->observe(opponent, st, pl_color, &mv);
			dt = timer_now() - t0;
			wprintf(L"Opponent observed the move [%.0Lf ms]\n", dt/1e6);
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
		state_score(st, final_score, true);
		wprintf(L"Game over: %lc wins by %.1f points.\n", color_char(winner), final_score[winner] - final_score[loser]);
	} else if (st->passes == 3) {
		wprintf(L"Game over: %lc wins by resignation\n", color_char(winner));
	}

	wprintf(L"Average thinking time: %.2Lf ms\n", sum_dt/t/1e6);

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
