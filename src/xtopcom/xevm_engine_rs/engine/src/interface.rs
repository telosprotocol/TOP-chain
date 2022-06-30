#![allow(unused)]

use crate::prelude::*;

mod interface {
    use std::ffi::c_void;

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
    pub extern "C" fn new_engine() -> *mut c_void {
        sdk::log("========= new_engine =========");

        Box::into_raw(Box::new(
            Engine::new(Runtime.sender_address(), Runtime, &Runtime).sdk_unwrap(),
        )) as *mut c_void
    }

    #[no_mangle]
    pub extern "C" fn free_engine(ptr: *mut c_void) {
        sdk::log("========= free_engine =========");

        let engine_raw = ptr as *mut Engine<'static, Runtime, Runtime>;
        unsafe {
            let engine = Box::from_raw(engine_raw);
        }
    }

    #[no_mangle]
    pub extern "C" fn engine_call_contract(ptr: *mut c_void) -> bool {
        assert!(!ptr.is_null());

        let engine_ptr = ptr as *mut Engine<'static, Runtime, Runtime>;

        let bytes = Runtime.read_input().to_vec();
        let args = FunctionCallArgs::parse_from_bytes(&bytes).sdk_expect("ERR_DESERIALIZE");
        sdk::log(format!("{:?}", args).as_str());

       Engine::call_with_args(unsafe {&mut *engine_ptr}, args).map(|res| {
            sdk::log(format!("[rust_evm][interface]res: {:?}", res).as_str());
            res.write_to_bytes().sdk_expect("ERR_SERIALIZE")
        })
        .sdk_process()
    }

    #[no_mangle]
    pub extern "C" fn do_mock_add_balance(
        address: *const u8,
        address_len: u64,
        value_1: u64,
        value_2: u64,
        value_3: u64,
        value_4: u64,
    ) {
        sdk::log("========= mock set_balance =========");
        let address = unsafe {
            assert!(!address.is_null());
            core::slice::from_raw_parts(address, address_len as usize)
        };
        let eth_address = Address::new(H160::from_slice(&hex::decode(address).unwrap()[..]));

        let mut io = Runtime;
        // let mut engine = Engine::new(eth_address, io, &io).sdk_unwrap();
        // let origin = io.sender_address();
        let origin_value = get_balance(&mut io, &eth_address);
        let mut value_u256 = U256::from(value_1);
        value_u256 = (value_u256 << 64) + U256::from(value_2);
        value_u256 = (value_u256 << 64) + U256::from(value_3);
        value_u256 = (value_u256 << 64) + U256::from(value_4);
        set_balance(
            &mut io,
            &eth_address,
            &Wei::new(origin_value.raw() + value_u256),
        );
    }
}
