#![allow(unused)]
use crate::error::{BalanceOverflow, EngineError, EngineErrorKind, EngineStateError};
use crate::parameters::{ResultLog, SubmitResult, TransactionStatus};
use crate::prelude::*;
use core::cell::RefCell;
use engine_precompiles::{Precompile, PrecompileConstructorContext, Precompiles};
use engine_sdk::dup_cache::{DupCache, PairDupCache};
use engine_sdk::env::Env;
use engine_sdk::io::{StorageIntermediate, IO};
use evm::backend::{Apply, ApplyBackend, Backend, Basic, Log};
use evm::{executor, Config, CreateScheme, ExitError, ExitReason};

pub(crate) const CONFIG: &Config = &Config::london();

struct StackExecutorParams {
    precompiles: Precompiles,
    gas_limit: u64,
}

impl StackExecutorParams {
    fn new(gas_limit: u64, /*current_account_id: AccountId,*/ random_seed: H256) -> Self {
        Self {
            precompiles: Precompiles::new_london(PrecompileConstructorContext {
                // current_account_id,
                random_seed,
            }),
            gas_limit,
        }
    }

    fn make_executor<'a, 'env, I: IO + Copy, E: Env>(
        &'a self,
        engine: &'a Engine<'env, I, E>,
    ) -> executor::stack::StackExecutor<
        'static,
        'a,
        executor::stack::MemoryStackState<Engine<'env, I, E>>,
        Precompiles,
    > {
        let metadata = executor::stack::StackSubstateMetadata::new(self.gas_limit, CONFIG);
        let state = executor::stack::MemoryStackState::new(metadata, engine);
        executor::stack::StackExecutor::new_with_precompiles(state, CONFIG, &self.precompiles)
    }
}

trait ExitIntoResult {
    /// Checks if the EVM exit is ok or an error.
    fn into_result(self, data: Vec<u8>) -> Result<TransactionStatus, EngineErrorKind>;
}

impl ExitIntoResult for ExitReason {
    fn into_result(self, data: Vec<u8>) -> Result<TransactionStatus, EngineErrorKind> {
        use ExitReason::*;
        match self {
            Succeed(_) => Ok(TransactionStatus::Succeed(data)),
            Revert(_) => Ok(TransactionStatus::Revert(data)),
            Error(ExitError::OutOfOffset) => Ok(TransactionStatus::OutOfOffset),
            Error(ExitError::OutOfFund) => Ok(TransactionStatus::OutOfFund),
            Error(ExitError::OutOfGas) => Ok(TransactionStatus::OutOfGas),
            Error(e) => Err(e.into()),
            Fatal(e) => Err(e.into()),
        }
    }
}

#[derive(BorshSerialize, BorshDeserialize, Default, Clone)]
pub struct EngineState {
    /// Chain id, according to the EIP-155 / ethereum-lists spec.
    pub chain_id: [u8; 32],
    // /// Account which can upgrade this contract.
    // /// Use empty to disable updatability.
    // pub owner_id: AccountId,
    // /// Account of the bridge prover.
    // /// Use empty to not use base token as bridged asset.
    // pub bridge_prover_id: AccountId,
    // /// How many blocks after staging upgrade can deploy it.
    // pub upgrade_delay_blocks: u64
}

// impl From<NewCallArgs> for EngineState {
//     fn from(args: NewCallArgs) -> Self {
//         EngineState {
//             chain_id: args.chain_id,
//             // owner_id: args.owner_id,
//             // bridge_prover_id: args.bridge_prover_id,
//             // upgrade_delay_blocks: args.upgrade_delay_blocks,
//         }
//     }
// }

pub struct Engine<'env, I: IO, E: Env> {
    state: EngineState,
    origin: Address,
    gas_price: U256,
    // current_account_id: AccountId,
    io: I,
    env: &'env E,
    generation_cache: RefCell<BTreeMap<Address, u32>>,
    account_info_cache: RefCell<DupCache<Address, Basic>>,
    contract_storage_cache: RefCell<PairDupCache<Address, H256, H256>>,
}

