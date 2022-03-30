package jsonrpcserver

import (
	"jsonrpcdemo/jsonrpc/client"

	"github.com/ethereum/go-ethereum/common"
	"github.com/ethereum/go-ethereum/core/types"
)

// Server struct
type Server struct {
	client        client.Client
	chainId       uint64
	networkId     string
	archivePoint  string
	clinetVersion string
}

type params struct {
	From     string `json:"from"`
	To       string `json:"to"`
	Gas      string `json:"gas"`
	GasPrice string `json:"gasPrice"`
	Value    string `json:"value"`
	Data     string `json:"data"`
}

type responseBody struct {
	JsonRPC string      `json:"jsonrpc"`
	Id      interface{} `json:"id"`
	Result  interface{} `json:"result"`
}

type ErrorBody struct {
	Code    int    `json:"code"`
	Message string `json:"message"`
	Data    string `json:"data"`
}

type Transaction struct {
	BlockHash        common.Hash    `json:"blockHash"`
	BlockNumber      string         `json:"blockNumber"`
	From             common.Address `json:"from"`
	Gas              string         `json:"gas"`
	GasPrice         string         `json:"gasPrice"`
	Hash             common.Hash    `json:"hash"`
	Input            string         `json:"input"`
	Nonce            string         `json:"nonce"`
	To               common.Address `json:"to"`
	TransactionIndex string         `json:"transactionIndex"`
	Value            string         `json:"value"`
	V                string         `json:"v"`
	R                common.Hash    `json:"r"`
	S                common.Hash    `json:"S"`
}

type responseTransaction struct {
	JsonRPC string       `json:"jsonrpc"`
	Id      interface{}  `json:"id"`
	Result  *Transaction `json:"result"`
}

type TransactionReceipt struct {
	BlockHash         common.Hash    `json:"blockHash"`
	BlockNumber       string         `json:"blockNumber"`
	ContractAddress   common.Address `json:"contractAddress"`
	CumulativeGasUsed string         `json:"cumulativeGasUsed"`
	From              common.Address `json:"from"`
	GasUsed           string         `json:"gasUsed"`
	Logs              []*types.Log   `json:"logs"`
	LogsBloom         types.Bloom    `json:"logsBloom"`
	Status            string         `json:"status"`
	To                common.Address `json:"to"`

	TransactionHash  common.Hash `json:"transactionHash"`
	TransactionIndex string      `json:"transactionIndex"`

	Root common.Hash `json:"root"`
}

type responseReceipt struct {
	JsonRPC string              `json:"jsonrpc"`
	Id      interface{}         `json:"id"`
	Result  *TransactionReceipt `json:"result"`
}

type Block struct {
	Number       string         `json:"number"`
	Hash         common.Hash    `json:"hash"`
	ParentHash   common.Hash    `json:"parentHash"`
	Nonce        string         `json:"nonce"`
	LogsBloom    string         `json:"logsBloom"`
	Miner        common.Address `json:"miner"`
	Difficulty   string         `json:"difficulty"`
	GasLimit     string         `json:"gasLimit"`
	GasUsed      string         `json:"gasUsed"`
	ExtraData    string         `json:"extraData"`
	Size         string         `json:"size"`
	TimeStamp    string         `json:"timestamp"`
	Transactions interface{}    `json:"transactions"`
	Uncles       []common.Hash  `json:"uncles"`
}

type responseBlock struct {
	JsonRPC string      `json:"jsonrpc"`
	Id      interface{} `json:"id"`
	Result  *Block      `json:"result"`
}

type reqGetLog struct {
	FromBlock string   `json:"fromBlock"`
	ToBlock   string   `json:"toBlock"`
	Address   string   `json:"address"`
	Topics    []string `json:"topics"`
	BlockHash string   `json:"blockhash"`
}

var (
	ETH_CHAINID               string = "eth_chainId"
	NET_VERSION               string = "net_version"
	NET_LISTENING             string = "net_listening"
	ETH_SENDTRANSACTION       string = "eth_sendTransaction"
	ETH_CALL                  string = "eth_call"
	ETH_BLOCKNUMBER           string = "eth_blockNumber"
	ETH_GETBALANCE            string = "eth_getBalance"
	ETH_GETBLOCKBYHASH        string = "eth_getBlockByHash"
	ETH_GETBLOCKBYNUMBER      string = "eth_getBlockByNumber"
	ETH_GETTRANSACTIONBYHASH  string = "eth_getTransactionByHash"
	ETH_GASPRICE              string = "eth_gasPrice"
	EHT_GETCODE               string = "eth_getCode"
	ETH_GETTRANSACTIONCOUNT   string = "eth_getTransactionCount"
	ETH_ESTIMATEGAS           string = "eth_estimateGas"
	ETH_SENDRAWTRANSACTION    string = "eth_sendRawTransaction"
	ETH_GETTRANSACTIONRECEIPT string = "eth_getTransactionReceipt"
	ETH_GETLOGS               string = "eth_getLogs"
	ETH_GETSTORAGEAT          string = "eth_getStorageAt"
	ETH_SIGNTRANSACTION       string = "eth_signTransaction"
	ETH_ACCOUNTS              string = "eth_accounts"
	PERSONAL_UNLOCKACCOUNT    string = "personal_unlockAccount"

	WEB3_CLIENTVERSION string = "web3_clientVersion"

	WEB3_SHA3 string = "web3_sha3"
)

var (
	JsonMarshalErr   int = -4001
	JsonUnmarshalErr int = -4002
	ParameterErr     int = -4003
	IoutilErr        int = -4004
	UnkonwnErr       int = -4005
	CALLERR          int = -4006
)

const (
	HEX0 = "0x0"
	HEX1 = "0x1"
)
