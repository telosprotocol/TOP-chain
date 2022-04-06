package jsonrpcserver

import (
	"encoding/binary"
	"encoding/json"
	"strconv"
	"xevm/client"
	"xevm/logger"
	"xevm/util"

	"fmt"
	"io/ioutil"

	"net/http"

	"github.com/ethereum/go-ethereum/common"
)

//New Server
func NewJsonRpcServer(chainId, networkId, archivePoint, clinetVersion string) *Server {
	cid, err := strconv.ParseUint(chainId[2:], 16, 32)
	if err != nil {
		logger.SugarLogger.Errorf("NewJsonRpcServer chainId error:%v", err)
		panic(err)
	}
	return &Server{client: client.NewClient(), chainId: cid, networkId: networkId, archivePoint: archivePoint, clinetVersion: clinetVersion}
}

//Handle Request
func (s *Server) HandRequest(w http.ResponseWriter, req *http.Request) {
	defer req.Body.Close()
	w.Header().Set("Access-Control-Allow-Origin", "*")
	w.Header().Add("Access-Control-Allow-Headers", "Content-Type")
	w.Header().Set("content-type", "application/json")

	var REST []byte
	defer func() {
		w.Write(REST)
	}()

	defer func() {
		if err := recover(); err != nil {
			logger.SugarLogger.Errorf("HandRequest Error:%v", err)
			REST = util.ResponseErrFunc(ParameterErr, "2.0", 0, err.(error).Error())
		}
	}()

	body, err := ioutil.ReadAll(req.Body)
	if err != nil {
		logger.SugarLogger.Errorf("ioutil ReadAll error:%v", err)
		REST = util.ResponseErrFunc(IoutilErr, "", 0, err.Error())
		return
	}

	reqData := make(map[string]interface{})
	if err := json.Unmarshal(body, &reqData); err != nil {
		REST = util.ResponseErrFunc(JsonUnmarshalErr, "", 0, err.Error())
		return
	}

	method, err := util.GetString(reqData, "method")
	if err != nil {
		logger.SugarLogger.Errorf("get method error:%v", err)
		REST = util.ResponseErrFunc(ParameterErr, "", 0, err.Error())
		return
	}

	jsonrpc, err := util.GetString(reqData, "jsonrpc")
	if err != nil {
		logger.SugarLogger.Errorf("get jsonrpc error:%v", err)
		REST = util.ResponseErrFunc(ParameterErr, "", 0, err.Error())
		return
	}

	id, err := util.GetValue(reqData, "id")
	if err != nil {
		logger.SugarLogger.Errorf("getValue error:%v", err)
		REST = util.ResponseErrFunc(ParameterErr, jsonrpc, 0, err.Error())
		return
	}

	switch method {
	case ETH_CHAINID:
		resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: s.eth_chainId()})
		if err != nil {
			logger.SugarLogger.Errorf("eth_chainId Marshal error:%v", err)
			REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
		} else {
			logger.SugarLogger.Infof("eth_chainId success:%v", s.eth_chainId())
			REST = resp
		}
	case NET_VERSION:
		resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: s.net_version()})
		if err != nil {
			logger.SugarLogger.Errorf("net_version Marshal error:%v", err)
			REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
		} else {
			logger.SugarLogger.Infof("net_version success %v", s.net_version())
			REST = resp
		}
	case WEB3_CLIENTVERSION:
		resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: s.web3_clientVersion()})
		if err != nil {
			logger.SugarLogger.Errorf("web3_clientVersion Marshal error:%v", err)
			REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
		} else {
			logger.SugarLogger.Infof("web3_clientVersion success:%v", s.web3_clientVersion())
			REST = resp
		}
	case ETH_SENDRAWTRANSACTION:
		para, err := util.GetParam(reqData)
		if err != nil {
			logger.SugarLogger.Errorf("GetParam error:%v", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error()+fmt.Sprintf("para number:%v", len(para)))
		} else {
			if len(para) != 1 {
				logger.SugarLogger.Errorf("para lenght error,expect:%v,got:%v", 1, len(para))
				REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, fmt.Sprintf("para number error:%v", len(para)))
				break
			}
			rawTx := para[0].(string)
			logger.SugarLogger.Infof("eth_sendRawTransaction rawTx:%v", rawTx)
			hash, err := s.Eth_sendRawTransaction(rawTx)
			if err != nil {
				logger.SugarLogger.Errorf("eth_sendRawTransaction error:%v", err)
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
			} else {
				resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: hash})
				if err != nil {
					logger.SugarLogger.Errorf("eth_sendRawTransaction Marshal error:%v", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				} else {
					logger.SugarLogger.Infof("eth_sendRawTransaction success hash:%v", hash)
					REST = resp
				}
			}
		}
	case ETH_CALL:
		ret, err := s.Eth_call(reqData)
		if err != nil {
			logger.SugarLogger.Warnf("eth_call error:%v", err)
			var RetErr util.ErrorBody
			RetErr.Code = CALLERR
			RetErr.Message = err.Error()
			if len(ret) > 0 {
				btret := common.Hex2Bytes(ret)
				lenth := binary.BigEndian.Uint32(btret[64:68])
				data := btret[68 : lenth+68]
				errMsg := string(data)
				RetErr.Message = RetErr.Message + ": " + errMsg
				RetErr.Data = util.StringToHex(ret)
			}

			resp, err := json.Marshal(util.ResponseErr{JsonRPC: jsonrpc, Id: id, Error: &RetErr})
			if err != nil {
				logger.SugarLogger.Errorf("eth_call Marshal error:%v", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			} else {
				logger.SugarLogger.Errorf("eth_call error, res msg:%v", RetErr.Message)
				REST = resp
			}
		} else {
			res := util.StringToHex(ret)
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: res})
			if err != nil {
				logger.SugarLogger.Errorf("eth_call Marshal error:%v", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			} else {
				logger.SugarLogger.Infof("return eth_call success, res:%v", res)
				REST = resp
			}
		}
	case ETH_BLOCKNUMBER:
		num, err := s.Eth_blockNumber()
		if err != nil {
			logger.SugarLogger.Errorf("eth_blockNumber error:%v", err)
			REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
		} else {
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: num})
			if err != nil {
				logger.SugarLogger.Errorf("eth_blockNumber Marshal error:%v", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			} else {
				logger.SugarLogger.Infof("eth_blockNumber success num:%v", num)
				REST = resp
			}
		}
	case ETH_GETBALANCE:
		para, err := util.GetParam(reqData)
		if err != nil {
			logger.SugarLogger.Errorf("eth_getBalance util.GetParam error:%v", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error()+fmt.Sprintf("para number:%v", len(para)))
		} else {
			from := para[0].(string)
			logger.SugarLogger.Infof("eth_getBalance address:%v", from)
			blc, err := s.Eth_getBalance(from)
			if err != nil {
				logger.SugarLogger.Errorf("eth_getBalance error:%v", err)
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
			} else {
				resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: blc})
				if err != nil {
					logger.SugarLogger.Errorf("eth_getBalance Marshal error:%v", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				} else {
					logger.SugarLogger.Infof("eth_getBalance success from:%v,balance:%v", from, blc)
					REST = resp
				}
			}
		}
	case ETH_GASPRICE:
		price, err := s.Eth_gasPrice()
		if err != nil {
			logger.SugarLogger.Errorf("eth_gasPrice error:%v", err)
			REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			break
		}
		resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: price})
		if err != nil {
			logger.SugarLogger.Errorf("eth_gasPrice Marshal error:%v", err)
			REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
		} else {
			logger.SugarLogger.Infof("eth_gasPrice success gasprice:%v", price)
			REST = resp
		}
	case EHT_GETCODE:
		para, err := util.GetParam(reqData)
		if err != nil {
			logger.SugarLogger.Errorf("eth_getCode util.GetParam error:%v", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error()+fmt.Sprintf("para number:%v", len(para)))
		} else {
			addr := para[0].(string)
			logger.SugarLogger.Infof("eth_getCode address:%v", addr)
			code, err := s.Eth_getCode(addr)
			if err != nil {
				logger.SugarLogger.Errorf("eth_getCode error:%v", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				break
			}
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: code})
			if err != nil {
				logger.SugarLogger.Errorf("eth_getCode Marshal error:%v", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			} else {
				logger.SugarLogger.Infof("eth_getCode success code:%v", code)
				REST = resp
			}
		}
	case ETH_GETTRANSACTIONCOUNT:
		para, err := util.GetParam(reqData)
		if err != nil {
			logger.SugarLogger.Errorf("eth_getTransactionCount util.GetParam error:%v", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error()+fmt.Sprintf("para number:%v", len(para)))
		} else {
			addr := para[0].(string)
			logger.SugarLogger.Infof("eth_getTransactionCount address:%v", addr)
			count, err := s.Eth_getTransactionCount(addr)
			if err != nil {
				logger.SugarLogger.Errorf("eth_getTransactionCount error:", err)
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
			} else {
				resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: count})
				if err != nil {
					logger.SugarLogger.Errorf("eth_getTransactionCount Marshal error:", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				} else {
					logger.SugarLogger.Infof("eth_getTransactionCount success addr:%v,nonce:%v", addr, count)
					REST = resp
				}
			}
		}
	case ETH_ESTIMATEGAS:
		ret, err := s.Eth_estimateGas(reqData)
		if err != nil {
			var RetErr util.ErrorBody
			RetErr.Code = ESTIMATEGASERR
			RetErr.Message = err.Error()
			if len(ret) > 0 {
				btret := common.Hex2Bytes(ret)
				lenth := binary.BigEndian.Uint32(btret[64:68])
				data := btret[68 : lenth+68]
				errMsg := string(data)
				RetErr.Message = RetErr.Message + ": " + errMsg
				RetErr.Data = util.StringToHex(ret)
			}
			resp, err := json.Marshal(util.ResponseErr{JsonRPC: jsonrpc, Id: id, Error: &RetErr})
			if err != nil {
				logger.SugarLogger.Errorf("eth_estimateGas Marshal error:%v", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			} else {
				logger.SugarLogger.Errorf("eth_estimateGas error, msg:%v", RetErr.Message)
				REST = resp
			}
		} else {
			res := util.StringToHex(ret)
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: res})
			if err != nil {
				logger.SugarLogger.Errorf("eth_estimateGas Marshal error:%v", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			} else {
				logger.SugarLogger.Infof("eth_estimateGas success ret:%v", res)
				REST = resp
			}
		}
	case ETH_GETBLOCKBYHASH:
		para, err := util.GetParam(reqData)
		if err != nil || len(para) != 2 {
			logger.SugarLogger.Errorf("util.GetParam error:%v,para len:%v", err, len(para))
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error()+fmt.Sprintf("para number:%v", len(para)))
		} else {
			hash := para[0].(string)
			option := para[1].(bool)
			logger.SugarLogger.Infof("eth_getBlockByHash hash:%v,option:%v\n", hash, option)
			blk, err := s.Eth_getBlockByHash(hash, option)
			if err != nil {
				logger.SugarLogger.Errorf("eth_getBlockByHash error:%v", err)
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
			} else {
				resp, err := json.Marshal(responseBlock{JsonRPC: jsonrpc, Id: id, Result: blk})
				if err != nil {
					logger.SugarLogger.Errorf("eth_getBlockByHash Marshal error:%v", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				} else {
					logger.SugarLogger.Infof("eth_getBlockByHash success")
					REST = resp
				}
			}
		}
	case ETH_GETBLOCKBYNUMBER:
		para, err := util.GetParam(reqData)
		if err != nil || len(para) != 2 {
			logger.SugarLogger.Errorf("eth_getBlockByNumber util.GetParam error:%v,para len:%v", err, len(para))
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error()+fmt.Sprintf("para number:%v", len(para)))
		} else {
			strNum := para[0].(string)
			option := para[1].(bool)
			logger.SugarLogger.Infof("eth_getBlockByNumber number:%v,option:%v\n", strNum, option)
			blk, err := s.Eth_getBlockByNumber(strNum, option)
			if err != nil {
				logger.SugarLogger.Errorf("eth_getBlockByNumber error:%v", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			} else {
				resp, err := json.Marshal(responseBlock{JsonRPC: jsonrpc, Id: id, Result: blk})
				if err != nil {
					logger.SugarLogger.Errorf("eth_getBlockByNumber Marshal error:%v", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				} else {
					logger.SugarLogger.Infof("eth_getBlockByNumber success")
					REST = resp
				}
			}
		}
	case ETH_GETTRANSACTIONBYHASH:
		para, err := util.GetParam(reqData)
		if err != nil {
			logger.SugarLogger.Errorf("eth_getTransactionByHash util.GetParam error:%v", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error()+fmt.Sprintf("para number:%v", len(para)))
		} else {
			hash := para[0].(string)
			logger.SugarLogger.Infof("eth_getTransactionByHash hash:%v", hash)
			tx, err := s.Eth_getTransactionByHash(hash)
			if err != nil {
				logger.SugarLogger.Errorf("eth_getTransactionByHash error:%v", err)
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
			} else {
				resp, err := json.Marshal(responseTransaction{JsonRPC: jsonrpc, Id: id, Result: tx})
				if err != nil {
					logger.SugarLogger.Errorf("eth_getTransactionByHash Marshal error:%v", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				} else {
					logger.SugarLogger.Infof("eth_getTransactionByHash success")
					REST = resp
				}
			}
		}
	case ETH_GETTRANSACTIONRECEIPT:
		para, err := util.GetParam(reqData)
		if err != nil {
			logger.SugarLogger.Errorf("eth_getTransactionReceipt util.GetParam error:%v", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error()+fmt.Sprintf("para number:%v", len(para)))
		} else {
			hash := para[0].(string)
			logger.SugarLogger.Infof("eth_getTransactionReceipt hash:%v", hash)
			tc, err := s.Eth_getTransactionReceipt(hash)
			if err != nil {
				logger.SugarLogger.Errorf("eth_getTransactionReceipt error:%v", err)
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
			} else {
				resp, err := json.Marshal(responseReceipt{JsonRPC: jsonrpc, Id: id, Result: tc})
				if err != nil {
					logger.SugarLogger.Errorf("eth_getTransactionReceipt Marshal error:%v", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				} else {
					logger.SugarLogger.Infof("eth_getTransactionReceipt success")
					REST = resp
				}
			}
		}
	case ETH_GETLOGS:
		res, err := s.Eth_getLogs(reqData)
		if err != nil {
			logger.SugarLogger.Errorf("eth_getLogs error:%v", err)
			REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
		} else {
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: res})
			if err != nil {
				logger.SugarLogger.Errorf("eth_getLogs Marshal error:%v", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			} else {
				logger.SugarLogger.Infof("eth_getLogs success res:%v", res)
				REST = resp
			}
		}
	case ETH_GETSTORAGEAT:
		res, err := s.Eth_getStorageAt(reqData)
		if err != nil {
			logger.SugarLogger.Errorf("eth_getStorageAt error:%v", err)
			REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
		} else {
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: res})
			if err != nil {
				logger.SugarLogger.Errorf("eth_getStorageAt Marshal error:%v", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			} else {
				logger.SugarLogger.Infof("eth_getStorageAt success res:%v", res)
				REST = resp
			}
		}
	case WEB3_SHA3:
		res, err := s.Web3_sha3(reqData)
		if err != nil {
			logger.SugarLogger.Errorf("Web3_sha3 error:%v", err)
			REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
		} else {
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: res})
			if err != nil {
				logger.SugarLogger.Errorf("Web3_sha3 Marshal error:%v", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			} else {
				logger.SugarLogger.Infof("Web3_sha3 success res:%v", res)
				REST = resp
			}
		}
	default:
		logger.SugarLogger.Warnf("Error unsupport method:%v", method)
		REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, fmt.Errorf("Unsupport method:%v", method).Error())
	}
	fmt.Println("end HandRequest >>>>>>>>>>>>>>>:", method)
}
