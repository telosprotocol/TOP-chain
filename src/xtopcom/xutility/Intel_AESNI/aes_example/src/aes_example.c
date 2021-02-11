/* ex: set ts=4: */
/*
 based on code from http://www.progressive-coding.com/tutorial.php?id=3#end
 ---------------------------------------------------------------------------
 Copyright (c) 2007-2010, Laurent Haan, Luxembourg, LU. All rights reserved.
 
 LICENSE TERMS
 
 The redistribution and use of this software (with or without changes)
 is allowed without the payment of fees or royalties provided that:
 
  1. source code distributions include the above copyright notice, this
     list of conditions and the following disclaimer;
 
  2. binary distributions include the above copyright notice, this list
     of conditions and the following disclaimer in their documentation;
 
  3. the name of the copyright holder is not used to endorse products
     built using this software without specific written permission.
 
 DISCLAIMER
 
 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 23/02/2010
*/

/* 
 * See also http://en.wikipedia.org/wiki/Cipher_block_chaining#Cipher-block_chaining_.28CBC.29 for cipher mode explanations
 */
#include <stdio.h>
#include <malloc.h>
#include "my_getopt.h"

#ifdef __linux__
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

#define USE_INTEL_AES 1

#if(USE_INTEL_AES==1)
#include "iaesni.h"
#include "iaes_asm_interface.h"
#endif

#ifdef __linux__
#ifndef __LP64__
#define do_rdtsc _do_rdtsc
#endif
#endif

extern unsigned long long do_rdtsc(void);

int verbose=0;

//unsigned char key[32] = {"11111111111111111111111111111111"};
//unsigned char key[32] = {"0f0e0d0c0b0a09080706050403020100"};
//unsigned char gkey[32] = {0x0};
/* 
unsigned char gkey[32] = {
0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,
0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00
};
*/
// test key
//95A8EE8E89979B9EFDCBC6EB9797528D432DC26061553818EA635EC5D5A7727E
// test input
// 4EC137A426DABF8AA0BEB8BC0C2B89D6
unsigned char test_input[]={ 0x4E,0xC1,0x37,0xA4,0x26,0xDA,0xBF,0x8A,0xA0,0xBE,0xB8,0xBC,0x0C,0x2B,0x89,0xD6};
// ciphered output
// 2F9CFDDBFFCDE6B9F37EF8E40D512CF4
unsigned char enc_input[]={ 0x2F,0x9C,0xFD,0xDB,0xFF,0xCD,0xE6,0xB9,0xF3,0x7E,0xF8,0xE4,0x0D,0x51,0x2C,0xF4};
// dec output
// 110A3545CE49B84BBB7B35236108FA6E
unsigned char dec_input[]={ 0x11,0x0A,0x35,0x45,0xCE,0x49,0xB8,0x4B,0xBB,0x7B,0x35,0x23,0x61,0x08,0xFA,0x6E};
unsigned char gkey[32] = {
0x95,0xA8,0xEE,0x8E,0x89,0x97,0x9B,0x9E,0xFD,0xCB,0xC6,0xEB,0x97,0x97,0x52,0x8D,
0x43,0x2D,0xC2,0x60,0x61,0x55,0x38,0x18,0xEA,0x63,0x5E,0xC5,0xD5,0xA7,0x72,0x7E
};

unsigned char test_iv[16] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};

unsigned char sbox[256] =   {
//0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, //0
0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, //1
0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, //2
0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, //3
0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, //4
0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, //5
0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, //6
0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, //7
0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, //8
0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, //9
0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, //A
0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, //B
0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, //C
0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, //D
0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, //E
0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 }; //F

unsigned char rsbox[256] =
{ 0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb
, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb
, 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e
, 0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25
, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92
, 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84
, 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06
, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b
, 0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73
, 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e
, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b
, 0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4
, 0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f
, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef
, 0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61
, 0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d };

unsigned char getSBoxValue(unsigned char num)
{
    return sbox[num];
}

unsigned char getSBoxInvert(unsigned char num)
{
    return rsbox[num];
}

/* Rijndael's key schedule rotate operation
 * rotate the word eight bits to the left
 *
 * rotate(1d2c3a4f) = 2c3a4f1d
 *
 * word is an char array of size 4 (32 bit)
 */
void rotate(unsigned char *word)
{
    unsigned char c;
    int i;

    c = word[0];
    for (i = 0; i < 3; i++)
        word[i] = word[i+1];
    word[3] = c;
}
unsigned char Rcon[255] = {

0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8,
0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3,
0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f,
0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d,
0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab,
0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d,
0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25,
0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01,
0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d,
0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa,
0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a,
0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02,
0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a,
0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef,
0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94,
0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04,
0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f,
0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5,
0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33,
0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb};

unsigned char getRconValue(unsigned char num)
{
    return Rcon[num];
}

void core(unsigned char *word, int iteration)
{
    int i;

    /* rotate the 32-bit word 8 bits to the left */
    rotate(word);

    /* apply S-Box substitution on all 4 parts of the 32-bit word */
    for (i = 0; i < 4; ++i)
    {
        word[i] = getSBoxValue(word[i]);
    }

    /* XOR the output of the rcon operation with i to the first part (leftmost) only */
    word[0] = word[0]^getRconValue(iteration);
}

enum keySize{
    SIZE_16 = 16,
    SIZE_24 = 24,
    SIZE_32 = 32
    };

enum enc_dec_method {
		USE_SAMPLE_CODE=0x01,
		USE_iAES       =0x02,
		USE_iAES_BLK   =0x04,
		USE_iAES_CBC   =0x08,
		USE_iAES_CTR   =0x10,
		USE_ARRAY      =0x20,
		// if you add something to this enum then you need to add it (except for the USE_ARRAY entry) to the 
		// check_sizes_and_methods() routine 'method[]' array
		//
};

enum operMode{
    OPER_ENCRYPT=0,
    OPER_DECRYPT=1
    };

void expandKey(unsigned char *expandedKey, unsigned char *key, enum keySize, size_t expandedKeySize);


/* Rijndael's key expansion
 * expands an 128,192,256 key into an 176,208,240 bytes key
 *
 * expandedKey is a pointer to an char array of large enough size
 * key is a pointer to a non-expanded key
 */

void expandKey(unsigned char *expandedKey,
               unsigned char *key,
               enum keySize size,
               size_t expandedKeySize)
{
    /* current expanded keySize, in bytes */
    int currentSize = 0;
    int rconIteration = 1;
    int i;
    unsigned char t[4] = {0};   // temporary 4-byte variable

    /* set the 16,24,32 bytes of the expanded key to the input key */
    for (i = 0; i < size; i++)
        expandedKey[i] = key[i];
    currentSize += size;

	//printf("expandedKey size= %d and expandedKeySize= %d at %s %d\n", size, expandedKeySize, __FILE__, __LINE__);

    while (currentSize < expandedKeySize)
    {
        /* assign the previous 4 bytes to the temporary value t */
        for (i = 0; i < 4; i++)
        {
            t[i] = expandedKey[(currentSize - 4) + i];
        }

        /* every 16,24,32 bytes we apply the core schedule to t
         * and increment rconIteration afterwards
         */
        if(currentSize % size == 0)
        {
            core(t, rconIteration++);
        }

        /* For 256-bit keys, we add an extra sbox to the calculation */
        if(size == SIZE_32 && ((currentSize % size) == 16)) {
            for(i = 0; i < 4; i++)
                t[i] = getSBoxValue(t[i]);
        }

        /* We XOR t with the four-byte block 16,24,32 bytes before the new expanded key.
         * This becomes the next four bytes in the expanded key.
         */
        for(i = 0; i < 4; i++) {
            expandedKey[currentSize] = expandedKey[currentSize - size] ^ t[i];
            currentSize++;
        }
    }
}
void subBytes(unsigned char *state)
{
    int i;
    /* substitute all the values from the state with the value in the SBox
     * using the state value as index for the SBox
     */
    for (i = 0; i < 16; i++)
        state[i] = getSBoxValue(state[i]);
}
void shiftRow(unsigned char *state, unsigned char nbr)
{
    int i, j;
    unsigned char tmp;
    /* each iteration shifts the row to the left by 1 */
    for (i = 0; i < nbr; i++)
    {
        tmp = state[0];
        for (j = 0; j < 3; j++)
            state[j] = state[j+1];
        state[3] = tmp;
    }
}
void shiftRows(unsigned char *state)
{
    int i;
    /* iterate over the 4 rows and call shiftRow() with that row */
    for (i = 0; i < 4; i++)
        shiftRow(state+i*4, i);
}

void addRoundKey(unsigned char *state, unsigned char *roundKey)
{
    int i;
    for (i = 0; i < 16; i++)
        state[i] = state[i] ^ roundKey[i] ;
}

unsigned char galois_multiplication(unsigned char a, unsigned char b)
{
    unsigned char p = 0;
	unsigned char counter;
	unsigned char hi_bit_set;
	for(counter = 0; counter < 8; counter++) {
		if((b & 1) == 1)
			p ^= a;
		hi_bit_set = (a & 0x80);
		a <<= 1;
		if(hi_bit_set == 0x80)
			a ^= 0x1b;
		b >>= 1;
	}
	return p;
}
void mixColumn(unsigned char *column)
{
    unsigned char cpy[4];
	int i;
	for(i = 0; i < 4; i++)
    {
		cpy[i] = column[i];
    }
	column[0] = galois_multiplication(cpy[0],2) ^
                galois_multiplication(cpy[3],1) ^
                galois_multiplication(cpy[2],1) ^
                galois_multiplication(cpy[1],3);

	column[1] = galois_multiplication(cpy[1],2) ^
                galois_multiplication(cpy[0],1) ^
                galois_multiplication(cpy[3],1) ^
                galois_multiplication(cpy[2],3);

	column[2] = galois_multiplication(cpy[2],2) ^
                galois_multiplication(cpy[1],1) ^
                galois_multiplication(cpy[0],1) ^
                galois_multiplication(cpy[3],3);

	column[3] = galois_multiplication(cpy[3],2) ^
                galois_multiplication(cpy[2],1) ^
                galois_multiplication(cpy[1],1) ^
                galois_multiplication(cpy[0],3);
}

