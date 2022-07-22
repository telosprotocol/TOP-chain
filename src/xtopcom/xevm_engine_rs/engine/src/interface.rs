#![allow(unused)]

use crate::prelude::*;

mod interface {
    use std::ffi::{c_void, CStr};
    use std::os::raw::c_uchar;

    use crate::engine::{self, *};
    use crate::error::EngineErrorKind;
    use crate::prelude::*;
    use crate::{engine::Engine, proto_parameters::FunctionCallArgs};
    use engine_sdk::{
        env::Env,
        io::{StorageIntermediate, IO},
        runtime::Runtime,
    };
    use hex::{encode, ToHex};
    use protobuf::Message;

    #[no_mangle]
    pub extern "C" fn deploy_code() -> bool {
        sdk::log("========= deploy_code =========");
        let io = Runtime;
        let input = io.read_input().to_vec();
        let mut engine = Engine::new(io.sender_address(), io, &io, &io).sdk_unwrap();
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
        let mut engine = Engine::new(io.sender_address(), io, &io, &io).sdk_unwrap();
        Engine::call_with_args(&mut engine, args)
            .map(|res| {
                sdk::log(format!("[rust_evm][interface]res: {:?}", res).as_str());
                res.write_to_bytes().sdk_expect("ERR_SERIALIZE")
            })
            .sdk_process()
    }

    #[no_mangle]
    pub extern "C" fn unsafe_mint(
        engine_ptr: *mut c_void,
        executor_ptr: *mut c_void,
        address_ptr: *const c_uchar,
        address_size: u64,
        value_ptr: *const i8,
    ) -> bool {
        sdk::log("========= unsafe_mint =========");

        assert!(address_size == 20_u64);
        let address = H160::from_slice(unsafe {
            std::slice::from_raw_parts(address_ptr, address_size as usize)
        });

        let value_str = unsafe { CStr::from_ptr(value_ptr) }.to_str();
        if value_str.is_err() {
            return false;
        }
        let value_str = value_str.unwrap();

        let value = U256::from_dec_str(value_str);
        if (value.is_err()) {
            return false;
        }
        let value = value.unwrap();

        sdk::log(
            format!(
                "{}",
                format_args!("unsafe_mint to {} with token amount {}", address, value)
            )
            .as_str(),
        );

        Engine::unsafe_deposit(
            unsafe {
                &mut *unsafe { engine_ptr as *mut Engine<'_, '_, Runtime, Runtime, Runtime> }
            },
            executor_ptr,
            &Address::new(address),
            &Wei::new(value),
        );
        true
    }

    #[no_mangle]
    pub extern "C" fn unsafe_burn(
        engine_ptr: *mut c_void,
        executor_ptr: *mut c_void,
        address_ptr: *const c_uchar,
        address_size: u64,
        value_ptr: *const i8,
    ) -> bool {
        sdk::log("========= unsafe_burn =========");

        assert!(address_size == 20_u64);
        let address = H160::from_slice(unsafe {
            std::slice::from_raw_parts(address_ptr, address_size as usize)
        });

        let value_str = unsafe { CStr::from_ptr(value_ptr) }.to_str();
        if value_str.is_err() {
            return false;
        }
        let value_str = value_str.unwrap();

        let value = U256::from_dec_str(value_str);
        if (value.is_err()) {
            return false;
        }
        let value = value.unwrap();

        sdk::log(
            format!(
                "{}",
                format_args!("unsafe_burn from {} with token amount {}", address, value)
            )
            .as_str(),
        );

        Engine::unsafe_withdraw(
            unsafe {
                &mut *unsafe { engine_ptr as *mut Engine<'_, '_, Runtime, Runtime, Runtime> }
            },
            executor_ptr,
            &Address::new(address),
            &Wei::new(value),
        )
        .is_ok()
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
