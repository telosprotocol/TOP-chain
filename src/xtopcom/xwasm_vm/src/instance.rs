use std::ptr::NonNull;

use wasmer::{Exports, ImportObject, Instance as WasmerInstance, Module, Val};

use crate::backend::{Backend, BackendApi, Querier, Storage};
use crate::errors::{VmError, VmResult};
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

pub struct Instance<A: BackendApi, S: Storage, Q: Querier> {
    _inner: Box<WasmerInstance>,
    runtime: Runtime<A, S, Q>,
}

impl<A, S, Q> Instance<A, S, Q>
where
    A: BackendApi + 'static,
    S: Storage + 'static,
    Q: Querier + 'static,
{
    pub fn from_code(
        code: &[u8],
        backend: Backend<A, S, Q>,
        options: InstanceOptions,
        memory_limit: Option<Size>,
    ) -> VmResult<Self> {
        let module = compile(code, memory_limit)?;
        Instance::from_module(&module, backend, options.gas_limit, options.print_debug)
    }

    pub fn from_module(
        module: &Module,
        backend: Backend<A, S, Q>,
        gas_limit: u64,
        print_debug: bool,
    ) -> VmResult<Self> {
        let store = module.store();
        let runtime = Runtime::new(backend.api, gas_limit, print_debug);

        let mut import_obj = ImportObject::new();
        let mut env_imports = Exports::new();

        // todo insert import functions.
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

    pub(crate) fn call_function0(&self, name: &str, args: &[Val]) -> VmResult<()> {
        self.runtime.call_function0(name, args)
    }
    pub(crate) fn call_function1(&self, name: &str, args: &[Val]) -> VmResult<(Val)> {
        self.runtime.call_function1(name, args)
    }
}