void mixColumns(unsigned char *state)
{
    int i, j;
    unsigned char column[4];

    /* iterate over the 4 columns */
    for (i = 0; i < 4; i++)
    {
        /* construct one column by iterating over the 4 rows */
        for (j = 0; j < 4; j++)
        {
            column[j] = state[(j*4)+i];
        }

        /* apply the mixColumn on one column */
        mixColumn(column);

        /* put the values back into the state */
        for (j = 0; j < 4; j++)
        {
            state[(j*4)+i] = column[j];
        }
    }
}
void aes_round(unsigned char *state, unsigned char *roundKey)
{
    subBytes(state);
    shiftRows(state);
    mixColumns(state);
    addRoundKey(state, roundKey);
}
void createRoundKey(unsigned char *expandedKey, unsigned char *roundKey)
{
    int i,j;
    /* iterate over the columns */
    for (i = 0; i < 4; i++)
    {
        /* iterate over the rows */
        for (j = 0; j < 4; j++)
            roundKey[(i+(j*4))] = expandedKey[(i*4)+j];
    }
}


void aes_main(unsigned char *state, unsigned char *expandedKey, int nbrRounds)
{
    int i = 0;

    unsigned char roundKey[16];

    createRoundKey(expandedKey, roundKey);
    addRoundKey(state, roundKey);

    for (i = 1; i < nbrRounds; i++) {
        createRoundKey(expandedKey + 16*i, roundKey);
        aes_round(state, roundKey);
    }

    createRoundKey(expandedKey + 16*nbrRounds, roundKey);
    subBytes(state);
    shiftRows(state);
    addRoundKey(state, roundKey);
}

/* aes decryption -------------
 *
 */

void invSubBytes(unsigned char *state)
{
    int i;
    /* substitute all the values from the state with the value in the SBox
     * using the state value as index for the SBox
     */
    for (i = 0; i < 16; i++)
        state[i] = getSBoxInvert(state[i]);
}

void invShiftRow(unsigned char *state, unsigned char nbr)
{
    int i, j;
    unsigned char tmp;
    /* each iteration shifts the row to the right by 1 */
    for (i = 0; i < nbr; i++)
    {
        tmp = state[3];
        for (j = 3; j > 0; j--)
            state[j] = state[j-1];
        state[0] = tmp;
    }
}

void invShiftRows(unsigned char *state)
{
    int i;
    /* iterate over the 4 rows and call invShiftRow() with that row */
    for (i = 0; i < 4; i++)
        invShiftRow(state+i*4, i);
}

void invMixColumn(unsigned char *column)
{
    unsigned char cpy[4];
	int i;
	for(i = 0; i < 4; i++)
    {
		cpy[i] = column[i];
    }
	column[0] = galois_multiplication(cpy[0],14) ^
                galois_multiplication(cpy[3],9) ^
                galois_multiplication(cpy[2],13) ^
                galois_multiplication(cpy[1],11);
	column[1] = galois_multiplication(cpy[1],14) ^
                galois_multiplication(cpy[0],9) ^
                galois_multiplication(cpy[3],13) ^
                galois_multiplication(cpy[2],11);
	column[2] = galois_multiplication(cpy[2],14) ^
                galois_multiplication(cpy[1],9) ^
                galois_multiplication(cpy[0],13) ^
                galois_multiplication(cpy[3],11);
	column[3] = galois_multiplication(cpy[3],14) ^
                galois_multiplication(cpy[2],9) ^
                galois_multiplication(cpy[1],13) ^
                galois_multiplication(cpy[0],11);
}
void invMixColumns(unsigned char *state)
{
    int i, j;
    unsigned char column[4];

    /* iterate over the 4 columns */
    for (i = 0; i < 4; i++)
    {
        /* construct one column by iterating over the 4 rows */
        for (j = 0; j < 4; j++)
        {
            column[j] = state[(j*4)+i];
        }

        /* apply the invMixColumn on one column */
        invMixColumn(column);

        /* put the values back into the state */
        for (j = 0; j < 4; j++)
        {
            state[(j*4)+i] = column[j];
        }
    }
}

void aes_invRound(unsigned char *state, unsigned char *roundKey)
{

    invShiftRows(state);
    invSubBytes(state);
    addRoundKey(state, roundKey);
    invMixColumns(state);
}


void aes_invMain(unsigned char *state, unsigned char *expandedKey, int nbrRounds)
{
    int i = 0;

    unsigned char roundKey[16];

    createRoundKey(expandedKey + 16*nbrRounds, roundKey);
    addRoundKey(state, roundKey);

    for (i = nbrRounds-1; i > 0; i--) {
        createRoundKey(expandedKey + 16*i, roundKey);
        aes_invRound(state, roundKey);
    }

    createRoundKey(expandedKey, roundKey);
    invShiftRows(state);
    invSubBytes(state);
    addRoundKey(state, roundKey);
}
#define UNKNOWN_KEYSIZE -1
#define MEMORY_ALLOCATION_PROBLEM -2

int get_nrounds( enum keySize key_size_in_bytes, int line)
{
	int nrounds=0;
	switch (key_size_in_bytes)
	{
        case SIZE_16:
			nrounds = 10;
			break;
		case SIZE_24:
			nrounds = 12;
			break;
		case SIZE_32:
			nrounds = 14;
			break;
		default:
			printf("screw up in get_nrounds(%d) called by line= %d. Bye at %s %d\n", key_size_in_bytes, line, __FILE__, __LINE__);
			exit(2);
	}
	return nrounds;
}

unsigned long long iaes_time_expand_key( int crypt_method, unsigned char *key, enum keySize size, int loops)
{
    /* the expanded keySize */
    static int expandedKeySize=0;

    /* the expanded key */
    static unsigned char *expandedKey;
	int i;
	unsigned long long tsc_beg, tsc_end;

	if( expandedKeySize == 0)
	{
    	/* the number of rounds */
    	int nbrRounds;
    	/* set the number of rounds */
		nbrRounds = get_nrounds(size, __LINE__);
		// note that I save off key once at first calling.
		// So... if you change the key after the first time through here,
		// you'll need to change this code too.
		expandedKeySize = (16*(nbrRounds+1));
		expandedKey = (unsigned char *)malloc(expandedKeySize * sizeof(char));
		if (expandedKey == NULL)
		{
			printf("screw up here at %s %d\n", __FILE__, __LINE__);
			exit(2);
			return MEMORY_ALLOCATION_PROBLEM;
		}
	}
	if(crypt_method == OPER_ENCRYPT)
	{
		switch (size)
		{
			case SIZE_16:
				tsc_beg = do_rdtsc();
				for(i=0; i < loops; i++)
				{
					iEncExpandKey128(key, expandedKey);
				}
				tsc_end = do_rdtsc() - tsc_beg;
				break;
			case SIZE_24:
				tsc_beg = do_rdtsc();
				for(i=0; i < loops; i++)
				{
					iEncExpandKey192(key, expandedKey);
				}
				tsc_end = do_rdtsc() - tsc_beg;
				break;
			case SIZE_32:
				tsc_beg = do_rdtsc();
				for(i=0; i < loops; i++)
				{
					iEncExpandKey256(key, expandedKey);
				}
				tsc_end = do_rdtsc() - tsc_beg;
				break;
			default:
				printf("screw up here at %s %d\n", __FILE__, __LINE__);
				exit(2);
		}
	}
	else
	{
		switch (size)
		{
			case SIZE_16:
				tsc_beg = do_rdtsc();
				for(i=0; i < loops; i++)
				{
					iDecExpandKey128(key, expandedKey);
				}
				tsc_end = do_rdtsc() - tsc_beg;
				break;
			case SIZE_24:
				tsc_beg = do_rdtsc();
				for(i=0; i < loops; i++)
				{
					iDecExpandKey192(key, expandedKey);
				}
				tsc_end = do_rdtsc() - tsc_beg;
				break;
			case SIZE_32:
				tsc_beg = do_rdtsc();
				for(i=0; i < loops; i++)
				{
					iDecExpandKey256(key, expandedKey);
				}
				tsc_end = do_rdtsc() - tsc_beg;
				break;
			default:
				printf("screw up here at %s %d\n", __FILE__, __LINE__);
				exit(2);
		}
	}
	return tsc_end;
}

char aes_encrypt(unsigned int crypt_method,
				unsigned char *input,
				unsigned char *output,
				unsigned char *key,
				enum keySize size)
{
    /* the expanded keySize */
    static int expandedKeySize=0;

    /* the number of rounds */
    int nbrRounds;

    /* the expanded key */
    static unsigned char *expandedKey;

    /* the 128 bit block to encode */
    unsigned char block[16];

    int i,j;

    /* set the number of rounds */
	nbrRounds = get_nrounds(size, __LINE__);

	if(!(crypt_method & USE_iAES || crypt_method & USE_SAMPLE_CODE))
	{
			printf("For this routine you can only do crypt_methods:  USE_SAMPLE_CODE or USE_iAES at %s %d\n", __FILE__, __LINE__);
			exit(1);
	}

