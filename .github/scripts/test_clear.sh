#!/bin/bash
source /etc/profile

while getopts "o:i:d:h" arg;
do
    case $arg in
        o)
            OPT=$OPTARG
            ;;
        i)
            ID=$OPTARG
            ;;
        d)
            DIR=$OPTARG
            ;;
        h)
            echo -e "-o: 申明操作类型，clean or archive\n"
            echo -e "-i: 指定archive操作使用的run number\n"
            echo -e "-d: 指定archive操作使用的work dir\n"
            exit 0
            ;;
        ?)
            echo "unexpect param!"
            exit -1
            ;;
    esac
done

ARC_DIR=/tmp/archive

if [[ ${OPT} == "clean" ]];then
    ps -ef |grep "[.]/xtopchain"|awk '{print $2}'|xargs -I {} bash -c 'kill -9 {}'
    rm -fr /tmp/rec*
elif [[ ${OPT} == "archive" ]];then
    if [[ -z ${ID} ]];then
        ID=$(date --date='0 days ago' "+%m-%d_%H:%M")
    fi
    if [[ ! -d ${DIR} ]];then
        echo "${DIR} not exist"
        exit -1
    fi
    mkdir -p ${ARC_DIR}/${ID}
    ret=$(ls ${DIR}|grep core*|wc -l)
    if [[ ${ret} -gt 0 ]];then
        echo "find crash"
        mv ${DIR}/core* ${ARC_DIR}/${ID}/
        mv ${DIR}/xtopchain ${ARC_DIR}/${ID}/
    fi
    mv -f /tmp/rec1/log  ${ARC_DIR}/${ID}/rec1.log
    mv -f /tmp/rec2/log  ${ARC_DIR}/${ID}/rec2.log
    mv -f /tmp/rec3/log  ${ARC_DIR}/${ID}/rec3.log
    mv -f /tmp/rec4/log  ${ARC_DIR}/${ID}/rec4.log
    mv -f /tmp/rec5/log  ${ARC_DIR}/${ID}/rec5.log
    mv -f /tmp/rec6/log  ${ARC_DIR}/${ID}/rec6.log
    ps -ef |grep "[.]/xtopchain"|awk '{print $2}'|xargs -I {} bash -c 'kill -9 {}'
    rm -fr /tmp/rec*
else
    echo "invalid opt: ${OPT}"
    exit -1
fi