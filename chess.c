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
#if SIZE >= 12 && SIZE <= 22
	MAKE_RANDI512(move_random, -1, COUNT);
#elif SIZE >= 6 && SIZE <= 11
	MAKE_RANDI128(move_random, -1, COUNT);
#elif SIZE <= 5
	MAKE_RANDI32(move_random, -1, COUNT);
#endif


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
	wchar_t chars[NPIECES] = {L'·', L'♟', L'♟', L'♟', L'♟', L'♟', L'♟', L'♟', L'♟', L'♜', L'♞', L'♝', L'♚', L'♝', L'♞', L'♜', L'♛', L'♛', L'♛', L'♛', L'♛', L'♙', L'♙', L'♙', L'♙', L'♙', L'♙', L'♙', L'♙', L'♖', L'♘', L'♗', L'♔', L'♗', L'♘', L'♖', L'♕', L'♕', L'♕', L'♕', L'♕'};
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

#define MOVE_UNPACK(ff, tt, fy, fx, ty, tx) const char (ff) = mv >> 6; const char (tt) = mv & 0x3f; const char (fy) = mv >> 9; const char (fx) = (mv & (0x07 << 6)) >> 6; const char (ty) = (mv & (0x07 << 3)) >> 3; const char (tx) = mv & 0x07;

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

	for (int i = 0; i < COUNT; ++i) {	// TODO memcpy?
		st->board[i] = initial_board[i];
		st->colorAt[i] = initial_colorAt[i];
	}

	char initial_pieces[NPIECES] = {
		0xff,
		48, 49, 50, 51, 52, 53, 54, 55,	// WP1, WP2, WP3, WP4, WP5, WP6, WP7, WP8
		56, 57, 58, 60, 61, 62, 63, 59,	// WR1, WN1, WB1, WK, WB2, WN2, WR2, WQ1
		0xff, 0xff, 0xff, 0xff, 		// WQ2, WQ3, WQ4, WQ5
		8, 9, 10, 11, 12, 13, 14, 15, 	// BP1, BP2, BP3, BP4, BP5, BP6, BP7, BP8
		0, 1, 2, 4, 5, 6, 7, 3, 		// BR1, BN1, BB1, BK, BB2, BN2, BR2, BQ1
		0xff, 0xff, 0xff, 0xff, 		// BQ2, BQ3, BQ4, BQ5
	};

	for (int i = 0; i < NPIECES; ++i) {
		st->pieces[i] = initial_pieces[i];
	}

	for (int i = 0; i < 2; ++i) {
		st->canCastleWhite[i] = true;
		st->canCastleBlack[i] = true;
	}

	for (int i = 0; i < 8; ++i) {
		st->enPassantWhite[i] = false;
		st->enPassantBlack[i] = false;
	}

	return st;
}

// Deep copy st0 --> st1
void state_copy(state* st0, state* st1) {
	st1->nextPlayer = st0->nextPlayer;
	st1->status = st0->status;

	for (int i = 0; i < COUNT; ++i) {
		st1->board[i] = st0->board[i];
	}

	for (int i = 0; i < NPIECES; ++i) {
		st1->pieces[i] = st0->pieces[i];
	}
}

void state_destroy(state* st) {
	free(st);
}