	if( expandedKeySize == 0)
	{
		// note that I save off key once at first calling.
		// So... if you change the key after the first time through here,
		// you'll need to change this code too.
		expandedKeySize = (16*(nbrRounds+1));
		expandedKey = (unsigned char *)malloc(expandedKeySize * sizeof(char));
		if (expandedKey == NULL)
		{
			printf("screw up here at %s %d\n", __FILE__, __LINE__);
			exit(2);
			return MEMORY_ALLOCATION_PROBLEM;
		}
		//printf("expandedKeySize= %d key= %p expandedKey= %p at %s %d\n", expandedKeySize, key, expandedKey, __FILE__, __LINE__);
		/* expand the key into an 176, 208, 240 bytes key */
		if(crypt_method & USE_iAES)
		{
			switch (size)
			{
				case SIZE_16:
					iEncExpandKey128(key, expandedKey);
					break;
				case SIZE_24:
					iEncExpandKey192(key, expandedKey);
					break;
				case SIZE_32:
					iEncExpandKey256(key, expandedKey);
					break;
				default:
					printf("screw up here at %s %d\n", __FILE__, __LINE__);
					exit(2);
			}
		}
		else
		{
			expandKey(expandedKey, key, size, expandedKeySize);
		}
			
    }

    /* Set the block values, for the block:
     * a0,0 a0,1 a0,2 a0,3
     * a1,0 a1,1 a1,2 a1,3
     * a2,0 a2,1 a2,2 a2,3
     * a3,0 a3,1 a3,2 a3,3
     * the mapping order is a0,0 a1,0 a2,0 a3,0 a0,1 a1,1 ... a2,3 a3,3
     */

	// input, output, key, numblocks
	if(crypt_method & USE_iAES)
	{
		switch (size)
		{
			case SIZE_16:
				intel_AES_enc128(input,output,expandedKey,1);
				break;
			case SIZE_24:
				intel_AES_enc192(input,output,expandedKey,1);
				break;
			case SIZE_32:
				intel_AES_enc256(input,output,expandedKey,1);
				break;
			default:
				printf("screw up here at %s %d\n", __FILE__, __LINE__);
				exit(2);
		}
	}
	else
	{
		/* iterate over the columns */
		for (i = 0; i < 4; i++)
		{
			/* iterate over the rows */
			for (j = 0; j < 4; j++)
				block[(i+(j*4))] = input[(i*4)+j];
		}

		/* encrypt the block using the expandedKey */
		aes_main(block, expandedKey, nbrRounds);


		/* unmap the block again into the output */
		for (i = 0; i < 4; i++)
		{
			/* iterate over the rows */
			for (j = 0; j < 4; j++)
				output[(i*4)+j] = block[(i+(j*4))];
		}
    }
    return 0;
}


char aes_decrypt(unsigned int crypt_method,
				unsigned char *input,
				unsigned char *output,
				unsigned char *key,
				enum keySize size)
{
    /* the expanded keySize */
    static int expandedKeySize=0;

    /* the number of rounds */
    int nbrRounds;

    /* the expanded key */
    static unsigned char *expandedKey;

    /* the 128 bit block to decode */
    unsigned char block[16];

    int i,j;

    /* set the number of rounds */
	nbrRounds = get_nrounds(size, __LINE__);

	if(!(crypt_method & USE_iAES || crypt_method & USE_SAMPLE_CODE))
	{
			printf("For this routine you can only do crypt_methods:  USE_SAMPLE_CODE or USE_iAES at %s %d\n", __FILE__, __LINE__);
			exit(1);
	}

	if( expandedKeySize == 0)
	{
		// note that I save off key once at first calling.
		// So... if you change the key after the first time through here,
		// you'll need to change this code too.
		expandedKeySize = (16*(nbrRounds+1));
		expandedKey = (unsigned char *)malloc(expandedKeySize * sizeof(char));
		if (expandedKey == NULL)
		{
			printf("screw up here at %s %d\n", __FILE__, __LINE__);
			exit(2);
			return MEMORY_ALLOCATION_PROBLEM;
		}
    	/* expand the key into an 176, 208, 240 bytes key */
		if(crypt_method & USE_iAES)
		{
			switch (size)
			{
				case SIZE_16:
					iEncExpandKey128(key, expandedKey);
					break;
				case SIZE_24:
					iEncExpandKey192(key, expandedKey);
					break;
				case SIZE_32:
					iEncExpandKey256(key, expandedKey);
					break;
				default:
					printf("screw up here at %s %d\n", __FILE__, __LINE__);
					exit(2);
			}
		}
		else
		{
			expandKey(expandedKey, key, size, expandedKeySize);
		}
    }

    /* Set the block values, for the block:
     * a0,0 a0,1 a0,2 a0,3
     * a1,0 a1,1 a1,2 a1,3
     * a2,0 a2,1 a2,2 a2,3
     * a3,0 a3,1 a3,2 a3,3
     * the mapping order is a0,0 a1,0 a2,0 a3,0 a0,1 a1,1 ... a2,3 a3,3
     */

	// input, output, key, numblocks
	if(crypt_method & USE_iAES)
	{
		//printf("going to do intel_AES_dec256() input4= 0x%x key= 0x%x at %s %d\n", 
		//		*(unsigned int *)input, *(unsigned int *)expandedKey, __FILE__, __LINE__);
		switch (size)
		{
			case SIZE_16:
				intel_AES_dec128(input,output,expandedKey,1); // problem
				break;
			case SIZE_24:
				intel_AES_dec192(input,output,expandedKey,1); // problem
				break;
			case SIZE_32:
				intel_AES_dec256(input,output,expandedKey,1); // problem
				break;
			default:
				printf("screw up here at %s %d\n", __FILE__, __LINE__);
				exit(2);
		}
	}
	else
	{
		/* iterate over the columns */
		for (i = 0; i < 4; i++)
		{
			/* iterate over the rows */
			for (j = 0; j < 4; j++)
				block[(i+(j*4))] = input[(i*4)+j];
		}

		/* decrypt the block using the expandedKey */
		aes_invMain(block, expandedKey, nbrRounds);

		/* unmap the block again into the output */
		for (i = 0; i < 4; i++)
		{
			/* iterate over the rows */
			for (j = 0; j < 4; j++)
				output[(i*4)+j] = block[(i+(j*4))];
		}
	}
	return 0;
}

/* end of encrypt/decrypt part
 * now do block cipher mode
 */


enum modeOfOperation{
    OFB,
    CFB,
    CBC
    };

//void encrypt(FILE *in, FILE *out, enum modeOfOperation, char *key);


void encrypt_array(int key_size_in_bytes, unsigned int crypt_method, int in_filesize, 
		unsigned char *in, unsigned char *out, enum modeOfOperation mode, unsigned char *key)
{
    /* the non-expanded keySize */
    enum keySize size = key_size_in_bytes;

    /* the AES input/output */
    unsigned char plaintext[16] = {0};
    unsigned char input[16] = {0};
    unsigned char output[16] = {0};
    unsigned char ciphertext[16] = {0};
    unsigned char IV[16] = {0};

    /* the AES key */

    /* char firstRound */
    char firstRound = 1;

    int fileSize;
    size_t read;
    int i, ibuf;

	if(sizeof(IV) == sizeof(test_iv)) // should be
	{
		memcpy(IV, test_iv, sizeof(IV));
	}

    if (in != NULL)
    {
        fileSize = in_filesize;

		for(ibuf=0; ibuf < in_filesize; ibuf += 16)
        {
			read = (in_filesize - ibuf > 16 ? 16 : in_filesize - ibuf);
			memcpy(plaintext, in+ibuf, read);
            if (mode == CFB)
            {
                if (firstRound)
                {
                    aes_encrypt(crypt_method, IV, output, key, size);
                    firstRound = 0;
                }
                else
                {
                    aes_encrypt(crypt_method, input, output, key, size);
                }
                for (i = 0; i < 16; i++)
                {
                    ciphertext[i] = plaintext[i] ^ output[i];
                }
				memcpy(out + ibuf, ciphertext, read);
                memcpy(input, ciphertext, 16*sizeof(unsigned char));
            }
            else if (mode == OFB)
            {
                if (firstRound)
                {
                    aes_encrypt(crypt_method, IV, output, key, size);
                    firstRound = 0;
                }
                else
                {
                    aes_encrypt(crypt_method, input, output, key, size);
                }
                for (i = 0; i < 16; i++)
                {
                    ciphertext[i] = plaintext[i] ^ output[i];
                }
				memcpy(out + ibuf, ciphertext, read);
                memcpy(input, output, 16*sizeof(unsigned char));
            }
            else if (mode == CBC)
            {
                /* padd with 0 bytes */
                if (read < 16)
                {
                    for (i = read; i < 16; i++)
                        plaintext[i] = 0;
                }

                for (i = 0; i < 16; i++)
                {
                    input[i] = plaintext[i] ^ ((firstRound) ? IV[i] : ciphertext[i]);
                }
                firstRound = 0;
                aes_encrypt(crypt_method, input, ciphertext, key, size);
                /* always 16 bytes because of the padding for CBC */
				memcpy(out + ibuf, ciphertext, 16);
            }
        }
    }
}

/* decrypt a file
 */
