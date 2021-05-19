#!/bin/bash
source /etc/profile

NUM=$1
echo "run number: "${NUM}

xtop=cbuild/bin/Linux/xtopchain
topio=cbuild/bin/Linux/topio
solib=cbuild/lib/Linux/libxtopchain.so.*

cpwd=$(pwd)
clear=${cpwd}/.github/scripts/test_clear.sh
workdir=${cpwd}/scripts/deploy_allinone

if [ ! -f ${xtop} ];then
    echo ${xtop}" no exist!"
fi
if [ ! -f ${topio} ];then
    echo ${topio}" no exist!"
fi
if [ ! -f ${solib} ];then
    echo ${solib}" no exist!"
fi
if [ ! -d ${workdir} ];then
    echo ${workdir}" no exist!"
fi

rm -f ${workdir}/xtopchain ${workdir}/topio ${workdir}/libxtopchain.so
cp ${xtop} ${workdir}/
cp ${topio} ${workdir}/
cp ${solib} ${workdir}/libxtopchain.so

sh ${clear} -o clean

cd ${workdir}
export TOPIO_HOME=${workdir}
echo "====== deploy start ======"
sh run.sh
echo "====== deploy end ======"
echo "====== wait genesis 300s ======"
sleep 300
echo "====== check genesis ======"
ret=$(grep -a 'vnode mgr' /tmp/rec*/log/xtop*log|grep -a consensus|grep -a 'starts at'|wc -l)
if [[ -z ${ret} ]];then
    echo "consensus start log not match"
    sh ${clear} -o archive -i ${NUM} -d ${workdir}
    exit -1
fi
echo "hit log: "${ret}
echo "====== test tx ======"
./topio wallet setDefaultAccount T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC
sleep 1
accounts_info=$(./topio wallet listaccounts)
balance=$(echo "${accounts_info}"|grep T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC -A 3|grep balance)
if [[ ${balance} != "balance: 2999997000.000000 TOP" ]];then
    echo "check god balance fail, see follow output:"
    echo "${accounts_info}"
    sh ${clear} -o archive -i ${NUM} -d ${workdir}
    exit -1
fi
addr=$(./topio wallet createaccount | grep -a "T00000"|awk -F ':' '{print $2}')
echo "new local addr: "${addr}
tx_info=$(./topio transfer ${addr} 123456 tx_test)
tx=$(echo "${tx_info}" | grep -a "Transaction hash"|awk -F ':' '{print $2}')
if [[ -z ${tx} ]];then
    echo "tx fail, see follow output:"
    echo "${tx_info}"
    sh ${clear} -o archive -i ${NUM} -d ${workdir}
    exit -1
fi
echo "tx: "${tx}
sleep 60
tx_ret=$(./topio querytx ${tx})
stat=$(echo "${tx_ret}" | grep -a '\"exec_status\"'|grep "success"|wc -l)
if [[ ${stat} -eq 1 ]];then
    echo "====== tx success, end ======"
    sh ${clear} -o clean
else
    echo "tx check fail, see follow output:"
    echo "${tx_ret}"
    sh ${clear} -o archive -i ${NUM} -d ${workdir}
    echo "====== tx fail, end ======"
    exit -1
fi