#define BOARD(y, x) board[(y)*SIZE+(x)]
#define COLORAT(y, x) colorAt[(y)*SIZE+(x)]

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
// loc is location/index of piece
// Note: doesn't check for en-passant captures of pawns
static bool is_under_attack(char loc, piece* board, color friendly, color enemy) {
	const char py = loc >> 3;
	const char px = loc & 0x07;

	if (friendly == WHITE) {
		// Scan horz/vert for rooks & queens
		if (py > 0) {	// Look up
			for (char y = py - 1; y >= 0; --y) {
				if (BOARD(y, px) == BR1 || BOARD(y, px) == BR2 || is_black_queen(BOARD(y, px))) return true;
				if (BOARD(y, px) != EMPTY) break;
			}
		}
		if (px > 0) {	// Look left
			for (char x = px - 1; x >= 0; --x) {
				if (BOARD(py, x) == BR1 || BOARD(py, x) == BR2 || is_black_queen(BOARD(py, x))) return true;
				if (BOARD(py, x) != EMPTY) break;
			}
		}
		if (px < 7) {	// Look right
			for (char x = px + 1; x <= 7; ++x) {
				if (BOARD(py, x) == BR1 || BOARD(py, x) == BR2 || is_black_queen(BOARD(py, x))) return true;
				if (BOARD(py, x) != EMPTY) break;
			}
		}
		if (py < 7) {	// Look down
			for (char y = py + 1; y <= 7; ++y) {
				if (BOARD(y, px) == BR1 || BOARD(y, px) == BR2 || is_black_queen(BOARD(y, px))) return true;
				if (BOARD(y, px) != EMPTY) break;
			}
		}
		// Scan diag for bishops & queens
		const char dNE = min(px, py);
		if (dNE > 0) {	// Look up-left
			for (char i = 1; i <= dNE; ++i) {
				if (BOARD(py-i, px-i) == BB1 || BOARD(py-i, px-i) == BB2 || is_black_queen(BOARD(py-i, px-i))) return true;
				if (BOARD(py-i, px-i) != EMPTY) break;
			}
		}
		const char dNW = min(7-px, py);
		if (dNW > 0) {	// Look up-right
			for (char i = 1; i <= dNW; ++i) {
				if (BOARD(py-i, px+i) == BB1 || BOARD(py-i, px+i) == BB2 || is_black_queen(BOARD(py-i, px+i))) return true;
				if (BOARD(py-i, px+i) != EMPTY) break;
			}
		}
		const char dSE = min(px, 7-py);
		if (dSE > 0) {	// Look down-left
			for (char i = 1; i <= dSE; ++i) {
				if (BOARD(py+i, px-i) == BB1 || BOARD(py+i, px-i) == BB2 || is_black_queen(BOARD(py+i, px-i))) return true;
				if (BOARD(py+i, px-i) != EMPTY) break;
			}
		}
		const char dSW = min(7-px, 7-py);
		if (dSW > 0) {	// Look down-right
			for (char i = 1; i <= dSW; ++i) {
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
			for (char y = py - 1; y >= 0; --y) {
				if (BOARD(y, px) == WR1 || BOARD(y, px) == WR2 || is_white_queen(BOARD(y, px))) return true;
				if (BOARD(y, px) != EMPTY) break;
			}
		}
		if (px > 0) {	// Look left
			for (char x = px - 1; x >= 0; --x) {
				if (BOARD(py, x) == WR1 || BOARD(py, x) == WR2 || is_white_queen(BOARD(py, x))) return true;
				if (BOARD(py, x) != EMPTY) break;
			}
		}
		if (px < 7) {	// Look right
			for (char x = px + 1; x <= 7; ++x) {
				if (BOARD(py, x) == WR1 || BOARD(py, x) == WR2 || is_white_queen(BOARD(py, x))) return true;
				if (BOARD(py, x) != EMPTY) break;
			}
		}
		if (py < 7) {	// Look down
			for (char y = py + 1; y <= 7; ++y) {
				if (BOARD(y, px) == WR1 || BOARD(y, px) == WR2 || is_white_queen(BOARD(y, px))) return true;
				if (BOARD(y, px) != EMPTY) break;
			}
		}
		// Scan diag for bishops & queens
		const char dNE = min(px, py);
		if (dNE > 0) {	// Look up-left
			for (char i = 1; i <= dNE; ++i) {
				if (BOARD(py-i, px-i) == WB1 || BOARD(py-i, px-i) == WB2 || is_white_queen(BOARD(py-i, px-i))) return true;
				if (BOARD(py-i, px-i) != EMPTY) break;
			}
		}
		const char dNW = min(7-px, py);
		if (dNW > 0) {	// Look up-right
			for (char i = 1; i <= dNW; ++i) {
				if (BOARD(py-i, px+i) == WB1 || BOARD(py-i, px+i) == WB2 || is_white_queen(BOARD(py-i, px+i))) return true;
				if (BOARD(py-i, px+i) != EMPTY) break;
			}
		}
		const char dSE = min(px, 7-py);
		if (dSE > 0) {	// Look down-left
			for (char i = 1; i <= dSE; ++i) {
				if (BOARD(py+i, px-i) == WB1 || BOARD(py+i, px-i) == WB2 || is_white_queen(BOARD(py+i, px-i))) return true;
				if (BOARD(py+i, px-i) != EMPTY) break;
			}
		}
		const char dSW = min(7-px, 7-py);
		if (dSW > 0) {	// Look down-right
			for (char i = 1; i <= dSW; ++i) {
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

	wprintf(L"%s plays\n", (nextPlayer == WHITE) ? L"White" : L"Black");
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

	piece* board = st->board;
	wprintf(L"    a  b  c  d  e  f  g  h\n\n");
	for (int y = 0; y < SIZE; ++y) {
		wprintf(L"%d   ", y+1);
		for (int x = 0; x < SIZE; ++x) {
			wprintf(L"%lc  ", piece_char(BOARD(y, x)));
		}
		wprintf(L"  %d\n", y+1);
	}
	wprintf(L"\n    a  b  c  d  e  f  g  h\n");

	wprintf(L"Squares under attack:\n");
	char testlocs[64];
	double testboard[64];
	for (int i = 0; i < 64; ++i) {
		testlocs[i] = i;
		testboard[i] = is_under_attack(i, board, nextPlayer, enemy) ? 1.0 : 0.0;
	}
	chess_print_heatmap(st, testlocs, testboard, 64);
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
	bool* canCastleWhite = st->canCastleWhite;
	bool* canCastleBlack = st->canCastleBlack;
	bool* enPassantWhite = st->enPassantWhite;
	bool* enPassantBlack = st->enPassantBlack;
	
	if (chess_is_game_over(st)) {
		return false;
	}

	if (mv == MOVE_RESIGN) {
		return true;
	}

	if (mv < 0 || mv > MOVE_MAX) {
		wprintf(L"out of bounds\n");
		return false;
	}

	MOVE_UNPACK(ff, tt, fy, fx, ty, tx);
	const signed char dx = tx - fx;
	const signed char dy = ty - fy;
	const char ldx = abs(tx - fx);
	const char ldy = abs(ty - fy);

	if (ff == tt) {
		wprintf(L"not moving\n");
		return false;
	}

	piece pc = BOARD(fy, fx);

	// Do not play anything that's not own color (including "NOBODY" = empty squares)
	if (colorAt[ff] != friendly) {
		wprintf(L"not own color: colorAt[%d] = %d != %d\n", ff, colorAt[ff], friendly);
		return false;
	}

	// Do not trample own piece
	if (colorAt[tt] == friendly) {
		wprintf(L"trampling own color: colorAt[%d] = %d\n", tt, colorAt[tt]);
		return false;
	}

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
			if (fy == 3 && ty == 2 && ldx == 1 && BOARD(ty+1, tx) >= BP1 && BOARD(ty+1, tx) <= BP8 && enPassantBlack[BOARD(ty+1, tx) - BP1]) break;	// En-passant capture
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
			if (fy == 4 && ty == 5 && ldx == 1 && BOARD(ty+1, tx) >= WP1 && BOARD(ty+1, tx) <= WP8 && enPassantWhite[BOARD(ty+1, tx) - WP1]) break;	// En-passant capture
			return false;
			break;
		case WR1:
		case WR2:
		case BR1:
		case BR2:
			if (dy == 0) {	// Horizontal
				if (dx < 0) {	// Left
					for (char x = fx - 1; x >= tx + 1; --x) if (COLORAT(fy, x) != NOBODY) return false;
					break;
				} else {	// Right (dx > 0); the (dx == 0) case is handled earlier with "no movement"
					for (char x = fx + 1; x <= tx - 1; ++x) if (COLORAT(fy, x) != NOBODY) return false;
					break;
				}
			} else if (dx == 0) {	// Vertical
				if (dy < 0) {	// Up
					for (char y = fy - 1; y >= ty + 1; --y) if (COLORAT(y, fx) != NOBODY) return false;
					break;
				} else {	// Down (dy > 0); the (dy == 0) case is handled earlier with "no movement"
					for (char y = fy + 1; y <= ty - 1; ++y) if (COLORAT(y, fx) != NOBODY) return false;
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
					for (char i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx-i) != NOBODY) return false;
					break;
				} else {		// Up-right (dx > 0); (dx == 0) ruled out earlier
					for (char i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx+i) != NOBODY) return false;
					break;
				}
			} else {	// (dy > 0); (dy == 0) ruled out earlier
				if (dx < 0) {	// Down-left
					for (char i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx-i) != NOBODY) return false;
					break;
				} else {		// Down-right (dx > 0); (dx == 0) ruled out earlier
					for (char i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx+i) != NOBODY) return false;
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
					for (char y = fy - 1; y >= ty + 1; --y) if (COLORAT(y, fx) != NOBODY) return false;
					break;
				} else if (ldx == ldy) {
					if (dx < 0) {	// Up-left
						for (char i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx-i) != NOBODY) return false;
						break;
					} else {		// Up-right (dx > 0)
						for (char i = 1; i <= ldy - 1; ++i) if (COLORAT(fy-i, fx+i) != NOBODY) return false;
						break;
					}
				}
			} else if (dy == 0) {
				if (dx < 0) {	// Left
					for (char x = fx - 1; x >= tx + 1; --x) if (COLORAT(fy, x) != NOBODY) return false;
					break;
				} else {	// Right (dx > 0); the (dx == 0) case is handled earlier with "no movement"
					for (char x = fx + 1; x <= tx - 1; ++x) if (COLORAT(fy, x) != NOBODY) return false;
					break;
				}
			} else {
				if (dx == 0) {	// Down
					for (char y = fy + 1; y <= ty - 1; ++y) if (COLORAT(y, fx) != NOBODY) return false;
					break;
				} else if (ldx == ldy) {
					if (dx < 0) {	// Down-left
						for (char i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx-i) != NOBODY) return false;
						break;
					} else {		// Down-right (dx > 0)
						for (char i = 1; i <= ldy - 1; ++i) if (COLORAT(fy+i, fx+i) != NOBODY) return false;
						break;
					}
				}
			}
			return false;
			break;
		case WK:
			if (ff == 60 && (tt == 58 || tt == 62) && !is_under_attack(60, board, friendly, enemy)) {
				if (tt == 58 && canCastleWhite[0]												// Queen-side castle
					 && colorAt[57] == NOBODY && colorAt[58] == NOBODY && colorAt[59] == NOBODY	// Ensure path free for king & rook
					 && !is_under_attack(58, board, friendly, enemy)							// Don't go through check
					 && !is_under_attack(59, board, friendly, enemy)) break;
				if (tt == 62 && canCastleWhite[1]								// King-side castle
					 && colorAt[61] == NOBODY && colorAt[62] == NOBODY			// Ensure path free for king & rook
					 && !is_under_attack(61, board, friendly, enemy)			// Don't go through check
					 && !is_under_attack(62, board, friendly, enemy)) break;
			}
			if (ldx <= 1 && ldy <= 1) break;	// 1-square king move
			return false;
			break;
		case BK:
			if (ff == 4 && (tt == 2 || tt == 6) && !is_under_attack(4, board, friendly, enemy)) {
				if (tt == 2 && canCastleBlack[0]												// Queen-side castle
					 && colorAt[1] == NOBODY && colorAt[2] == NOBODY && colorAt[3] == NOBODY	// Ensure path free for king & rook
					 && !is_under_attack(1, board, friendly, enemy)								// Don't go through check
					 && !is_under_attack(2, board, friendly, enemy)
					 && !is_under_attack(3, board, friendly, enemy)) break;
				if (tt == 6 && canCastleBlack[1]								// King-side castle
					 && colorAt[5] == NOBODY && colorAt[6] == NOBODY			// Ensure path free for king & rook
					 && !is_under_attack(5, board, friendly, enemy)				// Don't go through check
					 && !is_under_attack(6, board, friendly, enemy)) break;
			}
			if (ldx <= 1 && ldy <= 1) break;	// 1-square king move
			return false;
			break;
		default:
			return false;
			break;
	}

	return true;
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
	int num = 0;

	if (chess_is_game_over(st)) {
		return 0;
	}

	move_list[num++] = MOVE_RESIGN;

	move mv;
	// TODO move_list[num++] = ...
	return num;
}

