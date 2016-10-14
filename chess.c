#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "chess.h"
#include "rand.h"
#include "utils.h"

INIT_MAKE_RANDI(42, 43);
MAKE_RANDI128(move_random, -1, COUNT);


wchar_t color_char(color player) {
	if (player == BLACK) {
		return L'B';
	} else if (player == WHITE) {
		return L'W';
	} else {
		return L'·';
	}
}

color color_opponent(color player) {
	return (player == BLACK) ? WHITE : (player == WHITE ? BLACK : NOBODY);
}


// Maps ['a' to 'h'] and ['1' to '8'] to the range of [0 to 7], or returns -1
static inline int char_index(char c) {
	if (c >= 'a' && c <= 'h') {
		return (c - 'a');
	} else if (c >= '1' && c <= '8') {
		return (c - '1');
	} else {
		return -1;
	}
}

wchar_t heatmap_char(float val) {
	if (val >= 1.0) {
		return L'+';
	} else if (val >= 0.9) {
		return (int)(val*10) + L'0';
	} else if (val >= 0.0) {
		return (int)(val*10) + L'₀';
	} else {
		return L'₋';
	}
}


static inline wchar_t piece_char(piece pc) {
	wchar_t chars[NPIECES] = {L'!', L'♟', L'♟', L'♟', L'♟', L'♟', L'♟', L'♟', L'♟', L'♜', L'♞', L'♝', L'♚', L'♝', L'♞', L'♜', L'♛', L'♛', L'♛', L'♛', L'♛', L'♙', L'♙', L'♙', L'♙', L'♙', L'♙', L'♙', L'♙', L'♖', L'♘', L'♗', L'♔', L'♗', L'♘', L'♖', L'♕', L'♕', L'♕', L'♕', L'♕'};
	return chars[pc];
}



move* move_create() {
	move* mv;
	if (!(mv = (move*)malloc(sizeof(move)))) {
		return NULL;
	}
	return mv;
}

void move_destroy(move* mv) {
	free(mv);
}

// Returns whether move correctly parsed
bool move_parse(move* mv, char str[5]) {
	if (str[0] == ':' && str[1] == '/') {
		*mv = MOVE_RESIGN;
		return true;
	}
	
	int fx = char_index(str[0]), fy = char_index(str[1]), tx = char_index(str[3]), ty = char_index(str[4]);
	if (fx < 0 || fy < 0 || tx < 0 || ty < 0 || (fx == tx && fy == ty)) {
		return false;
	}
	
	*mv = (fy << 9) + (fx << 6) + (ty << 3) + tx;
	return true;
}

#define MOVE_UNPACK(ff, tt, fy, fx, ty, tx) const loc (ff) = mv >> 6; const loc (tt) = mv & 0x3f; const coord (fy) = mv >> 9; const coord (fx) = (mv & (0x07 << 6)) >> 6; const coord (ty) = (mv & (0x07 << 3)) >> 3; const coord (tx) = mv & 0x07;

// str must be a wchar_t[6]
void move_sprint(wchar_t str[6], move* mv_ptr) {
	move mv = *mv_ptr;
	if (mv == MOVE_RESIGN) {
		swprintf(str, 6, L":/");
		return;
	}

	MOVE_UNPACK(ff, tt, fy, fx, ty, tx);

	swprintf(str, 6, L"%c%c-%c%c", fx + 'a', fy + '1', tx + 'a', ty + '1');
}

void move_print(move* mv) {
	wchar_t str[6];
	move_sprint(str, mv);
	wprintf(str);
}


#define BOARD(y, x) board[((y)<<3)+(x)]
#define COLORAT(y, x) colorAt[((y)<<3)+(x)]
#define PIECE_UNPACK(pc, pp, py, px) (pp) = locs[(pc)]; (py) = (pp) >> 3; (px) = (pp) & 0x07;
#define ADDMOVE(pp, my, mx) { mv = ((pp)<<6)+((my)<<3)+(mx); if (chess_is_move_legal(st, &mv)) nextMoves[numNext++] = mv; }

