#![no_std]
#![no_main]
#![allow(non_camel_case_types)]
use core::panic::PanicInfo;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
pub extern "C" fn __libc_start_main() {
    let mut msg: [core::ffi::c_char; 13] = [72, 101, 108, 108, 111, 44, 32, 119, 111, 114, 108, 100, 33];
    unsafe { printf(1, msg.as_mut_ptr()) };
}
