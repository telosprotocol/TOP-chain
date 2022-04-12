## use protobuf for both rust and c

``` BASH
src/xtopcom/xevm_engine_rs/common_protobuf_type/proto$ protoc --rust_out ../../engine/src proto_parameters.proto
src/xtopcom/xevm_engine_rs/common_protobuf_type/proto$ protoc --rust_out ../../engine-types/src/types/ proto_basic.proto
src/xtopcom/xevm_engine_rs/common_protobuf_type/proto$ protoc --cpp_out ../../../xevm_runner/proto proto_parameters.proto 
src/xtopcom/xevm_engine_rs/common_protobuf_type/proto$ protoc --cpp_out ../../../xevm_runner/proto proto_basic.proto

```