// Generates a list of possible moves for the current state, puts them into st->nextMoves, then updates & returns st->numNext
static void generate_next_moves(state* st) {
	const color friendly = st->nextPlayer;
	const color enemy = color_opponent(st->nextPlayer);
	loc* locs = st->locs;
	color* colorAt = st->colorAt;
	move* nextMoves = st->nextMoves;
	uint8_t numNext = 0;

	loc pp;
	coord py, px, len;
	move mv;
	piece pc;

	if (friendly == WHITE) {
		bool* couldCastleWhite = st->couldCastleWhite;

		// Generate pawn moves
		for (pc = WP1; pc <= WP8; ++pc) {
			if (locs[pc] == LOC_NULL) continue;
			PIECE_UNPACK(pc, pp, py, px);
			if (py > 0)  ADDMOVE(pp, py-1, px);
			if (py == 6) ADDMOVE(pp, py-2, px);
			if (px > 0)  ADDMOVE(pp, py-1, px-1);
			if (px < 7)  ADDMOVE(pp, py-1, px+1);
		}

		// Generate rook moves
		for (pc = WR1; pc <= WR2; pc += WR2-WR1) {
			if (locs[pc] == LOC_NULL) continue;
			PIECE_UNPACK(pc, pp, py, px);
			if (py > 0) {	// Up
				for (int y = py - 1; y >= 0; --y) {
					if (COLORAT(y, px) != friendly) ADDMOVE(pp, y, px);
					if (COLORAT(y, px) != NOBODY) break;
				}
			}
			if (px > 0) {	// Left
				for (int x = px - 1; x >= 0; --x) {
					if (COLORAT(py, x) != friendly) ADDMOVE(pp, py, x);
					if (COLORAT(py, x) != NOBODY) break;
				}
			}
			if (px < 7) {	// Right
				for (int x = px + 1; x <= 7; ++x) {
					if (COLORAT(py, x) != friendly) ADDMOVE(pp, py, x);
					if (COLORAT(py, x) != NOBODY) break;
				}
			}
			if (py < 7) {	// Down
				for (int y = py + 1; y <= 7; ++y) {
					if (COLORAT(y, px) != friendly) ADDMOVE(pp, y, px);
					if (COLORAT(y, px) != NOBODY) break;
				}
			}
		}

		// Generate knight moves
		for (pc = WN1; pc <= WN2; pc += WN2-WN1) {
			if (locs[pc] == LOC_NULL) continue;
			PIECE_UNPACK(pc, pp, py, px);
			if (py >= 1) {
				if (px >= 2) ADDMOVE(pp, py-1, px-2);
				if (px <= 5) ADDMOVE(pp, py-1, px+2);
				if (py >= 2) {
					if (px >= 1) ADDMOVE(pp, py-2, px-1);
					if (px <= 6) ADDMOVE(pp, py-2, px+1);
				}
			}
			if (py <= 6) {
				if (px >= 2) ADDMOVE(pp, py+1, px-2);
				if (px <= 5) ADDMOVE(pp, py+1, px+2);
				if (py <= 5) {
					if (px >= 1) ADDMOVE(pp, py+2, px-1);
					if (px <= 6) ADDMOVE(pp, py+2, px+1);
				}
			}
		}

		// Generate bishop moves
		for (pc = WB1; pc <= WB2; pc += WB2-WB1) {
			if (locs[pc] == LOC_NULL) continue;
			PIECE_UNPACK(pc, pp, py, px);
			len = min(py, px);
			if (len > 0) {	// Up-left
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py-i, px-i) != friendly) ADDMOVE(pp, py-i, px-i);
					if (COLORAT(py-i, px-i) != NOBODY) break;
				}
			}
			len = min(py, 7-px);
			if (len > 0) {	// Up-right
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py-i, px+i) != friendly) ADDMOVE(pp, py-i, px+i);
					if (COLORAT(py-i, px+i) != NOBODY) break;
				}
			}
			len = min(7-py, px);
			if (len > 0) {	// Down-left
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py+i, px-i) != friendly) ADDMOVE(pp, py+i, px-i);
					if (COLORAT(py+i, px-i) != NOBODY) break;
				}
			}
			len = min(7-py, 7-px);
			if (len > 0) {	// Down-left
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py+i, px+i) != friendly) ADDMOVE(pp, py+i, px+i);
					if (COLORAT(py+i, px+i) != NOBODY) break;
				}
			}
		}

		// Generate queen moves
		for (pc = WQ1; pc <= WQ5; ++pc) {
			if (locs[pc] == LOC_NULL) continue;
			PIECE_UNPACK(pc, pp, py, px);
			wprintf(L"queen = %d, at (y, x) = %d (%d, %d)\n", pc, pp, py, px);
			len = min(py, px);	// Up-left
			if (len > 0) {
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py-i, px-i) != friendly) ADDMOVE(pp, py-i, px-i);
					if (COLORAT(py-i, px-i) != NOBODY) break;
				}
			}
			if (py > 0) {	// Up
				for (int y = py - 1; y >= 0; --y) {
					if (COLORAT(y, px) != friendly) ADDMOVE(pp, y, px);
					if (COLORAT(y, px) != NOBODY) break;
				}
			}
			len = min(py, 7-px);	// Up-right
			if (len > 0) {
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py-i, px+i) != friendly) ADDMOVE(pp, py-i, px+i);
					if (COLORAT(py-i, px+i) != NOBODY) break;
				}
			}
			if (px > 0) {	// Left
				for (int x = px - 1; x >= 0; --x) {
					if (COLORAT(py, x) != friendly) ADDMOVE(pp, py, x);
					if (COLORAT(py, x) != NOBODY) break;
				}
			}
			if (px < 7) {	// Right
				for (int x = px + 1; x <= 7; ++x) {
					if (COLORAT(py, x) != friendly) ADDMOVE(pp, py, x);
					if (COLORAT(py, x) != NOBODY) break;
				}
			}
			len = min(7-py, px);	// Down-left
			if (len > 0) {
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py+i, px-i) != friendly) ADDMOVE(pp, py+i, px-i);
					if (COLORAT(py+i, px-i) != NOBODY) break;
				}
			}
			if (py < 7) {	// Down
				for (int y = py + 1; y <= 7; ++y) {
					if (COLORAT(y, px) != friendly) ADDMOVE(pp, y, px);
					if (COLORAT(y, px) != NOBODY) break;
				}
			}
			len = min(7-py, 7-px);	// Down-left
			if (len > 0) {
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py+i, px+i) != friendly) ADDMOVE(pp, py+i, px+i);
					if (COLORAT(py+i, px+i) != NOBODY) break;
				}
			}
		}

		// Generate king moves (assumed always exists)
		pc = WK;
		PIECE_UNPACK(pc, pp, py, px);
		if (py >= 1) {
			if (px >= 1) ADDMOVE(pp, py-1, px-1);
			ADDMOVE(pp, py-1, px);
			if (px <= 6) ADDMOVE(pp, py-1, px+1);
		}
		if (px >= 1) ADDMOVE(pp, py, px-1);
		if (px <= 6) ADDMOVE(pp, py, px+1);
		if (py <= 6) {
			if (px >= 1) ADDMOVE(pp, py+1, px-1);
			ADDMOVE(pp, py+1, px);
			if (px <= 6) ADDMOVE(pp, py+1, px+1);
		}
		if (couldCastleWhite[0]) {
			ADDMOVE(pp, py, px-2);
		}
		if (couldCastleWhite[1]) {
			ADDMOVE(pp, py, px+2);
		}

	} else if (friendly == BLACK) {
		bool* couldCastleBlack = st->couldCastleBlack;

		// Generate pawn moves
		for (pc = BP1; pc <= BP8; ++pc) {
			if (locs[pc] == LOC_NULL) continue;
			PIECE_UNPACK(pc, pp, py, px);
			if (py < 7)  ADDMOVE(pp, py+1, px);
			if (py == 1) ADDMOVE(pp, py+2, px);
			if (px > 0)  ADDMOVE(pp, py+1, px-1);
			if (px < 7)  ADDMOVE(pp, py+1, px+1);
		}

		// Generate rook moves
		for (pc = BR1; pc <= BR2; pc += BR2-BR1) {
			if (locs[pc] == LOC_NULL) continue;
			PIECE_UNPACK(pc, pp, py, px);
			if (py > 0) {	// Up
				for (int y = py - 1; y >= 0; --y) {
					if (COLORAT(y, px) != friendly) ADDMOVE(pp, y, px);
					if (COLORAT(y, px) == enemy) break;
				}
			}
			if (px > 0) {	// Left
				for (int x = px - 1; x >= 0; --x) {
					if (COLORAT(py, x) != friendly) ADDMOVE(pp, py, x);
					if (COLORAT(py, x) == enemy) break;
				}
			}
			if (px < 7) {	// Right
				for (int x = px + 1; x <= 7; ++x) {
					if (COLORAT(py, x) != friendly) ADDMOVE(pp, py, x);
					if (COLORAT(py, x) == enemy) break;
				}
			}
			if (py < 7) {	// Down
				for (int y = py + 1; y <= 7; ++y) {
					if (COLORAT(y, px) != friendly) ADDMOVE(pp, y, px);
					if (COLORAT(y, px) == enemy) break;
				}
			}
		}

		// Generate knight moves
		for (pc = BN1; pc <= BN2; pc += BN2-BN1) {
			if (locs[pc] == LOC_NULL) continue;
			PIECE_UNPACK(pc, pp, py, px);
			if (py >= 1) {
				if (px >= 2) ADDMOVE(pp, py-1, px-2);
				if (px <= 5) ADDMOVE(pp, py-1, px+2);
				if (py >= 2) {
					if (px >= 1) ADDMOVE(pp, py-2, px-1);
					if (px <= 6) ADDMOVE(pp, py-2, px+1);
				}
			}
			if (py <= 6) {
				if (px >= 2) ADDMOVE(pp, py+1, px-2);
				if (px <= 5) ADDMOVE(pp, py+1, px+2);
				if (py <= 5) {
					if (px >= 1) ADDMOVE(pp, py+2, px-1);
					if (px <= 6) ADDMOVE(pp, py+2, px+1);
				}
			}
		}

		// Generate bishop moves
		for (pc = BB1; pc <= BB2; pc += BB2-BB1) {
			if (locs[pc] == LOC_NULL) continue;
			PIECE_UNPACK(pc, pp, py, px);
			len = min(py, px);
			if (len > 0) {	// Up-left
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py-i, px-i) != friendly) ADDMOVE(pp, py-i, px-i);
					if (COLORAT(py-i, px-i) != NOBODY) break;
				}
			}
			len = min(py, 7-px);
			if (len > 0) {	// Up-right
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py-i, px+i) != friendly) ADDMOVE(pp, py-i, px+i);
					if (COLORAT(py-i, px+i) != NOBODY) break;
				}
			}
			len = min(7-py, px);
			if (len > 0) {	// Down-left
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py+i, px-i) != friendly) ADDMOVE(pp, py+i, px-i);
					if (COLORAT(py+i, px-i) != NOBODY) break;
				}
			}
			len = min(7-py, 7-px);
			if (len > 0) {	// Down-left
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py+i, px+i) != friendly) ADDMOVE(pp, py+i, px+i);
					if (COLORAT(py+i, px+i) != NOBODY) break;
				}
			}
		}

		// Generate queen moves
		for (pc = BQ1; pc <= BQ5; ++pc) {
			if (locs[pc] == LOC_NULL) continue;
			PIECE_UNPACK(pc, pp, py, px);
			len = min(py, px);	// Up-left
			if (len > 0) {
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py-i, px-i) != friendly) ADDMOVE(pp, py-i, px-i);
					if (COLORAT(py-i, px-i) != NOBODY) break;
				}
			}
			if (py > 0) {	// Up
				for (int y = py - 1; y >= 0; --y) {
					if (COLORAT(y, px) != friendly) ADDMOVE(pp, y, px);
					if (COLORAT(y, px) != NOBODY) break;
				}
			}
			len = min(py, 7-px);	// Up-right
			if (len > 0) {
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py-i, px+i) != friendly) ADDMOVE(pp, py-i, px+i);
					if (COLORAT(py-i, px+i) != NOBODY) break;
				}
			}
			if (px > 0) {	// Left
				for (int x = px - 1; x >= 0; --x) {
					if (COLORAT(py, x) != friendly) ADDMOVE(pp, py, x);
					if (COLORAT(py, x) != NOBODY) break;
				}
			}
			if (px < 7) {	// Right
				for (int x = px + 1; x <= 7; ++x) {
					if (COLORAT(py, x) != friendly) ADDMOVE(pp, py, x);
					if (COLORAT(py, x) != NOBODY) break;
				}
			}
			len = min(7-py, px);	// Down-left
			if (len > 0) {
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py+i, px-i) != friendly) ADDMOVE(pp, py+i, px-i);
					if (COLORAT(py+i, px-i) != NOBODY) break;
				}
			}
			if (py < 7) {	// Down
				for (int y = py + 1; y <= 7; ++y) {
					if (COLORAT(y, px) != friendly) ADDMOVE(pp, y, px);
					if (COLORAT(y, px) != NOBODY) break;
				}
			}
			len = min(7-py, 7-px);	// Down-left
			if (len > 0) {
				for (coord i = 1; i <= len; ++i) {
					if (COLORAT(py+i, px+i) != friendly) ADDMOVE(pp, py+i, px+i);
					if (COLORAT(py+i, px+i) != NOBODY) break;
				}
			}
		}

		// Generate king moves (assumed always exists)
		pc = BK;
		PIECE_UNPACK(pc, pp, py, px);
		if (py >= 1) {
			if (px >= 1) ADDMOVE(pp, py-1, px-1);
			ADDMOVE(pp, py-1, px);
			if (px <= 6) ADDMOVE(pp, py-1, px+1);
		}
		if (px >= 1) ADDMOVE(pp, py, px-1);
		if (px <= 6) ADDMOVE(pp, py, px+1);
		if (py <= 6) {
			if (px >= 1) ADDMOVE(pp, py+1, px-1);
			ADDMOVE(pp, py+1, px);
			if (px <= 6) ADDMOVE(pp, py+1, px+1);
		}
		if (couldCastleBlack[0]) {
			ADDMOVE(pp, py, px-2);
		}
		if (couldCastleBlack[1]) {
			ADDMOVE(pp, py, px+2);
		}
	}

	st->numNext = numNext;