void decrypt_array(
				int key_size_in_bytes, 
				unsigned int crypt_method, 
				int in_filesize, 
				unsigned char *in, 
				unsigned char *out, 
				enum modeOfOperation mode, 
				unsigned char *key,
				unsigned int origFileSize)
{
    /* the non-expanded keySize */
    enum keySize size = key_size_in_bytes;

    /* the AES input/output */
    unsigned char ciphertext[16] = {0};
    unsigned char input[16] = {0};
    unsigned char output[16] = {0};
    unsigned char plaintext[16] = {0};
    unsigned char IV[16] = {0};

    /* the AES key */
    //unsigned char key[32] = {0x0};

    /* char firstRound */
    char firstRound = 1;

    int fileSize = 0, originalFileSize;
    size_t read;
    int i, ibuf, curpos;

	if(sizeof(IV) == sizeof(test_iv)) // should be
	{
		memcpy(IV, test_iv, sizeof(IV));
	}
	else
	{
		printf("screw up at %s %d\n", __FILE__, __LINE__);
		exit(1);
	}

    if (in != NULL)
    {
        fileSize = in_filesize;
		originalFileSize = origFileSize;
		if(verbose > 1)
		{
			printf("in function %s fileSize= %d at %s %d\n", __FUNCTION__, originalFileSize, __FILE__, __LINE__);
		}

		//printf("got inarray= 0x%x at %s %d\n", *(unsigned int *)in, __FILE__, __LINE__);
		for(ibuf=0; ibuf < in_filesize; ibuf += 16)
        {
			read = (in_filesize - ibuf > 16 ? 16 : in_filesize - ibuf);
			memcpy(ciphertext, in+ibuf, read);

            if (mode == CFB)
            {
                if (firstRound)
                {
                    aes_encrypt(crypt_method, IV, output, key, size);
                    firstRound = 0;
                }
                else
                {
                    aes_encrypt(crypt_method, input, output, key, size);
                }
                for (i = 0; i < 16; i++)
                {
                    plaintext[i] = output[i] ^ ciphertext[i];
                }
				memcpy(out + ibuf, ciphertext, read);
                memcpy(input, ciphertext, 16*sizeof(unsigned char));
            }
            else if (mode == OFB)
            {
                if (firstRound)
                {
                    aes_encrypt(crypt_method, IV, output, key, size);
                    firstRound = 0;
                }
                else
                {
                    aes_encrypt(crypt_method, input, output, key, size);
                }
                for (i = 0; i < 16; i++)
                {
                    plaintext[i] = output[i] ^ ciphertext[i];
                }
				memcpy(out + ibuf, ciphertext, read);
                memcpy(input, output, 16*sizeof(unsigned char));
            }
            else if(mode == CBC)
            {
				//printf("got mode= cbc\n");
                aes_decrypt(crypt_method, ciphertext, output, key, size);
                for (i = 0; i < 16; i++)
                {
                    plaintext[i] = ((firstRound) ? IV[i] : input[i]) ^ output[i];
                }
                firstRound = 0;
                if (originalFileSize < 16)
                {
					memcpy(out + ibuf, plaintext, originalFileSize);
                }
                else
                {
					memcpy(out + ibuf, plaintext, read);
                    originalFileSize -= 16;
                }
                memcpy(input, ciphertext, 16*sizeof(unsigned char));
            }
        }
    }
}

void encrypt(int key_size_in_bytes, unsigned int crypt_method, FILE *in, FILE *out, enum modeOfOperation mode, unsigned char *key)
{
    /* the non-expanded keySize */
    enum keySize size = key_size_in_bytes;

    /* the AES input/output */
    unsigned char plaintext[16] = {0};
    unsigned char input[16] = {0};
    unsigned char output[16] = {0};
    unsigned char ciphertext[16] = {0};
    unsigned char IV[16] = {0};

    /* the AES key */

    /* char firstRound */
    char firstRound = 1;

    int fileSize;
    size_t read;
    int i;

    if (in != NULL)
    {
        fseek(in,0,SEEK_END);
        fileSize = ftell(in);
        fseek(in,0,SEEK_SET);

		if(sizeof(IV) == sizeof(test_iv)) // should be
		{
			memcpy(IV, test_iv, sizeof(IV));
		}
        /* add the file header */
        fwrite(&mode, sizeof(enum modeOfOperation), 1, out);
        fwrite(&fileSize, sizeof(fileSize), 1, out);

        while ((read = fread(plaintext, sizeof(unsigned char), 16, in)) > 0)
        {

            if (mode == CFB)
            {
                if (firstRound)
                {
                    aes_encrypt(crypt_method, IV, output, key, size);
                    firstRound = 0;
                }
                else
                {
                    aes_encrypt(crypt_method, input, output, key, size);
                }
                for (i = 0; i < 16; i++)
                {
                    ciphertext[i] = plaintext[i] ^ output[i];
                }
                fwrite(ciphertext, sizeof(unsigned char), read, out);
                memcpy(input, ciphertext, 16*sizeof(unsigned char));
            }
            else if (mode == OFB)
            {
                if (firstRound)
                {
                    aes_encrypt(crypt_method, IV, output, key, size);
                    firstRound = 0;
                }
                else
                {
                    aes_encrypt(crypt_method, input, output, key, size);
                }
                for (i = 0; i < 16; i++)
                {
                    ciphertext[i] = plaintext[i] ^ output[i];
                }
                fwrite(ciphertext, sizeof(unsigned char), read, out);
                memcpy(input, output, 16*sizeof(unsigned char));
            }
            else if (mode == CBC)
            {
                /* padd with 0 bytes */
                if (read < 16)
                {
                    for (i = read; i < 16; i++)
                        plaintext[i] = 0;
                }

                for (i = 0; i < 16; i++)
                {
                    input[i] = plaintext[i] ^ ((firstRound) ? IV[i] : ciphertext[i]);
                }
                firstRound = 0;
                aes_encrypt(crypt_method, input, ciphertext, key, size);
                /* always 16 bytes because of the padding for CBC */
                fwrite(ciphertext, sizeof(unsigned char), 16, out);
            }
        }
    }
}

void ck_bytes_read(size_t bytes_read, size_t bytes_want, char *file, int line)
{
	if(bytes_read != bytes_want)
	{
		fprintf(stderr, "bytes_read(%d) != bytes_want(%d) at %s %d\n", (int)bytes_read, (int)bytes_want, file, line);
		exit(1);
	}
}

/* decrypt a file
 */
void decrypt(int key_size_in_bytes, unsigned int crypt_method, FILE *in, FILE *out, unsigned char *key)
{
    /* the non-expanded keySize */
    enum keySize size = key_size_in_bytes;

    /* the AES input/output */
    unsigned char ciphertext[16] = {0};
    unsigned char input[16] = {0};
    unsigned char output[16] = {0};
    unsigned char plaintext[16] = {0};
    unsigned char IV[16] = {0};

    /* the AES key */
    //unsigned char key[32] = {0x0};

    /* char firstRound */
    char firstRound = 1;

    enum modeOfOperation mode;

    int originalFileSize = 0;
    size_t read;
    int i;

    if (in != NULL)
    {
		size_t bytes_want=0;
        fseek(in,0,SEEK_SET);
        read = fread(&mode, 1, (bytes_want = sizeof(enum modeOfOperation)), in);
		ck_bytes_read(read, bytes_want, __FILE__, __LINE__);
        read = fread(&originalFileSize, 1, (bytes_want = sizeof(originalFileSize)), in);
		ck_bytes_read(read, bytes_want, __FILE__, __LINE__);

		if(sizeof(IV) == sizeof(test_iv)) // should be
		{
			memcpy(IV, test_iv, sizeof(IV));
		}
		if(verbose > 1)
		{
			printf("in function %s fileSize= %d at %s %d\n", __FUNCTION__, originalFileSize, __FILE__, __LINE__);
		}

        while ((read = fread(ciphertext, sizeof(unsigned char), 16, in)) > 0)
        {
			//printf("got to %s %d\n", __FILE__, __LINE__);
            if (mode == CFB)
            {
                if (firstRound)
                {
                    aes_encrypt(crypt_method, IV, output, key, size);
                    firstRound = 0;
                }
                else
                {
                    aes_encrypt(crypt_method, input, output, key, size);
                }
                for (i = 0; i < 16; i++)
                {
                    plaintext[i] = output[i] ^ ciphertext[i];
                }
                fwrite(plaintext, sizeof(unsigned char), read, out);
                memcpy(input, ciphertext, 16*sizeof(unsigned char));
            }
            else if (mode == OFB)
            {
                if (firstRound)
                {
                    aes_encrypt(crypt_method, IV, output, key, size);
                    firstRound = 0;
                }
                else
                {
                    aes_encrypt(crypt_method, input, output, key, size);
                }
                for (i = 0; i < 16; i++)
                {
                    plaintext[i] = output[i] ^ ciphertext[i];
                }
                fwrite(plaintext, sizeof(unsigned char), read, out);
                memcpy(input, output, 16*sizeof(unsigned char));
            }
            else if(mode == CBC)
            {
				//printf("got mode= cbc\n");
                aes_decrypt(crypt_method, ciphertext, output, key, size);
                for (i = 0; i < 16; i++)
                {
                    plaintext[i] = ((firstRound) ? IV[i] : input[i]) ^ output[i];
					//printf("char[%d]= %c\n", i, plaintext[i]);
                }
                firstRound = 0;
                if (originalFileSize < 16)
                {
                    fwrite(plaintext, sizeof(unsigned char), originalFileSize, out);
                }
                else
                {
                    fwrite(plaintext, sizeof(unsigned char), read, out);
                    originalFileSize -= 16;
                }
                memcpy(input, ciphertext, 16*sizeof(unsigned char));
            }

        }
    }
}

