package main

import "C"

import (
	"jsonrpcdemo/grpc/grpcserver"
	"log"

	"github.com/spf13/viper"
)

func main() {}

//export RunGrpc
func RunGrpc() {
	viper := viper.New()
	viper.SetConfigName("config")
	viper.SetConfigType("yaml")
	viper.AddConfigPath("./config")
	if err := viper.ReadInConfig(); err != nil {
		log.Fatal("ReadInConfig fail:", err.Error())
	}

	grpcPort := viper.GetString("grpcPort")

	g := grpcserver.NewGreeter(grpcPort)
	g.RunGrpcServer()
}
