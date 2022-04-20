#![allow(unused)]

use crate::prelude::*;

mod interface {
    use crate::engine::*;
    use crate::error::EngineErrorKind;
    use crate::prelude::*;
    use crate::{engine::Engine, proto_parameters::FunctionCallArgs};
    use engine_sdk::{
        env::Env,
        io::{StorageIntermediate, IO},
        runtime::Runtime,
    };
    use protobuf::Message;

    #[no_mangle]
    pub extern "C" fn deploy_code() -> bool {
        sdk::log("========= deploy_code =========");
        let io = Runtime;
        let input = io.read_input().to_vec();
        let mut engine = Engine::new(io.sender_address(), io, &io).sdk_unwrap();
        let args = FunctionCallArgs::parse_from_bytes(&input).sdk_expect("ERR_DESERIALIZE");
        Engine::deploy_code_with_args(&mut engine, args)
            .map(|res| {
                sdk::log(format!("[rust_evm][interface]res: {:?}", res).as_str());
                res.write_to_bytes().sdk_expect("ERR_SERIALIZE")
            })
            .sdk_process()
    }

    #[no_mangle]
    pub extern "C" fn call_contract() -> bool {
        sdk::log("========= call_contract =========");
        let io = Runtime;
        let bytes = io.read_input().to_vec();
        let args = FunctionCallArgs::parse_from_bytes(&bytes).sdk_expect("ERR_DESERIALIZE");
        sdk::log(format!("{:?}", args).as_str());
        let mut engine = Engine::new(io.sender_address(), io, &io).sdk_unwrap();
        Engine::call_with_args(&mut engine, args)
            .map(|res| {
                sdk::log(format!("[rust_evm][interface]res: {:?}", res).as_str());
                res.write_to_bytes().sdk_expect("ERR_SERIALIZE")
            })
            .sdk_process()
    }

    #[no_mangle]
    pub extern "C" fn mock_add_balance() {
        println!("========= set_balance =========");
        let mut io = Runtime;
        let mut engine = Engine::new(io.sender_address(), io, &io).sdk_unwrap();
        let origin = io.sender_address();
        let origin_value = get_balance(&mut io, &origin);
        set_balance(&mut io, &origin, &Wei::new(origin_value.raw() + 10000000));
    }
}
