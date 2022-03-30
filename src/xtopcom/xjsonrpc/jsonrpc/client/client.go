package client

import (
	"log"

	"github.com/ethereum/go-ethereum/core/types"
)

type client struct {
	/*cli grpcClient*/
}

//New client
func NewClient() *client {
	//connect grpc
	//TODO

	return &client{}
}

//cal contract
func (c *client) Eth_call(origin, contract, input, value string) (string, string, error) {
	return "call success!", "", nil
}

//get gas Price
func (c *client) Eth_gasPrice() (uint64, error) {
	return 10000, nil
}

//get gas limit
func (c *client) Get_gasLimit() (uint64, error) {
	return 10000, nil
}

//Get from Balance
func (c *client) GetBalance(from string) (uint64, error) {
	return 1000, nil
}

//get block by hash
func (c *client) GetBlockByHash(hash string) (*TopBlock, error) {
	return &TopBlock{}, nil
}

//get block by Number
func (c *client) GetBlockByNumber(num uint64) (*TopBlock, error) {
	return &TopBlock{}, nil
}

//Get Code by contract Address
func (c *client) GetCode(contractAddr string) (string, error) {
	log.Println("contract:", contractAddr)
	return "0x600160008035811a818181146012578301005b601b6001356025565b806000526020", nil
}

//Get address Nonce
func (c *client) GetNonce(addr string) (uint64, error) {
	return 1, nil
}

//Get Transaction By Hash
func (c *client) GetTransactionByHash(hash string) (*TopTransaction, error) {
	return &TopTransaction{}, nil
}

//Get Transaction Receipt by hash
func (c *client) GetTransactionReceipt(hash string) (*TopTransaction, error) {
	return &TopTransaction{}, nil
}

//GetStorageAt
func (c *client) GetStorageAt(addr, hash, tag string) (string, error) {
	return "resp.Storage", nil
}

//get Logs
func (c *client) GetLogs(address string, fromB, toB uint64, topics []string, blockH string) ([]*types.Log, error) {
	var resLogs []*types.Log
	return resLogs, nil
}

//Get Max BlockNumber
func (c *client) GetMaxBlockNumber() (uint64, error) {
	return 100, nil
}

//Returns Keccak-256 (not the standardized SHA3-256) of the given data.
func (c *client) Web3_sha3(data string) (string, error) {
	return "sha3 ok!", nil
}
