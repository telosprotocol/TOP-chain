package main

import "C"

import (
	"context"
	"log"
	"net/http"
	"time"
	"unsafe"
	"xevm/jsonrpcserver"
	"xevm/util"

	"github.com/mattn/go-pointer"
)

//export RunJsonRpc
func RunJsonRpc(chainId, netWorkId, archivePoint, clinetVersion, jsonrpcPort string) unsafe.Pointer {
	log.SetFlags(log.Lshortfile | log.LstdFlags)

	chid := util.MakeString(chainId)
	netid := util.MakeString(netWorkId)
	archivep := util.MakeString(archivePoint)
	cltv := util.MakeString(clinetVersion)
	jsonp := util.MakeString(jsonrpcPort)

	srv := &http.Server{Addr: jsonp}
	s := jsonrpcserver.NewJsonRpcServer(chid, netid, archivep, cltv)
	http.HandleFunc("/", s.HandRequest)

	go func() {
		log.Println("running jsonrpc server:", jsonp)
		err := srv.ListenAndServe()
		if err != nil {
			log.Fatalf("%v\n", err)
			return
		}
	}()
	return pointer.Save(srv)
}

//export StopJsonRpc
func StopJsonRpc(srv unsafe.Pointer) C.int {
	s := pointer.Restore(srv).(*http.Server)
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	if err := s.Shutdown(ctx); err != nil {
		log.Printf("StopJsonRpc failed:%v\n", err)
		return C.int(-1)
	}
	return C.int(0)
}

func main() {}
