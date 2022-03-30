package jsonrpcserver

import (
	"encoding/binary"
	"encoding/json"
	"jsonrpcdemo/jsonrpc/client"
	"jsonrpcdemo/jsonrpc/util"
	"strconv"

	"fmt"
	"io/ioutil"
	"log"

	"net/http"

	"github.com/ethereum/go-ethereum/common"
)

//New Server
func NewJsonRpcServer(chainId, networkId, archivePoint, clinetVersion string) *Server {
	cid, err := strconv.ParseUint(chainId[2:], 16, 32)
	if err != nil {
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
			log.Println("Error:", err)
			REST = util.ResponseErrFunc(ParameterErr, "2.0", 0, err.(error).Error())
		}
	}()

	body, err := ioutil.ReadAll(req.Body)
	if err != nil {
		log.Printf("ioutil ReadAll error:%v\n", err)
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
		log.Printf("get method error:%v\n", err)
		REST = util.ResponseErrFunc(ParameterErr, "", 0, err.Error())
		return
	}

	jsonrpc, err := util.GetString(reqData, "jsonrpc")
	if err != nil {
		log.Printf("get jsonrpc error:%v\n", err)
		REST = util.ResponseErrFunc(ParameterErr, "", 0, err.Error())
		return
	}

	id, err := util.GetValue(reqData, "id")
	if err != nil {
		log.Printf("getValue error:%v\n", err)
		REST = util.ResponseErrFunc(ParameterErr, jsonrpc, 0, err.Error())
		return
	}

	switch method {
	case ETH_CHAINID:
		resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: s.eth_chainId()})
		if err != nil {
			log.Println("eth_chainId Marshal error:", err)
			REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
		} else {
			fmt.Println("eth_chainId success res>>>", s.eth_chainId())
			REST = resp
		}
	case NET_VERSION:
		resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: s.net_version()})
		if err != nil {
			log.Println("net_version Marshal error:", err)
			REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
		} else {
			log.Println("net_version success res>>>", s.net_version())
			REST = resp
		}
	case WEB3_CLIENTVERSION:
		resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: s.web3_clientVersion()})
		if err != nil {
			log.Println("web3_clientVersion Marshal error:", err)
			REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
		} else {
			fmt.Println("web3_clientVersion success :", s.web3_clientVersion())
			REST = resp
		}
	case ETH_SENDRAWTRANSACTION:
		para, err := util.GetRaw(reqData)
		if err != nil {
			log.Println("getRaw error:", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error())
		} else {
			hash, err := s.Eth_sendRawTransaction(para[0].(string))
			if err != nil {
				log.Println("eth_sendRawTransaction error:", err)
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
			} else {
				resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: hash})
				if err != nil {
					log.Println("eth_sendRawTransaction Marshal error:", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				} else {
					log.Println("eth_sendRawTransaction success hash>>>", hash)
					REST = resp
				}
			}
		}
	case ETH_CALL:
		ret, err := s.Eth_call(reqData)
		if err != nil {
			log.Println("eth_call error:", err)
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
				log.Println("eth_call Marshal error:", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

			} else {
				REST = resp
			}
		} else {
			res := util.StringToHex(ret)
			log.Println("eth_call success res>>>", res)
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: res})
			if err != nil {
				log.Println("eth_call Marshal error:", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

			} else {
				log.Println("return eth_call res length>>>", len(res))
				REST = resp
			}
		}
	case ETH_BLOCKNUMBER:
		num, err := s.Eth_blockNumber()
		if err != nil {
			REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())

		} else {
			//resNum := fmt.Sprintf("%X", num)
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: num})
			if err != nil {
				log.Println("eth_blockNumber Marshal error:", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

			} else {
				log.Println("eth_blockNumber success res>>>", num)
				REST = resp
			}
		}
	case ETH_GETBALANCE:
		para, err := util.GetParam(reqData)
		if err != nil || len(para) == 0 {
			log.Println("util.GetParam error:", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error())

		} else {
			from := para[0].(string)
			blc, err := s.Eth_getBalance(from)
			if err != nil {
				log.Println("eth_getBalance error:", err)
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())

			} else {
				//metamask's decimal is 18,kto is 11,we need do blc*Pow10(7).
				// bigB := new(big.Int).SetUint64(blc)
				// bl := bigB.Mul(bigB, big.NewInt(ETHKTODIC))

				// resBalance := fmt.Sprintf("%X", bl)

				resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: blc})
				if err != nil {
					log.Println("eth_getBalance Marshal error:", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

				} else {
					log.Println("eth_getBalance success res>>>", from, util.StringToHex(blc))
					REST = resp
				}
			}
		}
	case ETH_GASPRICE:
		price, err := s.Eth_gasPrice()
		if err != nil {
			REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

		}
		resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: price})
		if err != nil {
			log.Println("eth_gasPrice Marshal error:", err)
			REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

		} else {
			REST = resp
		}
	case EHT_GETCODE:
		para, err := util.GetParam(reqData)
		if err != nil || len(para) == 0 {
			log.Println("util.GetParam error:", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error())

		} else {
			code, err := s.Eth_getCode(para[0].(string))
			if err != nil {
				log.Println("eth_getCode error:", err)
				code = "0x"
			}

			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: code})
			if err != nil {
				log.Println("eth_getCode Marshal error:", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

			} else {
				log.Println("eth_getCode success res>>>", code)
				REST = resp
			}
		}
	case ETH_GETTRANSACTIONCOUNT:
		para, err := util.GetParam(reqData)
		if err != nil || len(para) == 0 {
			log.Println("util.GetParam error:", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error())

		} else {
			addr := para[0].(string)
			count, err := s.Eth_getTransactionCount(addr)
			if err != nil {
				log.Println("eth_getTransactionCount error:", err)
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())

			} else {
				//hexCount := fmt.Sprintf("%X", count)
				resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: count})
				if err != nil {
					log.Println("eth_getTransactionCount Marshal error:", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

				} else {
					log.Println("eth_getTransactionCount success res>>>", "addr:", addr, "nonce", count)
					REST = resp
				}
			}
		}
	case ETH_ESTIMATEGAS:
		ret, err := s.Eth_estimateGas(reqData)
		if err != nil {
			var RetErr util.ErrorBody
			RetErr.Code = -4677
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
				log.Println("eth_estimateGas Marshal error:", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

			} else {
				log.Println("eth_estimateGas success ret>>>", ret)
				REST = resp
			}
		} else {
			res := util.StringToHex(ret)
			log.Println("eth_estimateGas success res>>>", res)
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: res})
			if err != nil {
				log.Println("eth_estimateGas Marshal error:", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

			} else {
				log.Println("eth_estimateGas success res>>>", res)
				REST = resp
			}
		}
	case ETH_GETBLOCKBYHASH:
		para, err := util.GetParam(reqData)
		if err != nil || len(para) == 0 {
			log.Println("util.GetParam error:", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error())
		} else {
			blk, err := s.Eth_getBlockByHash(para[0].(string), para[1].(bool))
			if err != nil {
				log.Println("eth_getBlockByHash error:", err)
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
			} else {
				resp, err := json.Marshal(responseBlock{JsonRPC: jsonrpc, Id: id, Result: blk})
				if err != nil {
					log.Println("eth_getBlockByHash Marshal error:", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				} else {
					log.Println("eth_getBlockByHash success res>>>")
					REST = resp
				}
			}
		}
	case ETH_GETBLOCKBYNUMBER:
		para, err := util.GetParam(reqData)
		if err != nil || len(para) != 2 {
			log.Println("util.GetParam error:", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error())
		} else {
			strNum := para[0].(string)
			var num uint64
			if strNum == "latest" {
				n, err := s.client.GetMaxBlockNumber()
				if err != nil {
					REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error())
					break
				}
				num = n
			} else {
				n, err := util.HexToUint64(strNum)
				if err != nil {
					log.Println("hexToUint64 error:", err)
					REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
					break
				}
				num = n
			}

			if num <= 0 {
				num = 1
			}
			blk, err := s.Eth_getBlockByNumber(num, para[1].(bool))
			if err != nil {
				log.Println("eth_getBlockByNumber error:", err)
				resE, err := json.Marshal(responseBlock{JsonRPC: jsonrpc, Id: id, Result: nil})
				if err != nil {
					log.Println("eth_getBlockByHash Marshal error:", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
					break
				}
				REST = resE
			} else {
				resp, err := json.Marshal(responseBlock{JsonRPC: jsonrpc, Id: id, Result: blk})
				if err != nil {
					log.Println("eth_getBlockByNumber Marshal error:", err)
					//REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
					resE, err := json.Marshal(responseBlock{JsonRPC: jsonrpc, Id: id, Result: nil})
					if err != nil {
						log.Println("eth_getBlockByHash Marshal error:", err)
						REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
						break
					}
					REST = resE
				} else {
					log.Println("eth_getBlockByNumber success res>>>", num)
					REST = resp
				}
			}
		}
	case ETH_GETTRANSACTIONBYHASH:
		para, err := util.GetParam(reqData)
		if err != nil || len(para) == 0 {
			log.Println("util.GetParam error:", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error())
		} else {
			hash := para[0].(string)
			tx, err := s.Eth_getTransactionByHash(hash)
			if err != nil {
				log.Println("eth_getTransactionByHash error:", err)
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
			} else {
				resp, err := json.Marshal(responseTransaction{JsonRPC: jsonrpc, Id: id, Result: tx})
				if err != nil {
					log.Println("eth_getTransactionByHash Marshal error:", err)
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				} else {
					log.Println("eth_getTransactionByHash success res>>>", hash)
					REST = resp
				}
			}
		}
	case ETH_GETTRANSACTIONRECEIPT:
		para, err := util.GetParam(reqData)
		if err != nil || len(para) == 0 {
			log.Println("util.GetParam error:", err)
			REST = util.ResponseErrFunc(ParameterErr, jsonrpc, id, err.Error())
		} else {
			tc, err := s.Eth_getTransactionReceipt(para[0].(string))
			if err != nil {
				REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
			} else {
				resp, err := json.Marshal(responseReceipt{JsonRPC: jsonrpc, Id: id, Result: tc})
				if err != nil {
					REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
				} else {
					log.Println("eth_getTransactionReceipt success res>>>", para[0].(string))
					REST = resp
				}
			}
		}
	case ETH_GETLOGS:
		res, err := s.Eth_getLogs(reqData)
		if err != nil {
			log.Println("eth_getLogs error:", err)
			REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
		} else {
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: res})
			if err != nil {
				log.Println("eth_getLogs Marshal error:", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())
			} else {
				log.Println("eth_getLogs success res>>>", res)
				REST = resp
			}
		}
	case ETH_GETSTORAGEAT:
		res, err := s.Eth_getStorageAt(reqData)
		if err != nil {
			log.Println("eth_getStorageAt error:", err)
			REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
		} else {
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: res})
			if err != nil {
				log.Println("eth_getStorageAt Marshal error:", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

			} else {
				fmt.Println("eth_getStorageAt success res>>>", res)
				REST = resp
			}
		}
	case WEB3_SHA3:
		res, err := s.Web3_sha3(reqData)
		if err != nil {
			log.Println("Web3_sha3 error:", err)
			REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, err.Error())
		} else {
			resp, err := json.Marshal(responseBody{JsonRPC: jsonrpc, Id: id, Result: res})
			if err != nil {
				log.Println("Web3_sha3 Marshal error:", err)
				REST = util.ResponseErrFunc(JsonMarshalErr, jsonrpc, id, err.Error())

			} else {
				fmt.Println("Web3_sha3 success res>>>", res)
				REST = resp
			}
		}

	default:
		log.Printf("Error unsupport method:%v\n", method)
		REST = util.ResponseErrFunc(UnkonwnErr, jsonrpc, id, fmt.Errorf("Unsupport method:%v", method).Error())
	}
	log.Println("end HandRequest >>>>>>>>>>>>>>>")
}
