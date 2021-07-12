#!/bin/bash

STAT=$1
CONTENT=$2
ATTACHMENT="./ut_report/report.tar.gz"
TO="dev-chain@topnetwork.org"
CC="xtest@topnetwork.org"
# TO="aries.zhang@topnetwork.org"
# CC="helen.huang@topnetwork.org"

if [ -f ${ATTACHMENT} ];then
    echo "${CONTENT}" | mailx -s "GITHUB CI ${STAT}" -a ${ATTACHMENT} -c ${CC} ${TO}
else
    echo "${CONTENT}" | mailx -s "GITHUB CI ${STAT}" -c ${CC} ${TO}
fi