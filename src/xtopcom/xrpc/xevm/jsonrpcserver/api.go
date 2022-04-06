package jsonrpcserver

import (
	"errors"
	"fmt"
	"log"
	"xevm/client"
	"xevm/logger"
	"xevm/util"

	"github.com/ethereum/go-ethereum/common"
	"github.com/ethereum/go-ethereum/core/types"
	"github.com/goinggo/mapstructure"
)

//Returns the current chainId.
func (s *Server) eth_chainId() string {
	return util.Uint64ToHexString(s.chainId)
}

//Returns the current network id.
func (s *Server) net_version() string {
	return s.networkId
}

//Returns the current client version.
func (s *Server) web3_clientVersion() string {
	return s.clinetVersion
}

//Returns the number of most recent block.
func (s *Server) Eth_blockNumber() (string, error) {
	blockH, err := s.client.GetMaxBlockNumber()
	if err != nil {
		return "", err
	}
	return util.Uint64ToHexString(blockH), nil
}

//Returns code at a given address.
func (s *Server) Eth_getCode(addr string) (string, error) {
	return s.client.GetCode(addr)
}

//Returns the number of transactions sent from an address.
func (s *Server) Eth_getTransactionCount(addr string) (string, error) {
	nonce, err := s.client.GetNonce(addr)
	if err != nil {
		return "", err
	}
	return util.Uint64ToHexString(nonce), nil
}

//Returns the current price per gas in wei.
func (s *Server) Eth_gasPrice() (string, error) {
	price, err := s.client.Eth_gasPrice()
	if err != nil {
		return "", err
	}
	return util.Uint64ToHexString(price), nil
}

//Returns a list of addresses owned by client.
func (s *Server) Eth_accounts() []common.Address {
	var accounts []common.Address
	return accounts
}

//Returns the balance of the account of given address.
func (s *Server) Eth_getBalance(from string) (string, error) {
	balance, err := s.client.GetBalance(from)
	if err != nil {
		return "", err
	}
	return util.Uint64ToHexString(balance), nil
}

//send signed transaction
func (s *Server) Eth_sendRawTransaction(rawTx string) (string, error) {
	log.Println("Into eth_sendRawTransaction===========", rawTx)

	etx, err := util.DecodeRawTx(rawTx)
	if err != nil {
		return "", err
	}
	//log.Printf("ethtx params:{to:%v,amount:%v,nounce:%v,hash:%v,gas:%v,gasPrice:%v,txType:%v,chainID:%v,tx lenght:%v}\n", etx.To(), etx.Value(), etx.Nonce(), etx.Hash(), etx.Gas(), etx.GasPrice(), etx.Type(), etx.ChainId(), len(etx.Data()))

	//check chainId
	if etx.ChainId().Uint64() != s.chainId {
		return "", fmt.Errorf("Eth_sendRawTransaction Wrong chainId,expect %v,got:%v", s.chainId, etx.ChainId().Int64())
	}

	//verify eth tx
	err = util.VerifyEthSignature(etx)
	if err != nil {
		return "", err
	}

	//convert eth tx to Top tx
	// if C.wrapEthTx(C.CString(rawTx)) != 0 {
	// 	return "", errors.New("sendRawTransaction failed!")
	// }

	return etx.Hash().Hex(), nil
}

//Executes a new message call immediately without creating a transaction on the block chain.
func (s *Server) Eth_call(mp map[string]interface{}) (string, error) {
	Para, err := util.GetParam(mp) //v.([]interface{})
	if err != nil {
		return "", err
	}

	var para params
	if len(Para) > 0 {
		err := mapstructure.Decode(Para[0].(map[string]interface{}), &para)
		if err != nil {
			return "", err
		}
	} else {
		return "", errors.New(fmt.Errorf("Eth_call: Decode Para[%v] failed!", Para).Error())
	}

	ret, _, err := s.client.Eth_call(para.From, para.To, para.Data, util.Check0x(para.Value))
	if err != nil {
		return ret, err
	}
	return ret, nil
}

