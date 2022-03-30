package main

import "C"

import (
	"jsonrpcdemo/jsonrpc/jsonrpcserver"
	"log"
	"net/http"

	"github.com/spf13/viper"
)

func main() {}

//export RunJsonRpc
func RunJsonRpc() {
	log.SetFlags(log.Lshortfile | log.LstdFlags)

	viper := viper.New()
	viper.SetConfigName("config")
	viper.SetConfigType("yaml")
	viper.AddConfigPath("./config")
	if err := viper.ReadInConfig(); err != nil {
		log.Fatal("ReadInConfig fail:", err.Error())
	}

	chainId := viper.GetString("chainId")
	netWorkId := viper.GetString("netWorkId")

	jsonrpcPort := viper.GetString("jsonrpcPort")
	archivePoint := viper.GetString("archivePoint")
	clinetVersion := viper.GetString("clinetVersion")

	s := jsonrpcserver.NewJsonRpcServer(chainId, netWorkId, archivePoint, clinetVersion)
	http.HandleFunc("/", s.HandRequest)

	log.Println("running jsonrpc server:", jsonrpcPort)
	err := http.ListenAndServe(jsonrpcPort, nil)
	if err != nil {
		log.Fatalf("Start server failed:%v\n", err)
	}
}
