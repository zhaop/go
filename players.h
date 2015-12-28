#ifndef PLAYERS_H
#define PLAYERS_H

#include "go.h"

typedef struct {
	const char* name;
	move_result (*const play)(state*, move*, void*);
	void* params;
} player;

move_result human_play(state*, move*, void*);

move_result randy_play(state*, move*, void*);

typedef struct {
	int N;
} karl_params;

move_result karl_play(state*, move*, void*);

#endif