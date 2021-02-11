/*
 ---------------------------------------------------------------------------
 Copyright (c) 1998-2008, Brian Gladman, Worcester, UK. All rights reserved.

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
 Issue Date: 20/12/2007
*/

#define DUAL_CORE

#if defined( DUAL_CORE ) || defined( DLL_IMPORT ) && defined( DYNAMIC_LINK )
#ifndef __linux__
#include <windows.h>
#include <time.h>
#else
#define _GNU_SOURCE
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>

#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "aes.h"
#include "aesopt.h"
#include "aestst.h"
#include "aesaux.h"
#include "rdtsc.h"
#include "iaesni.h"
#include "iaes_asm_interface.h"
#include "process_options.h"


#if defined( USE_VIA_ACE_IF_PRESENT )

#include "aes_via_ace.h"

#else

#define aligned_array(type, name, no, stride) type name[no]
#define aligned_auto(type, name, no, stride)  type name[no]

#endif

#define TEST_CBC
#if 0
#define TEST_ECB
#define TEST_CFB
#define TEST_OFB
#endif
#define TEST_CTR
#define TEST_CTR_MT

//#define WHOLE_BLOCKS
#define VALIDATE_IN_TIMING

#if defined( DLL_IMPORT ) && defined( DYNAMIC_LINK )
fn_ptrs fn;
#endif

#define SAMPLE1  1000
#define SAMPLE2 10000

#ifndef LONG
#define LONG unsigned int
#endif

#ifndef UINT
#define UINT unsigned int
#endif

#ifndef UCHAR
#define UCHAR unsigned char
#endif

#ifndef DWORD
#define DWORD unsigned int
#endif

#define MAX_NUM_THREADS 256

typedef struct {
#ifdef __linux__
	pthread_barrier_t *mutex_start;
	pthread_barrier_t *mutex_end;
#else
	volatile LONG *mutex_start;
	volatile LONG *mutex_end;
#endif
	UINT k_len;
	UINT num_threads;
	UINT blocks;
	UINT thread_num;
	UINT num_iterations;
	//void (__cdecl *proc)(sAesData *);
	//KEYGENPROC key_gen_proc;
	//sAesData data;
	UCHAR *key;
	double elapsed_secs;
	double avg_clocks;
	double MB_per_sec;
	//cbuf_inc ctr_inc2;
	//char padd[256];
} ThreadCommand;


ThreadCommand cmds[MAX_NUM_THREADS];

#ifdef __linux__
	pthread_barrier_t mutex_start;
	pthread_barrier_t mutex_end;
#else
	volatile unsigned int mutex_start;
	volatile unsigned int mutex_end;
#endif

#ifndef __linux__
SYSTEM_INFO si;
#endif

