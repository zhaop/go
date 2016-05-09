#ifndef PLAYERS_KARL_H
#define PLAYERS_KARL_H

#include "players.h"

typedef struct {
	int N;
} karl_params;

move_result karl_play(player*, state*, move*);

#endif