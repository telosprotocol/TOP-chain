/* process_options.c from Intel.
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

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "my_getopt.h"

#ifdef __linux__
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

#define __PROCESS_OPTIONS_C

#include "process_options.h"


int process_options(int argc, char **argv)
{
    int c;
    int digit_optind = 0;
	int want_aesni=0, got_cpu_mask=0;

	memset(options.cpu_mask, 0, sizeof(options.cpu_mask));
	options.threads = -1; // all the cpus
	if(check_for_aes_instructions() == 1)
	{
		printf("This cpu supports AES-NI\n");
		options.crypt_method = AES_NI_CTR;
		want_aesni = 1;
	}
	else
	{
		printf("This cpu does not support AES-NI\n");
		options.crypt_method = ASM_CTR;
		want_aesni = 0;
	}
	options.loops = 10000;
	options.blocks = 1000;
	options.verbose = 0; // not verbose
	options.cpus_used = 0; // mask is empty
	options.skip_tests = 0;
	options.skip_timings = 0;
	options.key_len = 0;
	options.min_time = 0;

	while (1) {
        int this_option_optind = my_optind ? my_optind : 1;
        int option_index = 0;
        struct option long_options[] = {
            {"threads", 1, 0, 0, "-threads=number_of_threads   enter the number of threads to use in the parallel.\n"
				"   section of the code.\n"
				"   The default is to start one thread per cpu.\n"
				"   If you enter a number N > 0 and < total number of cpus then the 1st N logical cpus will be used.\n"
				"   If you enter a 0 then the parallel section will not be run.\n"
				"   You can override which cpus are used with the -cpu_mask option below.\n"
				"   Note that for cpus with more than 1 logical cpu per core, you may or may not get much speedup.\n"
				"   For example, on Westmere with HT on, AES_NI_CBC_ENC mode gets almost perfect speedup using HT\n"
				"   whereas the other modes get very little (if any) speedup due to HT (since just 1 logical cpu"
				"   is enough to keep the pipeline full).\n"
			},
            {"cpu_mask", 1, 0, 0, "-cpu_mask=0xhexnum    mask to use to indicate on which cpus to run.\n"
				"   If you enter -threads=1 or if you omit the -threads option then you \n"
				"   don't need the -cpu_mask option. If you enter -cpu_mask it overrides the -thread option\n"
				"   The right-most bit of the mask is the OS cpu 0. Example mask: -cpu_mask=0x0f means\n"
				"   use the first 4 cpus.\n"
				"   You can also enter a binary mask. If you start the mask with '0b' then it will be interpreted\n"
				"   as a binary string:  0x1101.\n"
			},
            {"crypt_method",     1, 0, 0, "-crypt_method=method where method can be:\n"
				"   ASM_CBC_ENC, ASM_CBC_DEC, ASM_CTR, AES_NI_CBC_ENC, AES_NI_CBC_DEC or AES_NI_CTR.\n"
				"   This option lets you specify the work that gets done in the parallel section.\n"
				"      ASM_CBC_ENC   means use non-AES-NI assembly code CBC mode to encrypt.\n"
				"      ASM_CBC_DEC   means use non-AES-NI assembly code CBC mode to decrypt.\n"
				"      AES_NI_CBC_ENC  means use AES-NI assembly code in the AES_NI library for CBC mode to encrypt\n"
				"      AES_NI_CBC_DEC  means use AES-NI assembly code in the AES_NI library for CBC mode to decrypt\n"
				"      ASM_CTR   means use non-AES-NI assembly code CTR mode.\n"
				"      AES_NI_CTR  means use AES-NI assembly code in the AES_NI library for CTR mode\n"
				"   The ASM_* options let you use the non-AESNI assembly code in the parallel section.\n"
				"   The AES_NI_* options let you use the AESNI assembly code in the parallel section.\n"
				"   The default is AES_NI_CTR on AES-NI cpus and ASM_CTR on other cpus.\n"
			},
            {"loops",     1, 0, 0, "-loops=number_of_loops   The number of loops to use in the parallel section.\n"
				"   Larger numbers make the parallel section run longer.\n"
				"   The default is 10,000\n"
			},
            {"min_time",  1, 0, 0, "-min_time=minimum_run_time  The min amount of time a measurement should take in seconds.\n"
				"   Larger numbers make the parallel section run longer.\n"
				"   You can use this parameter to automatically increment the number of loops (by 10x each attempt)\n"
				"   until the measurement takes at least 'min_time' seconds.\n"
				"   The default is 0 (which means just use the number of loops to regulate the time).\n"
			},
            {"key_len",     1, 0, 0, "-key_len=size_of_key   The size of the key in bits. Must be 128, 192 or 256\n"
				"   The default is 128\n"
			},
            {"blocks",     1, 0, 0, "-blocks=number_of_blocks   The number of blocks to process in the parallel section.\n"
				"   Each block is 16 bytes. The maximum is 10,000 blocks.\n"
				"   More blocks takes longer to process than fewer blocks.\n"
				"   The default is 1,000\n"
			},
            {"skip_tests",     0, 0, 0, "-skip_tests   Don't run the initial correctness tests.\n"
				"   The default is to run the tests.\n"
			},
            {"skip_timings",     0, 0, 0, "-skip_timings   Don't run the initial timings.\n"
				"   The default is to run the initial timings.\n"
			},
            {"verbose", 0, 0, 'v', "-v or -verbose for verbose mode. Default is not verbose."},
            {0, 0, 0, 0}
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
			if(strcmp(long_options[option_index].name, "threads") == 0)
			{
				if(got_cpu_mask == 0)
				{
					options.threads = atoi(my_optarg);
				}
				else
				{
					printf("Option -cpu_mask entered and it overrides -threads option. '-threads=%s' ignored at %s %d\n", 
							my_optarg, __FILE__, __LINE__);
				}
			}
			else if(strcmp(long_options[option_index].name, "cpu_mask") == 0)
			{
				unsigned int len, i, val, mode;
				char mask[300];
				int used = -1;
				char *cp;
				len = strlen(my_optarg);
				if(len >= sizeof(mask)) { len = sizeof(mask)-1; }
				strncpy(mask, my_optarg, len);
				mode = 1; // hex is the default
				options.threads = 0;
				if(memcmp(mask, "0b", 2) == 0)
				{
					// string is a binary string of 0's and 1's
					mode = 2; // binary string
					mask[1] = '0';
					for(i=0; i < len; i++)
					{
						if(mask[len-i-1] == '1') 
						{ 
							options.cpu_mask[i] = 1; 
							used = i+1; 
							++options.threads;
						}
					}
				}
				else
				{
					if(memcmp(mask, "0x", 2) == 0) { mask[1] = '0'; }
					for(i=0; i < len; i++)
					{
						unsigned int hex;
						char str[10];
						str[0] = mask[len-i-1];
						str[1] = 0;
						sscanf(str, "%x", &hex);
						if((hex & 1) != 0) { options.cpu_mask[i*4+0] = 1; used = i*4+1; ++options.threads;}
						if((hex & 2) != 0) { options.cpu_mask[i*4+1] = 1; used = i*4+2; ++options.threads;}
						if((hex & 4) != 0) { options.cpu_mask[i*4+2] = 1; used = i*4+3; ++options.threads;}
						if((hex & 8) != 0) { options.cpu_mask[i*4+3] = 1; used = i*4+4; ++options.threads;}
					}
				}
				options.cpus_used = used;
				got_cpu_mask = 1; 
			}
			else if(strcmp(long_options[option_index].name, "crypt_method") == 0)
			{
				if(strstr(my_optarg, "ASM_CBC_ENC") != NULL)
				{
					options.crypt_method = ASM_CBC_ENC;
					want_aesni = 0;
				}
				else if(strstr(my_optarg, "ASM_CBC_DEC") != NULL)
				{
					options.crypt_method = ASM_CBC_DEC;
					want_aesni = 0;
				}
				else if(strstr(my_optarg, "AES_NI_CBC_ENC") != NULL)
				{
					options.crypt_method = AES_NI_CBC_ENC;
					want_aesni = 1;
				}
				else if(strstr(my_optarg, "AES_NI_CBC_DEC") != NULL)
				{
					options.crypt_method = AES_NI_CBC_DEC;
					want_aesni = 1;
				}
				else if(strstr(my_optarg, "ASM_CTR") != NULL)
				{
					options.crypt_method = ASM_CTR;
					want_aesni = 0;
				}
				else if(strstr(my_optarg, "AES_NI_CTR") != NULL)
				{
					options.crypt_method = AES_NI_CTR;
					want_aesni = 1;
				}
			}
			else if(strcmp(long_options[option_index].name, "skip_tests") == 0)
			{
				options.skip_tests = 1;
			}
			else if(strcmp(long_options[option_index].name, "skip_timings") == 0)
			{
				options.skip_timings = 1;
			}
			else if(strcmp(long_options[option_index].name, "loops") == 0)
			{
				options.loops = atoi(my_optarg);
			}
			else if(strcmp(long_options[option_index].name, "key_len") == 0)
			{
				options.key_len = atoi(my_optarg);
			}
			else if(strcmp(long_options[option_index].name, "min_time") == 0)
			{
				options.min_time = atof(my_optarg);
			}
			else if(strcmp(long_options[option_index].name, "blocks") == 0)
			{
				options.blocks = atoi(my_optarg);
				if(options.blocks > 10000)
				{
					printf("max blocks is > 10000. Got -blocks=%d. Bye at %s %d\n", options.blocks, __FILE__, __LINE__);
					exit(2);
				}
			}
			else if(strcmp(long_options[option_index].name, "verbose") == 0)
			{
            	options.verbose++;
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
            options.verbose++;
            printf ("verbose set to %d\n", options.verbose);
            break;

       case 'h':
			printf("usage:\n");
			{
				int i;
				for(i=0; i < sizeof(long_options)/sizeof(long_options[0]); i++)
				{
					if(long_options[i].desc != NULL) { printf("%s\n", long_options[i].desc); }
				}
			}
			exit(2);
            break;

       case '?':
			printf ("?? getopt returned character code 0%o ??\n", c);
			printf("usage:\n");
			{
				int i;
				for(i=0; i < sizeof(long_options)/sizeof(long_options[0]); i++)
				{
					if(long_options[i].desc != NULL) { printf("%s\n", long_options[i].desc); }
				}
			}
			exit(2);
            break;

		default:
			printf ("?? getopt returned character code 0%o ??\n", c);
			exit(2);
        }
	}

	if (my_optind < argc) {
        printf ("non-option ARGV-elements: ");
        while (my_optind < argc)
            printf ("%s ", argv[my_optind++]);
        printf ("\n");
		exit(2);
    }
	if(want_aesni == 1 && check_for_aes_instructions() == 0)
	{
		printf("you requested an AES-NI method but this cpu doesn't support AES-NI.\n"
			   "Use -crypt_method option to change the mthod or -h option to see help info.\n"
			   "Bye at %s %d\n", __FILE__, __LINE__);
		exit(2);
	}
	return 0;
}


