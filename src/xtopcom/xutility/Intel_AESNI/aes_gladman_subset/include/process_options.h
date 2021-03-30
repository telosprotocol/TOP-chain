/* process_options.h from Intel.
 * Modified by Patrick Fay
 *
Copyright (c) 2010, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, 
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors 
      may be used to endorse or promote products derived from this software 
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ---------------------------------------------------------------------------
 Issue Date: Aug 6, 2010
*/

#ifdef __PROCESS_OPTIONS_C
#define MYEXTERN 
#define MYEXTERN2 extern
#else
#define MYEXTERN extern
#define MYEXTERN2
#endif

MYEXTERN struct {
	char cpu_mask[512];
	double min_time;
	int cpus_used;
	int threads;
	int crypt_method;
	int skip_tests;
	int skip_timings;
	int loops;
	int key_len;
	int blocks;
	int verbose;
} options;

#define AES_NI_CTR     1
#define ASM_CTR        2
#define AES_NI_CBC_ENC 3
#define AES_NI_CBC_DEC 4
#define ASM_CBC_ENC    5
#define ASM_CBC_DEC    6


#if defined(__cplusplus)
extern "C"
{
#endif

MYEXTERN2 int process_options(int argc, char **argv);

#if defined(__cplusplus)
}
#endif