//Generates and returns an estimate of how much gas is necessary to allow the transaction to complete. The transaction will not be added to the blockchain.
//Note that the estimate may be significantly more than the amount of gas actually used by the transaction, for a variety of reasons including EVM mechanics and node performance.
func (s *Server) Eth_estimateGas(mp map[string]interface{}) (string, error) {
	Para, err := util.GetParam(mp) //v.([]interface{})
	if err != nil {
		return "", err
	}
	var para params
	if len(Para) > 0 {
		err := mapstructure.Decode(Para[0].(map[string]interface{}), &para)
		if err != nil {
			return "", err
		}
	} else {
		return "", errors.New("Eth_estimateGas wrong params!")
	}

	logger.SugarLogger.Infof("Eth_estimateGas params: from=%v,to=%v,gas=%v,gasPrice=%v,value=%v,data lenght=%v\n", para.From, para.To, para.Gas, para.GasPrice, para.Value, len(para.Data))

	limit, err := s.client.Get_gasLimit()
	if err != nil {
		return "", err
	}

	if len(para.Data) <= 0 {
		return fmt.Sprintf("%X", limit), nil
	}

	if len(para.To) <= 0 {
		return fmt.Sprintf("%X", limit), nil
	}

	ret, gas, err := s.client.Eth_call(para.From, para.To, para.Data, util.Check0x(para.Value))
	if err != nil {
		logger.SugarLogger.Errorf("Eth_estimateGas error:%v,ret:%v", err, ret)
		return ret, err
	}

	logger.SugarLogger.Infof("Eth_estimateGas successfully,gas:%v", gas)
	return gas, nil
}

//Top block convert to eth blocks
func (s *Server) TopBlockToEthBlock(tb *client.TopBlock, op bool) (*Block, error) {
	return &Block{}, nil
}

//Returns information about a block by block hash.
func (s *Server) Eth_getBlockByHash(hash string, bl bool) (*Block, error) {
	tb, err := s.client.GetBlockByHash(hash)
	if err != nil {
		return nil, err
	}
	return s.TopBlockToEthBlock(tb, false)
}

//Returns information about a block by block number.
func (s *Server) Eth_getBlockByNumber(strNum string, bl bool) (*Block, error) {
	if strNum == "earliest" || strNum == "pending" {
		return nil, fmt.Errorf("param %v not supported", strNum)
	}
	var num uint64
	if strNum == "latest" {
		n, err := s.client.GetMaxBlockNumber()
		if err != nil {
			return nil, err
		}
		num = n
	} else {
		n, err := util.HexToUint64(strNum)
		if err != nil {
			log.Println("hexToUint64 error:", err)
			return nil, err
		}
		num = n
	}

	tb, err := s.client.GetBlockByNumber(num)
	if err != nil {
		return nil, err
	}
	return s.TopBlockToEthBlock(tb, bl)
}

//convert top tx to eth tx
func TopTxToTransaction(topTx *client.TopTransaction, tb *client.TopBlock) *Transaction {
	return &Transaction{}
}

//convert top tx to receipt
func TopTxToReceipt(topTx *client.TopTransaction, tb *client.TopBlock) *TransactionReceipt {
	return &TransactionReceipt{}
}

//top txs to eth txs
func (s *Server) TopTxsToEthTxs(block *client.TopBlock, txs []*client.TopTransaction) []*TransactionReceipt {
	var etxs []*TransactionReceipt
	for _, ktx := range txs {
		etxs = append(etxs, TopTxToReceipt(ktx, block))
	}
	return etxs
}

//get txs hashes from transactions
func (s *Server) getTxsHashes(txs []*client.TopTransaction) []common.Hash {
	var hashes []common.Hash
	for _, tx := range txs {
		hashes = append(hashes, common.BytesToHash(tx.Hash))
	}
	return hashes
}