double get_elapsed_time(void)
{
	double xxx, yyy;
	static double prev_value = 0.0;
#ifndef __linux__
	//static MY_ELAP_TIME timebuffer_beg;
	static double wrap_around=0.0;
#else
	struct timeval timebuffer_beg; 
	struct timezone tzp;
#endif

#ifndef __linux__
	//_ftime( &timebuffer_beg );
	//xxx = (double)(timebuffer_beg.time) + ((double)(timebuffer_beg.millitm)/1000.0);
	xxx = (double)timeGetTime(); // DWORD ms since booting
	xxx /= 1000.0;
	if(xxx < prev_value)
	{
		/* try to handle the wrap around possibility. 
		 * need to account for slight differences in different cpus (maybe). 
		 * Don't want to say we've wrapped around if the this cpu is just a fraction behind prev cpu.
		 * If windows uses a system time then all clocks should see the same value but I don't 
		 * know how windows does this.
		 */
		if((prev_value - xxx) > 100.0)
		{
			/* lets just way use if the diff is > 100 secs 
			 * I'm betting that windows won't wrap around again (49 days) while id_cpu is running.
			 */
			wrap_around = 4294967.2950; //4gb 0xffffffff div by 1000
			printf("resetting wrap_around to %f at %s %d\n", wrap_around, __FILE__, __LINE__);
		}
	}
	xxx += wrap_around;
	prev_value = xxx;
#else
	gettimeofday(&timebuffer_beg, &tzp);
	xxx = (double)(timebuffer_beg.tv_sec) + ((double)(timebuffer_beg.tv_usec)/1.0e6);
#endif
	return xxx;

}


int ck_mem(char *filenm, int line)
{
#ifndef __linux__
		if(_heapchk()!=_HEAPOK)
		{
			fprintf(stderr, "got a heapchk != _HEAPOK. Called by %s %d at %s %d\n", filenm, line, __FILE__, __LINE__);
			DebugBreak();
			fprintf(stderr, "got a debugbreak at %s %d\n", __FILE__, __LINE__);
			fflush(NULL);
			exit(2);
		}
#endif
		return 0;
}

int compare_files(char *file1, char *file2)
{
	FILE *stream[2];
	int i, read, curpos, reqsz, tot_read[2], fileSize[2];
	unsigned char *buf[2];
	char *filenm[2];

	filenm[0] = file1;
	filenm[1] = file2;

	for(i=0; i < 2; i++)
	{
			stream[i] = fopen( filenm[i], "rb" );
			if(stream[i] == NULL)
			{
			  printf( "File %s could not be opened\n" , filenm[i]);
			  exit(2);
			}
			fseek(stream[i],0,SEEK_END);
			fileSize[i] = ftell(stream[i]);
			fseek(stream[i],0,SEEK_SET);

			buf[i] = (unsigned char *)malloc((fileSize[i]+32) * sizeof(unsigned char));
			memset(buf[i], 0, (fileSize[i]+32) * sizeof(unsigned char));
			curpos = 0;
			reqsz = 1024;
			tot_read[i] = 0;
			for(curpos = 0; curpos < fileSize[i]; curpos += reqsz)
			{
				tot_read[i] += fread(buf[i]+curpos, sizeof(unsigned char), reqsz, stream[i]);
			}
			if(tot_read[i] != fileSize[i])
			{
				printf("Error: file= %s fileSize= %d but tot_read= %d at %s %d\n", filenm[i], fileSize[i], tot_read[i], __FILE__, __LINE__);
				exit(1);
			}
			fclose(stream[i]);
	}
	if(fileSize[0] != fileSize[1])
	{
		printf("error: file %s size= %d is not equal file %s size= %d. Error at %s %d\n", 
			filenm[0], fileSize[0], filenm[1], fileSize[1], __FILE__, __LINE__);
		exit(1);
	}
	for(curpos = 0; curpos < fileSize[0]; curpos++)
	{
		if(buf[0][curpos] != buf[1][curpos])
		{
			printf("Error: file1= %s, file2= %s, curpos= %d, char1= 0x%x, char2= 0x%x at %s %d\n", 
					filenm[0], filenm[1], curpos, buf[0][curpos], buf[1][curpos], __FILE__, __LINE__);
			exit(1);
		}
	}
	free(buf[0]);
	free(buf[1]);
	printf("---No error on compare_files of %s and %s at %s %d\n", file1, file2, __FILE__, __LINE__);
	return 0;
}


int load_up_array(int oper_type, FILE *stream_in, unsigned char **inbuf, unsigned char **outbuf, 
		unsigned char **inbuf_base, unsigned char **outbuf_base, unsigned int *origFileSize)
{
	int whole_fileSize, fileSize, curpos, reqsz, read, tot_read;
	unsigned char *in, *out;
	int offset=0, sz, i, pagesize;
	unsigned int lwrptr, leftover, addtoit;
	enum modeOfOperation mode;
#ifndef __linux__
	static SYSTEM_INFO sSysInfo;           // useful system information
#endif


	fseek(stream_in,0,SEEK_END);
	whole_fileSize = ftell(stream_in);
	fseek(stream_in,0,SEEK_SET);
	if(oper_type == OPER_ENCRYPT)
	{
		fileSize = whole_fileSize;
		*origFileSize = fileSize;
	}
	else
	{
		size_t bytes_read, bytes_want;
		fseek(stream_in,0,SEEK_SET);
		bytes_read = fread(&mode, 1, (bytes_want = sizeof(enum modeOfOperation)), stream_in);
		ck_bytes_read(bytes_read, bytes_want, __FILE__, __LINE__);
		bytes_read = fread(&fileSize, 1, (bytes_want = sizeof(fileSize)), stream_in);
		ck_bytes_read(bytes_read, bytes_want, __FILE__, __LINE__);
		*origFileSize = fileSize;
		offset = ftell(stream_in);
		fileSize = whole_fileSize - offset;
	}
	sz = (fileSize+32) * sizeof(unsigned char);
#ifdef __linux__
	pagesize = sysconf(_SC_PAGESIZE);
#else
	GetSystemInfo(&sSysInfo);  // fill the system information structure
	pagesize = sSysInfo.dwPageSize;
#endif
	i = 1;
	sz += pagesize;
	//fprintf(stderr, ", sz= %d, align page size= %d at %s %d\n", 
	//		sz, pagesize, __FILE__, __LINE__);
	in  = (unsigned char *)malloc(sz);
	out = (unsigned char *)malloc(sz);
	*inbuf_base  = in;
	*outbuf_base = out;
	*inbuf  = in;
	*outbuf = out;
	memset(*inbuf,  0, sz);
	memset(*outbuf, 0, sz);
	memcpy(&lwrptr, &in, sizeof(lwrptr));
	leftover = lwrptr % pagesize;
	addtoit  = 0;
	if(leftover != 0)
	{
		addtoit= pagesize - leftover;
	}
	in += addtoit;

	memcpy(&lwrptr, &out, sizeof(lwrptr));
	leftover = lwrptr % pagesize;
	addtoit  = 0;
	if(leftover != 0)
	{
		addtoit= pagesize - leftover;
	}
	out += addtoit;
	*inbuf  = in;
	*outbuf = out;

	if(*inbuf == NULL || *outbuf == NULL)
	{
		printf("malloc failed at %s %d\n", __FILE__, __LINE__);
		exit(2);
	}
	curpos = 0;
	reqsz = 1024;
	tot_read = 0;
	for(curpos = 0; curpos < fileSize; curpos += reqsz)
	{
		read = fread(in+curpos, sizeof(unsigned char), reqsz, stream_in);
		tot_read += read;
	}
	if(verbose > 1)
	{
		printf("fileSize= %d tot_read= %d at %s %d\n", fileSize, tot_read, __FILE__, __LINE__);
	}
	return fileSize;
}

char *get_method_string(int method)
{
	static char *method_string[]= {
		"unknown",
		"SAMPLE_CODE",
		"intel AES CBC library routines",
		"intel AES CTR library routines",
		"intel AES BLK library routines",
		"intel AES routine with sample code CBC logic",
	};
	if(method & USE_SAMPLE_CODE)
	{
		return method_string[1];
	}
	else if(method & USE_iAES_CBC)
	{
		return method_string[2];
	}
	else if(method & USE_iAES_CTR)
	{
		return method_string[3];
	}
	else if(method & USE_iAES_BLK)
	{
		return method_string[4];
	}
	else if(method & USE_iAES)
	{
		return method_string[5];
	}
	else
	{
		return method_string[0];
	}
}

#ifdef __linux__
#define MY_CDECL
#else
#define MY_CDECL __cdecl
#endif

typedef void (MY_CDECL *KEYGENPROC)(UCHAR *key,UCHAR *round_keys);
typedef void (MY_CDECL *AESPROC)(sAesData *);
typedef void (MY_CDECL *AESPROC2)(UCHAR *cipherText, UCHAR *plainText, UCHAR *key, size_t numBlocks, UCHAR *iv);

enum keyIndex{
    KEY_INDEX16 = 0,
    KEY_INDEX24 = 1,
    KEY_INDEX32 = 2,
    };

struct cfg_struct {
	int        crypt_method;
	int        key_index;
	KEYGENPROC proc_key_enc;
	AESPROC    proc_enc;
	AESPROC2   proc_both_enc;
	KEYGENPROC proc_key_dec;
	AESPROC    proc_dec;
	AESPROC2   proc_both_dec;
} cfg[] = {
	{ USE_iAES_CBC, KEY_INDEX16, iEncExpandKey128, iEnc128_CBC, intel_AES_enc128_CBC, iDecExpandKey128, iDec128_CBC, intel_AES_dec128_CBC},
	{ USE_iAES_CBC, KEY_INDEX24, iEncExpandKey192, iEnc192_CBC, intel_AES_enc192_CBC, iDecExpandKey192, iDec192_CBC, intel_AES_dec192_CBC},
	{ USE_iAES_CBC, KEY_INDEX32, iEncExpandKey256, iEnc256_CBC, intel_AES_enc256_CBC, iDecExpandKey256, iDec256_CBC, intel_AES_dec256_CBC},
	{ USE_iAES_CTR, KEY_INDEX16, iEncExpandKey128, iEnc128_CTR, intel_AES_encdec128_CTR, iEncExpandKey128, iEnc128_CTR, intel_AES_encdec128_CTR},
	{ USE_iAES_CTR, KEY_INDEX24, iEncExpandKey192, iEnc192_CTR, intel_AES_encdec192_CTR, iEncExpandKey192, iEnc192_CTR, intel_AES_encdec192_CTR},
	{ USE_iAES_CTR, KEY_INDEX32, iEncExpandKey256, iEnc256_CTR, intel_AES_encdec256_CTR, iEncExpandKey256, iEnc256_CTR, intel_AES_encdec256_CTR},
};

