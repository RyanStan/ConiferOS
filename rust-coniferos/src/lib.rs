// #![no_std] tells the Rust compiler not to link to the full standard library.
// We still will selectively use parts of the standard library like core that we compile from source
// (see config.toml).
#![no_std]
// #![crate_type = "staticlib"] generates a static archive file, .a, instead of a .rlib.
#![crate_type = "staticlib"]

use core::panic::PanicInfo;

// Callable from C. No mangle instructs the compiler to preserve symbol names.
// extern C tells the compiler ot use C calling conventions, thus making this callable from C.
#[no_mangle]
pub extern "C" fn rust_plus_2_callable(x: i32) -> i32 {
    x + 2
}

/// This function is called on panic.
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