const STATE_KEY: &[u8; 5] = b"STATE";

pub type EngineResult<T> = Result<T, EngineError>;

impl<'env, I: IO + Copy, E: Env> Engine<'env, I, E> {
    pub fn new(
        origin: Address,
        // current_account_id: AccountId,
        io: I,
        env: &'env E,
    ) -> Result<Self, EngineStateError> {
        Ok(Self::new_with_state(
            EngineState::default(),
            origin,
            // current_account_id,
            io,
            env,
        ))
    }

    pub fn new_with_state(
        state: EngineState,
        origin: Address,
        // current_account_id: AccountId,
        io: I,
        env: &'env E,
    ) -> Self {
        Self {
            state,
            origin,
            gas_price: U256::zero(),
            // current_account_id,
            io,
            env,
            generation_cache: RefCell::new(BTreeMap::new()),
            account_info_cache: RefCell::new(DupCache::default()),
            contract_storage_cache: RefCell::new(PairDupCache::default()),
        }
    }

    pub fn deploy_code_with_input(&mut self, input: Vec<u8>) -> EngineResult<SubmitResult> {
        let origin = Address::new(self.origin());
        let value = Wei::zero();
        self.deploy_code(origin, value, input, u64::MAX, Vec::new())
    }

    pub fn deploy_code(
        &mut self,
        origin: Address,
        value: Wei,
        input: Vec<u8>,
        gas_limit: u64,
        access_list: Vec<(H160, Vec<H256>)>, // See EIP-2930
    ) -> EngineResult<SubmitResult> {
        let executor_params = StackExecutorParams::new(
            gas_limit,
            // self.current_account_id.clone(),
            self.env.random_seed(),
        );
        let mut executor = executor_params.make_executor(self);
        let address = executor.create_address(CreateScheme::Legacy {
            caller: origin.raw(),
        });
        let (exit_reason, return_value) =
            executor.transact_create(origin.raw(), value.raw(), input, gas_limit, access_list);
        let result = if exit_reason.is_succeed() {
            address.0.to_vec()
        } else {
            return_value
        };

        let used_gas = executor.used_gas();
        let status = match exit_reason.into_result(result) {
            Ok(status) => status,
            Err(e) => {
                increment_nonce(&mut self.io, &origin);
                return Err(e.with_gas_used(used_gas));
            }
        };

        let (values, logs) = executor.into_state().deconstruct();

        let logs = logs.into_iter().map(|log| log.into()).collect();

        self.apply(values, Vec::<Log>::new(), true);

        Ok(SubmitResult::new(status, used_gas, logs))
    }

    pub fn call_with_args(&mut self, args: CallArgs) -> EngineResult<SubmitResult> {
        let origin = Address::new(self.origin());
        match args {
            CallArgs::V2(call_args) => {
                let contract = call_args.contract;
                let value = call_args.value.into();
                let input = call_args.input;
                self.call(&origin, &contract, value, input, u64::MAX, Vec::new())
            }
        }
    }

    pub fn call(
        &mut self,
        origin: &Address,
        contract: &Address,
        value: Wei,
        input: Vec<u8>,
        gas_limit: u64,
        access_list: Vec<(H160, Vec<H256>)>, // See EIP-2930
    ) -> EngineResult<SubmitResult> {
        let executor_params = StackExecutorParams::new(
            gas_limit,
            // self.current_account_id.clone(),
            self.env.random_seed(),
        );
        let mut executor = executor_params.make_executor(self);
        let (exit_reason, result) = executor.transact_call(
            origin.raw(),
            contract.raw(),
            value.raw(),
            input,
            gas_limit,
            access_list,
        );

        let used_gas = executor.used_gas();
        let status = match exit_reason.into_result(result) {
            Ok(status) => status,
            Err(e) => {
                increment_nonce(&mut self.io, origin);
                return Err(e.with_gas_used(used_gas));
            }
        };

        let (values, logs) = executor.into_state().deconstruct();
        let logs = logs.into_iter().map(|log| log.into()).collect();

        // There is no way to return the logs to the NEAR log method as it only
        // allows a return of UTF-8 strings.
        self.apply(values, Vec::<Log>::new(), true);

        Ok(SubmitResult::new(status, used_gas, logs))
    }
}