int get_key_index(int key_size_in_bytes)
{
	int which_key;
	switch (key_size_in_bytes)
	{
		case 16:
			which_key = 0;
			break;
		case 24:
			which_key = 1;
			break;
		case 32:
			which_key = 2;
			break;
		default:
			printf("invalid key_size_in_bytes= %d at %s %d\n", key_size_in_bytes, __FILE__, __LINE__);
			exit(2);
	}
	return which_key;
}

int do_encrypt_decrypt(int key_size_in_bytes, char *fl_input, char *fl_enc, char *fl_dec, 
		int encrypt_method, int decrypt_method, int outer_loops)
{
	int oper;
	char *oper_desc[]={"encrypt", "decrypt"};
	enum modeOfOperation mode = CBC;
	int i, crypt_method[2];
	double key_enc_cycles = 0.0, key_dec_cycles=0.0;
	int key_index, cfg_index;
	unsigned long long tsc_beg, tsc_end;
	AESPROC2   proc_both;
	AESPROC    proc_crypt;
	KEYGENPROC proc_key;

	unlink(fl_enc); 
	unlink(fl_dec);
	crypt_method[OPER_ENCRYPT] = encrypt_method;
	crypt_method[OPER_DECRYPT] = decrypt_method;

	tsc_end = iaes_time_expand_key( OPER_ENCRYPT, gkey, key_size_in_bytes, outer_loops);
	tsc_end = iaes_time_expand_key( OPER_ENCRYPT, gkey, key_size_in_bytes, outer_loops);
	key_enc_cycles = (double)(tsc_end)/(double)(outer_loops);
	printf("cycles per key enc expansion = %f\n", key_enc_cycles);
	tsc_end = iaes_time_expand_key( OPER_DECRYPT, gkey, key_size_in_bytes, outer_loops);
	tsc_end = iaes_time_expand_key( OPER_DECRYPT, gkey, key_size_in_bytes, outer_loops);
	key_dec_cycles = (double)(tsc_end)/(double)(outer_loops);
	printf("cycles per key dec expansion = %f\n", key_dec_cycles);

	key_index = get_key_index(key_size_in_bytes);
	cfg_index = -1;
	for(i=0; i < sizeof(cfg)/sizeof(cfg[0]); i++)
	{
		if((cfg[i].crypt_method & encrypt_method) && cfg[i].key_index == key_index)
		{
			cfg_index = i;
			break;
		}
	}


	for(oper= OPER_ENCRYPT; oper <= OPER_DECRYPT; oper++)
	{
		FILE *stream_in, *stream_out;
		char *in, *out;
		int fileSize=0;
		double tm_beg, tm_end, bytes_processed;

		in  = (oper == OPER_ENCRYPT ? fl_input : fl_enc);
		out = (oper == OPER_ENCRYPT ? fl_enc   : fl_dec);
		stream_in = fopen( in, "rb" );
		if(stream_in == NULL)
		{
		  printf( "File %s could not be opened\n" , in);
		  exit(2);
		}
		stream_out = fopen( out, "wb" );
		if(stream_out == NULL)
		{
		  printf( "File %s could not be opened\n" , out);
		  exit(2);
		}
		if(((crypt_method[oper] & USE_iAES_BLK) || (crypt_method[oper] & USE_iAES_CBC) || (crypt_method[oper] & USE_iAES_CTR)) 
					&& (crypt_method[oper] & USE_ARRAY) == 0)
		{
			printf("Sorry, intel AES CBC or CTR library routines only work on arrays. Bye at %s %d\n", __FILE__, __LINE__);
			exit(1);
		}

		if(cfg_index > -1)
		{
			if(oper == OPER_ENCRYPT)
			{
				proc_both  = cfg[cfg_index].proc_both_enc;
				proc_crypt = cfg[cfg_index].proc_enc;
				proc_key   = cfg[cfg_index].proc_key_enc;
			}
			else
			{
				proc_both  = cfg[cfg_index].proc_both_dec;
				proc_crypt = cfg[cfg_index].proc_dec;
				proc_key   = cfg[cfg_index].proc_key_dec;
			}
		}
			
		printf("\nusing %s method on %s: ", oper_desc[oper], (crypt_method[oper] & USE_ARRAY ? "arrays" : "files"));
		printf("%s", get_method_string(crypt_method[oper]));

		if(crypt_method[oper] & USE_ARRAY)
		{
			int numBlocks, extra;
			unsigned int origFileSize=0;
			unsigned char *inbuf=NULL, *outbuf=NULL, *inbuf_base=NULL, *outbuf_base=NULL;
			unsigned char local_test_iv[16];
			DEFINE_ROUND_KEYS
			sAesData aesData;

			fileSize = load_up_array(oper, stream_in, &inbuf, &outbuf, &inbuf_base, &outbuf_base, &origFileSize);
			printf(", size= %d bytes\n", fileSize);
			numBlocks = fileSize/16;
			extra = fileSize % 16;
			if(extra > 0)
			{
				numBlocks++;
			}
			aesData.in_block = inbuf;
			aesData.out_block = outbuf;
			aesData.expanded_key = expandedKey;
			aesData.num_blocks = numBlocks;
			aesData.iv = local_test_iv;

			if((crypt_method[oper] & USE_iAES_CTR) || (crypt_method[oper] & USE_iAES_CBC))
			{

				bytes_processed = 0;
				memcpy(local_test_iv, test_iv, 16);
				if(cfg_index == -1)
				{
					printf("missed cfg table match for USE_iAES_CTR or USE_iAES_CBC at %s %d\n", __FILE__, __LINE__);
					exit(2);
				}

				tm_beg = get_elapsed_time();
				tsc_beg = do_rdtsc();
				for(i=0; i < outer_loops; i++)
				{
#if 0
					proc_both(inbuf, outbuf, gkey, numBlocks, local_test_iv);
#else
  					proc_key(gkey, expandedKey);
   					proc_crypt(&aesData);
#endif
				}
				tsc_end = do_rdtsc();
				tm_end = get_elapsed_time();

				bytes_processed = numBlocks * 16;
				bytes_processed *= outer_loops;

				if(verbose > 2)
				{
					printf("%s took %f secs at %s %d\n", get_method_string(crypt_method[oper]), tm_end-tm_beg, __FILE__, __LINE__);
				}
			}
			else if(crypt_method[oper] & USE_iAES_BLK)
			{
				bytes_processed = 0;
				tm_beg = get_elapsed_time();
				tsc_beg = do_rdtsc();
				for(i=0; i < outer_loops; i++)
				{
					if(oper == OPER_ENCRYPT)
					{
						switch (key_size_in_bytes)
						{
							case 16:
								intel_AES_enc128(inbuf, outbuf, gkey, numBlocks);
								break;
							case 24:
								intel_AES_enc192(inbuf, outbuf, gkey, numBlocks);
								break;
							case 32:
								intel_AES_enc256(inbuf, outbuf, gkey, numBlocks);
								break;
						}
					}
					else
					{
						switch (key_size_in_bytes)
						{
							case 16:
								intel_AES_dec128(inbuf, outbuf, gkey, numBlocks);
								break;
							case 24:
								intel_AES_dec192(inbuf, outbuf, gkey, numBlocks);
								break;
							case 32:
								intel_AES_dec256(inbuf, outbuf, gkey, numBlocks);
								break;
						}
					}
					bytes_processed += numBlocks * 16;
				}
				tsc_end = do_rdtsc();
				tm_end = get_elapsed_time();
				if(verbose > 2)
				{
					printf("USE_iAES_BLK|ARRAY took %f secs at %s %d\n", tm_end-tm_beg, __FILE__, __LINE__);
				}
			}
			else
			{
				bytes_processed = 0;
				if(oper == OPER_ENCRYPT)
				{
					encrypt_array(key_size_in_bytes, crypt_method[oper], fileSize, inbuf, outbuf, CBC, gkey);
					tm_beg = get_elapsed_time();
					tsc_beg = do_rdtsc();
					for(i=0; i < outer_loops; i++)
					{
						encrypt_array(key_size_in_bytes, crypt_method[oper], fileSize, inbuf, outbuf, CBC, gkey);
						bytes_processed += fileSize;
					}
					tsc_end = do_rdtsc();
					tm_end = get_elapsed_time();
				}
				else
				{
					if(verbose > 3)
					{
						printf("fileSize= %d input_buf4= 0x%x outbuf= 0x%x at %s %d\n", 
						fileSize, *(unsigned int *)inbuf, *(unsigned int *)outbuf, __FILE__, __LINE__);
					}
					decrypt_array(key_size_in_bytes, crypt_method[oper], fileSize, inbuf, outbuf, CBC, gkey, origFileSize);
					tm_beg = get_elapsed_time();
					tsc_beg = do_rdtsc();
					for(i=0; i < outer_loops; i++)
					{
						decrypt_array(key_size_in_bytes, crypt_method[oper], fileSize, inbuf, outbuf, CBC, gkey, origFileSize);
						bytes_processed += fileSize;
					}
					tsc_end = do_rdtsc();
					tm_end = get_elapsed_time();
				}
			}

			// write the output file
			if(verbose > 1)
			{
				printf("using array size= %d at %s %d\n", fileSize, __FILE__, __LINE__);
			}
			if(oper == OPER_ENCRYPT)
			{
				fwrite(&mode, sizeof(enum modeOfOperation), 1, stream_out);
				fwrite(&fileSize, sizeof(fileSize), 1, stream_out);
				// have to write out 16 byte chunk for cbc encoded stream
				fwrite(outbuf, 16, numBlocks, stream_out);
			}
			else
			{
				fwrite(outbuf, sizeof(unsigned char), origFileSize, stream_out);
			}
			free(inbuf_base);
			free(outbuf_base);
			if(verbose > 1)
			{
				printf("tm_beg= %f tm_end= %f bytes= %g mb/s= %f at %s %d\n", 
						tm_beg, tm_end, bytes_processed, 1.0e-6*bytes_processed/(tm_end-tm_beg), __FILE__, __LINE__);
			}
		}
		else
		{
			int read, curpos, reqsz, tot_read=0;
			unsigned char *inbuf, *outbuf;

			bytes_processed = 0;
			fseek(stream_in,0,SEEK_END);
			fileSize = ftell(stream_in);
			fseek(stream_in,0,SEEK_SET);
			printf(", size= %d bytes\n", fileSize);

			tm_beg = get_elapsed_time();
			tsc_beg = do_rdtsc();
			for(i=0; i < outer_loops; i++)
			{
				if(oper == OPER_ENCRYPT)
				{
					encrypt(key_size_in_bytes, crypt_method[oper], stream_in, stream_out, CBC, gkey);
				}
				else
				{
					decrypt(key_size_in_bytes, crypt_method[oper], stream_in, stream_out, gkey);
				}
			}
			bytes_processed += fileSize;
			tsc_end = do_rdtsc();
			tm_end = get_elapsed_time();
		}
		if(verbose > 0)
		{
			double tsc_diff = (double)(unsigned long long)(tsc_end - tsc_beg);
			double bytes_per_iter=0, tot_cycles_per_byte, key_cycles;
			tot_cycles_per_byte = tsc_diff/bytes_processed;
			printf("%s took %f seconds: %f MB/sec or total cycles/byte %f\n", 
				oper_desc[oper], tm_end-tm_beg, 1.0e-6 * bytes_processed/(tm_end-tm_beg), tot_cycles_per_byte);
			bytes_per_iter = bytes_processed / (double)outer_loops;
			if(oper == OPER_ENCRYPT)
			{
				key_cycles = key_enc_cycles;
			}
			else
			{
				key_cycles = key_dec_cycles;
			}
			printf("Estimated key expansion cycles/byte %f,  total - key exp: %f\n", 
				key_cycles/bytes_per_iter, tot_cycles_per_byte - key_cycles/bytes_per_iter);
		}
		fseek(stream_in,0,SEEK_END);
		fseek(stream_out,0,SEEK_END);
		if(verbose > 1)
		{
			printf("at bottom oper= %d, input file size= %d, output size= %d bytes at %s %d\n", 
				oper, (int)ftell(stream_in), (int)ftell(stream_out), __FILE__, __LINE__);
		}
		fclose( stream_in );
		fclose( stream_out );
	}

#ifndef CK_CTR_MODE
	if(encrypt_method & USE_iAES_CTR)
#endif
	{
		compare_files(fl_input, fl_dec);
	}

	printf("\n");

	return 0;

}

