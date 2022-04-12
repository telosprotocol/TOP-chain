protoc --rust_out ../../engine/src proto_parameters.proto
protoc --rust_out ../../engine-types/src/types/ proto_basic.proto
protoc --cpp_out ../../../xevm_runner/proto proto_parameters.proto 
protoc --cpp_out ../../../xevm_runner/proto proto_basic.proto