#undef ADDMOVE

}

// Malloc & init a state
state* state_create() {
	state* st;
	if (!(st = (state*)malloc(sizeof(state)))) {
		return NULL;
	}

	st->nextPlayer = WHITE;
	st->status = PLAYING;

	piece initial_board[COUNT] = {
		BR1,	BN1,	BB1,	BQ1, 	BK, 	BB2,	BN2,	BR2,	
		BP1,	BP2,	BP3,	BP4,	BP5,	BP6,	BP7,	BP8,	
		EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY ,	EMPTY,	EMPTY,	EMPTY,	
		EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	
		EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	
		EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	
		WP1,	WP2,	WP3,	WP4,	WP5,	WP6,	WP7,	WP8,	
		WR1,	WN1,	WB1,	WQ1, 	WK, 	WB2,	WN2,	WR2,	
	};

	color initial_colorAt[COUNT] = {
		BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	
		BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	
		NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	
		NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	
		NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	
		NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	
		WHITE,	WHITE,	WHITE,	WHITE,	WHITE,	WHITE,	WHITE,	WHITE,	
		WHITE,	WHITE,	WHITE,	WHITE,	WHITE,	WHITE,	WHITE,	WHITE,	
	};

	// piece initial_board[COUNT] = {
	// 	BR1,	BN1,	BB1,	BQ1, 	BK, 	BB2,	BN2,	EMPTY,	
	// 	BP1,	BP2,	BP3,	BP4,	BP5,	BP6,	BP7,	BP8,	
	// 	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY ,	EMPTY,	EMPTY,	EMPTY,	
	// 	EMPTY,	EMPTY,	EMPTY,	EMPTY,	BB2,	EMPTY,	EMPTY,	EMPTY,	
	// 	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	
	// 	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	EMPTY,	
	// 	WP1,	WP2,	WP3,	EMPTY,	EMPTY,	EMPTY,	WP7,	WP8,	
	// 	WR1,	EMPTY,	EMPTY,	EMPTY, 	WK, 	EMPTY,	EMPTY,	WR2,	
	// };

	// color initial_colorAt[COUNT] = {
	// 	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	NOBODY,	
	// 	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	BLACK,	
	// 	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	
	// 	NOBODY,	NOBODY,	NOBODY,	NOBODY,	BLACK,	NOBODY,	NOBODY,	NOBODY,	
	// 	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	
	// 	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	NOBODY,	
	// 	WHITE,	WHITE,	WHITE,	NOBODY,	NOBODY,	NOBODY,	WHITE,	WHITE,	
	// 	WHITE,	NOBODY,	NOBODY,	NOBODY,	WHITE,	NOBODY,	NOBODY,	WHITE,	
	// };

	for (int i = 0; i < COUNT; ++i) {	// TODO memcpy?
		st->board[i] = initial_board[i];
		st->colorAt[i] = initial_colorAt[i];
	}

	loc initial_locs[NPIECES] = {
		LOC_NULL,
		48, 49, 50, 51, 52, 53, 54, 55,	// WP1, WP2, WP3, WP4, WP5, WP6, WP7, WP8
		56, 57, 58, 60, 61, 62, 63, 59,	// WR1, WN1, WB1, WK, WB2, WN2, WR2, WQ1
		LOC_NULL, LOC_NULL, LOC_NULL, LOC_NULL, 		// WQ2, WQ3, WQ4, WQ5
		8, 9, 10, 11, 12, 13, 14, 15, 	// BP1, BP2, BP3, BP4, BP5, BP6, BP7, BP8
		0, 1, 2, 4, 5, 6, 7, 3, 		// BR1, BN1, BB1, BK, BB2, BN2, BR2, BQ1
		LOC_NULL, LOC_NULL, LOC_NULL, LOC_NULL, 		// BQ2, BQ3, BQ4, BQ5
	};

	for (int i = 0; i < NPIECES; ++i) {
		st->locs[i] = initial_locs[i];
	}

	for (int i = 0; i < 2; ++i) {
		st->couldCastleWhite[i] = true;
		st->couldCastleBlack[i] = true;
	}

	for (int i = 0; i < 8; ++i) {
		st->enPassantWhite[i] = false;
		st->enPassantBlack[i] = false;
	}

	long double t0 = timer_now();
	generate_next_moves(st);
	long double dt = timer_now() - t0;

	wprintf(L"Generating initial nextMoves took [%.2Lf us]\n", dt*1e6);

	return st;
}

// Deep copy st0 --> st1
void state_copy(state* st0, state* st1) {
	st1->status = st0->status;
	st1->nextPlayer = st0->nextPlayer;

	for (int i = 0; i < COUNT; ++i) {
		st1->board[i] = st0->board[i];
		st1->colorAt[i] = st0->colorAt[i];
	}

	for (int i = 0; i < NPIECES; ++i) {
		st1->locs[i] = st0->locs[i];
	}

	for (int i = 0; i < 2; ++i) {
		st1->couldCastleWhite[i] = st0->couldCastleWhite[i];
		st1->couldCastleBlack[i] = st0->couldCastleBlack[i];
	}

	for (int i = 0; i < 8; ++i) {
		st1->enPassantWhite[i] = st0->enPassantWhite[i];
		st1->enPassantBlack[i] = st0->enPassantBlack[i];
	}

	for (int i = 0; i < NMOVES; ++i) {
		st1->nextMoves[i] = st0->nextMoves[i];
	}

	st1->numNext = st0->numNext;
}

void state_destroy(state* st) {
	free(st);
}

static bool is_black_queen(piece pc) {
	return pc >= BQ1 && pc <= BQ5;
}

static bool is_black_pawn(piece pc) {
	return pc >= BP1 && pc <= BP8;
}

static bool is_white_queen(piece pc) {
	return pc >= WQ1 && pc <= WQ5;
}

static bool is_white_pawn(piece pc) {
	return pc >= WP1 && pc <= WP8;
}

