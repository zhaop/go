#ifndef RAND_H
#define RAND_H

#include <stdint.h>


// Sorry for macro function declaration ugliness; in cases where randi performance is critical, declaring a static function seems fastest

// Initialize with two different non-zero seeds
// uint64_t SEED1, SEED2
#define INIT_MAKE_RANDI(SEED1, SEED2) \
static uint64_t xorshift128plus_state[2] = {(SEED1), (SEED2)}; \
static uint64_t xorshift128plus() { \
	uint64_t x = xorshift128plus_state[0]; \
	uint64_t const y = xorshift128plus_state[1]; \
	xorshift128plus_state[0] = y; \
	x ^= x << 23; \
	xorshift128plus_state[1] = x ^ y ^ (x >> 17) ^ (y >> 26); \
	return xorshift128plus_state[1] + y; \
}


// Make a function that returns a random integer from PARAM_A to PARAM_B
// (PARAM_B - PARAM_A) < 32
// BUG avail could become negative (probability for L=26: (6/32)^60 ~= 10^-43 every inbuf refill)
// string FUNCTION_NAME (without double_quotes)
// int PARAM_A
// int PARAM_B
#define MAKE_RANDI32(FUNCTION_NAME, PARAM_A, PARAM_B) \
static int FUNCTION_NAME(void) { \
	static int outbuf[60]; \
	static uint8_t avail = 0; \
	if (avail > 0) { \
		--avail; \
		return outbuf[avail]; \
	} \
	const int a = (PARAM_A); \
	const uint8_t L = (PARAM_B) - a; \
	static uint64_t inbuf[5]; \
	uint8_t v; \
	inbuf[0] = xorshift128plus(); \
	inbuf[1] = xorshift128plus(); \
	inbuf[2] = xorshift128plus(); \
	inbuf[3] = xorshift128plus(); \
	inbuf[4] = xorshift128plus(); \
	const uint8_t* bf = (uint8_t*)inbuf; \
	v = bf[0] & 0x1F;							if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[0] & 0xE0) >> 3) + (bf[1] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[1] & 0x7C) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[2] & 0x1F;							if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[2] & 0xE0) >> 3) + (bf[3] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[3] & 0x7C) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[4] & 0x1F;							if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[4] & 0xE0) >> 3) + (bf[5] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[5] & 0x7C) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[6] & 0x1F;							if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[6] & 0xE0) >> 3) + (bf[7] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[7] & 0x7C) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[8] & 0x1F;							if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[8] & 0xE0) >> 3) + (bf[9] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[9] & 0x7C) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[10] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[10] & 0xE0) >> 3) + (bf[11] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[11] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[12] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[12] & 0xE0) >> 3) + (bf[13] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[13] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[14] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[14] & 0xE0) >> 3) + (bf[15] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[15] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[16] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[16] & 0xE0) >> 3) + (bf[17] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[17] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[18] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[18] & 0xE0) >> 3) + (bf[19] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[19] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[20] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[20] & 0xE0) >> 3) + (bf[21] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[21] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[22] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[22] & 0xE0) >> 3) + (bf[23] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[23] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[24] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[24] & 0xE0) >> 3) + (bf[25] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[25] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[26] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[26] & 0xE0) >> 3) + (bf[27] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[27] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[28] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[28] & 0xE0) >> 3) + (bf[29] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[29] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[30] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[30] & 0xE0) >> 3) + (bf[31] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[31] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[32] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[32] & 0xE0) >> 3) + (bf[33] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[33] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[34] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[34] & 0xE0) >> 3) + (bf[35] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[35] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[36] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[36] & 0xE0) >> 3) + (bf[37] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[37] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[38] & 0x1F;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[38] & 0xE0) >> 3) + (bf[39] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[39] & 0x7C) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	--avail; \
	return outbuf[avail]; \
}


