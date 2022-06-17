use std::mem;
use std::vec::Vec;

/// Describes some data allocated in Wasm's linear memory.
/// A pointer to an instance of this can be returned over FFI boundaries.
///
/// This struct is crate internal since the cosmwasm-vm defines the same type independently.
#[repr(C)]
pub struct Region {
    /// The beginning of the region expressed as bytes from the beginning of the linear memory
    pub offset: u32,
    /// The number of bytes available in this region
    pub capacity: u32,
    /// The number of bytes used in this region
    pub length: u32,
}

/// Creates a memory region of capacity `size` and length 0. Returns a pointer to the Region.
/// This is the same as the `allocate` export, but designed to be called internally.
pub fn alloc(size: usize) -> *mut Region {
    let data: Vec<u8> = Vec::with_capacity(size);
    let data_ptr = data.as_ptr() as usize;

    let region = build_region_from_components(
        u32::try_from(data_ptr).expect("pointer doesn't fit in u32"),
        u32::try_from(data.capacity()).expect("capacity doesn't fit in u32"),
        0,
    );
    mem::forget(data);
    Box::into_raw(region)
}

/// Similar to alloc, but instead of creating a new vector it consumes an existing one and returns
/// a pointer to the Region (preventing the memory from being freed until explicitly called later).
///
/// The resulting Region has capacity = length, i.e. the buffer's capacity is ignored.
pub fn release_buffer(buffer: Vec<u8>) -> *mut Region {
    let region = build_region(&buffer);
    mem::forget(buffer);
    Box::into_raw(region)
}

/// Return the data referenced by the Region and
/// deallocates the Region (and the vector when finished).
/// Warning: only use this when you are sure the caller will never use (or free) the Region later
///
/// # Safety
///
/// The ptr must refer to a valid Region, which was previously returned by alloc,
/// and not yet deallocated. This call will deallocate the Region and return an owner vector
/// to the caller containing the referenced data.
///
/// Naturally, calling this function twice on the same pointer will double deallocate data
/// and lead to a crash. Make sure to call it exactly once (either consuming the input in
/// the wasm code OR deallocating the buffer from the caller).
pub unsafe fn consume_region(ptr: *mut Region) -> Vec<u8> {
    assert!(!ptr.is_null(), "Region pointer is null");
    let region = Box::from_raw(ptr);

    let region_start = region.offset as *mut u8;
    // This case is explicitely disallowed by Vec
    // "The pointer will never be null, so this type is null-pointer-optimized."
    assert!(!region_start.is_null(), "Region starts at null pointer");

    Vec::from_raw_parts(
        region_start,
        region.length as usize,
        region.capacity as usize,
    )
}

/// Returns a box of a Region, which can be sent over a call to extern
/// note that this DOES NOT take ownership of the data, and we MUST NOT consume_region
/// the resulting data.
/// The Box must be dropped (with scope), but not the data
pub fn build_region(data: &[u8]) -> Box<Region> {
    let data_ptr = data.as_ptr() as usize;
    build_region_from_components(
        u32::try_from(data_ptr).expect("pointer doesn't fit in u32"),
        u32::try_from(data.len()).expect("length doesn't fit in u32"),
        u32::try_from(data.len()).expect("length doesn't fit in u32"),
    )
}

fn build_region_from_components(offset: u32, capacity: u32, length: u32) -> Box<Region> {
    Box::new(Region {
        offset,
        capacity,
        length,
    })
}

extern "C" {
    fn get_args() -> u32;

}

#[no_mangle]
pub extern "C" fn allocate(size: usize) -> u32 {
    alloc(size) as u32
}

/// deallocate expects a pointer to a Region created with allocate.
/// It will free both the Region and the memory referenced by the Region.
#[no_mangle]
pub extern "C" fn deallocate(pointer: u32) {
    // auto-drop Region on function end
    let _ = unsafe { consume_region(pointer as *mut Region) };
}

#[no_mangle]
fn test() -> u32 {
    // let result = 0;
    unsafe {
        // let data: Vec<u8> = get_args1();
        let region_ptr = get_args();
        let data = consume_region(region_ptr as *mut Region);
        if data.len() != 6 {
            return 1;
        }
        if data == vec![49, 50, 51, 52, 53, 54] {
            return 2;
        }
        return 3;
    }
}