void ck_print(char *file, int line)
{
	printf("got to %s %d\n", file, line);
	fflush(NULL);
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


void ECBenc(unsigned char *buf, int len, f_ectx cx[1])
{   int cnt = len / AES_BLOCK_SIZE;

    while(cnt--)
        f_enc_blk(cx, buf, buf), buf += AES_BLOCK_SIZE;
}

void ECBdec(unsigned char *buf, int len, f_dctx cx[1])
{   int cnt = len / AES_BLOCK_SIZE;

    while(cnt--)
        f_dec_blk(cx, buf, buf), buf += AES_BLOCK_SIZE;
}

void CBCenc(unsigned char *buf, int len, unsigned char *iv, f_ectx cx[1])
{   int cnt = len / AES_BLOCK_SIZE, i;

    while(cnt--)
    {
        for(i = 0; i < AES_BLOCK_SIZE; i++)
            buf[i] ^= iv[i];

        f_enc_blk(cx, buf, buf);
        memcpy(iv, buf, AES_BLOCK_SIZE);
        buf += AES_BLOCK_SIZE;
    }
}

void CBCdec(unsigned char *buf, int len, unsigned char *iv, f_dctx cx[1])
{   unsigned char temp[AES_BLOCK_SIZE];
    int cnt = len / AES_BLOCK_SIZE, i;

    while( cnt-- )
    {
        memcpy(temp, buf, AES_BLOCK_SIZE);
        f_dec_blk(cx, buf, buf);

        for(i = 0; i < AES_BLOCK_SIZE; i++)
            buf[i] ^= iv[i];

        memcpy(iv, temp, AES_BLOCK_SIZE);
        buf += AES_BLOCK_SIZE;
    }
}

void CFBenc(unsigned char *buf, int len, unsigned char *iv, f_ectx cx[1])
{   int i, nb, cnt = f_info(cx);

    if(cnt)
    {
        nb = AES_BLOCK_SIZE - cnt;
        if(len < nb) nb = len;

        for(i = 0; i < nb; i++)
            buf[i] ^= iv[i + cnt];

        memcpy(iv + cnt, buf, nb);
        len -= nb, buf += nb, cnt += nb;
    }

    while(len)
    {
        cnt = (len > AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : len;
        f_enc_blk(cx, iv, iv);
        for(i = 0; i < cnt; i++)
            buf[i] ^= iv[i];
        memcpy(iv, buf, cnt);
        len -= cnt, buf += cnt;
    }

    f_info(cx) = (cnt % AES_BLOCK_SIZE);
}

void CFBdec(unsigned char *buf, int len, unsigned char *iv, f_ectx cx[1])
{   unsigned char temp[AES_BLOCK_SIZE];
    int i, nb, cnt = f_info(cx);

    if(cnt)
    {
        nb = AES_BLOCK_SIZE - cnt;
        if(len < nb) nb = len;
        memcpy(temp, buf, nb);

        for(i = 0; i < nb; i++)
            buf[i] ^= iv[i + cnt];

        memcpy(iv + cnt, temp, nb);
        len -= nb, buf += nb, cnt += nb;
    }

    while(len)
    {
        cnt = (len > AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : len;
        f_enc_blk(cx, iv, iv);
        memcpy(temp, buf, cnt);

        for(i = 0; i < cnt; i++)
            buf[i] ^= iv[i];

        memcpy(iv, temp, cnt);
        len -= cnt, buf += cnt;
    }
    f_info(cx) = (cnt % AES_BLOCK_SIZE);
}

void OFBenc(unsigned char *buf, int len, unsigned char *iv, f_ectx cx[1])
{   int i, nb, cnt = f_info(cx);

    if(cnt)
    {
        nb = AES_BLOCK_SIZE - cnt;
        if(len < nb) nb = len;

        for(i = 0; i < nb; i++)
            buf[i] ^= iv[i + cnt];

        len -= nb, buf += nb, cnt += nb;
    }

    while(len)
    {
        cnt = (len > AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : len;
        f_enc_blk(cx, iv, iv);

        for(i = 0; i < cnt; i++)
            buf[i] ^= iv[i];

        len -= cnt, buf += cnt;
    }

    f_info(cx) = (cnt % AES_BLOCK_SIZE);
}

void OFBdec(unsigned char *buf, int len, unsigned char *iv, f_ectx cx[1])
{   int i, nb, cnt = f_info(cx);

    if( cnt )
    {
        nb = AES_BLOCK_SIZE - cnt;
        if(len < nb) nb = len;

        for(i = 0; i < nb; i++)
            buf[i] ^= iv[i + cnt];

        len -= nb, buf += nb, cnt += nb;
    }

    while(len)
    {
        cnt = (len > AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : len;
        f_enc_blk(cx, iv, iv);

        for(i = 0; i < cnt; i++)
            buf[i] ^= iv[i];

        len -= cnt, buf += cnt;
    }

    f_info(cx) = (cnt % AES_BLOCK_SIZE);
}

void CTRcry(unsigned char *buf, int len, unsigned char *cbuf, cbuf_inc *incf, f_ectx cx[1])
{   int i, cnt;
    uint_8t ecbuf[AES_BLOCK_SIZE];

    while(len)
    {
        cnt = (len > AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : len;
        f_enc_blk(cx, cbuf, ecbuf);
        if(cnt == AES_BLOCK_SIZE)
            incf(cbuf);

        for(i = 0; i < cnt; i++)
            buf[i] ^= ecbuf[i];

        len -= cnt, buf += cnt;
    }
}

int time_base(double *av, double *sig)
{   int                 i, tol, lcnt, sam_cnt;
    double              cy, av1, sig1;
	unsigned long long cy0;

    tol = 10; lcnt = sam_cnt = 0;
    while(!sam_cnt)
    {
        av1 = sig1 = 0.0;

        for(i = 0; i < SAMPLE1; ++i)
        {
            cy0 = read_tsc();
            cy = (double)(read_tsc() - cy0);

            av1 += cy;
            sig1 += cy * cy;
        }

        av1 /= SAMPLE1;
        sig1 = sqrt((sig1 - av1 * av1 * SAMPLE1) / SAMPLE1);
        sig1 = (sig1 < 0.05 * av1 ? 0.05 * av1 : sig1);

        *av = *sig = 0.0;
        for(i = 0; i < SAMPLE2; ++i)
        {
            cy0 = read_tsc();
            cy = (double)(read_tsc() - cy0);

            if(cy > av1 - sig1 && cy < av1 + sig1)
            {
                *av += cy;
                *sig += cy * cy;
                sam_cnt++;
            }
        }

        if(10 * sam_cnt > 9 * SAMPLE2)
        {
            *av /= sam_cnt;
            *sig = sqrt((*sig - *av * *av * sam_cnt) / sam_cnt);
            if(*sig > (tol / 100.0) * *av)
                sam_cnt = 0;
        }
        else
        {
            if(lcnt++ == 10)
            {
                lcnt = 0; tol += 5;
                if(tol > 30)
                    return 0;
            }
            sam_cnt = 0;
        }
    }

    return 1;
}

#ifdef TEST_ECB

int time_ecb_enc(unsigned int k_len, int blocks, double *av, double *sig)
{   int                 i, tol, lcnt, sam_cnt;
    double              cy, av1, sig1;
    unsigned char       key[2 * AES_BLOCK_SIZE];
    unsigned char       vb[10000 * AES_BLOCK_SIZE];
	unsigned long long  cy0;

    aligned_auto(unsigned char, pt, 10000 * AES_BLOCK_SIZE, 16);
    aligned_auto(f_ectx, ecx, 1, 16);

    block_rndfill(key, 2 * AES_BLOCK_SIZE);
    f_enc_key(ecx, key, k_len);
    block_rndfill(pt, blocks * AES_BLOCK_SIZE);
    memcpy(vb, pt, blocks * AES_BLOCK_SIZE);
    f_ecb_enc(ecx, pt, pt, blocks * AES_BLOCK_SIZE);
#ifdef VALIDATE_IN_TIMING
    ECBenc(vb, blocks * AES_BLOCK_SIZE, ecx);
    if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
        goto error;
#endif
    tol = 10; lcnt = sam_cnt = 0;
    while(!sam_cnt)
    {
        av1 = sig1 = 0.0;

        for(i = 0; i < SAMPLE1; ++i)
        {
            cy0 = read_tsc();
            f_ecb_enc(ecx, pt, pt, blocks * AES_BLOCK_SIZE);
            cy = (double)(read_tsc() - cy0);

            av1 += cy;
            sig1 += cy * cy;
#ifdef VALIDATE_IN_TIMING
            ECBenc(vb, blocks * AES_BLOCK_SIZE, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error;
#endif
        }

        av1 /= SAMPLE1;
        sig1 = sqrt((sig1 - av1 * av1 * SAMPLE1) / SAMPLE1);
        sig1 = (sig1 < 0.05 * av1 ? 0.05 * av1 : sig1);

        *av = *sig = 0.0;
#if 0
        for(i = 0; i < SAMPLE2; ++i)
        {
            cy0 = read_tsc();
            f_ecb_enc(ecx, pt, pt, blocks * AES_BLOCK_SIZE);
            cy = (double)(read_tsc() - cy0);

            if(cy > av1 - sig1 && cy < av1 + sig1)
            {
                *av += cy;
                *sig += cy * cy;
                sam_cnt++;
            }
#ifdef VALIDATE_IN_TIMING
            ECBenc(vb, blocks * AES_BLOCK_SIZE, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error;
#endif
        }
        if(10 * sam_cnt > 9 * SAMPLE2)
        {
            *av /= sam_cnt;
            *sig = sqrt((*sig - *av * *av * sam_cnt) / sam_cnt);
            if(*sig > (tol / 100.0) * *av)
                sam_cnt = 0;
        }
        else
        {
            if(lcnt++ == 10)
            {
                lcnt = 0; tol += 5;
                if(tol > 30)
                    return 0;
            }
            sam_cnt = 0;
        }
#else
        cy0 = read_tsc();
        for(i = 0; i < SAMPLE2; ++i)
        {
            f_ecb_enc(ecx, pt, pt, blocks * AES_BLOCK_SIZE);

        }
        cy = (double)(read_tsc() - cy0);
        *av = cy/(double)SAMPLE2;
        *sig = cy * cy;
        sam_cnt++;
#endif

    }

    return 1;
#ifdef VALIDATE_IN_TIMING
error:
    printf("\nECB Encryption data error in timing");
    exit(1);
#endif
}

int time_ecb_dec(unsigned int k_len, int blocks, double *av, double *sig)
{   int                 i, tol, lcnt, sam_cnt;
    double              cy, av1, sig1;
    unsigned char       key[2 * AES_BLOCK_SIZE];
    unsigned char       vb[10000 * AES_BLOCK_SIZE];
	unsigned long long cy0;

    aligned_auto(unsigned char, pt, 10000 * AES_BLOCK_SIZE, 16);
    aligned_auto(f_dctx, dcx, 1, 16);

    block_rndfill(key, 2 * AES_BLOCK_SIZE);
    f_dec_key(dcx, key, k_len);
    block_rndfill(pt, blocks * AES_BLOCK_SIZE);
    memcpy(vb, pt, blocks * AES_BLOCK_SIZE);
    f_ecb_dec(dcx, pt, pt, blocks * AES_BLOCK_SIZE);
#ifdef VALIDATE_IN_TIMING
    ECBdec(vb, blocks * AES_BLOCK_SIZE, dcx);
    if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
        goto error;
#endif
    tol = 10; lcnt = sam_cnt = 0;

    while(!sam_cnt)
    {
        av1 = sig1 = 0.0;

        for(i = 0; i < SAMPLE1; ++i)
        {
            cy0 = read_tsc();
            f_ecb_dec(dcx, pt, pt, blocks * AES_BLOCK_SIZE);
            cy = (double)(read_tsc() - cy0);

            av1 += cy;
            sig1 += cy * cy;
#ifdef VALIDATE_IN_TIMING
            ECBdec(vb, blocks * AES_BLOCK_SIZE, dcx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error;
#endif
        }

        av1 /= SAMPLE1;
        sig1 = sqrt((sig1 - av1 * av1 * SAMPLE1) / SAMPLE1);
        sig1 = (sig1 < 0.05 * av1 ? 0.05 * av1 : sig1);

        *av = *sig = 0.0;
#if 0
        for(i = 0; i < SAMPLE2; ++i)
        {
            cy0 = read_tsc();
            f_ecb_dec(dcx, pt, pt, blocks * AES_BLOCK_SIZE);
            cy = (double)(read_tsc() - cy0);

            if(cy > av1 - sig1 && cy < av1 + sig1)
            {
                *av += cy;
                *sig += cy * cy;
                sam_cnt++;
            }
#ifdef VALIDATE_IN_TIMING
            ECBdec(vb, blocks * AES_BLOCK_SIZE, dcx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error;
#endif
        }

        if(10 * sam_cnt > 9 * SAMPLE2)
        {
            *av /= sam_cnt;
            *sig = sqrt((*sig - *av * *av * sam_cnt) / sam_cnt);
            if(*sig > (tol / 100.0) * *av)
                sam_cnt = 0;
        }
        else
        {
            if(lcnt++ == 10)
            {
                lcnt = 0; tol += 5;
                if(tol > 30)
                    return 0;
            }
            sam_cnt = 0;
        }
#else
        cy0 = read_tsc();
        for(i = 0; i < SAMPLE2; ++i)
        {
            f_ecb_dec(dcx, pt, pt, blocks * AES_BLOCK_SIZE);
		}
        cy = (double)(read_tsc() - cy0);

        *av = cy / (double)(SAMPLE2);
        *sig = 0;
        sam_cnt++;
#endif
    }

    return 1;
#ifdef VALIDATE_IN_TIMING
error:
    printf("\nECB Encryption data error in timing");
    exit(1);
#endif
}

#endif

#ifdef TEST_CBC

int time_cbc_enc(unsigned int k_len, int blocks, double *av, double *sig)
{   int                 i, tol, lcnt, sam_cnt;
    double              cy, av1, sig1;
    unsigned char       key[2 * AES_BLOCK_SIZE];
    unsigned char       vb[10000 * AES_BLOCK_SIZE];
    unsigned char       viv[AES_BLOCK_SIZE];

    aligned_auto(unsigned char, pt, 10000 * AES_BLOCK_SIZE, 16);
    aligned_auto(unsigned char, iv, AES_BLOCK_SIZE, 16);
    aligned_auto(f_ectx, ecx, 1, 16);

    block_rndfill(key, 2 * AES_BLOCK_SIZE);
    f_enc_key(ecx, key, k_len);
    block_rndfill(iv, AES_BLOCK_SIZE);
    memcpy(viv, iv, AES_BLOCK_SIZE);
    block_rndfill(pt, blocks * AES_BLOCK_SIZE);
    memcpy(vb, pt, blocks * AES_BLOCK_SIZE);
    f_cbc_enc(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
#ifdef VALIDATE_IN_TIMING
    CBCenc(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
    if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
        goto error1;
    if(memcmp(viv, iv, AES_BLOCK_SIZE))
        goto error2;
#endif
    tol = 10; lcnt = sam_cnt = 0;
    while(!sam_cnt)
    {
        av1 = sig1 = 0.0;

        for(i = 0; i < SAMPLE1; ++i)
        {
            cy = (double)read_tsc();
            f_cbc_enc(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            av1 += cy;
            sig1 += cy * cy;
#ifdef VALIDATE_IN_TIMING
            CBCenc(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        av1 /= SAMPLE1;
        sig1 = sqrt((sig1 - av1 * av1 * SAMPLE1) / SAMPLE1);
        sig1 = (sig1 < 0.05 * av1 ? 0.05 * av1 : sig1);

        *av = *sig = 0.0;
        for(i = 0; i < SAMPLE2; ++i)
        {
            cy = (double)read_tsc();
            f_cbc_enc(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            if(cy > av1 - sig1 && cy < av1 + sig1)
            {
                *av += cy;
                *sig += cy * cy;
                sam_cnt++;
            }
#ifdef VALIDATE_IN_TIMING
            CBCenc(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        if(10 * sam_cnt > 9 * SAMPLE2)
        {
            *av /= sam_cnt;
            *sig = sqrt((*sig - *av * *av * sam_cnt) / sam_cnt);
            if(*sig > (tol / 100.0) * *av)
                sam_cnt = 0;
        }
        else
        {
            if(lcnt++ == 10)
            {
                lcnt = 0; tol += 5;
                if(tol > 30)
                    return 0;
            }
            sam_cnt = 0;
        }
    }

    return 1;
#ifdef VALIDATE_IN_TIMING
error1:
    printf("\nCBC Encryption data error in timing");
    exit(1);
error2:
    printf("\nCBC Encryption iv error in timing");
    exit(1);
#endif
}

int time_cbc_dec(unsigned int k_len, int blocks, double *av, double *sig)
{   int                 i, tol, lcnt, sam_cnt;
    double              cy, av1, sig1;
    unsigned char       key[2 * AES_BLOCK_SIZE];
    unsigned char       vb[10000 * AES_BLOCK_SIZE];
    unsigned char       viv[AES_BLOCK_SIZE];

    aligned_auto(unsigned char, pt, 10000 * AES_BLOCK_SIZE, 16);
    aligned_auto(unsigned char, iv, AES_BLOCK_SIZE, 16);
    aligned_auto(f_dctx, dcx, 1, 16);


    block_rndfill(key, 2 * AES_BLOCK_SIZE);
    f_dec_key(dcx, key, k_len);
    block_rndfill(iv, AES_BLOCK_SIZE);
    memcpy(viv, iv, AES_BLOCK_SIZE);
    block_rndfill(pt, blocks * AES_BLOCK_SIZE);
    memcpy(vb, pt, blocks * AES_BLOCK_SIZE);
    f_cbc_dec(dcx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
#ifdef VALIDATE_IN_TIMING
    CBCdec(vb, blocks * AES_BLOCK_SIZE, viv, dcx);
    if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
        goto error1;
    if(memcmp(viv, iv, AES_BLOCK_SIZE))
        goto error2;
#endif
    tol = 10; lcnt = sam_cnt = 0;
    while(!sam_cnt)
    {
        av1 = sig1 = 0.0;

        for(i = 0; i < SAMPLE1; ++i)
        {
            cy = (double)read_tsc();
            f_cbc_dec(dcx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            av1 += cy;
            sig1 += cy * cy;
#ifdef VALIDATE_IN_TIMING
            CBCdec(vb, blocks * AES_BLOCK_SIZE, viv, dcx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        av1 /= SAMPLE1;
        sig1 = sqrt((sig1 - av1 * av1 * SAMPLE1) / SAMPLE1);
        sig1 = (sig1 < 0.05 * av1 ? 0.05 * av1 : sig1);

        *av = *sig = 0.0;
        for(i = 0; i < SAMPLE2; ++i)
        {
            cy = (double)read_tsc();
            f_cbc_dec(dcx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            if(cy > av1 - sig1 && cy < av1 + sig1)
            {
                *av += cy;
                *sig += cy * cy;
                sam_cnt++;
            }
#ifdef VALIDATE_IN_TIMING
            CBCdec(vb, blocks * AES_BLOCK_SIZE, viv, dcx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        if(10 * sam_cnt > 9 * SAMPLE2)
        {
            *av /= sam_cnt;
            *sig = sqrt((*sig - *av * *av * sam_cnt) / sam_cnt);
            if(*sig > (tol / 100.0) * *av)
                sam_cnt = 0;
        }
        else
        {
            if(lcnt++ == 10)
            {
                lcnt = 0; tol += 5;
                if(tol > 30)
                    return 0;
            }
            sam_cnt = 0;
        }
    }

    return 1;
#ifdef VALIDATE_IN_TIMING
error1:
    printf("\nCBC Decryption data error in timing");
    exit(1);
error2:
    printf("\nCBC Decryption iv error in timing");
    exit(1);
#endif
}

#endif

#ifdef TEST_CFB

int time_cfb_enc(unsigned int k_len, int blocks, double *av, double *sig)
{   int                 i, tol, lcnt, sam_cnt;
    double              cy, av1, sig1;
    unsigned char       key[2 * AES_BLOCK_SIZE];
    unsigned char       vb[10000 * AES_BLOCK_SIZE];
    unsigned char       viv[AES_BLOCK_SIZE];

    aligned_auto(unsigned char, pt, 10000 * AES_BLOCK_SIZE, 16);
    aligned_auto(unsigned char, iv, AES_BLOCK_SIZE, 16);
    aligned_auto(f_ectx, ecx, 1, 16);

    block_rndfill(key, 2 * AES_BLOCK_SIZE);
    f_enc_key(ecx, key, k_len);
    block_rndfill(iv, AES_BLOCK_SIZE);
    memcpy(viv, iv, AES_BLOCK_SIZE);
    block_rndfill(pt, blocks * AES_BLOCK_SIZE);
    memcpy(vb, pt, blocks * AES_BLOCK_SIZE);
    f_cfb_enc(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
#ifdef VALIDATE_IN_TIMING
    CFBenc(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
    if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
        goto error1;
    if(memcmp(viv, iv, AES_BLOCK_SIZE))
        goto error2;
#endif
    tol = 10; lcnt = sam_cnt = 0;
    while(!sam_cnt)
    {
        av1 = sig1 = 0.0;

        for(i = 0; i < SAMPLE1; ++i)
        {
            cy = (double)read_tsc();
            f_cfb_enc(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            av1 += cy;
            sig1 += cy * cy;
#ifdef VALIDATE_IN_TIMING
            CFBenc(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        av1 /= SAMPLE1;
        sig1 = sqrt((sig1 - av1 * av1 * SAMPLE1) / SAMPLE1);
        sig1 = (sig1 < 0.05 * av1 ? 0.05 * av1 : sig1);

        *av = *sig = 0.0;
        for(i = 0; i < SAMPLE2; ++i)
        {
            cy = (double)read_tsc();
            f_cfb_enc(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            if(cy > av1 - sig1 && cy < av1 + sig1)
            {
                *av += cy;
                *sig += cy * cy;
                sam_cnt++;
            }
#ifdef VALIDATE_IN_TIMING
            CFBenc(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        if(10 * sam_cnt > 9 * SAMPLE2)
        {
            *av /= sam_cnt;
            *sig = sqrt((*sig - *av * *av * sam_cnt) / sam_cnt);
            if(*sig > (tol / 100.0) * *av)
                sam_cnt = 0;
        }
        else
        {
            if(lcnt++ == 10)
            {
                lcnt = 0; tol += 5;
                if(tol > 30)
                    return 0;
            }
            sam_cnt = 0;
        }
    }

    return 1;
#ifdef VALIDATE_IN_TIMING
error1:
    printf("\nCFB Encryption data error in timing");
    exit(1);
error2:
    printf("\nCFB Encryption iv error in timing");
    exit(1);
#endif
}

int time_cfb_dec(unsigned int k_len, int blocks, double *av, double *sig)
{   int                 i, tol, lcnt, sam_cnt;
    double              cy, av1, sig1;
    unsigned char       key[2 * AES_BLOCK_SIZE];
    unsigned char       vb[10000 * AES_BLOCK_SIZE];
    unsigned char       viv[AES_BLOCK_SIZE];

    aligned_auto(unsigned char, pt, 10000 * AES_BLOCK_SIZE, 16);
    aligned_auto(unsigned char, iv, AES_BLOCK_SIZE, 16);
    aligned_auto(f_ectx, ecx, 1, 16);

    block_rndfill(key, 2 * AES_BLOCK_SIZE);
    f_enc_key(ecx, key, k_len);
    block_rndfill(iv, AES_BLOCK_SIZE);
    memcpy(viv, iv, AES_BLOCK_SIZE);
    block_rndfill(pt, blocks * AES_BLOCK_SIZE);
    memcpy(vb, pt, blocks * AES_BLOCK_SIZE);
    f_cfb_dec(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
#ifdef VALIDATE_IN_TIMING
    CFBdec(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
    if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
        goto error1;
    if(memcmp(viv, iv, AES_BLOCK_SIZE))
        goto error2;
#endif
    tol = 10; lcnt = sam_cnt = 0;
    while(!sam_cnt)
    {
        av1 = sig1 = 0.0;

        for(i = 0; i < SAMPLE1; ++i)
        {
            cy = (double)read_tsc();
            f_cfb_dec(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            av1 += cy;
            sig1 += cy * cy;
#ifdef VALIDATE_IN_TIMING
            CFBdec(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        av1 /= SAMPLE1;
        sig1 = sqrt((sig1 - av1 * av1 * SAMPLE1) / SAMPLE1);
        sig1 = (sig1 < 0.05 * av1 ? 0.05 * av1 : sig1);

        *av = *sig = 0.0;
        for(i = 0; i < SAMPLE2; ++i)
        {
            cy = (double)read_tsc();
            f_cfb_dec(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            if(cy > av1 - sig1 && cy < av1 + sig1)
            {
                *av += cy;
                *sig += cy * cy;
                sam_cnt++;
            }
#ifdef VALIDATE_IN_TIMING
            CFBdec(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        if(10 * sam_cnt > 9 * SAMPLE2)
        {
            *av /= sam_cnt;
            *sig = sqrt((*sig - *av * *av * sam_cnt) / sam_cnt);
            if(*sig > (tol / 100.0) * *av)
                sam_cnt = 0;
        }
        else
        {
            if(lcnt++ == 10)
            {
                lcnt = 0; tol += 5;
                if(tol > 30)
                    return 0;
            }
            sam_cnt = 0;
        }
    }

    return 1;
#ifdef VALIDATE_IN_TIMING
error1:
    printf("\nCFB Decryption data error in timing");
    exit(1);
error2:
    printf("\nCFB Decryption iv error in timing");
    exit(1);
#endif
}

#endif

#ifdef TEST_OFB

int time_ofb_enc(unsigned int k_len, int blocks, double *av, double *sig)
{   int                 i, tol, lcnt, sam_cnt;
    double              cy, av1, sig1;
    unsigned char       key[2 * AES_BLOCK_SIZE];
    unsigned char       vb[10000 * AES_BLOCK_SIZE];
    unsigned char       viv[AES_BLOCK_SIZE];

    aligned_auto(unsigned char, pt, 10000 * AES_BLOCK_SIZE, 16);
    aligned_auto(unsigned char, iv, AES_BLOCK_SIZE, 16);
    aligned_auto(f_ectx, ecx, 1, 16);

    block_rndfill(key, 2 * AES_BLOCK_SIZE);
    f_enc_key(ecx, key, k_len);
    block_rndfill(iv, AES_BLOCK_SIZE);
    memcpy(viv, iv, AES_BLOCK_SIZE);
    block_rndfill(pt, blocks * AES_BLOCK_SIZE);
    memcpy(vb, pt, blocks * AES_BLOCK_SIZE);
    f_ofb_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
#ifdef VALIDATE_IN_TIMING
    OFBenc(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
    if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
        goto error1;
    if(memcmp(viv, iv, AES_BLOCK_SIZE))
        goto error2;
#endif
    tol = 10; lcnt = sam_cnt = 0;
    while(!sam_cnt)
    {
        av1 = sig1 = 0.0;

        for(i = 0; i < SAMPLE1; ++i)
        {
            cy = (double)read_tsc();
            f_ofb_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            av1 += cy;
            sig1 += cy * cy;
#ifdef VALIDATE_IN_TIMING
            OFBenc(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        av1 /= SAMPLE1;
        sig1 = sqrt((sig1 - av1 * av1 * SAMPLE1) / SAMPLE1);
        sig1 = (sig1 < 0.05 * av1 ? 0.05 * av1 : sig1);

        *av = *sig = 0.0;
        for(i = 0; i < SAMPLE2; ++i)
        {
            cy = (double)read_tsc();
            f_ofb_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            if(cy > av1 - sig1 && cy < av1 + sig1)
            {
                *av += cy;
                *sig += cy * cy;
                sam_cnt++;
            }
#ifdef VALIDATE_IN_TIMING
            OFBenc(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        if(10 * sam_cnt > 9 * SAMPLE2)
        {
            *av /= sam_cnt;
            *sig = sqrt((*sig - *av * *av * sam_cnt) / sam_cnt);
            if(*sig > (tol / 100.0) * *av)
                sam_cnt = 0;
        }
        else
        {
            if(lcnt++ == 10)
            {
                lcnt = 0; tol += 5;
                if(tol > 30)
                    return 0;
            }
            sam_cnt = 0;
        }
    }

    return 1;
#ifdef VALIDATE_IN_TIMING
error1:
    printf("\nOFB Encryption data error in timing");
    exit(1);
error2:
    printf("\nOFB Encryption iv error in timing");
    exit(1);
#endif
}

int time_ofb_dec(unsigned int k_len, int blocks, double *av, double *sig)
{   int                 i, tol, lcnt, sam_cnt;
    double              cy, av1, sig1;
    unsigned char       key[2 * AES_BLOCK_SIZE];
    unsigned char       vb[10000 * AES_BLOCK_SIZE];
    unsigned char       viv[AES_BLOCK_SIZE];

    aligned_auto(unsigned char, pt, 10000 * AES_BLOCK_SIZE, 16);
    aligned_auto(unsigned char, iv, AES_BLOCK_SIZE, 16);
    aligned_auto(f_ectx, ecx, 1, 16);

    block_rndfill(key, 2 * AES_BLOCK_SIZE);
    f_enc_key(ecx, key, k_len);
    block_rndfill(iv, AES_BLOCK_SIZE);
    memcpy(viv, iv, AES_BLOCK_SIZE);
    block_rndfill(pt, blocks * AES_BLOCK_SIZE);
    memcpy(vb, pt, blocks * AES_BLOCK_SIZE);
    f_ofb_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
#ifdef VALIDATE_IN_TIMING
    OFBdec(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
    if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
        goto error1;
    if(memcmp(viv, iv, AES_BLOCK_SIZE))
        goto error2;
#endif
    tol = 10; lcnt = sam_cnt = 0;
    while(!sam_cnt)
    {
        av1 = sig1 = 0.0;

        for(i = 0; i < SAMPLE1; ++i)
        {
            cy = (double)read_tsc();
            f_ofb_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            av1 += cy;
            sig1 += cy * cy;
#ifdef VALIDATE_IN_TIMING
            OFBdec(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        av1 /= SAMPLE1;
        sig1 = sqrt((sig1 - av1 * av1 * SAMPLE1) / SAMPLE1);
        sig1 = (sig1 < 0.05 * av1 ? 0.05 * av1 : sig1);

        *av = *sig = 0.0;
        for(i = 0; i < SAMPLE2; ++i)
        {
            cy = (double)read_tsc();
            f_ofb_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, iv);
            cy = (double)read_tsc() - cy;

            if(cy > av1 - sig1 && cy < av1 + sig1)
            {
                *av += cy;
                *sig += cy * cy;
                sam_cnt++;
            }
#ifdef VALIDATE_IN_TIMING
            OFBdec(vb, blocks * AES_BLOCK_SIZE, viv, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, iv, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        if(10 * sam_cnt > 9 * SAMPLE2)
        {
            *av /= sam_cnt;
            *sig = sqrt((*sig - *av * *av * sam_cnt) / sam_cnt);
            if(*sig > (tol / 100.0) * *av)
                sam_cnt = 0;
        }
        else
        {
            if(lcnt++ == 10)
            {
                lcnt = 0; tol += 5;
                if(tol > 30)
                    return 0;
            }
            sam_cnt = 0;
        }
    }

    return 1;
#ifdef VALIDATE_IN_TIMING
error1:
    printf("\nOFB Decryption data error in timing");
    exit(1);
error2:
    printf("\nOFB Decryption iv error in timing");
    exit(1);
#endif
}

#endif

#ifdef TEST_CTR

int time_ctr_crypt(unsigned int k_len, int blocks, cbuf_inc ctr_inc, double *av, double *sig, int loops)
{   int                 i, tol, lcnt, sam_cnt, sample2;
    double              cy, av1, sig1, av0;
	unsigned long long  cy0;
    unsigned char       key[2 * AES_BLOCK_SIZE];
    unsigned char       vb[10000 * AES_BLOCK_SIZE];
    unsigned char       viv[AES_BLOCK_SIZE];

    aligned_auto(unsigned char, pt, 10000 * AES_BLOCK_SIZE, 16);
    aligned_auto(unsigned char, cbuf, AES_BLOCK_SIZE, 16);
    aligned_auto(f_ectx, ecx, 1, 16);

    block_rndfill(key, 2 * AES_BLOCK_SIZE);
    f_enc_key(ecx, key, k_len);
#if 1
	if(blocks == 1)
	{
	av0 = 0;
	for(i = 0; i < SAMPLE2; ++i)
    {
        cy0 = read_tsc();
    	f_enc_key(ecx, key, k_len);
        cy0 = read_tsc() - cy0;
        av0 += (double)cy0;
	}
	//printf("cycles per ctr key_encryption= %f at %s %d\n", av0/(double)(SAMPLE2), __FILE__, __LINE__);
	//printf("ctr encrypt ");
	}
#endif
    block_rndfill(cbuf, AES_BLOCK_SIZE);
    memcpy(viv, cbuf, AES_BLOCK_SIZE);
    block_rndfill(pt, blocks * AES_BLOCK_SIZE);
    memcpy(vb, pt, blocks * AES_BLOCK_SIZE);
    f_ctr_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, cbuf, ctr_inc);
#ifdef VALIDATE_IN_TIMING
    CTRcry(vb, blocks * AES_BLOCK_SIZE, viv, ctr_inc, ecx);
    if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
        goto error1;
    if(memcmp(viv, cbuf, AES_BLOCK_SIZE))
        goto error2;
#endif
    tol = 10; lcnt = sam_cnt = 0;
    while(!sam_cnt)
    {
        av1 = sig1 = 0.0;

        for(i = 0; i < SAMPLE1; ++i)
        {
            cy = (double)read_tsc();
            f_ctr_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, cbuf, ctr_inc);
            cy = (double)read_tsc() - cy;

            av1 += cy;
            sig1 += cy * cy;
#ifdef VALIDATE_IN_TIMING
            CTRcry(vb, blocks * AES_BLOCK_SIZE, viv, ctr_inc, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, cbuf, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        av1 /= SAMPLE1;
        sig1 = sqrt((sig1 - av1 * av1 * SAMPLE1) / SAMPLE1);
        sig1 = (sig1 < 0.05 * av1 ? 0.05 * av1 : sig1);

        *av = *sig = 0.0;
		sample2 = loops;
		if(sample2 == 0)
		{
			sample2 = SAMPLE2;
		}
        for(i = 0; i < sample2; ++i)
        {
            cy = (double)read_tsc();
            f_ctr_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, cbuf, ctr_inc);
            cy = (double)read_tsc() - cy;

            if(cy > av1 - sig1 && cy < av1 + sig1)
            {
                *av += cy;
                *sig += cy * cy;
                sam_cnt++;
            }
#ifdef VALIDATE_IN_TIMING
            CTRcry(vb, blocks * AES_BLOCK_SIZE, viv, ctr_inc, ecx);
            if(memcmp(pt, vb, blocks * AES_BLOCK_SIZE))
                goto error1;
            if(memcmp(viv, cbuf, AES_BLOCK_SIZE))
                goto error2;
#endif
        }

        if(10 * sam_cnt > 9 * sample2)
        {
            *av /= sam_cnt;
            *sig = sqrt((*sig - *av * *av * sam_cnt) / sam_cnt);
            if(*sig > (tol / 100.0) * *av)
                sam_cnt = 0;
        }
        else
        {
            if(lcnt++ == 10)
            {
                lcnt = 0; tol += 5;
                if(tol > 30)
                    return 0;
            }
            sam_cnt = 0;
        }
    }

    return 1;
#ifdef VALIDATE_IN_TIMING
error1:
    printf("\nOFB Decryption data error in timing");
    exit(1);
error2:
    printf("\nOFB Decryption cbuf error in timing");
    exit(1);
#endif
}

double compute_MB_per_sec(double elapsed_time, int num_iterations, int blocks)
{
	double MB_per_sec = 0;
	if(elapsed_time > 0)
	{
		MB_per_sec = (double)num_iterations;
		MB_per_sec *= (double)(blocks*16);
		MB_per_sec /= elapsed_time;
		MB_per_sec *= 1.0e-6;
		//printf("num_iter= %d, blocks= %d, elapsed_time= %f at %s %d\n", num_iterations, blocks, elapsed_time, __FILE__, __LINE__);
	}
	return MB_per_sec;
}


double time_ctr_crypt2(unsigned int k_len, int blocks, cbuf_inc ctr_inc, int loops, int threads, ThreadCommand *cmd)
{   int                 i, tol, lcnt, sam_cnt, sample2;
    double              av, cy, av1, sig1, av0, elapsed_time;
	unsigned long long  cy0;
    unsigned char       key[2 * AES_BLOCK_SIZE];
    unsigned char       vb[10002 * AES_BLOCK_SIZE];
    unsigned char       viv[AES_BLOCK_SIZE];

    aligned_auto(unsigned char, pt, 10002 * AES_BLOCK_SIZE, 16);
    aligned_auto(unsigned char, pt2, 10002 * AES_BLOCK_SIZE, 16);
    aligned_auto(unsigned char, cbuf, AES_BLOCK_SIZE, 16);
    aligned_auto(f_ectx, ecx, 1, 16);
    aligned_auto(f_dctx, dcx, 1, 16);

    block_rndfill(key, 2 * AES_BLOCK_SIZE);
    f_enc_key(ecx, key, k_len);
    f_dec_key(dcx, key, k_len);
    block_rndfill(cbuf, AES_BLOCK_SIZE);
    memcpy(viv, cbuf, AES_BLOCK_SIZE);
    block_rndfill(pt, blocks * AES_BLOCK_SIZE);
    block_rndfill(pt2, blocks * AES_BLOCK_SIZE);
    memcpy(vb, pt, blocks * AES_BLOCK_SIZE);
    //f_ctr_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, cbuf, ctr_inc);
    tol = 10; lcnt = sam_cnt = 0;
	av1 = sig1 = 0.0;

	sample2 = loops;
	if(sample2 == 0)
	{
		sample2 = SAMPLE2;
	}
#if 0
	for(i = 0; i < 100; ++i)
	{
		f_ctr_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, cbuf, ctr_inc);
	}
#endif

	if(cmd != NULL)
	{
#ifdef __linux__
		pthread_barrier_wait(cmd->mutex_start);
#else
		InterlockedIncrement(cmd->mutex_start);
		while(*cmd->mutex_start < cmd->num_threads);
#endif
		//printf("in crypt2 thread= %d at %s %d\n", cmd->thread_num, __FILE__, __LINE__);
	}

	if(options.crypt_method == ASM_CBC_ENC)
	{
		elapsed_time = get_elapsed_time();
		cy0 = read_tsc();
		for(i = 0; i < sample2; ++i)
		{
			f_cbc_enc(ecx, pt, pt, blocks * AES_BLOCK_SIZE, cbuf);
		}
		av = (double)(read_tsc() - cy0);
		elapsed_time = get_elapsed_time() - elapsed_time;
	}
	else if(options.crypt_method == ASM_CBC_DEC)
	{
		elapsed_time = get_elapsed_time();
		cy0 = read_tsc();
		for(i = 0; i < sample2; ++i)
		{
			f_cbc_dec(dcx, pt, pt, blocks * AES_BLOCK_SIZE, cbuf);
		}
		av = (double)(read_tsc() - cy0);
		elapsed_time = get_elapsed_time() - elapsed_time;
	}
	else if(options.crypt_method == ASM_CTR)
	{
		elapsed_time = get_elapsed_time();
		cy0 = read_tsc();
		for(i = 0; i < sample2; ++i)
		{
			f_ctr_cry(ecx, pt, pt, blocks * AES_BLOCK_SIZE, cbuf, ctr_inc);
		}
		av = (double)(read_tsc() - cy0);
		elapsed_time = get_elapsed_time() - elapsed_time;
	}
	else if(options.crypt_method == AES_NI_CTR)
	{
		unsigned char local_test_iv[16];
		DEFINE_ROUND_KEYS
		sAesData aesData;
		aesData.in_block = pt;
		aesData.out_block = pt;
		aesData.expanded_key = (unsigned char *)ecx->ks;
		aesData.num_blocks = blocks;
		aesData.iv = local_test_iv;
		memcpy(local_test_iv, viv, 16);

		if(k_len == 16) 
		{
			elapsed_time = get_elapsed_time();
			cy0 = read_tsc();
			for(i = 0; i < sample2; ++i)
			{
				iEncExpandKey128(key,aesData.expanded_key);
				iEnc128_CTR(&aesData);
			}
			av = (double)(read_tsc() - cy0);
			elapsed_time = get_elapsed_time() - elapsed_time;
		} 
		else if (k_len == 24)
		{
			elapsed_time = get_elapsed_time();
			cy0 = read_tsc();
			for(i = 0; i < sample2; ++i)
			{
				iEncExpandKey192(key,aesData.expanded_key);
				iEnc192_CTR(&aesData);
			}
			av = (double)(read_tsc() - cy0);
			elapsed_time = get_elapsed_time() - elapsed_time;
		} 
		else if (k_len == 32)
		{
			elapsed_time = get_elapsed_time();
			cy0 = read_tsc();
			for(i = 0; i < sample2; ++i)
			{
				iEncExpandKey256(key,aesData.expanded_key);
				iEnc256_CTR(&aesData);
			}
			av = (double)(read_tsc() - cy0);
			elapsed_time = get_elapsed_time() - elapsed_time;
		}
		//printf("AES_NI_CTR cycles= %f at %s %d\n", av, __FILE__, __LINE__);
	}
	else if(options.crypt_method == AES_NI_CBC_ENC)
	{
		unsigned char local_test_iv[16];
		DEFINE_ROUND_KEYS
		sAesData aesData;
		aesData.in_block = pt;
		aesData.out_block = pt;
		aesData.expanded_key = (unsigned char *)ecx->ks;
		aesData.num_blocks = blocks;
		aesData.iv = local_test_iv;
		memcpy(local_test_iv, viv, 16);

		if(k_len == 16) 
		{
			elapsed_time = get_elapsed_time();
			cy0 = read_tsc();
			for(i = 0; i < sample2; ++i)
			{
				iEncExpandKey128(key,aesData.expanded_key);
				iEnc128_CBC(&aesData);
			}
			av = (double)(read_tsc() - cy0);
			elapsed_time = get_elapsed_time() - elapsed_time;
		} 
		else if (k_len == 24)
		{
			elapsed_time = get_elapsed_time();
			cy0 = read_tsc();
			for(i = 0; i < sample2; ++i)
			{
				iEncExpandKey192(key,aesData.expanded_key);
				iEnc192_CBC(&aesData);
			}
			av = (double)(read_tsc() - cy0);
			elapsed_time = get_elapsed_time() - elapsed_time;
		} 
		else if (k_len == 32)
		{
			elapsed_time = get_elapsed_time();
			cy0 = read_tsc();
			for(i = 0; i < sample2; ++i)
			{
				iEncExpandKey256(key,aesData.expanded_key);
				iEnc256_CBC(&aesData);
			}
			av = (double)(read_tsc() - cy0);
			elapsed_time = get_elapsed_time() - elapsed_time;
		}
	}
	else if(options.crypt_method == AES_NI_CBC_DEC)
	{
		unsigned char local_test_iv[16];
		DEFINE_ROUND_KEYS
		sAesData aesData;
		aesData.in_block = pt;
		aesData.out_block = pt;
		aesData.expanded_key = (unsigned char *)ecx->ks;
		aesData.num_blocks = blocks;
		aesData.iv = local_test_iv;
		memcpy(local_test_iv, viv, 16);

		if(k_len == 16) 
		{
			elapsed_time = get_elapsed_time();
			cy0 = read_tsc();
			for(i = 0; i < sample2; ++i)
			{
				iDecExpandKey128(key,aesData.expanded_key);
				iDec128_CBC(&aesData);
			}
			av = (double)(read_tsc() - cy0);
			elapsed_time = get_elapsed_time() - elapsed_time;
		} 
		else if (k_len == 24)
		{
			elapsed_time = get_elapsed_time();
			cy0 = read_tsc();
			for(i = 0; i < sample2; ++i)
			{
				iDecExpandKey192(key,aesData.expanded_key);
				iDec192_CBC(&aesData);
			}
			av = (double)(read_tsc() - cy0);
			elapsed_time = get_elapsed_time() - elapsed_time;
		} 
		else if (k_len == 32)
		{
			elapsed_time = get_elapsed_time();
			cy0 = read_tsc();
			for(i = 0; i < sample2; ++i)
			{
				iDecExpandKey256(key,aesData.expanded_key);
				iDec256_CBC(&aesData);
			}
			av = (double)(read_tsc() - cy0);
			elapsed_time = get_elapsed_time() - elapsed_time;
		}
	}

	if(cmd != NULL)
	{
		cmd->elapsed_secs = elapsed_time;
		cmd->MB_per_sec = compute_MB_per_sec(elapsed_time, cmd->num_iterations, cmd->blocks);
		cmd->avg_clocks = av;
    	cmd->avg_clocks /= (double)cmd->num_iterations;
    	cmd->avg_clocks /= (double)(cmd->blocks*16);
#ifdef __linux__
		pthread_barrier_wait(cmd->mutex_end);
#else
		InterlockedIncrement(cmd->mutex_end);
		while(*cmd->mutex_end < cmd->num_threads);
#endif
	}

    return av;
#ifdef VALIDATE_IN_TIMING
error1:
    printf("\nCTR Decryption data error in timing");
    exit(1);
error2:
    printf("\nCTR Decryption cbuf error in timing");
    exit(1);
#endif
}

#endif

void ctr_inc(unsigned char x[AES_BLOCK_SIZE])
{
    if(!(++(x[0])))
        if(!(++(x[1])))
            if(!(++(x[2])))
                ++(x[3]);
}

#define BUFLEN  (1000 * AES_BLOCK_SIZE)

void print_time(const char *str)
{
   char outstr[200];
   time_t t;
   struct tm *tmp;
#ifdef __linux__
   char fmt[] = {"%F %R.%S"};
#else
   char fmt[] = {"%Y-%m-%d %H:%M.%S"};
#endif

   t = time(NULL);
   tmp = localtime(&t);
   if (tmp == NULL) {
	   perror("localtime");
	   exit(EXIT_FAILURE);
   }

   if (strftime(outstr, sizeof(outstr), fmt, tmp) == 0) {
	   fprintf(stderr, "strftime returned 0");
	   exit(EXIT_FAILURE);
   }
	printf("%s %s\n", str, outstr);
	return;
}

int my_set_affinity(int use_cpu)
{
	int rc;
#ifdef __linux__

#ifdef __CPU_ISSET
#define MY_CPU_SET   __CPU_SET
#define MY_CPU_ZERO  __CPU_ZERO
#define MY_CPU_ISSET __CPU_ISSET
#else
#define MY_CPU_SET   CPU_SET
#define MY_CPU_ZERO  CPU_ZERO
#define MY_CPU_ISSET CPU_ISSET
#endif

	cpu_set_t currentCPU;

	MY_CPU_ZERO(&currentCPU);
	MY_CPU_SET(use_cpu, &currentCPU);
#ifdef __2_parameter_glibc__ 
	/* stupid glibc only takes 2 parameters */
#ifdef HAVE_GETTID
	rc = sched_setaffinity (gettid(), &currentCPU);
#else
	rc = sched_setaffinity (0, &currentCPU);
#endif

#else 

#ifdef HAVE_GETTID
	rc = sched_setaffinity (gettid(), sizeof(currentCPU), &currentCPU);
#else
	rc = sched_setaffinity (0, sizeof(currentCPU), &currentCPU);
#endif

#endif 

#else
	SetThreadAffinityMask(GetCurrentThread(), (1 << use_cpu) );
	//printf("mask for thread %d is 0x%x\n", cmd->thread_num, 1<<(cmd->thread_num));
#endif
	return 0;
}

DWORD ThreadFunction(ThreadCommand *cmd)
{
	UINT i;
	unsigned long long start, stop;
	char str[54];
	int which_cpu, use_cpu;
	static int first_time=0;

	use_cpu = cmd->thread_num;
	if(options.cpus_used > 0)
	{
		which_cpu = -1;
		for(i=0; i < options.cpus_used; i++)
		{
			if(options.cpu_mask[i] == 1)
			{
				++which_cpu;
				if(which_cpu == cmd->thread_num)
				{
					use_cpu = i;
					break;
				}
			}
		}
	}

	my_set_affinity(use_cpu);
#ifndef __linux__
	_snprintf(str, sizeof(str), "Thread %d done at ", cmd->thread_num);
#endif

#ifndef __linux__
	//SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL );
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST );
	Sleep(10);
#endif
	if(first_time == 0) { printf("Thread %d using cpu= %d\n", cmd->thread_num, use_cpu); }
	time_ctr_crypt2(cmd->k_len, cmd->blocks, ctr_inc, cmd->num_iterations, cmd->num_threads, cmd);
	//print_time(str);
	first_time = 1;
	return 0;
}


int BuildTest(unsigned int k_len, int blocks, cbuf_inc ctr_inc2, int loops, int threads_max)
{
	UINT i;
	int num_threads;
	static int first_time=0;
   
#ifdef __linux__
	num_threads = sysconf(_SC_NPROCESSORS_CONF);
#else
    HANDLE ph;
    DWORD_PTR afp = (DWORD_PTR)NULL;
    DWORD_PTR afs = (DWORD_PTR)NULL;
    ph = GetCurrentProcess();
    if(GetProcessAffinityMask(ph, &afp, &afs))
    {
		if(first_time == 0) { printf("Setting  process affinity mask to system mask= 0x%p\n", afs); }
      	if(!SetProcessAffinityMask(ph, afs))
       	{
       		printf("Couldn't set Process Affinity Mask at %s %d\n\n", __FILE__, __LINE__); 
			exit(2);
       	}
    	GetProcessAffinityMask(ph, &afp, &afs);
		if(first_time == 0) { printf("got process affinity mask = 0x%p\n", afp); }
    }
    else
    {
        printf("Couldn't get Process Affinity Mask\n\n"); 
		exit(2);
    }
	GetSystemInfo(&si);
	num_threads = si.dwNumberOfProcessors;
#endif
	if(threads_max > 0 && num_threads > threads_max)
	{
		num_threads = threads_max;
	}
	if(first_time == 0) { printf("Going to start %d threads\n", num_threads); }
#ifdef __linux__
	if(pthread_barrier_init(&mutex_start, NULL, num_threads) != 0) {
		fprintf(stderr, "pthread_barrier_init failed at %s %d\n", __FILE__, __LINE__);
		perror("got pthread_barrier_init: ");
		exit(2);
	}
	if(pthread_barrier_init(&mutex_end, NULL, num_threads) != 0) {
		fprintf(stderr, "pthread_barrier_init failed at %s %d\n", __FILE__, __LINE__);
		perror("got pthread_barrier_init: ");
		exit(2);
	}
#else
	mutex_start = mutex_end = 0;
#endif

	for (i=0;i<num_threads;i++)
	{
		//cmds[i].data.in_block = (UCHAR *)_aligned_malloc(block_size,cfg->alignment);
		//if (!in_place) cmds[i].data.out_block = (UCHAR *)_aligned_malloc(block_size,cfg->alignment);
		cmds[i].num_iterations = loops;
		cmds[i].k_len = k_len;
		cmds[i].blocks = blocks;
		//cmds[i].ctr_inc2 = ctr_inc2;
		cmds[i].num_threads = num_threads;
		cmds[i].thread_num = i;
		cmds[i].mutex_start = &mutex_start;
		cmds[i].mutex_end = &mutex_end;
	}
	first_time = 1;
	return num_threads;
}

char *get_method_string(int method, int *uses_aesni)
{
	int i;
	static char *unk={"unknown method"};
	static int mthd[] ={ ASM_CBC_ENC, ASM_CBC_DEC, ASM_CTR, AES_NI_CBC_ENC, AES_NI_CBC_DEC, AES_NI_CTR};
	static char *str[]={ "ASM_CBC_ENC", "ASM_CBC_DEC", "ASM_CTR", "AES_NI_CBC_ENC", "AES_NI_CBC_DEC", "AES_NI_CTR"};
	static int requires_aesni[] ={ 0, 0, 0, 1, 1, 1};
	for(i=0; i < sizeof(mthd)/sizeof(mthd[0]); i++)
	{
		if(mthd[i] == method)
		{
			if(uses_aesni != NULL)
			{
				*uses_aesni = requires_aesni[i];
			}
			return str[i];
		}
	}
	return unk;
}

void ExecuteTest(int verbose, int num_threads, double *elapsed_secs, double *min, double *max, double *avg)
{
#ifdef __linux__
	int h[MAX_NUM_THREADS];
	pthread_t tid[MAX_NUM_THREADS];
#else
	HANDLE h[MAX_NUM_THREADS];
#endif
	UINT i;
	double amin = 1e38,amax = 0,sum = 0, MB_per_sec = 0, tmax=0, tmin=1e38, tsum=0;

#ifdef __linux__
	for (i=1;i<num_threads;i++)
	{
		if(pthread_create(&tid[i], NULL, (void *)ThreadFunction, &cmds[i]) != 0)
		{
			fprintf(stderr, "Error on thread creation at %s %d\n", __FILE__, __LINE__);
			exit(1);
		}
	}
#else
	for (i=1;i<num_threads;i++)
	{
		h[i] = CreateThread(NULL,0, (LPTHREAD_START_ROUTINE)ThreadFunction, (void *)&cmds[i], 0, NULL);
	}

	//WaitForMultipleObjects(num_threads,h,TRUE,INFINITE);
#endif
	ThreadFunction(&cmds[0]);

	for (i=0;i<num_threads;i++)
	{
		if (cmds[i].elapsed_secs < tmin) tmin = cmds[i].elapsed_secs;
		if (cmds[i].elapsed_secs > tmax) tmax = cmds[i].elapsed_secs;
		if (cmds[i].avg_clocks < amin) amin = cmds[i].avg_clocks;
		if (cmds[i].avg_clocks > amax) amax = cmds[i].avg_clocks;
		if(verbose > 0) { printf("thread= %d, cycles/byte %f\n", i, cmds[i].avg_clocks); }
		MB_per_sec += compute_MB_per_sec(cmds[i].elapsed_secs, cmds[i].num_iterations, cmds[i].blocks);
		sum += cmds[i].avg_clocks;
		tsum += cmds[i].elapsed_secs;
		//MB_per_sec += cmds[i].MB_per_sec;
	}

	sum /= num_threads;
	tsum /= num_threads;
	MB_per_sec /= num_threads;
	if(verbose > 0)
	{
		printf("num_threads= %d, time= %f, method= %s, MB/sec= %f, cycles/byte min= %f, max= %f, avg= %f at %s %d\n", 
			num_threads, tsum, get_method_string(options.crypt_method, NULL), MB_per_sec, amin, amax, sum, __FILE__, __LINE__);
	}

	*elapsed_secs = tsum;
	*min = amin;
	*max = amax;
	*avg = sum;

}


int main(int argc, char **argv)
{   int     i, k, err, blocks, len, len2;
    double  a0, av, sig, td;
    unsigned char   buf1[BUFLEN];
    unsigned char   buf2[BUFLEN];
    unsigned char   iv1[AES_BLOCK_SIZE];
    unsigned char   iv2[AES_BLOCK_SIZE];
    unsigned char   key[32];
    f_ectx ecx1[1];
    f_dctx dcx1[1];
    aligned_auto(unsigned char, buf3, BUFLEN, 16);
    aligned_auto(unsigned char, iv3, AES_BLOCK_SIZE, 16);
    aligned_auto(f_ectx, ecx2, 1, 16);
    aligned_auto(f_dctx, dcx2, 1, 16);

#if defined( DLL_IMPORT ) && defined( DYNAMIC_LINK )
    HINSTANCE   h_dll;
#endif

	process_options(argc, argv);

#if defined( DUAL_CORE ) && defined( _WIN32 )
	{
		// we need to constrain the process to one core in order to
		// obtain meaningful timing data
		HANDLE ph;
		DWORD_PTR afp = (DWORD_PTR)NULL;
		DWORD_PTR afs = (DWORD_PTR)NULL;


		ph = GetCurrentProcess();
		if(GetProcessAffinityMask(ph, &afp, &afs))
		{
			int i;
			for(i=0; i < sizeof(DWORD_PTR)*8; i++)
			{
				if((afp & (1 << i)) != 0)
				{
					//afp &= (GetCurrentProcessorNumber() + 1);
					afp = (1 << i);
					if(!SetProcessAffinityMask(ph, afp))
					{
						printf("Couldn't set Process Affinity Mask\n\n"); return -1;
					}
				}
			}
		}
		else
		{
			printf("Couldn't get Process Affinity Mask\n\n"); return -1;
		}
    }
#endif

#if defined( DLL_IMPORT ) && defined( DYNAMIC_LINK )
    if(!(h_dll = init_dll(&fn)))
        return -1;
#elif defined(STATIC_TABLES)
    aes_init();
#endif

	if(f_talign(0,16) != EXIT_SUCCESS)
		return -1;

	if(options.skip_tests == 1)
	{
		printf("\nSkipping running initial correctness tests (due to -skip_tests option).\n");
	}
	else
	{
		printf("\nThe tests below don't use AES-NI\n");
    printf("\nRun tests for the AES algorithm");
#if defined( DLL_IMPORT )
    printf(" (DLL Version)");
#endif
#if defined( __cplusplus )
    printf(" (CPP Version)");
#endif

    for(k = 128; k <= 256; k += 64)
    {
        printf("\n\n%03i Bit Keys", k);
#ifdef TEST_ECB
        err = 0;
        for(i = 0; i < 100; ++i)
        {
            block_rndfill(key, 2 * AES_BLOCK_SIZE);
            f_enc_key(ecx1, key, k);
            f_enc_key(ecx2, key, k);
            f_dec_key(dcx1, key, k);
            f_dec_key(dcx2, key, k);

            block_rndfill(buf1, BUFLEN);
            memcpy(buf2, buf1, BUFLEN);
            memcpy(buf3, buf1, BUFLEN);

            td = rand32() / (65536.0 * 65536.0);
            len = (unsigned int)(0.5 * BUFLEN * (1.0 + td));
            len = AES_BLOCK_SIZE * (len / AES_BLOCK_SIZE);

            ECBenc(buf2, len, ecx1);
            f_ecb_enc(ecx2, buf3, buf3, len);

            if(memcmp(buf2, buf3, len)) err |= 1;
            if((err & 1) && !(err & 256))
                printf("\nECB encryption FAILURE");

            ECBdec(buf2, len, dcx1);
            f_ecb_dec(dcx2, buf3, buf3, len);

            if(memcmp(buf1, buf2, len)) err |= 2;
            if(memcmp(buf1, buf3, len)) err |= 4;
            if((err & 4) && !(err & 512))
                printf("\nECB decryption FAILURE");
            if(err & 1)
                err |= 256;
            if(err & 4)
                err |= 512;
        }
        if(!err)
            printf("\nECB encrypt and decrypt of data correct");
#endif

#ifdef TEST_CBC
        err = 0;
        for(i = 0; i < 100; ++i)
        {
            block_rndfill(key, 2 * AES_BLOCK_SIZE);
            f_enc_key(ecx1, key, k);
            f_enc_key(ecx2, key, k);
            f_dec_key(dcx1, key, k);
            f_dec_key(dcx2, key, k);

            block_rndfill(iv1, AES_BLOCK_SIZE);
            memcpy(iv2, iv1, AES_BLOCK_SIZE);
            memcpy(iv3, iv1, AES_BLOCK_SIZE);

            block_rndfill(buf1, BUFLEN);
            memcpy(buf2, buf1, BUFLEN);
            memcpy(buf3, buf1, BUFLEN);

            td = rand32() / (65536.0 * 65536.0);
            len = (unsigned int)(0.5 * BUFLEN * (1.0 + td));
            len = AES_BLOCK_SIZE * (len / AES_BLOCK_SIZE);

            CBCenc(buf2, len, iv2, ecx1);
            f_cbc_enc(ecx2, buf3, buf3, len, iv3);

            if(memcmp(buf2, buf3, len)) err |= 1;
            if(memcmp(iv2, iv3, AES_BLOCK_SIZE)) err |= 2;
            if((err & 1) && !(err & 256))
                printf("\nCBC encryption FAILURE");

            memcpy(iv2, iv1, AES_BLOCK_SIZE);
            memcpy(iv3, iv1, AES_BLOCK_SIZE);
            CBCdec(buf2, len, iv2, dcx1);
            f_cbc_dec(dcx2, buf3, buf3, len, iv3);

            if(memcmp(buf1, buf2, len)) err |= 4;
            if(memcmp(buf1, buf3, len)) err |= 8;
            if(memcmp(buf2, buf3, len)) err |= 16;
            if(memcmp(iv2, iv3, AES_BLOCK_SIZE)) err |= 32;
            if((err & 16) && !(err & 512))
                printf("\nCBC decryption FAILURE");
            if(err & 1)
                err |= 256;
            if(err & 16)
                err |= 512;
        }
        if(!(err & ~(2 | 4 | 16 | 32)))
            printf("\nCBC encrypt and decrypt of data correct");
        if(err & (2 | 32))
        {
            printf(" (mismatch of final IV on ");
            if(err & 2)
                printf("encrypt");
            if((err & (2 | 32)) == 34)
                printf(" and ");
            if(err & 32)
                printf("decrypt");
            printf(")");
        }
#endif

#ifdef TEST_CFB
        err = 0;
        for(i = 0; i < 100; ++i)
        {
            block_rndfill(key, 2 * AES_BLOCK_SIZE);
            f_enc_key(ecx1, key, k);
            f_enc_key(ecx2, key, k);
            f_dec_key(dcx1, key, k);
            f_dec_key(dcx2, key, k);

            block_rndfill(iv1, AES_BLOCK_SIZE);
            memcpy(iv2, iv1, AES_BLOCK_SIZE);
            memcpy(iv3, iv1, AES_BLOCK_SIZE);

            block_rndfill(buf1, BUFLEN);
            memcpy(buf2, buf1, BUFLEN);
            memcpy(buf3, buf1, BUFLEN);

            f_info(ecx1) = 0;
            f_mode_reset(ecx2);
            td = rand32() / (65536.0 * 65536.0);
            len = (unsigned int)(0.5 * BUFLEN * (1.0 + td));
            td = rand32() / (65536.0 * 65536.0);
            len2 = (unsigned int)(td * len);
#ifdef WHOLE_BLOCKS
            len = AES_BLOCK_SIZE * (len / AES_BLOCK_SIZE);
            len2 = AES_BLOCK_SIZE * (len2 / AES_BLOCK_SIZE);
#endif
            f_cfb_enc(ecx2, buf3, buf3, len2, iv3);
            f_cfb_enc(ecx2, buf3 + len2, buf3 + len2, len - len2, iv3);

            CFBenc(buf2, len, iv2, ecx1);
            if(memcmp(buf2, buf3, len)) err |= 1;
            if(memcmp(iv2, iv3, AES_BLOCK_SIZE)) err |= 2;
            if((err & 1) && !(err & 256))
                printf("\nCFB encryption FAILURE");

            memcpy(iv2, iv1, AES_BLOCK_SIZE);
            memcpy(iv3, iv1, AES_BLOCK_SIZE);

            f_info(ecx1) = 0;
            f_mode_reset(ecx2);
            CFBdec(buf2, len, iv2, ecx1);
            td = rand32() / (65536.0 * 65536.0);
            len2 = (unsigned int)(td * len);
#ifdef WHOLE_BLOCKS
            len2 = AES_BLOCK_SIZE * (len2 / AES_BLOCK_SIZE);
#endif
            f_cfb_dec(ecx2, buf3, buf3, len2, iv3);
            f_cfb_dec(ecx2, buf3 + len2, buf3 + len2, len - len2, iv3);

            if(memcmp(buf1, buf2, len)) err |= 4;
            if(memcmp(buf1, buf3, len)) err |= 8;
            if(memcmp(buf2, buf3, len)) err |= 16;
            if(memcmp(iv2, iv3, AES_BLOCK_SIZE)) err |= 32;
            if((err & 16) && !(err & 512))
                printf("\nCFB decryption FAILURE");
            if(err & 1)
                err |= 256;
            if(err & 16)
                err |= 512;
        }
        if(!(err & ~(2 | 4 | 16 | 32)))
            printf("\nCFB encrypt and decrypt of data correct");
        if(err & (2 | 32))
        {
            printf(" (mismatch of final IV on ");
            if(err & 2)
                printf("encrypt");
            if((err & (2 | 32)) == 34)
                printf(" and ");
            if(err & 32)
                printf("decrypt");
            printf(")");
        }
#endif

#ifdef TEST_OFB
        err = 0;
        for(i = 0; i < 100; ++i)
        {
            block_rndfill(key, 2 * AES_BLOCK_SIZE);
            f_enc_key(ecx1, key, k);
            f_enc_key(ecx2, key, k);
            f_dec_key(dcx1, key, k);
            f_dec_key(dcx2, key, k);

            block_rndfill(iv1, AES_BLOCK_SIZE);
            memcpy(iv2, iv1, AES_BLOCK_SIZE);
            memcpy(iv3, iv1, AES_BLOCK_SIZE);

            block_rndfill(buf1, BUFLEN);
            memcpy(buf2, buf1, BUFLEN);
            memcpy(buf3, buf1, BUFLEN);

            f_info(ecx1) = 0;
            f_mode_reset(ecx2);
            td = rand32() / (65536.0 * 65536.0);
            len = (unsigned int)(0.5 * BUFLEN * (1.0 + td));
            td = rand32() / (65536.0 * 65536.0);
            len2 = (unsigned int)(td * len);
#ifdef WHOLE_BLOCKS
            len = AES_BLOCK_SIZE * (len / AES_BLOCK_SIZE);
            len2 = AES_BLOCK_SIZE * (len2 / AES_BLOCK_SIZE);
#endif
            f_ofb_cry(ecx2, buf3, buf3, len2, iv3);
            f_ofb_cry(ecx2, buf3 + len2, buf3 + len2, len - len2, iv3);

            OFBenc(buf2, len, iv2, ecx1);
            if(memcmp(buf2, buf3, len)) err |= 1;
            if(memcmp(iv2, iv3, AES_BLOCK_SIZE)) err |= 2;
            if((err & 1) && !(err & 256))
                printf("\nOFB encryption FAILURE");

            memcpy(iv2, iv1, AES_BLOCK_SIZE);
            memcpy(iv3, iv1, AES_BLOCK_SIZE);

            f_info(ecx1) = 0;
            f_mode_reset(ecx2);
            OFBdec(buf2, len, iv2, ecx1);
            td = rand32() / (65536.0 * 65536.0);
            len2 = (unsigned int)(td * len);
#ifdef WHOLE_BLOCKS
            len2 = AES_BLOCK_SIZE * (len2 / AES_BLOCK_SIZE);
#endif
            f_ofb_cry(ecx2, buf3, buf3, len2, iv3);
            f_ofb_cry(ecx2, buf3 + len2, buf3 + len2, len - len2, iv3);

            if(memcmp(buf1, buf2, len)) err |= 4;
            if(memcmp(buf1, buf3, len)) err |= 8;
            if(memcmp(buf2, buf3, len)) err |= 16;
            if(memcmp(iv2, iv3, AES_BLOCK_SIZE)) err |= 32;
            if((err & 16) && !(err & 512))
                printf("\nOFB decryption FAILURE");
            if(err & 1)
                err |= 256;
            if(err & 16)
                err |= 512;
        }
        if(!(err & ~(2 | 4 | 16 | 32)))
            printf("\nOFB encrypt and decrypt of data correct");
        if(err & (2 | 32))
        {
            printf(" (mismatch of final IV on ");
            if(err & 2)
                printf("encrypt");
            if((err & (2 | 32)) == 34)
                printf(" and ");
            if(err & 32)
                printf("decrypt");
            printf(")");
        }
#endif

#ifdef TEST_CTR
        err = 0;
        for(i = 0; i < 100; ++i)
        {
            block_rndfill(key, 2 * AES_BLOCK_SIZE);
            f_enc_key(ecx1, key, k);
            f_enc_key(ecx2, key, k);
            f_dec_key(dcx1, key, k);
            f_dec_key(dcx2, key, k);

            block_rndfill(iv1, AES_BLOCK_SIZE);
            memcpy(iv2, iv1, AES_BLOCK_SIZE);
            memcpy(iv3, iv1, AES_BLOCK_SIZE);

            block_rndfill(buf1, BUFLEN);
            memcpy(buf2, buf1, BUFLEN);
            memcpy(buf3, buf1, BUFLEN);

            f_info(ecx1) = 0;
            f_mode_reset(ecx2);
            td = rand32() / (65536.0 * 65536.0);
            len = (unsigned int)(0.5 * BUFLEN * (1.0 + td));
            td = rand32() / (65536.0 * 65536.0);
            len2 = (unsigned int)(td * len);
#ifdef WHOLE_BLOCKS
            len = AES_BLOCK_SIZE * (len / AES_BLOCK_SIZE);
            len2 = AES_BLOCK_SIZE * (len2 / AES_BLOCK_SIZE);
#endif
            f_ctr_cry(ecx2, buf3, buf3, len2, iv3, ctr_inc);
            f_ctr_cry(ecx2, buf3 + len2, buf3 + len2, len - len2, iv3, ctr_inc);

            CTRcry(buf2, len, iv2, ctr_inc, ecx1);
            if(memcmp(buf2, buf3, len)) err |= 1;
            if(memcmp(iv2, iv3, AES_BLOCK_SIZE)) err |= 2;
            if((err & 1) && !(err & 256))
                printf("\nCTR encryption FAILURE");

            memcpy(iv2, iv1, AES_BLOCK_SIZE);
            memcpy(iv3, iv1, AES_BLOCK_SIZE);

            f_info(ecx1) = 0;
            f_mode_reset(ecx2);
            td = rand32() / (65536.0 * 65536.0);
            len2 = (unsigned int)(td * len);
            CTRcry(buf2, len, iv2, ctr_inc, ecx1);
#ifdef WHOLE_BLOCKS
            len2 = AES_BLOCK_SIZE * (len2 / AES_BLOCK_SIZE);
#endif
            f_ctr_cry(ecx2, buf3, buf3, len2, iv3, ctr_inc);
            f_ctr_cry(ecx2, buf3 + len2, buf3 + len2, len - len2, iv3, ctr_inc);

            if(memcmp(buf1, buf2, len)) err |= 4;
            if(memcmp(buf1, buf3, len)) err |= 8;
            if(memcmp(buf2, buf3, len)) err |= 16;
            if(memcmp(iv2, iv3, AES_BLOCK_SIZE)) err |= 32;
            if((err & 16) && !(err & 512))
                printf("\nCTR decryption FAILURE");
            if(err & 1)
                err |= 256;
            if(err & 16)
                err |= 512;
        }
        if(!(err & ~(2 | 4 | 16 | 32)))
            printf("\nCTR encrypt and decrypt of data correct");
        if(err & (2 | 32))
        {
            printf(" (mismatch of final IV on ");
            if(err & 2)
                printf("encrypt");
            if((err & (2 | 32)) == 34)
                printf(" and ");
            if(err & 32)
                printf("decrypt");
            printf(")");
        }
#endif
    }
	}

	if(options.skip_timings == 1)
	{
		printf("\nSkip running initial timings (due to -skip_timings option).\n");
	}
	else
	{
#if defined( USE_VIA_ACE_IF_PRESENT )
    if(VIA_ACE_AVAILABLE)
        printf("\n\nAES Timing (Cycles/Byte) with the VIA ACE Engine");
    else
#endif
		printf("\n\nThe timings below don't use AES-NI\n");
    printf("\n\nAES Timing (Cycles/Byte)");
    printf("\nMode   Blocks:      1       10      100     1000");

#ifdef TEST_ECB
    printf("\necb encrypt ");
	//ck_print(__FILE__, __LINE__);
    for(blocks = 1; blocks < 10000; blocks *= 10)
    {
        //time_base(&a0, &sig);
		a0 = 0; sig = 0;
        time_ecb_enc(16, blocks, &av, &sig);
        sig *= 100.0 / av;
        av = (int)(100.0 * (av - a0) / (16.0 * blocks)) / 100.0;
        sig = (int)(10 * sig) / 10.0;
        printf("%9.2f", av);
    }

    printf("\necb decrypt ");
    for(blocks = 1; blocks < 10000; blocks *= 10)
    {
#if 0
        time_base(&a0, &sig);
#else
		a0 = sig = 0;
#endif
        time_ecb_dec(16, blocks, &av, &sig);
        sig *= 100.0 / av;
        av = (int)(100.0 * (av - a0) / (16.0 * blocks)) / 100.0;
        sig = (int)(10 * sig) / 10.0;
        printf("%9.2f", av);
    }
#endif

#ifdef TEST_CBC
    printf("\ncbc encrypt ");
    for(blocks = 1; blocks < 10000; blocks *= 10)
    {
#if 0
        time_base(&a0, &sig);
#else
		a0 = sig = 0;
#endif
        time_cbc_enc(16, blocks, &av, &sig);
        sig *= 100.0 / av;
        av = (int)(100.0 * (av - a0) / (16.0 * blocks)) / 100.0;
        sig = (int)(10 * sig) / 10.0;
        printf("%9.2f", av);
    }

    printf("\ncbc decrypt ");
    for(blocks = 1; blocks < 10000; blocks *= 10)
    {
#if 0
        time_base(&a0, &sig);
#else
		a0 = sig = 0;
#endif
        time_cbc_dec(16, blocks, &av, &sig);
        sig *= 100.0 / av;
        av = (int)(100.0 * (av - a0) / (16.0 * blocks)) / 100.0;
        sig = (int)(10 * sig) / 10.0;
        printf("%9.2f", av);
    }
#endif

#ifdef TEST_CFB
    printf("\ncfb encrypt ");
    for(blocks = 1; blocks < 10000; blocks *= 10)
    {
#if 0
        time_base(&a0, &sig);
#else
		a0 = sig = 0;
#endif
        time_cfb_enc(16, blocks, &av, &sig);
        sig *= 100.0 / av;
        av = (int)(100.0 * (av - a0) / (16.0 * blocks)) / 100.0;
        sig = (int)(10 * sig) / 10.0;
        printf("%9.2f", av);
    }

    printf("\ncfb decrypt ");
    for(blocks = 1; blocks < 10000; blocks *= 10)
    {
#if 0
        time_base(&a0, &sig);
#else
		a0 = sig = 0;
#endif
        time_cfb_dec(16, blocks, &av, &sig);
        sig *= 100.0 / av;
        av = (int)(100.0 * (av - a0) / (16.0 * blocks)) / 100.0;
        sig = (int)(10 * sig) / 10.0;
        printf("%9.2f", av);
    }
#endif

#ifdef TEST_OFB
    printf("\nofb encrypt ");
    for(blocks = 1; blocks < 10000; blocks *= 10)
    {
#if 0
        time_base(&a0, &sig);
#else
		a0 = sig = 0;
#endif
        time_ofb_enc(16, blocks, &av, &sig);
        sig *= 100.0 / av;
        av = (int)(100.0 * (av - a0) / (16.0 * blocks)) / 100.0;
        sig = (int)(10 * sig) / 10.0;
        printf("%9.2f", av);
    }

#endif

#if 1
#ifdef TEST_CTR
    printf("\nctr encrypt ");
    for(blocks = 1; blocks < 10000; blocks *= 10)
    {
#if 0
        time_base(&a0, &sig);
#else
		a0 = sig = 0;
#endif
        time_ctr_crypt(16, blocks, ctr_inc, &av, &sig, 0);
        sig *= 100.0 / av;
        av = (int)(100.0 * (av - a0) / (16.0 * blocks)) / 100.0;
        sig = (int)(10 * sig) / 10.0;
        printf("%9.2f", av);
    }
#if 0
    printf("\nctr encrypt ");
    blocks = 1000;
    {
		double tm_beg, tm_end, cycles_per_byte;
		//int loops= 600000 * 12;
		//int loops= 6000 * 12, threads=0;
		int loops= 10000, threads=0;
		//printf("method= %s ", get_method_string(options.crypt_method, NULL));
		//printf("big loop: loops= %d, blocks= %d\n", loops, blocks);
#if 0
        time_base(&a0, &sig);
#else
		a0 = sig = 0;
#endif
		print_time("Begin time: ");
		tm_beg = get_elapsed_time();
        av = time_ctr_crypt2(16, blocks, ctr_inc, loops, threads, NULL);
		tm_end = get_elapsed_time() - tm_beg;
        cycles_per_byte = av;
        cycles_per_byte /= (double)loops;
        cycles_per_byte /= (double)(blocks*16);
    	printf("\nctr encrypt ");
        printf("%9.2f\n", cycles_per_byte);
		printf("elapsed time= %f seconds\n", tm_end);
		print_time("End   time: ");
    }
#endif


#endif
#endif
	} // if skip_timings == 0

#ifdef TEST_CTR_MT
    printf("\nMulti-threaded test. Performance below does not include key expansion.\n");
	if(options.threads == 0)
	{
		printf("\nYou entered '-threads=0' so we won't run the multi-threaded test\n");
	}
	else
    {
		double tm_beg, tm_end, cycles_per_byte;
		double xmin, xmax, xavg, tave, min_time=0;
		int loops, uses_aesni=0, verbose = 1;
		int num_threads, key_len=128;
    	blocks = 1000;
		//int loops= 600000 * 12;
		loops= 60000 * 12; 
		loops= 10000;
		if(options.loops > 0) { loops = options.loops; }
		if(options.blocks > 0) { blocks = options.blocks; }
		if(options.key_len > 0) { key_len = options.key_len; }
		if(options.min_time > 0) { verbose = 0; min_time = options.min_time; }
		get_method_string(options.crypt_method, &uses_aesni);
		if(uses_aesni == 1)
		{
			printf("Performance below uses AES-NI\n");
		}
		else
		{
			printf("Performance below does not use AES-NI\n");
			if(key_len != 128)
			{
				printf("sorry, for non-AESNI routines, key_len must be 128. Bye at %s %d\n", __FILE__, __LINE__);
				exit(2);
			}
		}
		printf("options: method= %s initial_loops= %d, blocks= %d, key_len= %d bits, min_time= %f\n", 
				get_method_string(options.crypt_method, NULL), loops, blocks, key_len, min_time);
		tave = 0;
		while(tave <= options.min_time)
		{
			num_threads = BuildTest(key_len/8, blocks, ctr_inc, loops, options.threads);
			//print_time("Begin time: ");
			//tm_beg = get_elapsed_time();
			ExecuteTest(verbose, num_threads, &tave, &xmin, &xmax, &xavg);
			if(tave*10.0 >= options.min_time) { verbose = 1; }
			if(tave < options.min_time) { loops *= 2; }
			//tm_end = get_elapsed_time() - tm_beg;
			//print_time("End   time: ");
			//printf("elapsed time= %f seconds\n", tm_end);
		//printf("__summary__\tthreads=\t%d\tbytes=\t%d\tmethod=\t%s\tkey_len=\t%d\tloops=\t%d\tsecs=\t%f\tmin=\t%f\tmax=\t%f\tavg=\t%f\n",
		//		num_threads, blocks*16, get_method_string(options.crypt_method, NULL), key_len, loops, tave, xmin, xmax, xavg);
		}
		printf("__summary__\tthreads=\t%d\tbytes=\t%d\tmethod=\t%s\tkey_len=\t%d\tloops=\t%d\tsecs=\t%f\tmin=\t%f\tmax=\t%f\tavg=\t%f\n",
				num_threads, blocks*16, get_method_string(options.crypt_method, NULL), key_len, loops, tave, xmin, xmax, xavg);
    }



#endif

#if defined( DLL_IMPORT ) && defined( DYNAMIC_LINK )
    if(h_dll) FreeLibrary(h_dll);
#endif
    printf("\n");
    return 0;
}
