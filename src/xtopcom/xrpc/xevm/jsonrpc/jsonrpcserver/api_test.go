package jsonrpcserver

import (
	"encoding/json"
	"testing"
)

func TestEth_sendRawTransaction(t *testing.T) {
	rawTx := "0xf8920d85174876e80082520894d6139ea5fe0f3b54499e771417b0a5f56cd629b7880de0b6b3a7640000a477fb2c640000000000000000000000000000000000000000000000000de0b6b3a76400008240dea068374558f2dba5934f525aaf840a4e04d0506a33f94c5491f44db976f5f023f2a072caad5814801defb6c5fa3b0e7e6740fa264233673bd78912b11f439aa37aa9"
	chainId := "0x205d"
	netWorkId := "0x205d"
	server := NewJsonRpcServer(chainId, netWorkId, "archivePoint", "clinetVersion")
	hash, err := server.Eth_sendRawTransaction(rawTx)
	if err != nil {
		t.Fatalf("TestEth_sendRawTransaction err:%v", err)
	}
	t.Log("TestEth_sendRawTransaction hash:", hash)
}

func TestEth_blockNumber(t *testing.T) {
	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	blockH, err := server.Eth_blockNumber()
	if err != nil {
		t.Fatalf("TestEth_blockNumber err:%v", err)
	}
	t.Logf("TestEth_blockNumber oK:%v\n", blockH)
}

func TestEth_getCode(t *testing.T) {
	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	code, err := server.Eth_getCode("0xaaaaaaaaaaaaaaaaaaaaaaaaa")
	if err != nil {
		t.Fatalf("TestEth_getCode err:%v", err)
	}
	t.Logf("TestEth_getCode oK:%v\n", code)
}

func TestEth_getTransactionCount(t *testing.T) {
	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	nonce, err := server.Eth_getTransactionCount("0xbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb")
	if err != nil {
		t.Fatalf("TestEth_getTransactionCount err:%v", err)
	}
	t.Logf("TestEth_getTransactionCount oK:%v\n", nonce)
}
func TestEth_gasPrice(t *testing.T) {
	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	price, err := server.Eth_gasPrice()
	if err != nil {
		t.Fatalf("TestEth_gasPrice err:%v", err)
	}
	t.Logf("TestEth_gasPrice oK:%v\n", price)
}

func TestEth_getBalance(t *testing.T) {
	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	balance, err := server.Eth_getBalance("0xccccccccccccccccccccccccccccc")
	if err != nil {
		t.Fatalf("TestEth_getBalance err:%v", err)
	}
	t.Logf("TestEth_getBalance oK:%v\n", balance)
}

func TestEth_call(t *testing.T) {
	data := `{"id":337305,"jsonrpc":"2.0","method":"eth_call","params":[{"data":"0x06fdde03","from":"0x0000000000000000000000000000000000000000","to":"0xd6139ea5fe0f3b54499e771417b0a5f56cd629b7"},"latest"]}`
	reqData := make(map[string]interface{})

	if err := json.Unmarshal([]byte(data), &reqData); err != nil {
		t.Fatalf("TestEth_call json.Unmarshal err:%v", err)
	}

	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	ret, err := server.Eth_call(reqData)
	if err != nil {
		t.Fatalf("TestEth_call err:%v", err)
	}
	t.Logf("TestEth_call oK:%v\n", ret)
}

func TestEth_estimateGas(t *testing.T) {
	data := `{"id":337305,"jsonrpc":"2.0","method":"eth_estimateGas","params":[{"data":"0x06fdde03","from":"0x0000000000000000000000000000000000000000","to":"0xd6139ea5fe0f3b54499e771417b0a5f56cd629b7"},"latest"]}`
	reqData := make(map[string]interface{})

	if err := json.Unmarshal([]byte(data), &reqData); err != nil {
		t.Fatalf("TestEth_estimateGas json.Unmarshal err:%v", err)
	}

	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	ret, err := server.Eth_estimateGas(reqData)
	if err != nil {
		t.Fatalf("TestEth_estimateGas err:%v", err)
	}
	t.Logf("TestEth_estimateGas oK:%v\n", ret)
}

func TestEth_getBlockByHash(t *testing.T) {
	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	b, err := server.Eth_getBlockByHash("0xhashhhhhhhhhhhhhhhhhhhhhhhh", false)
	if err != nil {
		t.Fatalf("TestEth_getBlockByHash err:%v", err)
	}
	t.Logf("TestEth_getBlockByHash oK:%v\n", b)
}

func TestEth_getBlockByNumber(t *testing.T) {
	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	b, err := server.Eth_getBlockByNumber(10, false)
	if err != nil {
		t.Fatalf("TestEth_getBlockByNumber err:%v", err)
	}
	t.Logf("TestEth_getBlockByNumber oK:%v\n", b)
}

func TestEth_getTransactionByHash(t *testing.T) {
	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	tx, err := server.Eth_getTransactionByHash("hashhhhhhhhhhhhhhhhhhhhhhhhhhh")
	if err != nil {
		t.Fatalf("TestEth_getTransactionByHash err:%v", err)
	}
	t.Logf("TestEth_getTransactionByHash oK:%v\n", tx)
}

func TestEth_getTransactionReceipt(t *testing.T) {
	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	tx, err := server.Eth_getTransactionReceipt("hashhhhhhhhhhhhhhhhhhhhhhhhhhh")
	if err != nil {
		t.Fatalf("TestEth_getTransactionReceipt err:%v", err)
	}
	t.Logf("TestEth_getTransactionReceipt oK:%v\n", tx)
}

func TestEth_getStorageAt(t *testing.T) {
	data := `{"id":337305,"jsonrpc":"2.0","method":"eth_getStorageAt","params":["contractaddressssssssssssssssss","0x0","latest"]}`
	reqData := make(map[string]interface{})

	if err := json.Unmarshal([]byte(data), &reqData); err != nil {
		t.Fatalf("TestEth_getStorageAt json.Unmarshal err:%v", err)
	}

	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	ret, err := server.Eth_getStorageAt(reqData)
	if err != nil {
		t.Fatalf("TestEth_getStorageAt err:%v", err)
	}
	t.Logf("TestEth_getStorageAt oK:%v\n", ret)
}

func TestEth_getLogs(t *testing.T) {
	data := `{"jsonrpc":"2.0","method":"eth_getLogs","params":[{"topics":["0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b"]}],"id":74}`
	reqData := make(map[string]interface{})

	if err := json.Unmarshal([]byte(data), &reqData); err != nil {
		t.Fatalf("TestEth_getLogs json.Unmarshal err:%v", err)
	}

	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	ret, err := server.Eth_getLogs(reqData)
	if err != nil {
		t.Fatalf("TestEth_getLogs err:%v", err)
	}
	t.Logf("TestEth_getLogs oK:%v\n", ret)
}

func TestWeb3_sha3(t *testing.T) {
	data := `{"jsonrpc":"2.0","method":"web3_sha3","params":["94f5374fce5edbc8e2a8697c15331677e6ebf0b"],"id":74}`
	reqData := make(map[string]interface{})

	if err := json.Unmarshal([]byte(data), &reqData); err != nil {
		t.Fatalf("TestEth_getLogs json.Unmarshal err:%v", err)
	}

	server := NewJsonRpcServer("0x205d", "0x205d", "archivePoint", "clinetVersion")
	ret, err := server.Web3_sha3(reqData)
	if err != nil {
		t.Fatalf("TestWeb3_sha3 err:%v", err)
	}
	t.Logf("TestWeb3_sha3 oK:%v\n", ret)
}