int mk_a_file(int max_size, char *fl_out)
{
	int in_sz, i, combo0, combo1, crypt_method[2];
	FILE *stream_out;
	unsigned char str[1000];

	stream_out = fopen( fl_out, "wb" );
	if(stream_out == NULL)
	{
		printf( "File %s could not be opened\n" , fl_out);
		exit(2);
	}
	for(i=0; i < sizeof(str); i++)
	{
		unsigned char ch;
		ch = '0' + (i % 10);
		str[i] = ch;
	}
	for(i=0; i < max_size; i += sizeof(str))
	{
		int sz;
		if((i+sizeof(str)) < max_size)
		{
			sz = sizeof(str);
		}
		else
		{
			sz = max_size - i;
		}
		fwrite(str, sizeof(unsigned char), sz, stream_out);
	}
	fclose(stream_out);
	printf("made input file %s of %d bytes, %d KB, %d MB at %s %d\n", 
			fl_out, max_size, max_size/1024, max_size/(1024*1024), __FILE__, __LINE__);
	return 0;
}

int check_sizes_and_methods(int key_size, int max_size, char *fl_vary, char *fl_in, char *fl_enc, char *fl_dec, int outer_loops)
{
	int in_sz, i, combo0, combo1, crypt_method[2];
	char *fl_input;


	for(in_sz=0; in_sz < max_size; in_sz++)
	{
		int method[]={USE_SAMPLE_CODE, USE_iAES, USE_iAES_BLK, USE_iAES_CBC, USE_iAES_CTR};
		int use_file0, use_file1;
		if(in_sz == 0)
		{
			fl_input = fl_in;
		}
		else
		{
			mk_a_file(in_sz, fl_vary);
			fl_input = fl_vary;
		}

		for(combo0=0; combo0 < (sizeof(method)/sizeof(method[0])); combo0++)
		{
			crypt_method[0] = method[combo0];
			for(use_file0=0; use_file0 < 2; use_file0++)
			{
				if(use_file0 == 1) { crypt_method[0] |= USE_ARRAY; }
				if(use_file0 == 0 && ((crypt_method[0] & USE_iAES_BLK) ||
					(crypt_method[0] & USE_iAES_CBC) ||
					(crypt_method[0] & USE_iAES_CTR)))
				{
					continue; // haven't implemented 'read from file' for iaes_cbc yet. Data has to be in memory.
				}

				for(combo1=0; combo1 < (sizeof(method)/sizeof(method[0])); combo1++)
				{
					crypt_method[1] = method[combo1];
					for(use_file1=0; use_file1 < 2; use_file1++)
					{
						if(use_file1 == 1) { crypt_method[1] |= USE_ARRAY; }
						if(use_file1 == 0 && ((crypt_method[1] & USE_iAES_BLK) ||
							(crypt_method[1] & USE_iAES_CBC) ||
							(crypt_method[1] & USE_iAES_CTR)))
						{
							continue; // haven't implemented 'read from file' for iaes_cbc yet. Data has to be in memory.
						}
						if( crypt_method[0] != crypt_method[1] &&
							(((crypt_method[0] | crypt_method[1]) & USE_iAES_CBC) ||
							 ((crypt_method[0] | crypt_method[1]) & USE_iAES_CTR)) )
						{
							continue; // enc & dec method has to be the same for CBC & CTR modes.
						}

						do_encrypt_decrypt(key_size, fl_input, fl_enc, fl_dec, crypt_method[0], crypt_method[1], outer_loops);

					} // end of use_file1
				} // end of combo1
			} // end of use_file0
		}     // end of combo0
	} // loop over file sizes

	return 0;
}

//extern char *my_optarg;
//extern int my_optind, my_opterr, my_optopt;


