use std::borrow::{Borrow, BorrowMut};
use std::ops::AddAssign;
use std::ptr::NonNull;
use std::sync::{Arc, RwLock};

use crate::errors::{VmError, VmResult};
use wasmer::HostEnvInitError;
use wasmer::{Instance as WasmerInstance, Val, WasmerEnv};
use wasmer_middlewares::metering::{get_remaining_points, set_remaining_points, MeteringPoints};

/// Never can never be instantiated.
/// Replace this with the [never primitive type](https://doc.rust-lang.org/std/primitive.never.html) when stable.
#[derive(Debug)]
pub enum Never {}

#[derive(Copy, Clone, Debug)]
pub struct GasInfo {
    /// The gas cost of a computation that was executed already but not yet charged
    pub cost: u64,
    /// Gas that was used and charged externally.
    pub externally_used: u64,
}

impl GasInfo {
    #[allow(unused)]
    pub fn new(cost: u64, externally_used: u64) -> Self {
        GasInfo {
            cost,
            externally_used,
        }
    }
    pub fn with_cost(amount: u64) -> Self {
        GasInfo {
            cost: amount,
            externally_used: 0,
        }
    }
    #[allow(unused)]
    pub fn with_externally_used(amount: u64) -> Self {
        GasInfo {
            cost: 0,
            externally_used: amount,
        }
    }

    #[allow(unused)]
    pub fn free() -> Self {
        GasInfo {
            cost: 0,
            externally_used: 0,
        }
    }
}

impl AddAssign for GasInfo {
    fn add_assign(&mut self, other: Self) {
        *self = GasInfo {
            cost: self.cost + other.cost,
            externally_used: self.externally_used + other.cost,
        }
    }
}

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

pub struct Runtime {
    pub print_debug: bool,
    pub gas_config: GasConfig,
    data: Arc<RwLock<ContextData>>,

}

impl Clone for Runtime {
    fn clone(&self) -> Self {
        Runtime {
            print_debug: self.print_debug,
            gas_config: self.gas_config.clone(),
            data: self.data.clone(),
        }
    }
}

unsafe impl Send for Runtime {}
unsafe impl Sync for Runtime {}

impl WasmerEnv for Runtime {
    fn init_with_instance(&mut self, _instance: &WasmerInstance) -> Result<(), HostEnvInitError> {
        Ok(())
    }
}

impl Runtime {
    pub fn new(gas_limit: u64, print_debug: bool) -> Self {
        Runtime {
            print_debug,
            gas_config: GasConfig::default(),
            data: Arc::new(RwLock::new(ContextData::new(gas_limit))),
        }
    }

    pub fn with_context_data_mut<C, R>(&self, callback: C) -> R
    where
        C: FnOnce(&mut ContextData) -> R,
    {
        let mut guard = self.data.as_ref().write().unwrap();
        let context_data = guard.borrow_mut();
        callback(context_data)
    }
    pub fn with_context_data<C, R>(&self, callback: C) -> R
    where
        C: FnOnce(&ContextData) -> R,
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

    #[allow(unused)]
    pub fn call_function0(&self, name: &str, args: &[Val]) -> VmResult<()> {
        let result = self.call_function(name, args)?;
        let expected = 0;
        let actual = result.len();
        if actual != expected {
            return Err(VmError::result_mismatch(name, expected, actual));
        }
        Ok(())
    }

    pub fn call_function1(&self, name: &str, args: &[Val]) -> VmResult<Val> {
        let result = self.call_function(name, args)?;
        let expected = 1;
        let actual = result.len();
        if actual != expected {
            return Err(VmError::result_mismatch(name, expected, actual));
        }
        Ok(result[0].clone())
    }

    pub fn with_gas_state_mut<C, R>(&self, callback: C) -> R
    where
        C: FnOnce(&mut GasState) -> R,
    {
        self.with_context_data_mut(|context_data| callback(&mut context_data.gas_state))
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

pub struct ContextData {
    gas_state: GasState,
    // storage_readonly: bool,
    wasmer_instance: Option<NonNull<WasmerInstance>>,
}

impl ContextData {
    pub fn new(gas_limit: u64) -> Self {
        ContextData {
            gas_state: GasState::with_limit(gas_limit),
            // storage_readonly: true,
            wasmer_instance: None,
        }
    }
}

pub fn process_gas_info(runtime: &Runtime, info: GasInfo) -> VmResult<()> {
    // Ok(())
    let gas_left = runtime.get_gas_left();

    let new_limit = runtime.with_gas_state_mut(|gas_state| {
        gas_state.externally_used_gas += info.externally_used;
        // These lines reduce the amount of gas available to wasmer
        // so it can not consume gas that was consumed externally.
        gas_left
            .saturating_sub(info.externally_used)
            .saturating_sub(info.cost)
    });

    // This tells wasmer how much more gas it can consume from this point in time.
    runtime.set_gas_left(new_limit);

    if info.externally_used + info.cost > gas_left {
        Err(VmError::gas_depletion())
    } else {
        Ok(())
    }
}
