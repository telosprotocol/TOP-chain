package main

import "C"

import (
	"context"
	"log"
	"net/http"
	"time"
	"unsafe"
	"xevm/jsonrpcserver"

	"github.com/mattn/go-pointer"
)

//export RunJsonRpc
func RunJsonRpc(chainId, netWorkId, archivePoint, clinetVersion, jsonrpcPort string) unsafe.Pointer {
	log.SetFlags(log.Lshortfile | log.LstdFlags)

	srv := &http.Server{Addr: jsonrpcPort}
	s := jsonrpcserver.NewJsonRpcServer(chainId, netWorkId, archivePoint, clinetVersion)
	http.HandleFunc("/", s.HandRequest)

	go func() {
		log.Println("running jsonrpc server:", jsonrpcPort)
		err := srv.ListenAndServe()
		if err != nil {
			log.Fatalf("%v\n", err)
			return
		}
	}()
	return pointer.Save(srv)
}

//export StopJsonRpc
func StopJsonRpc(srv unsafe.Pointer) {
	s := pointer.Restore(srv).(*http.Server)
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	if err := s.Shutdown(ctx); err != nil {
		log.Fatalf("StopJsonRpc failed:%v\n", err)
	}
}

func main() {}