// pub fn compute_block_hash(chain_id: [u8; 32], block_height: u64, account_id: &[u8]) -> H256 {
//     debug_assert_eq!(BLOCK_HASH_PREFIX_SIZE, mem::size_of_val(&BLOCK_HASH_PREFIX));
//     debug_assert_eq!(BLOCK_HEIGHT_SIZE, mem::size_of_val(&block_height));
//     debug_assert_eq!(CHAIN_ID_SIZE, mem::size_of_val(&chain_id));
//     let mut data = Vec::with_capacity(
//         BLOCK_HASH_PREFIX_SIZE + BLOCK_HEIGHT_SIZE + CHAIN_ID_SIZE + account_id.len(),
//     );
//     data.push(BLOCK_HASH_PREFIX);
//     data.extend_from_slice(&chain_id);
//     data.extend_from_slice(account_id);
//     data.extend_from_slice(&block_height.to_be_bytes());

//     sdk::sha256(&data)
// }

// /// # engine state #
// pub fn get_state<I: IO>(io: &I) -> Result<EngineState, EngineStateError> {
//     match io.read_storage(&bytes_to_key(KeyPrefix::Config, STATE_KEY)) {
//         None => Err(EngineStateError::NotFound),
//         Some(bytes) => EngineState::try_from_slice(&bytes.to_vec())
//             .map_err(|_| EngineStateError::DeserializationFailed),
//     }
// }
// pub fn set_state<I: IO>(io: &mut I, state: EngineState) {
//     io.write_storage(
//         &bytes_to_key(KeyPrefix::Config, STATE_KEY),
//         &state.try_to_vec().expect("ERR_SER"),
//     );
// }

/// # balance #
pub fn add_balance<I: IO>(
    io: &mut I,
    address: &Address,
    amount: Wei,
) -> Result<(), BalanceOverflow> {
    let current_balance = get_balance(io, address);
    let new_balance = current_balance.checked_add(amount).ok_or(BalanceOverflow)?;
    set_balance(io, address, &new_balance);
    Ok(())
}
pub fn set_balance<I: IO>(io: &mut I, address: &Address, balance: &Wei) {
    io.write_storage(
        &address_to_key(KeyPrefix::Balance, address),
        &balance.to_bytes(),
    );
}
pub fn remove_balance<I: IO + Copy>(io: &mut I, address: &Address) {
    let balance = get_balance(io, address);
    // Apply changes for eth-connector. The `unwrap` is safe here because (a) if the connector
    // is implemented correctly then the total supply wll never underflow and (b) we are passing
    // in the balance directly so there will always be enough balance.

    // todo ?
    // EthConnectorContract::init_instance(*io)
    //     .internal_remove_eth(address, balance)
    //     .unwrap();
    io.remove_storage(&address_to_key(KeyPrefix::Balance, address));
}
pub fn get_balance<I: IO>(io: &I, address: &Address) -> Wei {
    let raw = io
        .read_u256(&address_to_key(KeyPrefix::Balance, address))
        .unwrap_or_else(|_| U256::zero());
    Wei::new(raw)
}

/// # code #
pub fn set_code<I: IO>(io: &mut I, address: &Address, code: &[u8]) {
    io.write_storage(&address_to_key(KeyPrefix::Code, address), code);
}
pub fn remove_code<I: IO>(io: &mut I, address: &Address) {
    io.remove_storage(&address_to_key(KeyPrefix::Code, address));
}
pub fn get_code<I: IO>(io: &I, address: &Address) -> Vec<u8> {
    io.read_storage(&address_to_key(KeyPrefix::Code, address))
        .map(|s| s.to_vec())
        .unwrap_or_default()
}
pub fn get_code_size<I: IO>(io: &I, address: &Address) -> usize {
    io.read_storage_len(&address_to_key(KeyPrefix::Code, address))
        .unwrap_or(0)
}