int main(int argc, char **argv)
{
	char *fl_in={"tmp.txt"};
	char *fl_input, *fl_vary={"tmp_in.txt"};
	//char *fl_in={"\\pfay\\bigfile.dat"};
	char *fl_enc={"tmp_enc.txt"}, *fl_dec={"tmp_dec.txt"};
	unsigned char output[32];
	int i, in_sz, oper, fileSize=0;
	int outer_loops=1;
	unsigned int crypt_method[2];
	char bmark_file_name[1024] = {"bigfile.txt"};
	int bmark_loops=200, bmark_file_size=50*1024*1024;
	int bmark_key_size=128, key_size_in_bytes=0;
	int check_sizes= -1, check_loops=1;
	int bmark_encrypt = USE_iAES_CBC | USE_ARRAY;
	int bmark_decrypt = USE_iAES_CBC | USE_ARRAY;

    int c;
    int digit_optind = 0;

   while (1) {
        int this_option_optind = my_optind ? my_optind : 1;
        int option_index = 0;
        struct option long_options[] = {
            {"bmark_file_name", 1, 0, 0, "-bmark_file_name=filenm  where filenm is the name of the file to be encrypted.\n"
				"   The file will be read into memory and encrypted -bmark_loops times.\n"
				"   You must enter the -bmark_file_size and -bmark_loops options if you don't want to use their default values.\n"
				"   The encrypted output will be written to 'tmp_enc.txt' in the current dir.\n"
				"   The decrypted output will be written to 'tmp_dec.txt' in the current dir.\n"
				"   The default is bigfile.txt.\n"
			},
            {"bmark_key_size", 1, 0, 0, "-bmark_key_size=num  where num must be 128, 192 or 256\n"
				"   The default is 128 bits\n"
			},
            {"bmark_file_size", 1, 0, 0, "-bmark_file_size=num  where num is the size in bytes of the benchmark file\n"
				"   The default is 50 MB\n"
			},
            {"bmark_loops",     1, 0, 0, "-bmark_loops=num  where num is the number of times to encrypt (and decrypt)\n"
			   "   the file in memory. The default is 200 iterations.\n"
			   "   If you use a low number then the elapsed time might show up as 0.\n"
			   "   The sample code takes 1000x longer than the iAES routines so keep that in mind.\n"
			},
            {"bmark_encrypt",     1, 0, 0, "-bmark_encrypt=method where method can be:\n"
				"   USE_SAMPLE_CODE or USE_iAES or USE_iAES_BLK or USE_iAES_CBC. You can also add USE_ARRAY.\n"
				"      USE_SAMPLE_CODE means don't use any AES-NI, just use the C code.\n"
				"      USE_iAES         means use low level AES-NI (at the block level). Still use sample code for CBC.\n"
				"      USE_iAES_BLK     means use AES-NI on the whole block but don't do CBC mode\n"
				"      USE_iAES_CBC     means use AES-NI on the whole block and do CBC mode in the iAES library\n"
				"      USE_iAES_CTR     means use AES-NI on the whole block and do CTR mode in the iAES library\n"
				"      USE_ARRAY        means do the data in memory (instead of reading from a file each time).\n"
				"                       If you enter USE_iAES_CBC or USE_iAES_CTR then USE_ARRAY is added.\n"
				"   So you can enter 'USE_iAES_CBC USE_ARRAY' to use AES-NI and CBC in the library and do the work in memory.\n"
				"   The sample code takes 1000x longer than the iAES routines so keep that in mind.\n"
				"   The sample code was not written for speed but for ease of reading. It is not optimized.\n"
				"   The default is 'USE_iAES_CBC USE_ARRAY'.\n"
			},
            {"bmark_decrypt",     1, 0, 0, "-bmark_decrypt=method where method be same as -bmark_encrypt above.\n"},
            {"bmark_crypt",       1, 0, 0, "-bmark_crypt=method where method be same as -bmark_encrypt above.\n"
				"   If you use bmark_crypt then both the encrypt and decrypt method is set to the input value.\n"},
            {"check_sizes", 1, 0, 0, "-check_sizes=num where num is the max size of file sizes to be checked.\n"
			   "   The program will for(i=0; i < check_sizes; i++)\n"
			   "      {\n"
			   "         create a test file of size i;\n"
			   "         loop over each encryption and decryption type;\n"
			   "         encrypt/decryptyption/compare the 'size i' test file;\n"
			   "      }\n"
				"   For -check_sizes=0 then I read a small file tmp_in.txt of 113 bytes.\n"
				"   The default is to not run this test as it can take a while and prints lots of output.\n"
			},
            {"check_loops", 1, 0, 0, "-check_loops=num where num is the number of time to run the loops. Default is 1.\n"
			   "   This number memory. The default is 200 iterations.\n"
			},
            {"verbose", 0, 0, 'v', "-v or -verbose for verbose mode. Default is not verbose."},
            //{0, 0, 0, 0}
        };

       c = my_getopt_long_only (argc, argv, "hvo:", long_options, &option_index);
        if (c == -1)
            break;

       switch (c) {
        case 0:
            printf ("option %s", long_options[option_index].name);
            if (my_optarg)
                printf (" with arg %s", my_optarg);
            printf ("\n");
			if(strcmp(long_options[option_index].name, "bmark_file_name") == 0)
			{
				strncpy(bmark_file_name, my_optarg, sizeof(bmark_file_name));
			}
			else if(strcmp(long_options[option_index].name, "bmark_key_size") == 0)
			{
				bmark_key_size = atoi(my_optarg);
				if(bmark_key_size != 128 && bmark_key_size != 192 && bmark_key_size != 256)
				{
					fprintf(stderr, "Got -bmark_key_size=%s. Key size must be 128, 192 or 256. Bye at %s %d\n", my_optarg, __FILE__, __LINE__);
					exit(2);
				}
			}
			else if(strcmp(long_options[option_index].name, "bmark_file_size") == 0)
			{
				bmark_file_size = atoi(my_optarg);
			}
			else if(strcmp(long_options[option_index].name, "bmark_loops") == 0)
			{
				bmark_loops = atoi(my_optarg);
			}
			else if(strcmp(long_options[option_index].name, "bmark_encrypt") == 0 ||
				strcmp(long_options[option_index].name, "bmark_crypt") == 0 ||
				strcmp(long_options[option_index].name, "bmark_decrypt") == 0)
			{
				int method=0;
				method = 0;
				if(strstr(my_optarg, "USE_iAES_CBC") != NULL)
				{
					method |= USE_iAES_CBC;
					method |= USE_ARRAY;
				}
				else if(strstr(my_optarg, "USE_iAES_CTR") != NULL)
				{
					method |= USE_iAES_CTR;
					method |= USE_ARRAY;
				}
				else if(strstr(my_optarg, "USE_iAES_BLK") != NULL)
				{
					method |= USE_iAES_BLK;
				}
				else if(strstr(my_optarg, "USE_iAE") != NULL)
				{
					method |= USE_iAES;
				}
				else if(strstr(my_optarg, "USE_SAMPLE_CODE") != NULL)
				{
					method |= USE_SAMPLE_CODE;
				}
				if(strstr(my_optarg, "USE_ARRAY") != NULL)
				{
					method |= USE_ARRAY;
				}
				if(strcmp(long_options[option_index].name, "bmark_crypt") == 0)
				{
					bmark_decrypt = method;
					bmark_encrypt = method;
				}
				else if(strcmp(long_options[option_index].name, "bmark_encrypt") == 0)
				{
					bmark_encrypt = method;
				}
				else
				{
					bmark_decrypt = method;
				}
			}
			else if(strcmp(long_options[option_index].name, "check_sizes") == 0)
			{
				check_sizes = atoi(my_optarg);
			}
			else if(strcmp(long_options[option_index].name, "check_loops") == 0)
			{
				outer_loops = atoi(my_optarg);
			}
			else 
			{
				printf("unrecognized option '%s'. Bye at %s %d\n", 
					long_options[option_index].name, __FILE__, __LINE__);
				exit(2);
			}
            break;

       case 'v':
            printf ("option v with value '%s'\n", my_optarg);
            verbose++;
            printf ("verbose set to %d\n", verbose);
            break;

       case 'h':
			printf("usage:\n");
			for(i=0; i < sizeof(long_options)/sizeof(long_options[0]); i++)
			{
				printf("%s\n", long_options[i].desc);
			}
			exit(2);
            break;

       case '?':
            break;

       default:
            printf ("?? getopt returned character code 0%o ??\n", c);
        }
    }

   if (my_optind < argc) {
        printf ("non-option ARGV-elements: ");
        while (my_optind < argc)
            printf ("%s ", argv[my_optind++]);
        printf ("\n");
    }

	if(check_for_aes_instructions() == 0)
	{
		printf("cpu doesn't support AES-NI. This program requires AES-NI support. Bye at %s %d\n",
			__FILE__, __LINE__);
		exit(2);
	}
	
	key_size_in_bytes = bmark_key_size / 8;

	printf("quick test: iAES encryption, using 256 bit key\n");
	aes_encrypt(USE_iAES, test_input, output, gkey, 32);

	printf("output is 0x");
	for(i=0; i < 16; i++) {printf("%.2x", (unsigned int)(output[i]));}
	printf("\n");
	printf("expect is 0x");
	for(i=0; i < 16; i++) {printf("%.2x", (unsigned int)(enc_input[i]));}
	printf("\n");
	for(i=0; i < 16; i++) { 
		if(output[i] != enc_input[i]) { 
			printf("---------------mismatch at char %d is 0x%.2x should be 0x%.2x\n",
				i, (unsigned int)output[i], (unsigned int)enc_input[i]);
			exit(2);
		}
	}
		
	printf("quick test: iAES decryption\n");
    aes_decrypt(USE_iAES, test_input, output, gkey, 32);

	printf("output is 0x");
	for(i=0; i < 16; i++) {printf("%.2x", (unsigned int)(output[i]));}
	printf("\n");
	printf("expect is 0x");
	for(i=0; i < 16; i++) {printf("%.2x", (unsigned int)(dec_input[i]));}
	printf("\n");
	for(i=0; i < 16; i++) { 
		if(output[i] != dec_input[i]) { 
			printf("---------------mismatch at char %d is 0x%.2x should be 0x%.2x\n",
				i, (unsigned int)output[i], (unsigned int)dec_input[i]);
			exit(2);
		}
	}
	printf("quick test passed\n\n");
		
	if(outer_loops < 1)
	{
			outer_loops = 1;
	}
	
	if(outer_loops > 0 && check_sizes > -1)
	{
		mk_a_file(113, "tmp_in.txt");
		check_sizes_and_methods(key_size_in_bytes, check_sizes, fl_vary, "tmp_in.txt", fl_enc, fl_dec, outer_loops);
	}

	if(bmark_file_size > 0 && strlen(bmark_file_name) > 0 && bmark_loops > 0)
	{
		verbose++;
		mk_a_file(bmark_file_size, bmark_file_name);
		printf("input_file (will be overwritten)= %s encoded_file= %s, decoded_file= %s\n",
			bmark_file_name, fl_enc, fl_dec, bmark_encrypt, bmark_decrypt, bmark_file_size, bmark_loops);
		printf("using key_size= %d, bme= 0x%x bmd= 0x%x, size= %d bytes, loops= %d\n\n",
			bmark_key_size, bmark_encrypt, bmark_decrypt, bmark_file_size, bmark_loops);
		printf("Note that each loop includes a call to the expand key routine\n");
#ifndef __linux__
		SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL );
#endif
		do_encrypt_decrypt(key_size_in_bytes, bmark_file_name, fl_enc, fl_dec, bmark_encrypt, bmark_decrypt, bmark_loops);
	}
	//do_encrypt_decrypt("bigfile.txt", fl_enc, fl_dec, USE_iAES_BLK | USE_ARRAY, USE_iAES_BLK | USE_ARRAY, 200);

skip_decrypt:
	printf("everything worked ok.\n");
	return 0;

}
