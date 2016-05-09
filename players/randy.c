#include "players/randy.h"
#include "go.h"

// Have bot play one move given current state
move_result randy_play(player* self, state* st, move* mv) {
	self = self;
	move move_list[NMOVES];
	return go_play_random_move(st, mv, move_list);
}