/// # nonce #
pub fn set_nonce<I: IO>(io: &mut I, address: &Address, nonce: &U256) {
    io.write_storage(
        &address_to_key(KeyPrefix::Nonce, address),
        &u256_to_arr(nonce),
    );
}
pub fn remove_nonce<I: IO>(io: &mut I, address: &Address) {
    io.remove_storage(&address_to_key(KeyPrefix::Nonce, address));
}
/// Checks the nonce to ensure that the address matches the transaction
/// nonce.
#[inline]
pub fn check_nonce<I: IO>(
    io: &I,
    address: &Address,
    transaction_nonce: &U256,
) -> Result<(), EngineErrorKind> {
    let account_nonce = get_nonce(io, address);

    if transaction_nonce != &account_nonce {
        return Err(EngineErrorKind::IncorrectNonce);
    }

    Ok(())
}
pub fn get_nonce<I: IO>(io: &I, address: &Address) -> U256 {
    io.read_u256(&address_to_key(KeyPrefix::Nonce, address))
        .unwrap_or_else(|_| U256::zero())
}
pub fn increment_nonce<I: IO>(io: &mut I, address: &Address) {
    let account_nonce = get_nonce(io, address);
    let new_nonce = account_nonce.saturating_add(U256::one());
    set_nonce(io, address, &new_nonce);
}

/// # others #
pub fn is_account_empty<I: IO>(io: &I, address: &Address) -> bool {
    get_balance(io, address).is_zero()
        && get_nonce(io, address).is_zero()
        && get_code_size(io, address) == 0
}
/// Increments storage generation for a given address.
pub fn set_generation<I: IO>(io: &mut I, address: &Address, generation: u32) {
    io.write_storage(
        &address_to_key(KeyPrefix::Generation, address),
        &generation.to_be_bytes(),
    );
}
pub fn get_generation<I: IO>(io: &I, address: &Address) -> u32 {
    io.read_storage(&address_to_key(KeyPrefix::Generation, address))
        .map(|value| {
            let mut bytes = [0u8; 4];
            value.copy_to_slice(&mut bytes);
            u32::from_be_bytes(bytes)
        })
        .unwrap_or(0)
}
fn remove_account<I: IO + Copy>(io: &mut I, address: &Address, generation: u32) {
    remove_nonce(io, address);
    remove_balance(io, address);
    remove_code(io, address);
    remove_all_storage(io, address, generation);
}

/// # storage #
pub fn remove_storage<I: IO>(io: &mut I, address: &Address, key: &H256, generation: u32) {
    io.remove_storage(storage_to_key(address, key, generation).as_ref());
}

pub fn set_storage<I: IO>(
    io: &mut I,
    address: &Address,
    key: &H256,
    value: &H256,
    generation: u32,
) {
    io.write_storage(storage_to_key(address, key, generation).as_ref(), &value.0);
}

pub fn get_storage<I: IO>(io: &I, address: &Address, key: &H256, generation: u32) -> H256 {
    io.read_storage(storage_to_key(address, key, generation).as_ref())
        .and_then(|value| {
            if value.len() == 32 {
                let mut buf = [0u8; 32];
                value.copy_to_slice(&mut buf);
                Some(H256(buf))
            } else {
                None
            }
        })
        .unwrap_or_default()
}

/// Removes all storage for the given address.
fn remove_all_storage<I: IO>(io: &mut I, address: &Address, generation: u32) {
    // FIXME: there is presently no way to prefix delete trie state.
    // NOTE: There is not going to be a method on runtime for this.
    //     You may need to store all keys in a list if you want to do this in a contract.
    //     Maybe you can incentivize people to delete dead old keys. They can observe them from
    //     external indexer node and then issue special cleaning transaction.
    //     Either way you may have to store the nonce per storage address root. When the account
    //     has to be deleted the storage nonce needs to be increased, and the old nonce keys
    //     can be deleted over time. That's how TurboGeth does storage.
    set_generation(io, address, generation + 1);
}