// Lots of information redundancy in parameters, but saves having to look it up on the board
// pp is location/index of piece
// Note: doesn't check for en-passant captures of pawns
static bool is_under_attack(loc pp, piece* board, color friendly) {
	const coord py = pp >> 3;
	const coord px = pp & 0x07;

	if (friendly == WHITE) {
		// Scan horz/vert for rooks & queens
		if (py > 0) {	// Look up
			for (coord y = py - 1; y >= 0; --y) {
				if (BOARD(y, px) == BR1 || BOARD(y, px) == BR2 || is_black_queen(BOARD(y, px))) return true;
				if (BOARD(y, px) != EMPTY) break;
			}
		}
		if (px > 0) {	// Look left
			for (coord x = px - 1; x >= 0; --x) {
				if (BOARD(py, x) == BR1 || BOARD(py, x) == BR2 || is_black_queen(BOARD(py, x))) return true;
				if (BOARD(py, x) != EMPTY) break;
			}
		}
		if (px < 7) {	// Look right
			for (coord x = px + 1; x <= 7; ++x) {
				if (BOARD(py, x) == BR1 || BOARD(py, x) == BR2 || is_black_queen(BOARD(py, x))) return true;
				if (BOARD(py, x) != EMPTY) break;
			}
		}
		if (py < 7) {	// Look down
			for (coord y = py + 1; y <= 7; ++y) {
				if (BOARD(y, px) == BR1 || BOARD(y, px) == BR2 || is_black_queen(BOARD(y, px))) return true;
				if (BOARD(y, px) != EMPTY) break;
			}
		}
		// Scan diag for bishops & queens
		const coord dNE = min(px, py);
		if (dNE > 0) {	// Look up-left
			for (coord i = 1; i <= dNE; ++i) {
				if (BOARD(py-i, px-i) == BB1 || BOARD(py-i, px-i) == BB2 || is_black_queen(BOARD(py-i, px-i))) return true;
				if (BOARD(py-i, px-i) != EMPTY) break;
			}
		}
		const coord dNW = min(7-px, py);
		if (dNW > 0) {	// Look up-right
			for (coord i = 1; i <= dNW; ++i) {
				if (BOARD(py-i, px+i) == BB1 || BOARD(py-i, px+i) == BB2 || is_black_queen(BOARD(py-i, px+i))) return true;
				if (BOARD(py-i, px+i) != EMPTY) break;
			}
		}
		const coord dSE = min(px, 7-py);
		if (dSE > 0) {	// Look down-left
			for (coord i = 1; i <= dSE; ++i) {
				if (BOARD(py+i, px-i) == BB1 || BOARD(py+i, px-i) == BB2 || is_black_queen(BOARD(py+i, px-i))) return true;
				if (BOARD(py+i, px-i) != EMPTY) break;
			}
		}
		const coord dSW = min(7-px, 7-py);
		if (dSW > 0) {	// Look down-right
			for (coord i = 1; i <= dSW; ++i) {
				if (BOARD(py+i, px+i) == BB1 || BOARD(py+i, px+i) == BB2 || is_black_queen(BOARD(py+i, px+i))) return true;
				if (BOARD(py+i, px+i) != EMPTY) break;
			}
		}

		// Pawns
		if (py > 0) {
			if (px > 0 && is_black_pawn(BOARD(py-1, px-1))) return true;
			if (px < 7 && is_black_pawn(BOARD(py-1, px+1))) return true;
		}

		// Knights
		if (py >= 1) {
			if (px >= 2 && (BOARD(py-1, px-2) == BN1 || BOARD(py-1, px-2) == BN2)) return true;
			if (px <= 5 && (BOARD(py-1, px+2) == BN1 || BOARD(py-1, px+2) == BN2)) return true;
			if (py >= 2) {
				if (px >= 1 && (BOARD(py-2, px-1) == BN1 || BOARD(py-2, px-1) == BN2)) return true;
				if (px <= 6 && (BOARD(py-2, px+1) == BN1 || BOARD(py-2, px+1) == BN2)) return true;
			}
		}
		if (py <= 6) {
			if (px >= 2 && (BOARD(py+1, px-2) == BN1 || BOARD(py+1, px-2) == BN2)) return true;
			if (px <= 5 && (BOARD(py+1, px+2) == BN1 || BOARD(py+1, px+2) == BN2)) return true;
			if (py <= 5) {
				if (px >= 1 && (BOARD(py+2, px-1) == BN1 || BOARD(py+2, px-1) == BN2)) return true;
				if (px <= 6 && (BOARD(py+2, px+1) == BN1 || BOARD(py+2, px+1) == BN2)) return true;
			}
		}

		// King(s)
		if (py >= 1) {
			if (px >= 1 && BOARD(py-1, px-1) == BK) return true;
			if (BOARD(py-1, px) == BK) return true;
			if (px <= 6 && BOARD(py-1, px+1) == BK) return true;
		}
		if (px >= 1 && BOARD(py, px-1) == BK) return true;
		if (px <= 6 && BOARD(py, px+1) == BK) return true;
		if (py <= 6) {
			if (px >= 1 && BOARD(py+1, px-1) == BK) return true;
			if (BOARD(py+1, px) == BK) return true;
			if (px <= 6 && BOARD(py+1, px+1) == BK) return true;
		}

	} else if (friendly == BLACK) {

		// Scan horz/vert for rooks & queens
		if (py > 0) {	// Look up
			for (coord y = py - 1; y >= 0; --y) {
				if (BOARD(y, px) == WR1 || BOARD(y, px) == WR2 || is_white_queen(BOARD(y, px))) return true;
				if (BOARD(y, px) != EMPTY) break;
			}
		}
		if (px > 0) {	// Look left
			for (coord x = px - 1; x >= 0; --x) {
				if (BOARD(py, x) == WR1 || BOARD(py, x) == WR2 || is_white_queen(BOARD(py, x))) return true;
				if (BOARD(py, x) != EMPTY) break;
			}
		}
		if (px < 7) {	// Look right
			for (coord x = px + 1; x <= 7; ++x) {
				if (BOARD(py, x) == WR1 || BOARD(py, x) == WR2 || is_white_queen(BOARD(py, x))) return true;
				if (BOARD(py, x) != EMPTY) break;
			}
		}
		if (py < 7) {	// Look down
			for (coord y = py + 1; y <= 7; ++y) {
				if (BOARD(y, px) == WR1 || BOARD(y, px) == WR2 || is_white_queen(BOARD(y, px))) return true;
				if (BOARD(y, px) != EMPTY) break;
			}
		}
		// Scan diag for bishops & queens
		const coord dNE = min(px, py);
		if (dNE > 0) {	// Look up-left
			for (coord i = 1; i <= dNE; ++i) {
				if (BOARD(py-i, px-i) == WB1 || BOARD(py-i, px-i) == WB2 || is_white_queen(BOARD(py-i, px-i))) return true;
				if (BOARD(py-i, px-i) != EMPTY) break;
			}
		}
		const coord dNW = min(7-px, py);
		if (dNW > 0) {	// Look up-right
			for (coord i = 1; i <= dNW; ++i) {
				if (BOARD(py-i, px+i) == WB1 || BOARD(py-i, px+i) == WB2 || is_white_queen(BOARD(py-i, px+i))) return true;
				if (BOARD(py-i, px+i) != EMPTY) break;
			}
		}
		const coord dSE = min(px, 7-py);
		if (dSE > 0) {	// Look down-left
			for (coord i = 1; i <= dSE; ++i) {
				if (BOARD(py+i, px-i) == WB1 || BOARD(py+i, px-i) == WB2 || is_white_queen(BOARD(py+i, px-i))) return true;
				if (BOARD(py+i, px-i) != EMPTY) break;
			}
		}
		const coord dSW = min(7-px, 7-py);
		if (dSW > 0) {	// Look down-right
			for (coord i = 1; i <= dSW; ++i) {
				if (BOARD(py+i, px+i) == WB1 || BOARD(py+i, px+i) == WB2 || is_white_queen(BOARD(py+i, px+i))) return true;
				if (BOARD(py+i, px+i) != EMPTY) break;
			}
		}

		// Pawns
		if (py < 7) {
			if (px > 0 && is_white_pawn(BOARD(py+1, px-1))) return true;
			if (px < 7 && is_white_pawn(BOARD(py+1, px+1))) return true;
		}

		// Knights
		if (py >= 1) {
			if (px >= 2 && (BOARD(py-1, px-2) == WN1 || BOARD(py-1, px-2) == WN2)) return true;
			if (px <= 5 && (BOARD(py-1, px+2) == WN1 || BOARD(py-1, px+2) == WN2)) return true;
			if (py >= 2) {
				if (px >= 1 && (BOARD(py-2, px-1) == WN1 || BOARD(py-2, px-1) == WN2)) return true;
				if (px <= 6 && (BOARD(py-2, px+1) == WN1 || BOARD(py-2, px+1) == WN2)) return true;
			}
		}
		if (py <= 6) {
			if (px >= 2 && (BOARD(py+1, px-2) == WN1 || BOARD(py+1, px-2) == WN2)) return true;
			if (px <= 5 && (BOARD(py+1, px+2) == WN1 || BOARD(py+1, px+2) == WN2)) return true;
			if (py <= 5) {
				if (px >= 1 && (BOARD(py+2, px-1) == WN1 || BOARD(py+2, px-1) == WN2)) return true;
				if (px <= 6 && (BOARD(py+2, px+1) == WN1 || BOARD(py+2, px+1) == WN2)) return true;
			}
		}

		// King(s)
		if (py >= 1) {
			if (px >= 1 && BOARD(py-1, px-1) == WK) return true;
			if (BOARD(py-1, px) == WK) return true;
			if (px <= 6 && BOARD(py-1, px+1) == WK) return true;
		}
		if (px >= 1 && BOARD(py, px-1) == WK) return true;
		if (px <= 6 && BOARD(py, px+1) == WK) return true;
		if (py <= 6) {
			if (px >= 1 && BOARD(py+1, px-1) == WK) return true;
			if (BOARD(py+1, px) == WK) return true;
			if (px <= 6 && BOARD(py+1, px+1) == WK) return true;
		}
	}

	return false;
}

