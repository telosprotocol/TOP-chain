package grpcserver

import (
	"context"
	"jsonrpcdemo/grpc/capi"
	"jsonrpcdemo/grpc/message"
	"log"
	"net"

	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

type Greeter struct{ grpcPort string }

func NewGreeter(gport string) *Greeter {
	return &Greeter{grpcPort: gport}
}

func (g *Greeter) RunGrpcServer() {
	lisn, err := net.Listen("tcp", g.grpcPort)
	if err != nil {
		log.Fatalf("grpc Listen error:%v", err)
	}
	server := grpc.NewServer()

	message.RegisterGreeterServer(server, g)

	reflection.Register(server)

	log.Printf("start grpc server...%v", g.grpcPort)
	if err := server.Serve(lisn); err != nil {
		panic(err)
	}
}

func (g *Greeter) GetBalance(ctx context.Context, in *message.ReqBalance) (*message.ResBalance, error) {
	log.Printf("GetBalance address = %v\n", in.Address)
	b, err := capi.GetBalance()
	if err != nil {
		return nil, err
	}
	return &message.ResBalance{Balance: b}, nil
}
