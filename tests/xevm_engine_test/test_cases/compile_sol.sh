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

address="0000000000000000000000000000000000000300"
bin_data="$( solc --bin $target_sol_contract | grep -v Interface | grep "$target_sol_contract" -A2 | tail -n 1)"
bin_runtime_data="$( solc --bin-runtime $target_sol_contract | grep -v Interface | grep "$target_sol_contract" -A2 | tail -n 1)"
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
echo "runtime-code"
echo "${bin_runtime_data}"
echo "========================"
echo "handy deploy code:"
echo "========================"
echo "var contractAbi = web3.eth.contract($abi_data);"
echo "var contractBin = '0x${bin_data}';"
echo "var gasValue = eth.estimateGas({data:contractBin});"
echo "var contract = contractAbi.new(
{
    from: web3.eth.accounts[0],
    data: contractBin,
    gas: gasValue
    }, function (e, contract){
    console.log(e, contract);
    if (typeof contract.address !== 'undefined') {
        console.log('Contract mined! address: ' + contract.address + '   transactionHash: ' + contract.transactionHash);
    }
})"
echo "========================"
echo "eth.sendTransaction({from:eth.accounts[0],to:"xxx",gas:100000,data:"DDD"})"