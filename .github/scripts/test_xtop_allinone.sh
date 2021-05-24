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
    exit -1
fi
if [ ! -f ${topio} ];then
    echo ${topio}" no exist!"
    exit -1
fi
if [ ! -f ${solib} ];then
    echo ${solib}" no exist!"
    exit -1
fi
if [ ! -d ${workdir} ];then
    echo ${workdir}" no exist!"
    exit -1
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
echo "====== wait genesis ======"
echo "sleep 360s"
sleep 360
echo "====== check genesis ======"
ret=$(grep -a 'vnode mgr' /tmp/rec*/log/xtop*log|grep -a consensus|grep -a 'starts at'|wc -l)
if [[ -z ${ret} ]];then
    echo "consensus start log not match"
    sh ${clear} -o archive -i ${NUM} -d ${workdir}
    exit -1
fi
echo "hit log, genesis success"
echo "====== test tx ======"
login_info=$(./topio wallet setDefaultAccount T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC)
login_ret=$(echo "${login_info}"|grep "successfully"|wc -l)
if [[ ${login_ret} -eq 1 ]];then
    echo "set default account success"
else
    echo "set default account fail, see follow output:"
    echo "${login_info}"
    sh ${clear} -o archive -i ${NUM} -d ${workdir}
    exit -1
fi
sleep 1
accounts_info=$(./topio wallet listaccounts)
balance=$(echo "${accounts_info}"|grep T00000Lhj29VReFAT958ZqFWZ2ZdMLot2PS5D5YC -A 3|grep balance)
if [[ ${balance} != "balance: 2999997000.000000 TOP" ]];then
    echo "check god balance fail, see follow output:"
    echo "${accounts_info}"
    sh ${clear} -o archive -i ${NUM} -d ${workdir}
    exit -1
fi
create_info=$(./topio wallet createaccount)
addr=$(echo "${create_info}" | grep -a "Account Address:"|awk -F ':' '{print $2}')
if [[ -z ${addr} ]];then
    echo "create account fail, see follow output:"
    echo "${create_info}"
    sh ${clear} -o archive -i ${NUM} -d ${workdir}
    exit -1
fi
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
echo "sleep 60s"
sleep 60
tx_ret=$(./topio querytx ${tx})
tx_stat=$(echo "${tx_ret}" | grep -a '\"exec_status\"'|grep "success"|wc -l)
address_ret=$(./topio chain queryaccount ${addr})
address_balance=$(echo "${address_ret}" | grep -a '\"balance\"'|grep "123456000000"|wc -l)
if [[ ${tx_stat} -eq 1 ]] && [[ ${address_balance} -eq 1 ]];then
    echo "====== tx check success, end ======"
    sh ${clear} -o clean
else
    echo "tx check fail, see follow output:"
    echo "query_tx_ret: "
    echo "${tx_ret}"
    echo "query_address_ret: 
    echo "${address_balance}"
    sh ${clear} -o archive -i ${NUM} -d ${workdir}
    echo "====== tx fail, end ======"
    exit -1
fi



