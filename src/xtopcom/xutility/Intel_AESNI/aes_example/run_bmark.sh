#!/bin/bash


#bin/aes_example86 -bmark_key_size=128 -bmark_crypt=USE_iAES_CTR -bmark_loops=1000000 -bmark_file_size=3200

nice -n -19 bin/aes_example64 -bmark_key_size=128 -bmark_crypt=USE_iAES_CTR -bmark_loops=3600000 -bmark_file_size=16000

#bin/aes_example64 -bmark_key_size=128 -bmark_crypt=USE_iAES_CBC -bmark_loops=10000000 -bmark_file_size=16
