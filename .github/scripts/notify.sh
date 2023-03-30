#!/bin/bash

IDENT=$1
STAT=$2
CONTENT=$3
ATTACHMENT=$4
TO="dev-chain@topnetwork.org"
CC="xtest@topnetwork.org"

if [ -f ${ATTACHMENT} ];then
    echo "${CONTENT}" | mailx -s "GITHUB CI (${IDENT}) ${STAT}" -a ${ATTACHMENT} -c ${CC} ${TO}
else
    echo "${CONTENT}" | mailx -s "GITHUB CI (${IDENT}) ${STAT}" -c ${CC} ${TO}
fi