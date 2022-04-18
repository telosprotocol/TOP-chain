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
        println!("========= deploy_code =========");
        let io = Runtime;
        let input = io.read_input().to_vec();
        let mut engine = Engine::new(io.sender_address(), io, &io).sdk_unwrap();
        Engine::deploy_code_with_input(&mut engine, input)
            .map(|res| {
                println!("[rust_evm][interface]res: {:?}", res);
                res.write_to_bytes().sdk_expect("ERR_SERIALIZE")
                // bincode::serialize(&res).sdk_expect("ERR_SERIALIZE")
            })
            .sdk_process()
        // TODO: charge for storage
    }
    #[no_mangle]
    pub extern "C" fn call_contract() -> bool {
        println!("========= call_contract =========");
        let io = Runtime;
        let bytes = io.read_input().to_vec();
        let args = FunctionCallArgs::parse_from_bytes(&bytes).sdk_expect("ERR_DESERIALIZE");
        // let args = bincode::deserialize::<CallArgs>(&bytes).sdk_expect("ERR_DESERIALIZE");
        // println!("{:?}", args);
        let mut engine = Engine::new(io.sender_address(), io, &io).sdk_unwrap();
        Engine::call_with_args(&mut engine, args)
            .map(|res| {
                println!("[rust_evm][interface]res: {:?}", res);
                res.write_to_bytes().sdk_expect("ERR_SERIALIZE")
                // bincode::serialize(&res).sdk_expect("ERR_SERIALIZE")
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
