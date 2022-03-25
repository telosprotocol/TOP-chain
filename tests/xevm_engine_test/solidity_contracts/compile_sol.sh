#! /bin/bash

target_sol_contract=$1

if [ ! -f "$1" ];then
    echo "usage: `bash compile_sol.sh contracts.sol`"
    exit
fi

if [ `solc --version | grep "0.6.4" -c` != 1 ];then
    echo "solc version wrong. use 0.6.4 for now"
    exit
fi

address="0000000000000000000000000000000000000001"
bin_data="$( solc --bin $target_sol_contract | grep -v Interface | grep "$target_sol_contract" -A2 | tail -n 1)"
abi_data="$( solc --abi $target_sol_contract | grep -v Interface | grep "$target_sol_contract" -A2 | tail -n 1)"
if [ ${#abi_data} == 0 -o ${#bin_data} == 0 ] 
then
    echo "length: ${#abi_data}"
    echo "length: ${#bin_data}"
    echo "!!!! compile contract $contract_file failed !!!!"
    exit
else
    short_name=`expr substr $target_sol_contract 1 3`
    res_code="var $short_name = eth.contract($abi_data).at(\"$address\")"
fi

echo "contract object code used in console:"
echo "============${target_sol_contract}============"
echo "${bin_data}"
echo "========================"
echo "${res_code}"
echo "========================"
echo ""