void state_print(state* st) {
	color nextPlayer = st->nextPlayer;
	color enemy = (st->nextPlayer == BLACK) ? WHITE : BLACK;
	game_status status = st->status;

	piece* board = st->board;
	color* colorAt = st->colorAt;
	wprintf(L"Squares under attack:\n");
	loc testlocs[64];
	double testboard[64];
	for (int i = 0; i < 64; ++i) {
		testlocs[i] = i;
		testboard[i] = is_under_attack(i, board, nextPlayer) ? 1.0 : 0.0;
	}
	chess_print_heatmap(st, testlocs, testboard, 64);

	move* nextMoves = st->nextMoves;
	const uint8_t numNext = st->numNext;
	wprintf(L"Possible next moves (%d): ", numNext);
	for (int i = 0; i < numNext; ++i) {
		wprintf(L"%lc ", piece_char(board[nextMoves[i]>>6]));
		move_print(&(nextMoves[i]));
		if (i < numNext - 1) {
			wprintf(L", ");
		}
	}
	wprintf(L"\n");

	bool* couldCastleWhite = st->couldCastleWhite;
	bool* couldCastleBlack = st->couldCastleBlack;
	if (couldCastleWhite[0] || couldCastleWhite[1] || couldCastleBlack[0] || couldCastleBlack[1]) {
		wprintf(L"Castling:  W --%c---%c-  B --%c---%c-\n", st->couldCastleWhite[0] ? 'Q' : '-', st->couldCastleWhite[1] ? 'K' : '-', st->couldCastleBlack[0] ? 'Q' : '-', st->couldCastleBlack[1] ? 'K' : '-');
	}

	bool* enPassantWhite = st->enPassantWhite;
	bool* enPassantBlack = st->enPassantBlack;
	if (enPassantWhite[0] || enPassantWhite[1] || enPassantWhite[2] || enPassantWhite[3] || enPassantWhite[4] || enPassantWhite[5] || enPassantWhite[6] || enPassantWhite[7] || enPassantBlack[0] || enPassantBlack[1] || enPassantBlack[2] || enPassantBlack[3] || enPassantBlack[4] || enPassantBlack[5] || enPassantBlack[6] || enPassantBlack[7]) {
		wprintf(L"En pass.:  W ");
		for (int i = 0; i < 8; ++i) wprintf(L"%c", st->enPassantWhite[i] ? i+'1' : '-');
		wprintf(L"  B ");
		for (int i = 0; i < 8; ++i) wprintf(L"%c", st->enPassantBlack[i] ? i+'1' : '-');
		wprintf(L"\n");
	}
	
	bool state_consistent = true;
	loc* locs = st->locs;
	for (int i = 0; i < NPIECES; ++i) {
		if (locs[i] != LOC_NULL && board[locs[i]] != i) {
			state_consistent = false;
			break;
		}
	}
	if (!state_consistent) {
		wprintf(L"Inconsistent state\n");
		for (int i = 0; i < NPIECES; ++i) {
			if (locs[i] != LOC_NULL && board[locs[i]] != i) {
				wprintf(L"Expected %lc  @ %c%c, got %lc\n", piece_char(i), 'a'+(locs[i]&0x07), '1'+(locs[i]>>3), piece_char(board[locs[i]]));
			}
		}
	}

	if (status != PLAYING) {
		wprintf(L"Game is over: ");
		switch (status) {
			case STALEMATE:
				wprintf(L"Stalemate\n");
				break;
			case CHECKMATE:
				wprintf(L"Checkmate by %s\n", (enemy == WHITE) ? L"White" : L"Black");
				break;
			case RESIGNED:
				wprintf(L"Resignation by %s\n", (nextPlayer == WHITE) ? L"White" : L"Black");
				break;
			default:
				break;
		}
	}

	wprintf(L"\n    a  b  c  d  e  f  g  h\n\n");
	for (int y = 0; y < SIZE; ++y) {
		wprintf(L"%d   ", y+1);
		for (int x = 0; x < SIZE; ++x) {
			if (BOARD(y, x) == EMPTY) {
				wprintf(L"%ls ", ((x + y) % 2 == 0) ? L"· " : L"∙ ");	// •·⋅∙
			} else {
				wprintf(L"%lc%c%c", piece_char(BOARD(y, x)), (locs[BOARD(y, x)] == SIZE*y+x) ? ' ' : '*',
					((COLORAT(y, x) == WHITE && BOARD(y, x) >= PIECE_WHITE_MIN && BOARD(y, x) <= PIECE_WHITE_MAX)
					 || (COLORAT(y, x) == BLACK && BOARD(y, x) >= PIECE_BLACK_MIN && BOARD(y, x) <= PIECE_BLACK_MAX)
					 || (COLORAT(y, x) == NOBODY && BOARD(y, x) == EMPTY)) ? ' ' : '~');
			}
		}
		wprintf(L"  %d\n", y+1);
	}
	wprintf(L"\n    a  b  c  d  e  f  g  h\n\n");

}

// Debug info about groups & ko
void state_dump(state* st) {
	state_print(st);
}

color state_winner(state* st) {
	switch (st->status) {
		case CHECKMATE:
		case RESIGNED:
			return color_opponent(st->nextPlayer);
			break;
		case PLAYING:
		case STALEMATE:
		default:
			return NOBODY;
			break;
	}
}


// Move-playing function updates game status
bool chess_is_game_over(state* st) {
	return st->status != PLAYING;
}

