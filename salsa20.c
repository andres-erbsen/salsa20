#include <stdio.h>
#include <stdint.h> // we use 32-bit words

// rotate x to left by n bits, the bits that go over the left edge reappear on the right
#define R(x,n) (((x) << (n)) | ((x) >> (32-(n))))

// addition wraps modulo 2^32
// the choice of 7,9,13,18 "doesn't seem very important" (spec)
#define quarter(a,b,c,d) do {\
	b ^= R(d+a, 7);\
	c ^= R(a+b, 9);\
	d ^= R(b+c, 13);\
	a ^= R(c+d, 18);\
} while (0)

void salsa20_words(uint32_t *out, uint32_t in[16]) {
	uint32_t x[4][4];
	int i;
	for (i=0; i<16; ++i) x[i/4][i%4] = in[i];
	for (i=0; i<10; ++i) { // 10 double rounds = 20 rounds
		// column round: quarter round on each column; start at ith element and wrap
		quarter(x[0][0], x[1][0], x[2][0], x[3][0]);
		quarter(x[1][1], x[2][1], x[3][1], x[0][1]);
		quarter(x[2][2], x[3][2], x[0][2], x[1][2]);
		quarter(x[3][3], x[0][3], x[1][3], x[2][3]);
		// row round: quarter round on each row; start at ith element and wrap around
		quarter(x[0][0], x[0][1], x[0][2], x[0][3]);
		quarter(x[1][1], x[1][2], x[1][3], x[1][0]);
		quarter(x[2][2], x[2][3], x[2][0], x[2][1]);
		quarter(x[3][3], x[3][0], x[3][1], x[3][2]);
	}
	for (i=0; i<16; ++i) out[i] = x[i/4][i%4] + in[i];
}

// inputting a key, message nonce, keystream index and constants to that transormation
void salsa20_block(uint8_t *out, uint8_t key[32], uint64_t nonce, uint64_t index) {
	static const char c[16] = "expand 32-byte k"; // arbitrary constant
	#define LE(p) ( (p)[0] | ((p)[1]<<8) | ((p)[2]<<16) | ((p)[3]<<24) )
	uint32_t in[16] = {LE(c),            LE(key),    LE(key+4),        LE(key+8),
	                   LE(key+12),       LE(c+4),    nonce&0xffffffff, nonce>>32,
	                   index&0xffffffff, index>>32,  LE(c+8),          LE(key+16),
	                   LE(key+20),       LE(key+24), LE(key+28),       LE(c+12)};
	uint32_t wordout[16];
	salsa20_words(wordout, in);
	int i;
	for (i=0; i<64; ++i) out[i] = 0xff & (wordout[i/4] >> (8*(i%4)));
}

// enc/dec: xor a message with transformations of key, a per-message nonce and block index
void salsa20(uint8_t *message, uint64_t mlen, uint8_t key[32], uint64_t nonce) {
	int i;
	uint8_t block[64];
	for (i=0; i<mlen; i++) {
		if (i%64 == 0) salsa20_block(block, key, nonce, i/64);
		message[i] ^= block[i%64];
	}
}

//Set 2, vector# 0:
//                         key = 00000000000000000000000000000000
//                               00000000000000000000000000000000
//                          IV = 0000000000000000
//               stream[0..63] = 9A97F65B9B4C721B960A672145FCA8D4
//                               E32E67F9111EA979CE9C4826806AEEE6
//                               3DE9C0DA2BD7F91EBCB2639BF989C625
//                               1B29BF38D39A9BDCE7C55F4B2AC12A39

int  main () {
	uint8_t key[32] = {0};
	uint64_t nonce = 0;
	uint8_t msg[64] = {0};
	
	salsa20(msg, sizeof(msg), key, nonce);
	int i; for (i=0; i<sizeof(msg); ++i) printf("%02X", msg[i]); printf("\n");
}
