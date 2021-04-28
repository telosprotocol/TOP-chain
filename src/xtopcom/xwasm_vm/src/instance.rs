use std::ptr::NonNull;

use wasmer::{Exports, Function, ImportObject, Instance as WasmerInstance, Module, Val};

use crate::errors::{VmError, VmResult};
use crate::imports::{native_c_call, native_c_depoly};
use crate::runtime::Runtime;
use crate::size::Size;

use crate::wasm_backend::compile;

#[derive(Copy, Clone, Debug)]
pub struct GasReport {
    pub limit: u64,
    pub remaining: u64,
    pub used_externally: u64,
    pub used_internally: u64,
}

#[derive(Copy, Clone, Debug)]
pub struct InstanceOptions {
    pub gas_limit: u64,
    pub print_debug: bool,
}

pub struct Instance {
    _inner: Box<WasmerInstance>,
    runtime: Runtime,
}

impl Instance {
    pub fn from_code(
        code: &[u8],
        options: InstanceOptions,
        memory_limit: Option<Size>,
    ) -> VmResult<Self> {
        let module = compile(code, memory_limit)?;
        Instance::from_module(&module, options.gas_limit, options.print_debug)
    }

    pub fn from_module(module: &Module, gas_limit: u64, print_debug: bool) -> VmResult<Self> {
        let store = module.store();
        let runtime = Runtime::new(gas_limit, print_debug);

        let mut import_obj = ImportObject::new();
        let mut env_imports = Exports::new();

        // todo insert import functions.
        env_imports.insert(
            "c_call",
            Function::new_native_with_env(store, runtime.clone(), native_c_call),
        );
        env_imports.insert(
            "c_depoly",
            Function::new_native_with_env(store, runtime.clone(), native_c_depoly),
        );

        import_obj.register("env", env_imports);

        let wasmer_instance = Box::from(WasmerInstance::new(module, &import_obj).map_err(
            |original| {
                VmError::instantiation_err(format!("Error instantiating module: :{:?}", original))
            },
        )?);

        let instance_ptr = NonNull::from(wasmer_instance.as_ref());
        runtime.set_wasmer_instance(Some(instance_ptr));
        runtime.set_gas_left(gas_limit);

        let instance = Instance {
            _inner: wasmer_instance,
            runtime,
        };
        Ok(instance)
    }

    pub fn get_gas_left(&self) -> u64 {
        self.runtime.get_gas_left()
    }

    pub fn set_gas_left(&self, gas_limit: u64) {
        self.runtime.set_gas_left(gas_limit);
    }

    #[allow(unused)]
    pub(crate) fn call_function0(&self, name: &str, args: &[Val]) -> VmResult<()> {
        self.runtime.call_function0(name, args)
    }
    pub(crate) fn call_function1(&self, name: &str, args: &[Val]) -> VmResult<Val> {
        self.runtime.call_function1(name, args)
    }
}
