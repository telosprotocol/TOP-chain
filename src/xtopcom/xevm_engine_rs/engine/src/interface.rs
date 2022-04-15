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
    pub extern "C" fn deploy_code() {
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
            .sdk_process();
        // TODO: charge for storage
    }
    #[no_mangle]
    pub extern "C" fn call_contract() {
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
            .sdk_process();
    }
    // #[no_mangle]
    // pub extern "C" fn serial_param_function_callargs(
    //     address: *const u8,
    //     address_len: u64,
    //     value: u64,
    //     params: *const u8,
    //     params_len: u64,
    //     max_output_len: u64,
    //     output: *mut u8,
    //     output_len: *mut u64,
    // ) {
    //     let address = unsafe {
    //         assert!(!address.is_null());
    //         core::slice::from_raw_parts(address, address_len as usize)
    //     };
    //     let params: Vec<u8> = unsafe {
    //         assert!(!params.is_null());
    //         core::slice::from_raw_parts(params, params_len as usize)
    //     }
    //     .iter()
    //     .cloned()
    //     .collect();
    //     let args = CallArgs::V2(FunctionCallArgsV2 {
    //         contract: Address::new(H160::from_slice(&hex::decode(address).unwrap()[..])),
    //         value: Wei::new(value.into()).to_bytes(),
    //         input: params,
    //     });
    //     let ser = bincode::serialize(&args).unwrap();
    //     // args.serialize(&mut ser).unwrap();

    //     if (ser.len() <= max_output_len as usize) {
    //         unsafe {
    //             assert!(!output.is_null());
    //             assert!(!output_len.is_null());
    //             for i in 0..ser.len() as usize {
    //                 let iter = (output as usize + i) as *mut u8;
    //                 *iter = ser[i];
    //             }
    //             *output_len = ser.len() as u64;
    //         };
    //         // println!("ser: {:?}", ser);
    //     }
    // }
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