// State is unchanged at the end (but it can change during execution)
bool chess_is_move_legal(state* st, move* mv_ptr) {
	const move mv = *mv_ptr;
	const color friendly = st->nextPlayer;
	const color enemy = (friendly == WHITE) ? BLACK : WHITE;
	piece* board = st->board;
	color* colorAt = st->colorAt;
	loc* locs = st->locs;
	bool* couldCastleWhite = st->couldCastleWhite;
	bool* couldCastleBlack = st->couldCastleBlack;
	bool* enPassantWhite = st->enPassantWhite;
	bool* enPassantBlack = st->enPassantBlack;
	
	if (chess_is_game_over(st)) {
		return false;
	}

	if (mv == MOVE_RESIGN) {
		return true;
	}

	if (mv < 0 || mv > MOVE_MAX) {
		return false;
	}

	MOVE_UNPACK(ff, tt, fy, fx, ty, tx);
	const int8_t dx = tx - fx;
	const int8_t dy = ty - fy;
	const coord ldx = abs(tx - fx);
	const coord ldy = abs(ty - fy);

	if (ff == tt) {
		return false;
	}

	piece pc = BOARD(fy, fx);

	// Do not play anything that's not own color (including "NOBODY" = empty squares)
	if (colorAt[ff] != friendly) {
		return false;
	}

	// Do not trample own piece
	if (colorAt[tt] == friendly) {
		return false;
	}

	// Is set to true in switch below (if move is en-passant)
	bool is_en_passant = false;

	// Check piece rules (invalid moves immediately return false; whitelisted moves pass through)
	switch (pc) {
		case WP1:
		case WP2:
		case WP3:
		case WP4:
		case WP5:
		case WP6:
		case WP7:
		case WP8:
			if (dy == -1 && ((tx == fx && colorAt[tt] == NOBODY) || (ldx == 1 && colorAt[tt] == enemy))) break;	// One forward or capture
			if (fy == 6 && ty == 4 && tx == fx && colorAt[tt] == NOBODY) break;	// Two forward
			if (fy == 3 && ty == 2 && ldx == 1 && BOARD(ty+1, tx) >= BP1 && BOARD(ty+1, tx) <= BP8 && enPassantBlack[BOARD(ty+1, tx) - BP1]) {
				is_en_passant = true;
				break;
			}
			return false;
			break;
		case BP1:
		case BP2:
		case BP3:
		case BP4:
		case BP5:
		case BP6:
		case BP7:
		case BP8:
			if (dy == 1 && ((tx == fx && colorAt[tt] == NOBODY) || (ldx == 1 && colorAt[tt] == enemy))) break;	// One forward or capture
			if (fy == 1 && ty == 3 && tx == fx && colorAt[tt] == NOBODY) break;	// Two forward
			if (fy == 4 && ty == 5 && ldx == 1 && BOARD(ty+1, tx) >= WP1 && BOARD(ty+1, tx) <= WP8 && enPassantWhite[BOARD(ty+1, tx) - WP1]) {
				is_en_passant = true;
				break;
			}
			return false;
			break;
		case WR1:
		case WR2:
		case BR1:
		case BR2:
			if (dy == 0) {	// Horizontal
				if (dx < 0) {	// Left
					for (coord x = fx - 1; x >= tx + 1; --x) if (COLORAT(fy, x) != NOBODY) return false;
					break;
				} else {	// Right (dx > 0); the (dx == 0) case is handled earlier with "no movement"
					for (coord x = fx + 1; x <= tx - 1; ++x) if (COLORAT(fy, x) != NOBODY) return false;
					break;
				}
			} else if (dx == 0) {	// Vertical
				if (dy < 0) {	// Up
					for (coord y = fy - 1; y >= ty + 1; --y) if (COLORAT(y, fx) != NOBODY) return false;
					break;
				} else {	// Down (dy > 0); the (dy == 0) case is handled earlier with "no movement"
					for (coord y = fy + 1; y <= ty - 1; ++y) if (COLORAT(y, fx) != NOBODY) return false;
					break;
				}
			}
			return false;
			break;
		case WN1:
		case WN2:
		case BN1:
		case BN2:
			if ((ldx == 1 && ldy == 2) || (ldx == 2 && ldy == 1)) break;
			return false;
			break;
		case WB1:
		case WB2:
		case BB1:
		case BB2:
			if (ldx != ldy) return false;	// Blacklist non-diag movement
			if (dy < 0) {
				if (dx < 0) {	// Up-left
					for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx-i) != NOBODY) return false;
					break;
				} else {		// Up-right (dx > 0); (dx == 0) ruled out earlier
					for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx+i) != NOBODY) return false;
					break;
				}
			} else {	// (dy > 0); (dy == 0) ruled out earlier
				if (dx < 0) {	// Down-left
					for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx-i) != NOBODY) return false;
					break;
				} else {		// Down-right (dx > 0); (dx == 0) ruled out earlier
					for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx+i) != NOBODY) return false;
					break;
				}
			}
			return false;
			break;
		case WQ1:
		case WQ2:
		case WQ3:
		case WQ4:
		case WQ5:
		case BQ1:
		case BQ2:
		case BQ3:
		case BQ4:
		case BQ5:
			if (dy < 0) {
				if (dx == 0) {	// Up
					for (coord y = fy - 1; y >= ty + 1; --y) if (COLORAT(y, fx) != NOBODY) return false;
					break;
				} else if (ldx == ldy) {
					if (dx < 0) {	// Up-left
						for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx-i) != NOBODY) return false;
						break;
					} else {		// Up-right (dx > 0)
						for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx+i) != NOBODY) return false;
						break;
					}
				}
			} else if (dy == 0) {
				if (dx < 0) {	// Left
					for (coord x = fx - 1; x >= tx + 1; --x) if (COLORAT(fy, x) != NOBODY) return false;
					break;
				} else {	// Right (dx > 0); the (dx == 0) case is handled earlier with "no movement"
					for (coord x = fx + 1; x <= tx - 1; ++x) if (COLORAT(fy, x) != NOBODY) return false;
					break;
				}
			} else {
				if (dx == 0) {	// Down
					for (coord y = fy + 1; y <= ty - 1; ++y) if (COLORAT(y, fx) != NOBODY) return false;
					break;
				} else if (ldx == ldy) {
					if (dx < 0) {	// Down-left
						for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx-i) != NOBODY) return false;
						break;
					} else {		// Down-right (dx > 0)
						for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx+i) != NOBODY) return false;
						break;
					}
				}
			}
			return false;
			break;
		case WK:
			if (ff == 60 && (tt == 58 || tt == 62) && !is_under_attack(60, board, friendly)) {
				if (tt == 58 && couldCastleWhite[0]												// Queen-side castle
					 && colorAt[57] == NOBODY && colorAt[58] == NOBODY && colorAt[59] == NOBODY	// Ensure path free for king & rook
					 && !is_under_attack(58, board, friendly)							// Don't go through check
					 && !is_under_attack(59, board, friendly)) break;
				if (tt == 62 && couldCastleWhite[1]								// King-side castle
					 && colorAt[61] == NOBODY && colorAt[62] == NOBODY			// Ensure path free for king & rook
					 && !is_under_attack(61, board, friendly)			// Don't go through check
					 && !is_under_attack(62, board, friendly)) break;
			}
			if (ldx <= 1 && ldy <= 1) break;	// 1-square king move
			return false;
			break;
		case BK:
			if (ff == 4 && (tt == 2 || tt == 6) && !is_under_attack(4, board, friendly)) {
				if (tt == 2 && couldCastleBlack[0]												// Queen-side castle
					 && colorAt[1] == NOBODY && colorAt[2] == NOBODY && colorAt[3] == NOBODY	// Ensure path free for king & rook
					 && !is_under_attack(1, board, friendly)								// Don't go through check
					 && !is_under_attack(2, board, friendly)
					 && !is_under_attack(3, board, friendly)) break;
				if (tt == 6 && couldCastleBlack[1]								// King-side castle
					 && colorAt[5] == NOBODY && colorAt[6] == NOBODY			// Ensure path free for king & rook
					 && !is_under_attack(5, board, friendly)				// Don't go through check
					 && !is_under_attack(6, board, friendly)) break;
			}
			if (ldx <= 1 && ldy <= 1) break;	// 1-square king move
			return false;
			break;
		default:
			return false;
			break;
	}

	// Castling has already been taken care of; must be legal by now
	if ((pc == WK || pc == BK) && ldx == 2) {
		return true;
	}

	// Make sure king doesn't end up in check
	const loc king_loc = (pc == WK || pc == BK) ? tt : locs[(friendly == WHITE) ? WK : BK];
	const piece possible_capture	= is_en_passant ? (friendly == WHITE ? BOARD(ty+1, tx) : BOARD(ty-1, tx)) : board[tt];
	const loc possible_capture_loc	= is_en_passant ? (friendly == WHITE ? SIZE*(ty+1)+tx  : SIZE*(ty-1)+tx ) : tt;
	
	// Simulate the move
	board[possible_capture_loc] = EMPTY;
	board[tt] = board[ff];
	board[ff] = EMPTY;

	const bool is_legal = !is_under_attack(king_loc, board, friendly);

	// Undo the move
	board[ff] = board[tt];
	board[tt] = EMPTY;
	board[possible_capture_loc] = possible_capture;

	return is_legal;
}

// Never resign
bool chess_is_move_reasonable(state* st, move* mv_ptr) {
	move mv = *mv_ptr;

	if (mv == MOVE_RESIGN) {
		return false;
	}

	return chess_is_move_legal(st, mv_ptr);
}

// Param move_list must be move[COUNT+1]
// Returns number of legally playable moves
int chess_get_legal_moves(state* st, move* move_list) {
	int num = chess_get_reasonable_moves(st, move_list);
	move_list[num++] = MOVE_RESIGN;
	return num;
}

// Like chess_get_legal_moves, but without resignations
int chess_get_reasonable_moves(state* st, move* move_list) {

	for (int i = 0; i < st->numNext; ++i) {
		move_list[i] = st->nextMoves[i];
	}

	// if (chess_is_game_over(st)) {
	// 	return 0;
	// }

	// move mv;
	// // Allow only non-losing passes
	// if (chess_is_move_reasonable(st, &mv)) {
	// 	move_list[num] = mv;
	// 	++num;
	// }

	// for (int i = 0; i < COUNT; ++i) {
	// 	mv = i;
	// 	if (chess_is_move_reasonable(st, &mv)) {
	// 		move_list[num] = mv;
	// 		++num;
	// 	}
	// }

	// // Or resign when nowhere to play
	// if (!num) {
	// 	move_list[num] = MOVE_RESIGN;
	// 	++num;
	// }

	return st->numNext;
}