// Make a function that returns a random integer from PARAM_A to PARAM_B
// (PARAM_B - PARAM_A) < 128
// Takes 8 random numbers of 7bits out of each 56 => 54 out of 384
//     [7]          [6]          [5]          [4]          [3]           [2]          [1]          [0]
// 1|111 1111   |1111 111|1   1111 11|11   1111 1|111   1111 |1111   111|1 1111   11|11 1111   1|111 1111
//    $7f          $fe  $01    $fc  $03     $f8  $07    $f0  $0f    $e0   $1f    $c0  $3f    $80   $7f
//  |    n8     |   n7   |     n6    |     n5    |     n4    |    n3    |     n2    |     n1    |   n0   
// BUG avail could become negative (probability for L=82: (46/128)^54 ~= 10^-24 every inbuf refill)
// string FUNCTION_NAME (without double_quotes)
// int PARAM_A
// int PARAM_B
#define MAKE_RANDI128(FUNCTION_NAME, PARAM_A, PARAM_B) \
static int FUNCTION_NAME(void) { \
	static int outbuf[54]; \
	static uint8_t avail = 0; \
	if (avail > 0) { \
		--avail; \
		return outbuf[avail]; \
	} \
	const int a = (PARAM_A); \
	const uint8_t L = (PARAM_B) - a; \
	static uint64_t inbuf[7]; \
	uint8_t v; \
	inbuf[0] = xorshift128plus(); \
	inbuf[1] = xorshift128plus(); \
	inbuf[2] = xorshift128plus(); \
	inbuf[3] = xorshift128plus(); \
	inbuf[4] = xorshift128plus(); \
	inbuf[5] = xorshift128plus(); \
	const uint8_t* bf = (uint8_t*)inbuf; \
	v = bf[0] & 0x7f;							if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[0] & 0x80) >> 1) + (bf[1] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[1] & 0xc0) >> 1) + (bf[2] & 0x1f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[2] & 0xe0) >> 1) + (bf[3] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[3] & 0xf0) >> 1) + (bf[4] & 0x07);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[4] & 0xf8) >> 1) + (bf[5] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[5] & 0xfc) >> 1) + (bf[6] & 0x01);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[6] & 0xfe) >> 1);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[7] & 0x7f;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[7] & 0x80) >> 1) + (bf[8] & 0x3f);		if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[8] & 0xc0) >> 1) + (bf[9] & 0x1f);		if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[9] & 0xe0) >> 1) + (bf[10] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[10] & 0xf0) >> 1) + (bf[11] & 0x07);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[11] & 0xf8) >> 1) + (bf[12] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[12] & 0xfc) >> 1) + (bf[13] & 0x01);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[13] & 0xfe) >> 1);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[14] & 0x7f;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[14] & 0x80) >> 1) + (bf[15] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[15] & 0xc0) >> 1) + (bf[16] & 0x1f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[16] & 0xe0) >> 1) + (bf[17] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[17] & 0xf0) >> 1) + (bf[18] & 0x07);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[18] & 0xf8) >> 1) + (bf[19] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[19] & 0xfc) >> 1) + (bf[20] & 0x01);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[20] & 0xfe) >> 1);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[21] & 0x7f;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[21] & 0x80) >> 1) + (bf[22] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[22] & 0xc0) >> 1) + (bf[23] & 0x1f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[23] & 0xe0) >> 1) + (bf[24] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[24] & 0xf0) >> 1) + (bf[25] & 0x07);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[25] & 0xf8) >> 1) + (bf[26] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[26] & 0xfc) >> 1) + (bf[27] & 0x01);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[27] & 0xfe) >> 1);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[28] & 0x7f;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[28] & 0x80) >> 1) + (bf[29] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[29] & 0xc0) >> 1) + (bf[30] & 0x1f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[30] & 0xe0) >> 1) + (bf[31] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[31] & 0xf0) >> 1) + (bf[32] & 0x07);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[32] & 0xf8) >> 1) + (bf[33] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[33] & 0xfc) >> 1) + (bf[34] & 0x01);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[34] & 0xfe) >> 1);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[35] & 0x7f;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[35] & 0x80) >> 1) + (bf[36] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[36] & 0xc0) >> 1) + (bf[37] & 0x1f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[37] & 0xe0) >> 1) + (bf[38] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[38] & 0xf0) >> 1) + (bf[39] & 0x07);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[39] & 0xf8) >> 1) + (bf[40] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[40] & 0xfc) >> 1) + (bf[41] & 0x01);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[41] & 0xfe) >> 1);						if (v <= L) outbuf[avail++] = v + a; \
	v = bf[42] & 0x7f;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[42] & 0x80) >> 1) + (bf[43] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[43] & 0xc0) >> 1) + (bf[44] & 0x1f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[44] & 0xe0) >> 1) + (bf[45] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[45] & 0xf0) >> 1) + (bf[46] & 0x07);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[46] & 0xf8) >> 1) + (bf[47] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	--avail; \
	return outbuf[avail]; \
}


