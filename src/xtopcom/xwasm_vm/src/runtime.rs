use std::borrow::{Borrow, BorrowMut};
use std::ptr::NonNull;
use std::sync::{Arc, RwLock};

// use wasmer::HostEnvInitError;
use wasmer::{Instance as WasmerInstance, Val, WasmerEnv};
use wasmer_middlewares::metering::{get_remaining_points, set_remaining_points, MeteringPoints};

use crate::backend::{Backend, BackendApi, Querier, Storage};
use crate::errors::{VmError, VmResult};

/// Never can never be instantiated.
/// Replace this with the [never primitive type](https://doc.rust-lang.org/std/primitive.never.html) when stable.
#[derive(Debug)]
pub enum Never {}

/** gas config **/

#[derive(Clone, PartialEq, Debug)]
pub struct GasConfig {
    // here might define some native functions (provided by VM)' costs
}

impl GasConfig {
    // here might define some const cost value
    // which might be the default value of above cost.
}

impl Default for GasConfig {
    fn default() -> Self {
        Self {}
    }
}

pub struct GasState {
    /// Gas limit for computation, including internally(wasm) and externally(native function) used gas.
    /// This is set when the Runtime is created. Never mutated.
    pub gas_limit: u64,
    /// Tracking the gas used in SDK
    pub externally_used_gas: u64,
}

impl GasState {
    fn with_limit(gas_limit: u64) -> Self {
        Self {
            gas_limit,
            externally_used_gas: 0,
        }
    }
}

/** runtime data **/

pub struct Runtime<A: BackendApi, S: Storage, Q: Querier> {
    pub api: A,
    pub print_debug: bool,
    pub gas_config: GasConfig,
    data: Arc<RwLock<ContextData<S, Q>>>,
}

impl<A: BackendApi, S: Storage, Q: Querier> Clone for Runtime<A, S, Q> {
    fn clone(&self) -> Self {
        Runtime {
            api: self.api,
            print_debug: self.print_debug,
            gas_config: self.gas_config.clone(),
            data: self.data.clone(),
        }
    }
}

// impl<A, S, Q> WasmerEnv for Runtime<A, S, Q> {
//     fn init_with_instance(&mut self, _instance: &WasmInstance) -> Result<(), HostEnvInitError> {
//         Ok(())
//     }
// }

impl<A: BackendApi, S: Storage, Q: Querier> Runtime<A, S, Q> {
    pub fn new(api: A, gas_limit: u64, print_debug: bool) -> Self {
        Runtime {
            api,
            print_debug,
            gas_config: GasConfig::default(),
            data: Arc::new(RwLock::new(ContextData::new(gas_limit))),
        }
    }

    pub fn with_context_data_mut<C, R>(&self, callback: C) -> R
    where
        C: FnOnce(&mut ContextData<S, Q>) -> R,
    {
        let mut guard = self.data.as_ref().write().unwrap();
        let context_data = guard.borrow_mut();
        callback(context_data)
    }
    pub fn with_context_data<C, R>(&self, callback: C) -> R
    where
        C: FnOnce(&ContextData<S, Q>) -> R,
    {
        let guard = self.data.as_ref().read().unwrap();
        let context_data = guard.borrow();
        callback(context_data)
    }

    pub fn with_wasmer_instance<C, R>(&self, callback: C) -> VmResult<R>
    where
        C: FnOnce(&WasmerInstance) -> VmResult<R>,
    {
        self.with_context_data(|context_data| match context_data.wasmer_instance {
            Some(instance_ptr) => {
                let instance_ref = unsafe { instance_ptr.as_ref() };
                callback(instance_ref)
            }
            None => Err(VmError::uninitialized_context_data("wasmer_instance")),
        })
    }

    fn call_function(&self, name: &str, args: &[Val]) -> VmResult<Box<[Val]>> {
        let func = self.with_wasmer_instance(|instance| {
            let func = instance.exports.get_function(name)?;
            Ok(func.clone())
        })?;
        func.call(args).map_err(|runtime_err| -> VmError {
            self.with_wasmer_instance::<_, Never>(|instance| {
                let err: VmError = match get_remaining_points(instance) {
                    MeteringPoints::Remaining(_) => VmError::from(runtime_err),
                    MeteringPoints::Exhausted => VmError::gas_depletion(),
                };
                Err(err)
            })
            .unwrap_err()
        })
    }

    pub fn call_function0(&self, name: &str, args: &[Val]) -> VmResult<()> {
        let result = self.call_function(name, args)?;
        let expected = 0;
        let actual = result.len();
        if actual != expected {
            return Err(VmError::result_mismatch(name, expected, actual));
        }
        Ok(())
    }

    pub fn call_function1(&self, name: &str, args: &[Val]) -> VmResult<(Val)> {
        let result = self.call_function(name, args)?;
        let expected = 1;
        let actual = result.len();
        if actual != expected {
            return Err(VmError::result_mismatch(name, expected, actual));
        }
        Ok((result[0].clone()))
    }

    pub fn set_wasmer_instance(&self, wasmer_instance: Option<NonNull<WasmerInstance>>) {
        self.with_context_data_mut(|context_data| {
            context_data.wasmer_instance = wasmer_instance;
        });
    }

    pub fn get_gas_left(&self) -> u64 {
        self.with_wasmer_instance(|instance| {
            Ok(match get_remaining_points(instance) {
                MeteringPoints::Remaining(count) => count,
                MeteringPoints::Exhausted => 0,
            })
        })
        .expect("Wasmer instance is not set. Should set instance first")
    }

    pub fn set_gas_left(&self, gas_limit: u64) {
        self.with_wasmer_instance(|instance| {
            set_remaining_points(instance, gas_limit);
            Ok(())
        })
        .expect("Wasmer instance is not set. Should set instance first.");
    }
}

pub struct ContextData<S: Storage, Q: Querier> {
    gas_state: GasState,
    storage: Option<S>,
    storage_readonly: bool,
    querier: Option<Q>,
    wasmer_instance: Option<NonNull<WasmerInstance>>,
}

impl<S: Storage, Q: Querier> ContextData<S, Q> {
    pub fn new(gas_limit: u64) -> Self {
        ContextData::<S, Q> {
            gas_state: GasState::with_limit(gas_limit),
            storage: None,
            storage_readonly: true,
            querier: None,
            wasmer_instance: None,
        }
    }
}
