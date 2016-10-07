#include "players/randy.h"
#include "chess.h"

// Have bot play one move given current state
move_result randy_play(player* self, state* st, move* mv) {
	self = self;
	move move_list[NMOVES];
	return chess_play_random_move(st, mv, move_list);
}

