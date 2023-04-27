#![allow(unused)]

use crate::prelude::*;

mod interface {
    use std::ffi::{c_void, CStr};
    use std::os::raw::c_uchar;
    use std::str::FromStr;

    use crate::engine::{self, *};
    use crate::error::EngineErrorKind;
    use crate::prelude::*;
    use crate::{engine::Engine, proto_parameters::FunctionCallArgs};
    use engine_eth2_types::eth2::PUBLIC_KEY_BYTES_LEN;
    use engine_sdk::{
        env::Env,
        io::{StorageIntermediate, IO},
        runtime::Runtime,
    };
    use eth2_hashing::{hash, hash32_concat};
    use hex::{encode, ToHex};
    use protobuf::Message;
    use tree_hash::TreeHash;

    const HASH_LEN: usize = 32;
    const SIGNATURE_LEN: usize = 96;

    #[no_mangle]
    pub extern "C" fn deploy_code() -> bool {
        // sdk::log("========= deploy_code =========");
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
        // sdk::log("========= call_contract =========");
        let io = Runtime;
        let bytes = io.read_input().to_vec();
        let args = FunctionCallArgs::parse_from_bytes(&bytes).sdk_expect("ERR_DESERIALIZE");
        // sdk::log(format!("{:?}", args).as_str());
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

    #[no_mangle]
    pub extern "C" fn unsafe_merkle_proof(
        leaf_ptr: *const u8,
        branch_ptr: *const u8,
        branch_size: u64,
        depth: u64,
        index: u64,
        root_ptr: *const u8,
    ) -> bool {
        assert_ne!(leaf_ptr, std::ptr::null());
        assert_ne!(root_ptr, std::ptr::null());
        if depth != 0 && branch_ptr.is_null() {
            return false;
        }
        let leaf = ethereum_types::H256::from_slice(unsafe {
            std::slice::from_raw_parts(leaf_ptr, HASH_LEN)
        });
        if branch_size as usize % HASH_LEN != 0 {
            return false;
        }
        let branch_data = unsafe { std::slice::from_raw_parts(branch_ptr, branch_size as usize) };
        let branch_num = branch_size as usize / HASH_LEN;
        let mut branch: Vec<ethereum_types::H256> = Vec::new();
        for i in 0..branch_num {
            let b =
                ethereum_types::H256::from_slice(&branch_data[i * HASH_LEN..((i + 1) * HASH_LEN)]);
            branch.push(b);
        }
        if branch.len() != depth as usize {
            return false;
        }
        let mut merkle_root = leaf.as_bytes().to_vec();
        for (i, leaf) in branch.iter().enumerate().take(depth as usize) {
            let ith_bit = (index >> i) & 0x01;
            if ith_bit == 1 {
                merkle_root = hash32_concat(leaf.as_bytes(), &merkle_root)[..].to_vec();
            } else {
                let mut input = merkle_root;
                input.extend_from_slice(leaf.as_bytes());
                merkle_root = hash(&input);
            }
        }
        unsafe {
            for i in 0..HASH_LEN {
                let ptr = (root_ptr as usize + i) as *mut u8;
                std::ptr::write(ptr, merkle_root[i]);
            }
        }
        return true;
    }

    #[no_mangle]
    pub extern "C" fn unsafe_verify_bls_signatures(
        signature_ptr: *const u8,
        pubkeys_ptr: *const u8,
        pubkeys_size: u64,
        signing_root_ptr: *const u8,
    ) -> bool {
        assert_ne!(signature_ptr, std::ptr::null());
        assert_ne!(pubkeys_ptr, std::ptr::null());
        assert_ne!(signing_root_ptr, std::ptr::null());
        let signature = bls::AggregateSignature::deserialize(unsafe {
            std::slice::from_raw_parts(signature_ptr, SIGNATURE_LEN)
        })
        .unwrap();
        if pubkeys_size as usize % PUBLIC_KEY_BYTES_LEN != 0 {
            return false;
        }
        let pubkeys_all_in_one =
            unsafe { std::slice::from_raw_parts(pubkeys_ptr, pubkeys_size as usize) };
        let pubkeys_num = pubkeys_size as usize / PUBLIC_KEY_BYTES_LEN;
        let mut branch_pubkeys: Vec<bls::PublicKey> = Vec::new();
        for i in 0..pubkeys_num {
            let raw =
                &pubkeys_all_in_one[i * PUBLIC_KEY_BYTES_LEN..((i + 1) * PUBLIC_KEY_BYTES_LEN)];
            let branch = bls::PublicKey::deserialize(raw).unwrap();
            branch_pubkeys.push(branch);
        }
        let signing_root = engine_eth2_types::H256::from(unsafe {
            std::slice::from_raw_parts(signing_root_ptr, HASH_LEN)
        });
        return signature
            .fast_aggregate_verify(signing_root.0, &branch_pubkeys.iter().collect::<Vec<_>>());
    }

    #[no_mangle]
    pub extern "C" fn unsafe_beacon_header_root(
        beacon_data_ptr: *const u8,
        data_size: u64,
        beacon_hash_ptr: *mut u8,
    ) -> bool {
        let beacon: engine_eth2_types::eth2::BeaconBlockHeader =
            rlp::decode(unsafe { std::slice::from_raw_parts(beacon_data_ptr, data_size as usize) })
                .unwrap();
        let root = beacon.tree_hash_root();
        let bytes = root.as_bytes();
        unsafe {
            for i in 0..HASH_LEN {
                let ptr = (beacon_hash_ptr as usize + i) as *mut u8;
                std::ptr::write(ptr, bytes[i]);
            }
        }
        return true;
    }

    #[no_mangle]
    pub extern "C" fn unsafe_compute_signing_root(
        object_root_ptr: *const u8,
        domain_data_ptr: *const u8,
        signing_root_ptr: *mut u8,
    ) -> bool {
        let object_root = engine_eth2_types::H256::from(unsafe {
            std::slice::from_raw_parts(object_root_ptr, HASH_LEN)
        });
        let domain = engine_eth2_types::H256::from(unsafe {
            std::slice::from_raw_parts(domain_data_ptr, HASH_LEN)
        });
        let binding = engine_eth2_types::eth2::SigningData {
            object_root,
            domain,
        }
        .tree_hash_root();
        let signing_root_data = binding.as_bytes();
        unsafe {
            for i in 0..HASH_LEN {
                let ptr = (signing_root_ptr as usize + i) as *mut u8;
                std::ptr::write(ptr, signing_root_data[i]);
            }
        }
        return true;
    }

    #[no_mangle]
    pub extern "C" fn unsafe_compute_committee_signing_root(
        object_root_ptr: *const u8,
        version: u64,
        signature_slot: u64,
        signing_root_ptr: *mut u8,
    ) -> bool {
        let object_root = engine_eth2_types::H256::from(unsafe {
            std::slice::from_raw_parts(object_root_ptr, HASH_LEN)
        });
        let mut netstr: String;
        if version == 0 {
            netstr = "mainnet".to_string();
        } else if version == 1{
            netstr = "kiln".to_string();
        } else if version == 2 {
            netstr = "goerli".to_string();
        } else if version == 3 {
            netstr = "sepolia".to_string();
        } else {
            return false;
        }

        let network = engine_eth2_utility::consensus::Network::from_str(netstr.as_str()).unwrap();
        let config = engine_eth2_utility::consensus::NetworkConfig::new(&network);
        let fork_version = config.compute_fork_version_by_slot(signature_slot).expect("Unsupported fork");
        let domain = engine_eth2_utility::consensus::compute_domain(
            engine_eth2_utility::consensus::DOMAIN_SYNC_COMMITTEE,
            fork_version,
            config.genesis_validators_root.into(),
        );
        let binding = engine_eth2_types::eth2::SigningData {
            object_root,
            domain,
        }
        .tree_hash_root();
        let signing_root_data = binding.as_bytes();
        unsafe {
            for i in 0..HASH_LEN {
                let ptr = (signing_root_ptr as usize + i) as *mut u8;
                std::ptr::write(ptr, signing_root_data[i]);
            }
        }
        return true;
    }

    #[no_mangle]
    pub extern "C" fn unsafe_sync_committee_root(
        data_ptr: *const u8,
        data_size: u64,
        committee_root_ptr: *mut u8,
    ) -> bool {
        assert_ne!(data_ptr, std::ptr::null());
        if data_size as usize % PUBLIC_KEY_BYTES_LEN != 0 {
            return false;
        }
        let pubkeys_all_in_one =
            unsafe { std::slice::from_raw_parts(data_ptr, data_size as usize) };
        let pubkeys_num = data_size as usize / PUBLIC_KEY_BYTES_LEN;

        let mut committee = engine_eth2_types::eth2::SyncCommittee {
            pubkeys: engine_eth2_types::eth2::SyncCommitteePublicKeys(engine_types::Vec::<
                engine_eth2_types::eth2::PublicKeyBytes,
            >::new()),
            aggregate_pubkey: engine_eth2_types::eth2::PublicKeyBytes([0; PUBLIC_KEY_BYTES_LEN]),
        };
        for i in 0..pubkeys_num {
            let raw =
                &pubkeys_all_in_one[i * PUBLIC_KEY_BYTES_LEN..((i + 1) * PUBLIC_KEY_BYTES_LEN)];
            let mut array = [0; PUBLIC_KEY_BYTES_LEN];
            array.copy_from_slice(&raw[0..PUBLIC_KEY_BYTES_LEN]);
            if i == (pubkeys_num - 1) {
                committee.aggregate_pubkey = engine_eth2_types::eth2::PublicKeyBytes(array);
            } else {
                committee
                    .pubkeys
                    .0
                    .push(engine_eth2_types::eth2::PublicKeyBytes(array));
            }
        }
        let binding = committee.tree_hash_root();
        let root = binding.as_bytes();
        unsafe {
            for i in 0..HASH_LEN {
                let ptr = (committee_root_ptr as usize + i) as *mut u8;
                std::ptr::write(ptr, root[i]);
            }
        }
        return true;
    }
}