/// # Engine #
impl<'env, I: IO + Copy, E: Env> evm::backend::Backend for Engine<'env, I, E> {
    /// Returns the "effective" gas price (as defined by EIP-1559)
    fn gas_price(&self) -> U256 {
        self.gas_price
    }

    /// Returns the origin address that created the contract.
    fn origin(&self) -> H160 {
        self.origin.raw()
    }

    /// Returns a block hash from a given index.
    ///
    /// Currently, this returns
    /// 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff if
    /// only for the 256 most recent blocks, excluding of the current one.
    /// Otherwise, it returns 0x0.
    ///
    /// In other words, if the requested block index is less than the current
    /// block index, return
    /// 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff.
    /// Otherwise, return 0.
    ///
    /// This functionality may change in the future. Follow
    /// [nearcore#3456](https://github.com/near/nearcore/issues/3456) for more
    /// details.
    ///
    /// See: https://doc.aurora.dev/develop/compat/evm#blockhash
    fn block_hash(&self, number: U256) -> H256 {
        unreachable!()
        // let idx = U256::from(self.env.block_height());
        // if idx.saturating_sub(U256::from(256)) <= number && number < idx {
        //     // since `idx` comes from `u64` it is always safe to downcast `number` from `U256`
        //     compute_block_hash(
        //         self.state.chain_id,
        //         number.low_u64(),
        //         self.current_account_id.as_bytes(),
        //     )
        // } else {
        //     H256::zero()
        // }
    }

    /// Returns the current block index number.
    fn block_number(&self) -> U256 {
        U256::from(self.env.block_height())
    }

    /// Returns a mocked coinbase which is the EVM address for the Aurora
    /// account, being 0x4444588443C3a91288c5002483449Aba1054192b.
    ///
    /// See: https://doc.aurora.dev/develop/compat/evm#coinbase
    fn block_coinbase(&self) -> H160 {
        H160([
            0x44, 0x44, 0x58, 0x84, 0x43, 0xC3, 0xa9, 0x12, 0x88, 0xc5, 0x00, 0x24, 0x83, 0x44,
            0x9A, 0xba, 0x10, 0x54, 0x19, 0x2b,
        ])
    }

    /// Returns the current block timestamp.
    fn block_timestamp(&self) -> U256 {
        U256::from(self.env.block_timestamp().secs())
    }

    /// Returns the current block difficulty.
    ///
    /// See: https://doc.aurora.dev/develop/compat/evm#difficulty
    fn block_difficulty(&self) -> U256 {
        // todo ?
        U256::zero()
    }

    /// Returns the current block gas limit.
    ///
    /// Currently, this returns 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
    /// as there isn't a gas limit alternative right now but this may change in
    /// the future.
    ///
    /// See: https://doc.aurora.dev/develop/compat/evm#gaslimit
    fn block_gas_limit(&self) -> U256 {
        U256::max_value()
    }

    /// Returns the current base fee for the current block.
    ///
    /// Currently, this returns 0 as there is no concept of a base fee at this
    /// time but this may change in the future.
    ///
    /// TODO: doc.aurora.dev link
    fn block_base_fee_per_gas(&self) -> U256 {
        U256::zero()
    }

    /// Returns the states chain ID.
    fn chain_id(&self) -> U256 {
        U256::from(self.state.chain_id)
    }

    /// Checks if an address exists.
    fn exists(&self, address: H160) -> bool {
        !is_account_empty(&self.io, &Address::new(address))
    }

    /// Returns basic account information.
    fn basic(&self, address: H160) -> Basic {
        let address = Address::new(address);
        let result = self
            .account_info_cache
            .borrow_mut()
            .get_or_insert_with(&address, || Basic {
                nonce: get_nonce(&self.io, &address),
                balance: get_balance(&self.io, &address).raw(),
            })
            .clone();
        result
    }

