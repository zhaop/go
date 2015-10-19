#include <locale.h>
#include <stdio.h>
#include <wchar.h>
#include "go.h"

int main(/*int argc, char* argv[]*/) {

	setlocale(LC_ALL, "");

	state* st = state_create();
	if (!st) return -1;

	move* mv = move_create();
	if (!mv) return -1;

	char mv_in[2];
	
	while (1) {
		state_print(st);
		wprintf(L"Your move: %lc ", color_char(st->nextPlayer));
		scanf("%s", mv_in);
		if (!move_parse(mv, mv_in)) {
			wprintf(L"Invalid input\n");
			continue;
		}

		if (!go_move_play(st, mv)) {
			wprintf(L"Invalid move\n");
			continue;
		}
		wprintf(L"%lc plays ", color_char((st->nextPlayer == BLACK) ? WHITE : BLACK));
		move_print(mv);
		wprintf(L"\n\n");
	}

	return 0;
}
