package util

import (
	"testing"
)

func TestDecodeRawTx(t *testing.T) {
	rawTx := "0xf8920d85174876e80082520894d6139ea5fe0f3b54499e771417b0a5f56cd629b7880de0b6b3a7640000a477fb2c640000000000000000000000000000000000000000000000000de0b6b3a76400008240dea068374558f2dba5934f525aaf840a4e04d0506a33f94c5491f44db976f5f023f2a072caad5814801defb6c5fa3b0e7e6740fa264233673bd78912b11f439aa37aa9"
	_, err := DecodeRawTx(rawTx)
	if err != nil {
		t.Fatalf("TestDecodeRawTx err:%v", err)
	}
	t.Log("TestDecodeRawTx ok!")
}

func TestParseEthSignature(t *testing.T) {
	rawTx := "0xf8920d85174876e80082520894d6139ea5fe0f3b54499e771417b0a5f56cd629b7880de0b6b3a7640000a477fb2c640000000000000000000000000000000000000000000000000de0b6b3a76400008240dea068374558f2dba5934f525aaf840a4e04d0506a33f94c5491f44db976f5f023f2a072caad5814801defb6c5fa3b0e7e6740fa264233673bd78912b11f439aa37aa9"
	etx, err := DecodeRawTx(rawTx)
	if err != nil {
		t.Fatalf("DecodeRawTx err:%v", err)
	}

	sign := ParseEthSignature(etx)
	if len(sign) <= 64 {
		t.Fatalf("ParseEthSignature signature lenght error:%v", len(sign))
	}
	t.Log("ParseEthSignature ok!")
}

func TestVerifyEthSignature(t *testing.T) {
	rawTx := "0xf8920d85174876e80082520894d6139ea5fe0f3b54499e771417b0a5f56cd629b7880de0b6b3a7640000a477fb2c640000000000000000000000000000000000000000000000000de0b6b3a76400008240dea068374558f2dba5934f525aaf840a4e04d0506a33f94c5491f44db976f5f023f2a072caad5814801defb6c5fa3b0e7e6740fa264233673bd78912b11f439aa37aa9"
	etx, err := DecodeRawTx(rawTx)
	if err != nil {
		t.Fatalf("TestVerifyEthSignature err:%v", err)
	}
	err = VerifyEthSignature(etx)
	if err != nil {
		t.Fatalf("TestVerifyEthSignature err:%v", err)
	}
	t.Log("TestVerifyEthSignature ok!")
}