    /// Returns the code of the contract from an address.
    fn code(&self, address: H160) -> Vec<u8> {
        get_code(&self.io, &Address::new(address))
    }

    /// Get storage value of address at index.
    fn storage(&self, address: H160, index: H256) -> H256 {
        let address = Address::new(address);
        let generation = *self
            .generation_cache
            .borrow_mut()
            .entry(address)
            .or_insert_with(|| get_generation(&self.io, &address));
        let result = *self
            .contract_storage_cache
            .borrow_mut()
            .get_or_insert_with((&address, &index), || {
                get_storage(&self.io, &address, &index, generation)
            });
        result
    }

    /// Get original storage value of address at index, if available.
    ///
    /// Since SputnikVM collects storage changes in memory until the transaction is over,
    /// the "original storage" will always be the same as the storage because no values
    /// are written to storage until after the transaction is complete.
    fn original_storage(&self, address: H160, index: H256) -> Option<H256> {
        Some(self.storage(address, index))
    }
}

impl<'env, J: IO + Copy, E: Env> ApplyBackend for Engine<'env, J, E> {
    fn apply<A, I, L>(&mut self, values: A, _logs: L, delete_empty: bool)
    where
        A: IntoIterator<Item = Apply<I>>,
        I: IntoIterator<Item = (H256, H256)>,
        L: IntoIterator<Item = Log>,
    {
        let mut writes_counter: usize = 0;
        let mut code_bytes_written: usize = 0;
        for apply in values {
            match apply {
                Apply::Modify {
                    address,
                    basic,
                    code,
                    storage,
                    reset_storage,
                } => {
                    let address = Address::new(address);
                    let generation = get_generation(&self.io, &address);
                    // println!("[aurora!!!][engine][apply] nonce: {}", basic.nonce);
                    set_nonce(&mut self.io, &address, &basic.nonce);
                    set_balance(&mut self.io, &address, &Wei::new(basic.balance));
                    writes_counter += 2; // 1 for nonce, 1 for balance

                    if let Some(code) = code {
                        set_code(&mut self.io, &address, &code);
                        code_bytes_written = code.len();

                        // println!("[aurora!!!][engine][apply] code_write_at_address");
                        sdk::log!(crate::prelude::format!(
                            "code_write_at_address {:?} {}",
                            address,
                            code_bytes_written,
                        )
                        .as_str());
                    }

                    let next_generation = if reset_storage {
                        remove_all_storage(&mut self.io, &address, generation);
                        generation + 1
                    } else {
                        generation
                    };

                    for (index, value) in storage {
                        if value == H256::default() {
                            remove_storage(&mut self.io, &address, &index, next_generation)
                        } else {
                            set_storage(&mut self.io, &address, &index, &value, next_generation)
                        }
                        writes_counter += 1;
                    }

                    // We only need to remove the account if:
                    // 1. we are supposed to delete an empty account
                    // 2. the account is empty
                    // 3. we didn't already clear out the storage (because if we did then there is
                    //    nothing to do)
                    if delete_empty
                        && is_account_empty(&self.io, &address)
                        && generation == next_generation
                    {
                        remove_account(&mut self.io, &address, generation);
                        writes_counter += 1;
                    }
                }
                Apply::Delete { address } => {
                    let address = Address::new(address);
                    let generation = get_generation(&self.io, &address);
                    remove_account(&mut self.io, &address, generation);
                    writes_counter += 1;
                }
            }
        }
        // These variable are only used if logging feature is enabled.
        // In production logging is always enabled so we can ignore the warnings.
        #[allow(unused_variables)]
        let total_bytes = 32 * writes_counter + code_bytes_written;
        #[allow(unused_assignments)]
        if code_bytes_written > 0 {
            writes_counter += 1;
        }
        // println!("[aurora!!!][engine][apply] ");
        sdk::log!(crate::prelude::format!("total_writes_count {}", writes_counter).as_str());
        sdk::log!(crate::prelude::format!("total_written_bytes {}", total_bytes).as_str());
    }
}