// Like chess_get_legal_moves, but without resignations
int chess_get_reasonable_moves(state* st, move* move_list) {
	int num = 0;

	if (chess_is_game_over(st)) {
		return 0;
	}

	move mv;
	// Allow only non-losing passes
	if (chess_is_move_reasonable(st, &mv)) {
		move_list[num] = mv;
		++num;
	}

	for (int i = 0; i < COUNT; ++i) {
		mv = i;
		if (chess_is_move_reasonable(st, &mv)) {
			move_list[num] = mv;
			++num;
		}
	}

	// Or resign when nowhere to play
	if (!num) {
		move_list[num] = MOVE_RESIGN;
		++num;
	}

	return num;
}

move_result chess_play_move(state* st, move* mv_ptr) {
	move mv = *mv_ptr;
	color friendly = st->nextPlayer;
	color enemy = (friendly == BLACK) ? WHITE : BLACK;

	if (mv == MOVE_RESIGN) {
		st->nextPlayer = enemy;
		return SUCCESS;
	}

	if (mv < 0 || mv >= MOVE_MAX) {
		return FAIL_BOUNDS;
	}

	st->nextPlayer = enemy;

	return SUCCESS;
}

// Plays a "random" move & stores it in mv
move_result chess_play_random_move(state* st, move* mv, move* move_list) {
	move tmp;
	int timeout = COUNT;	// Heuristics
	int rand_searches = 0;

	color me = st->nextPlayer;
	color notme = (me == BLACK) ? WHITE : BLACK;

	do {
		tmp = move_random();	// Random never resigns (mv = -2)
		rand_searches++;

		if (chess_play_move(st, &tmp) == SUCCESS) {
			*mv = tmp;
			return SUCCESS;
		}
	} while (rand_searches < timeout);

	// Few moves remaining; look for them
	int move_count = chess_get_legal_moves(st, move_list);
	if (move_count > 1) {
		tmp = move_list[RANDI(0, move_count)];
		return chess_play_move(st, &tmp);
	} else if (move_count == 1) {
		return chess_play_move(st, &move_list[0]);
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


void chess_print_heatmap(state* st, char* locations, double* values, int num_values) {
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
		case FAIL_CHECK:
			wprintf(L"King in check\n");
			break;
		case FAIL_OTHER:
		default:
			wprintf(L"Other\n");
			break;
	}
}