// Make a function that returns a random integer from PARAM_A to PARAM_B
// (PARAM_B - PARAM_A) < 512
// Takes 7 random numbers from 4 x 16b blocks
//          [3]                     [2]                     [1]                     [0]
// 1|111 1111 11|11 1111   111|1 1111 1111 |1111   1111 1|111 1111 11|11   1111 111|1 1111 1111
//      $7fc0     $3f    $e000    $1ff0     $0f     $f800     $7fc    $03    $fe00     $1ff
//  |    n6     |     n5      |     n4     |     n3      |     n2    |     n1      |    n0
// BUG avail could become negative (probability for L=362: (150/512)^70 ~= 10^-37 every inbuf refill)
// string FUNCTION_NAME (without double_quotes)
// int PARAM_A
// int PARAM_B
#define MAKE_RANDI512(FUNCTION_NAME, PARAM_A, PARAM_B) \
static int FUNCTION_NAME(void) { \
	static int outbuf[70]; \
	static uint8_t avail = 0; \
	if (avail > 0) { \
		--avail; \
		return outbuf[avail]; \
	} \
	const int a = (PARAM_A); \
	const uint16_t L = (PARAM_B) - a; \
	static uint64_t inbuf[10]; \
	uint16_t v; \
	inbuf[0] = xorshift128plus(); \
	inbuf[1] = xorshift128plus(); \
	inbuf[2] = xorshift128plus(); \
	inbuf[3] = xorshift128plus(); \
	inbuf[4] = xorshift128plus(); \
	inbuf[5] = xorshift128plus(); \
	inbuf[6] = xorshift128plus(); \
	inbuf[7] = xorshift128plus(); \
	inbuf[8] = xorshift128plus(); \
	inbuf[9] = xorshift128plus(); \
	const uint16_t* bf = (uint16_t*)inbuf; \
	v = bf[0] & 0x1ff;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[0] & 0xfe00) >> 7) + (bf[1] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[1] & 0x7fc) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[1] & 0xf800) >> 7) + (bf[2] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[2] & 0x1ff0) >> 4);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[2] & 0xe000) >> 7) + (bf[3] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[3] & 0x7fc0) >> 6);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[4] & 0x1ff;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[4] & 0xfe00) >> 7) + (bf[5] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[5] & 0x7fc) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[5] & 0xf800) >> 7) + (bf[6] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[6] & 0x1ff0) >> 4);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[6] & 0xe000) >> 7) + (bf[7] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[7] & 0x7fc0) >> 6);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[8] & 0x1ff;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[8] & 0xfe00) >> 7) + (bf[9] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[9] & 0x7fc) >> 2);						if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[9] & 0xf800) >> 7) + (bf[10] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[10] & 0x1ff0) >> 4);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[10] & 0xe000) >> 7) + (bf[11] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[11] & 0x7fc0) >> 6);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[12] & 0x1ff;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[12] & 0xfe00) >> 7) + (bf[13] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[13] & 0x7fc) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[13] & 0xf800) >> 7) + (bf[14] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[14] & 0x1ff0) >> 4);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[14] & 0xe000) >> 7) + (bf[15] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[15] & 0x7fc0) >> 6);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[16] & 0x1ff;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[16] & 0xfe00) >> 7) + (bf[17] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[17] & 0x7fc) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[17] & 0xf800) >> 7) + (bf[18] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[18] & 0x1ff0) >> 4);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[18] & 0xe000) >> 7) + (bf[19] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[19] & 0x7fc0) >> 6);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[20] & 0x1ff;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[20] & 0xfe00) >> 7) + (bf[21] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[21] & 0x7fc) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[21] & 0xf800) >> 7) + (bf[22] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[22] & 0x1ff0) >> 4);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[22] & 0xe000) >> 7) + (bf[23] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[23] & 0x7fc0) >> 6);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[24] & 0x1ff;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[24] & 0xfe00) >> 7) + (bf[25] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[25] & 0x7fc) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[25] & 0xf800) >> 7) + (bf[26] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[26] & 0x1ff0) >> 4);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[26] & 0xe000) >> 7) + (bf[27] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[27] & 0x7fc0) >> 6);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[28] & 0x1ff;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[28] & 0xfe00) >> 7) + (bf[29] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[29] & 0x7fc) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[29] & 0xf800) >> 7) + (bf[30] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[30] & 0x1ff0) >> 4);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[30] & 0xe000) >> 7) + (bf[31] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[31] & 0x7fc0) >> 6);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[32] & 0x1ff;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[32] & 0xfe00) >> 7) + (bf[33] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[33] & 0x7fc) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[33] & 0xf800) >> 7) + (bf[34] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[34] & 0x1ff0) >> 4);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[34] & 0xe000) >> 7) + (bf[35] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[35] & 0x7fc0) >> 6);					if (v <= L) outbuf[avail++] = v + a; \
	v = bf[36] & 0x1ff;								if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[36] & 0xfe00) >> 7) + (bf[37] & 0x03);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[37] & 0x7fc) >> 2);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[37] & 0xf800) >> 7) + (bf[38] & 0x0f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[38] & 0x1ff0) >> 4);					if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[38] & 0xe000) >> 7) + (bf[39] & 0x3f);	if (v <= L) outbuf[avail++] = v + a; \
	v = ((bf[39] & 0x7fc0) >> 6);					if (v <= L) outbuf[avail++] = v + a; \
	--avail; \
	return outbuf[avail]; \
}


#endif