use crate::errors::{VmError, VmResult};

use wasmer::{Array, ValueType, WasmPtr};

#[repr(C)]
#[derive(Default, Clone, Copy, Debug)]
struct Region {
    /// The beginning of the region expressed as bytes from the beginning of the linear memory
    pub offset: u32,
    /// The number of bytes available in this region
    pub capacity: u32,
    /// The number of bytes used in this region
    pub length: u32,
}
unsafe impl ValueType for Region {}

/// A prepared and sufficiently large memory Region is expected at ptr that points to pre-allocated memory.
///
/// Returns number of bytes written on success.
pub fn write_region(memory: &wasmer::Memory, ptr: u32, data: &[u8]) -> VmResult<()> {
    let mut region = get_region(memory, ptr)?;

    let region_capacity = region.capacity as usize;
    if data.len() > region_capacity {
        return Err(VmError::runtime_err("write_region msg 1"));
    }
    match WasmPtr::<u8, Array>::new(region.offset).deref(memory, 0, region.capacity) {
        Some(cells) => {
            // In case you want to do some premature optimization, this shows how to cast a `&'mut [Cell<u8>]` to `&mut [u8]`:
            // https://github.com/wasmerio/wasmer/blob/0.13.1/lib/wasi/src/syscalls/mod.rs#L79-L81
            for i in 0..data.len() {
                cells[i].set(data[i])
            }
            region.length = data.len() as u32;
            set_region(memory, ptr, region)?;
            Ok(())
        }
        None => Err(VmError::runtime_err("write_region msg 2")),
    }
}

/// Reads in a Region at ptr in wasm memory and returns a copy of it
fn get_region(memory: &wasmer::Memory, ptr: u32) -> VmResult<Region> {
    let wptr = WasmPtr::<Region>::new(ptr);
    println!("get_region at : {:?}", memory);
    match wptr.deref(memory) {
        Some(cell) => {
            let region = cell.get();
            validate_region(&region)?;
            Ok(region)
        }
        None => Err(VmError::runtime_err("get_region msg 1")),
    }
}

/// Performs plausibility checks in the given Region. Regions are always created by the
/// contract and this can be used to detect problems in the standard library of the contract.
fn validate_region(region: &Region) -> VmResult<()> {
    println!("validate_region : {:?}", region);
    if region.offset == 0 {
        return Err(VmError::runtime_err("validate_region msg 1"));
    }
    if region.length > region.capacity {
        return Err(VmError::runtime_err("validate_region msg 2"));
    }
    if region.capacity > (u32::MAX - region.offset) {
        return Err(VmError::runtime_err("validate_region msg 3"));
    }
    Ok(())
}

/// Overrides a Region at ptr in wasm memory with data
fn set_region(memory: &wasmer::Memory, ptr: u32, data: Region) -> VmResult<()> {
    let wptr = WasmPtr::<Region>::new(ptr);

    match wptr.deref(memory) {
        Some(cell) => {
            cell.set(data);
            Ok(())
        }
        None => Err(VmError::runtime_err("set_region msg 1")),
    }
}
