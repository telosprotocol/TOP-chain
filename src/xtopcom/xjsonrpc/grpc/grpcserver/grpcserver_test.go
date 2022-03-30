package grpcserver

import (
	"context"
	"jsonrpcdemo/grpc/message"
	"testing"
	"time"

	"google.golang.org/grpc"
)

func TestGetBalance(t *testing.T) {
	//建立链接
	conn, err := grpc.Dial("127.0.0.1:37399", grpc.WithInsecure())
	if err != nil {
		t.Fatal("connect grpc failed:", err)

	}

	defer conn.Close()

	clit := message.NewGreeterClient(conn)

	//设定请求超时时间 3s
	ctx, cancel := context.WithTimeout(context.Background(), time.Second*3)
	defer cancel()

	res, err := clit.GetBalance(ctx, &message.ReqBalance{Address: "address test"})
	if err != nil {
		t.Fatalf("getbalance failed:%v", err)
	}
	t.Log("address balance:", res.Balance)
}