//Returns the information about a transaction requested by transaction hash.
func (s *Server) Eth_getTransactionByHash(hash string) (*Transaction, error) {
	tx, err := s.client.GetTransactionByHash(util.Check0x(hash))
	if err != nil {
		logger.SugarLogger.Errorf("Eth_getTransactionByHash txhash:%v, error:%v", hash, err.Error())
		return nil, nil
	}
	b, err := s.client.GetBlockByNumber(tx.BlockHeignt)
	if err != nil {
		logger.SugarLogger.Errorf("Eth_getTransactionByHash GetBlockByNumber error:%v", err.Error())
		return nil, nil
	}

	return TopTxToTransaction(tx, b), nil
}

//Returns the receipt of a transaction by transaction hash.
func (s *Server) Eth_getTransactionReceipt(hash string) (*TransactionReceipt, error) {
	tx, err := s.client.GetTransactionByHash(util.Check0x(hash))
	if err != nil {
		logger.SugarLogger.Errorf("Eth_getTransactionReceipt txhash:%v, error:%v\n", hash, err.Error())
		return nil, nil
	}

	b, err := s.client.GetBlockByNumber(tx.BlockHeignt)
	if err != nil {
		logger.SugarLogger.Errorf("Eth_getTransactionReceipt GetBlockByNumber error:%v", err.Error())
		return nil, nil
	}

	return TopTxToReceipt(tx, b), nil
}

//Returns the value from a storage position at a given address.
func (s *Server) Eth_getStorageAt(mp map[string]interface{}) (string, error) {
	var addr, hash, tag string
	paras, err := util.GetParam(mp) //v.([]interface{})
	if err != nil {
		return "", err
	}

	if len(paras) == 1 {
		addr = paras[0].(string)
	} else if len(paras) == 2 {
		addr = paras[0].(string)
		hash = paras[1].(string)
	} else if len(paras) == 3 {
		addr = paras[0].(string)
		hash = paras[1].(string)
		tag = paras[2].(string)
	} else {
		return "", errors.New("eth_getStorageAt: params are wrong!")
	}
	return s.client.GetStorageAt(addr, hash, tag)
}

//Returns an array of all logs matching a given filter object.
func (s *Server) Eth_getLogs(mp map[string]interface{}) ([]*types.Log, error) {
	Para, err := util.GetParam(mp)
	if err != nil {
		return nil, err
	}
	var para reqGetLog
	if len(Para) > 0 {
		err := mapstructure.Decode(Para[0].(map[string]interface{}), &para)
		if err != nil {
			return nil, err
		}
	} else {
		return nil, errors.New("eth_getLogs: params are wrong!!")
	}

	logger.SugarLogger.Infof("eth_getLogs params: fromBlock=%v,toBlock=%v,address=%v,topics=%v,blockHash = %v\n", para.FromBlock, para.ToBlock, para.Address, para.Topics, para.BlockHash)

	var fromBlock, toBlock uint64
	if len(para.FromBlock) > 0 {
		fb, err := util.HexToUint64(para.FromBlock)
		if err != nil {
			logger.SugarLogger.Errorf("Eth_getLogs fromblock hexToUint64 error:%v", err)
			return nil, err
		}
		fromBlock = fb
	}
	if len(para.ToBlock) > 0 {
		tb, err := util.HexToUint64(para.ToBlock)
		if err != nil {
			logger.SugarLogger.Errorf("Eth_getLogs toblock hexToUint64 error:%v", err)
			return nil, err
		}
		toBlock = tb
	}

	return s.client.GetLogs(para.Address, fromBlock, toBlock, para.Topics, para.BlockHash)
}

func (s *Server) Web3_sha3(mp map[string]interface{}) (string, error) {
	para, err := util.GetParam(mp)
	if err != nil {
		logger.SugarLogger.Errorf("Web3_sha3 GetParam error:%v", err)
		return "", err
	}
	if len(para) != 1 {
		return "", errors.New(fmt.Sprintf("wrong sha3 param: %v", para))
	}

	data := para[0].(string)
	res, err := s.client.Web3_sha3(data)
	if err != nil {
		return "", err
	}
	return res, nil
}
