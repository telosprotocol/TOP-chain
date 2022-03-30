package client

import "github.com/ethereum/go-ethereum/core/types"

type Client interface {
	//cal contract
	Eth_call(origin, contract, input, value string) (string, string, error)

	//get gas price
	Eth_gasPrice() (uint64, error)

	//get gas limit
	Get_gasLimit() (uint64, error)

	//Get from Balance
	GetBalance(from string) (uint64, error)

	//get block by hash
	GetBlockByHash(hash string) (*TopBlock, error)

	//get block by Number
	GetBlockByNumber(num uint64) (*TopBlock, error)

	//Get Code by contract Address
	GetCode(contractAddr string) (string, error)

	//Get address Nonce
	GetNonce(addr string) (uint64, error)

	//Get Transaction By Hash
	GetTransactionByHash(hash string) (*TopTransaction, error)

	//Get Transaction Receipt by hash
	GetTransactionReceipt(hash string) (*TopTransaction, error)
	//GetStorageAt
	GetStorageAt(addr, hash, tag string) (string, error)

	//get Logs
	GetLogs(address string, fromB, toB uint64, topics []string, blockH string) ([]*types.Log, error)

	//Get Max BlockNumber
	GetMaxBlockNumber() (uint64, error)

	//Returns Keccak-256 (not the standardized SHA3-256) of the given data.
	Web3_sha3(data string) (string, error)
}
