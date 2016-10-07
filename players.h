#ifndef PLAYERS_H
#define PLAYERS_H

#include <stdint.h>
#include "chess.h"

struct player;
typedef struct player {
	const char* name;
	move_result (*const play)(struct player*, state*, move*);
	void (*const observe)(struct player*, state*, color, move*);
	void* params;
} player;


#endif