move_result chess_play_move(state* st, move* mv_ptr) {
	const move mv = *mv_ptr;
	const color friendly = st->nextPlayer;
	const color enemy = (friendly == WHITE) ? BLACK : WHITE;
	piece* board = st->board;
	color* colorAt = st->colorAt;
	loc* locs = st->locs;
	bool* couldCastleWhite = st->couldCastleWhite;
	bool* couldCastleBlack = st->couldCastleBlack;
	bool* enPassantWhite = st->enPassantWhite;
	bool* enPassantBlack = st->enPassantBlack;
	
	if (chess_is_game_over(st)) {
		return FAIL_GAME_ENDED;
	}

	if (mv == MOVE_RESIGN) {
		st->status = RESIGNED;
		for (int i = 0; i < 2; ++i) {
			st->couldCastleWhite[i] = false;
			st->couldCastleBlack[i] = false;
		}
		for (int i = 0; i < 8; ++i) {
			st->enPassantWhite[i] = false;
			st->enPassantBlack[i] = false;
		}
		return SUCCESS;
	}

	if (mv < 0 || mv > MOVE_MAX) {
		return FAIL_BOUNDS;
	}

	MOVE_UNPACK(ff, tt, fy, fx, ty, tx);
	const int8_t dx = tx - fx;
	const int8_t dy = ty - fy;
	const coord ldx = abs(tx - fx);
	const coord ldy = abs(ty - fy);

	if (ff == tt) {
		return FAIL_OCCUPIED;
	}

	piece pc = BOARD(fy, fx);

	// Do not play anything that's not own color (including "NOBODY" = empty squares)
	if (colorAt[ff] != friendly) {
		return FAIL_ILLEGAL;
	}

	// Do not trample own piece
	if (colorAt[tt] == friendly) {
		return FAIL_OCCUPIED;
	}

	// Is set to true in switch below (if move is en-passant)
	bool is_en_passant = false;

	// Check piece rules (invalid moves immediately return false; whitelisted moves pass through)
	switch (pc) {
		case WP1:
		case WP2:
		case WP3:
		case WP4:
		case WP5:
		case WP6:
		case WP7:
		case WP8:
			if (dy == -1 && ((tx == fx && colorAt[tt] == NOBODY) || (ldx == 1 && colorAt[tt] == enemy))) break;	// One forward or capture
			if (fy == 6 && ty == 4 && tx == fx && colorAt[tt] == NOBODY) break;	// Two forward
			if (fy == 3 && ty == 2 && ldx == 1 && BOARD(ty+1, tx) >= BP1 && BOARD(ty+1, tx) <= BP8 && enPassantBlack[BOARD(ty+1, tx) - BP1]) {
				is_en_passant = true;
				break;
			}
			return FAIL_ILLEGAL;
			break;
		case BP1:
		case BP2:
		case BP3:
		case BP4:
		case BP5:
		case BP6:
		case BP7:
		case BP8:
			if (dy == 1 && ((tx == fx && colorAt[tt] == NOBODY) || (ldx == 1 && colorAt[tt] == enemy))) break;	// One forward or capture
			if (fy == 1 && ty == 3 && tx == fx && colorAt[tt] == NOBODY) break;	// Two forward
			if (fy == 4 && ty == 5 && ldx == 1 && BOARD(ty+1, tx) >= WP1 && BOARD(ty+1, tx) <= WP8 && enPassantWhite[BOARD(ty+1, tx) - WP1]) {
				is_en_passant = true;
				break;
			}
			return FAIL_ILLEGAL;
			break;
		case WR1:
		case WR2:
		case BR1:
		case BR2:
			if (dy == 0) {	// Horizontal
				if (dx < 0) {	// Left
					for (coord x = fx - 1; x >= tx + 1; --x) if (COLORAT(fy, x) != NOBODY) return FAIL_ILLEGAL;
					break;
				} else {	// Right (dx > 0); the (dx == 0) case is handled earlier with "no movement"
					for (coord x = fx + 1; x <= tx - 1; ++x) if (COLORAT(fy, x) != NOBODY) return FAIL_ILLEGAL;
					break;
				}
			} else if (dx == 0) {	// Vertical
				if (dy < 0) {	// Up
					for (coord y = fy - 1; y >= ty + 1; --y) if (COLORAT(y, fx) != NOBODY) return FAIL_ILLEGAL;
					break;
				} else {	// Down (dy > 0); the (dy == 0) case is handled earlier with "no movement"
					for (coord y = fy + 1; y <= ty - 1; ++y) if (COLORAT(y, fx) != NOBODY) return FAIL_ILLEGAL;
					break;
				}
			}
			return FAIL_ILLEGAL;
			break;
		case WN1:
		case WN2:
		case BN1:
		case BN2:
			if ((ldx == 1 && ldy == 2) || (ldx == 2 && ldy == 1)) break;
			return FAIL_ILLEGAL;
			break;
		case WB1:
		case WB2:
		case BB1:
		case BB2:
			if (ldx != ldy) return FAIL_ILLEGAL;	// Blacklist non-diag movement
			if (dy < 0) {
				if (dx < 0) {	// Up-left
					for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx-i) != NOBODY) return FAIL_ILLEGAL;
					break;
				} else {		// Up-right (dx > 0); (dx == 0) ruled out earlier
					for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx+i) != NOBODY) return FAIL_ILLEGAL;
					break;
				}
			} else {	// (dy > 0); (dy == 0) ruled out earlier
				if (dx < 0) {	// Down-left
					for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx-i) != NOBODY) return FAIL_ILLEGAL;
					break;
				} else {		// Down-right (dx > 0); (dx == 0) ruled out earlier
					for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx+i) != NOBODY) return FAIL_ILLEGAL;
					break;
				}
			}
			return FAIL_ILLEGAL;
			break;
		case WQ1:
		case WQ2:
		case WQ3:
		case WQ4:
		case WQ5:
		case BQ1:
		case BQ2:
		case BQ3:
		case BQ4:
		case BQ5:
			if (dy < 0) {
				if (dx == 0) {	// Up
					for (coord y = fy - 1; y >= ty + 1; --y) if (COLORAT(y, fx) != NOBODY) return FAIL_ILLEGAL;
					break;
				} else if (ldx == ldy) {
					if (dx < 0) {	// Up-left
						for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx-i) != NOBODY) return FAIL_ILLEGAL;
						break;
					} else {		// Up-right (dx > 0)
						for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx+i) != NOBODY) return FAIL_ILLEGAL;
						break;
					}
				}
			} else if (dy == 0) {
				if (dx < 0) {	// Left
					for (coord x = fx - 1; x >= tx + 1; --x) if (COLORAT(fy, x) != NOBODY) return FAIL_ILLEGAL;
					break;
				} else {	// Right (dx > 0); the (dx == 0) case is handled earlier with "no movement"
					for (coord x = fx + 1; x <= tx - 1; ++x) if (COLORAT(fy, x) != NOBODY) return FAIL_ILLEGAL;
					break;
				}
			} else {
				if (dx == 0) {	// Down
					for (coord y = fy + 1; y <= ty - 1; ++y) if (COLORAT(y, fx) != NOBODY) return FAIL_ILLEGAL;
					break;
				} else if (ldx == ldy) {
					if (dx < 0) {	// Down-left
						for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx-i) != NOBODY) return FAIL_ILLEGAL;
						break;
					} else {		// Down-right (dx > 0)
						for (coord i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx+i) != NOBODY) return FAIL_ILLEGAL;
						break;
					}
				}
			}
			return FAIL_ILLEGAL;
			break;
		case WK:
			if (ff == 60 && (tt == 58 || tt == 62) && !is_under_attack(60, board, friendly)) {
				if (tt == 58 && couldCastleWhite[0]												// Queen-side castle
					 && colorAt[57] == NOBODY && colorAt[58] == NOBODY && colorAt[59] == NOBODY	// Ensure path free for king & rook
					 && !is_under_attack(58, board, friendly)							// Don't go through check
					 && !is_under_attack(59, board, friendly)) break;
				if (tt == 62 && couldCastleWhite[1]								// King-side castle
					 && colorAt[61] == NOBODY && colorAt[62] == NOBODY			// Ensure path free for king & rook
					 && !is_under_attack(61, board, friendly)			// Don't go through check
					 && !is_under_attack(62, board, friendly)) break;
			}
			if (ldx <= 1 && ldy <= 1) break;	// 1-square king move
			return FAIL_ILLEGAL;
			break;
		case BK:
			if (ff == 4 && (tt == 2 || tt == 6) && !is_under_attack(4, board, friendly)) {
				if (tt == 2 && couldCastleBlack[0]												// Queen-side castle
					 && colorAt[1] == NOBODY && colorAt[2] == NOBODY && colorAt[3] == NOBODY	// Ensure path free for king & rook
					 && !is_under_attack(1, board, friendly)								// Don't go through check
					 && !is_under_attack(2, board, friendly)
					 && !is_under_attack(3, board, friendly)) break;
				if (tt == 6 && couldCastleBlack[1]								// King-side castle
					 && colorAt[5] == NOBODY && colorAt[6] == NOBODY			// Ensure path free for king & rook
					 && !is_under_attack(5, board, friendly)				// Don't go through check
					 && !is_under_attack(6, board, friendly)) break;
			}
			if (ldx <= 1 && ldy <= 1) break;	// 1-square king move
			return FAIL_ILLEGAL;
			break;
		default:
			return FAIL_OTHER;
			break;
	}

	// Castling has already been taken care of; must be legal by now
	if ((pc == WK || pc == BK) && ldx == 2) {
		loc rook_ff, rook_tt;
		switch (tt) {
			case 2:		// Black queen-side
				rook_ff = 0;
				rook_tt = 3;
				break;
			case 6:		// Black king-side
				rook_ff = 7;
				rook_tt = 5;
				break;
			case 58:	// White queen-side
				rook_ff = 56;
				rook_tt = 59;
				break;
			case 62:	// White king-side
				rook_ff = 63;
				rook_tt = 61;
				break;
			default:
				return FAIL_OTHER;
				break;
		}
		
		st->nextPlayer = enemy;

		board[rook_tt] = board[rook_ff];
		board[rook_ff] = EMPTY;
		board[tt] = board[ff];
		board[ff] = EMPTY;
		
		colorAt[rook_tt] = colorAt[rook_ff];
		colorAt[rook_ff] = NOBODY;
		colorAt[tt] = colorAt[ff];
		colorAt[ff] = NOBODY;
		
		locs[board[tt]] = tt;
		locs[board[rook_tt]] = rook_tt;
		
		if (pc == WK) {
			for (int i = 0; i < 2; ++i) couldCastleWhite[i] = false;
			for (int i = 0; i < 8; ++i) enPassantWhite[i] = false;
		} else if (pc == BK) {
			for (int i = 0; i < 2; ++i) couldCastleBlack[i] = false;
			for (int i = 0; i < 8; ++i) enPassantBlack[i] = false;
		}
		
		goto generate_next_moves_before_returning_success;
	}

	// Make sure king doesn't end up in check
	const loc king_loc = (pc == WK || pc == BK) ? tt : locs[(friendly == WHITE) ? WK : BK];
	const piece possible_capture	= is_en_passant ? (friendly == WHITE ? BOARD(ty+1, tx) : BOARD(ty-1, tx)) : board[tt];
	const loc possible_capture_loc	= is_en_passant ? (friendly == WHITE ? SIZE*(ty+1)+tx  : SIZE*(ty-1)+tx ) : tt;
	
	// Simulate the move
	board[possible_capture_loc] = EMPTY;
	board[tt] = board[ff];
	board[ff] = EMPTY;

	if (!is_under_attack(king_loc, board, friendly)) {
		st->nextPlayer = enemy;

		// Pawn promotions (if there are enough queens)
		piece new_queen = EMPTY;
		if (is_white_pawn(pc) && ty == 0) {
			new_queen = (locs[WQ5] == LOC_NULL ? WQ5 : locs[WQ4] == LOC_NULL ? WQ4 : locs[WQ3] == LOC_NULL ? WQ3 : locs[WQ2] == LOC_NULL ? WQ2 : locs[WQ1] == LOC_NULL ? WQ1 : EMPTY);
		} else if (is_black_pawn(pc) && ty == 7) {
			new_queen = (locs[BQ5] == LOC_NULL ? BQ5 : locs[BQ4] == LOC_NULL ? BQ4 : locs[BQ3] == LOC_NULL ? BQ3 : locs[BQ2] == LOC_NULL ? BQ2 : locs[BQ1] == LOC_NULL ? BQ1 : EMPTY);
		}
		if (new_queen != EMPTY) {
			board[tt] = new_queen;
			locs[pc] = LOC_NULL;
		}

		colorAt[possible_capture_loc] = NOBODY;
		colorAt[tt] = colorAt[ff];
		colorAt[ff] = NOBODY;

		locs[board[tt]] = tt;
		if (possible_capture != EMPTY) locs[possible_capture] = LOC_NULL;

		if (friendly == WHITE) {
			if (couldCastleWhite[0] && (pc == WK || pc == WR1)) couldCastleWhite[0] = false;
			if (couldCastleWhite[1] && (pc == WK || pc == WR2)) couldCastleWhite[1] = false;
			for (int i = 0; i < 8; ++i) enPassantWhite[i] = false;
			if (is_white_pawn(pc) && dy == -2) enPassantWhite[pc - WP1] = true;
		} else if (friendly == BLACK) {
			if (couldCastleBlack[0] && (pc == BK || pc == BR1)) couldCastleBlack[0] = false;
			if (couldCastleBlack[1] && (pc == BK || pc == BR2)) couldCastleBlack[1] = false;
			for (int i = 0; i < 8; ++i) enPassantBlack[i] = false;
			if (is_black_pawn(pc) && dy == 2) enPassantBlack[pc - BP1] = true;
		}

		goto generate_next_moves_before_returning_success;

	} else {
		// Undo the move
		board[ff] = board[tt];
		board[tt] = EMPTY;
		board[possible_capture_loc] = possible_capture;
		return FAIL_CHECK;
	}

	return FAIL_OTHER;

	// goto considered harmful?
 generate_next_moves_before_returning_success:
		// Remember we're now thinking for the opponent of the player this function spent most time on
		generate_next_moves(st);

		if (st->numNext == 0) {
			// Game is ending here
			const loc opponent_king_loc = (pc == WK || pc == BK) ? tt : locs[(enemy == WHITE) ? WK : BK];
			if (is_under_attack(opponent_king_loc, board, enemy)) {
				st->status = CHECKMATE;
			} else {
				st->status = STALEMATE;
			}
		}
		return SUCCESS;
}

// Plays a "random" move & stores it in mv
move_result chess_play_random_move(state* st, move* mv, move* move_list) {
	move tmp = st->nextMoves[randi(0, st->numNext)];

	if (chess_play_move(st, &tmp) == SUCCESS) {
		*mv = tmp;
		return SUCCESS;
	}

	return FAIL_OTHER;
}

// Modifies st; stores result
// Assumes game isn't over
void chess_play_out(state* st, playout_result* result) {
	move mv;
	move mv_list[COUNT+1];
	while (!chess_is_game_over(st)) {
		if (chess_play_random_move(st, &mv, mv_list) != SUCCESS) {
			fwprintf(stderr, L"E: chess_play_out couldn't play any moves\n");
			result->winner = NOBODY;
			return;
		}
	}

	// TODO
	result->winner = NOBODY;
	return;
}


void chess_print_heatmap(state* st, loc* locations, double* values, int num_values) {
	piece* board = st->board;

	double valboard[COUNT];
	for (int i = 0; i < COUNT; ++i) {
		valboard[i] = NAN;
	}

	double minval = INFINITY;
	double maxval = -INFINITY;
	for (int i = 0; i < num_values; ++i) {
		valboard[locations[i]] = values[i];
		minval = fmin(minval, values[i]);
		maxval = fmax(maxval, values[i]);
	}

	wprintf(L"Between %.1f%% and %.1f%% (50%% is %lc)\n", minval*100, maxval*100, heatmap_char((0.5 - minval) / (maxval - minval) ));

	wprintf(L"    a  b  c  d  e  f  g  h  \n");
	for (int i = 0; i < SIZE; ++i) {
		wprintf(L"\n%c   ", i + '1');
		for (int j = 0; j < SIZE; ++j) {
			if (!isnan(valboard[i*SIZE+j])) {
				wprintf(L"%lc  ", heatmap_char( (valboard[i*SIZE+j] - minval) / (maxval - minval)));
			} else {
				wprintf(L"%lc  ", piece_char(EMPTY));
			}
		}
	}

	wprintf(L"\n\n");
}

void chess_print_move_result(move_result result) {
	switch (result) {
		case SUCCESS:
			wprintf(L"Success\n");
			break;
		case FAIL_GAME_ENDED:
			wprintf(L"Game ended\n");
			break;
		case FAIL_BOUNDS:
			wprintf(L"Out of bounds\n");
			break;
		case FAIL_OCCUPIED:
			wprintf(L"Square occupied\n");
			break;
		case FAIL_ILLEGAL:
			wprintf(L"Illegal\n");
			break;
		case FAIL_CHECK:
			wprintf(L"King in check\n");
			break;
		case FAIL_OTHER:
		default:
			wprintf(L"Other\n");
			break;
	}
}
