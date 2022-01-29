#![no_std]
#![feature(allocator_api)]
#![feature(alloc_error_handler)]

mod boot;
mod io;
mod mm;
mod pci;
mod acpi;
mod x86_64;

extern crate alloc;

use core::arch::asm;
use core::fmt::Write;
use alloc::vec::Vec;
use crate::boot::BootData;

#[no_mangle]
pub unsafe extern "C" fn kernel_main(boot_data_ptr: *mut BootData) -> ! {
    let boot_data = &*boot_data_ptr;
    if boot_data.signature != 0x534B52554E4B4C59 {
        panic!("Invalid boot signature");
    }

    boot::entry(boot_data);

    todo!();
    loop { asm!("hlt"); }
}

#[panic_handler]
unsafe fn panic(info: &core::panic::PanicInfo) -> ! {
    #[cfg(debug_assertions)] {
        let mut writer = io::serial::SerialWriter::new(io::serial::com1());
        write!(&mut writer, "\n\n!!! panic occurred: {} !!!", info).unwrap();
    }
    loop { asm!("pause"); }
}

#[alloc_error_handler]
fn alloc_error(layout: alloc::alloc::Layout) -> ! {
    panic!("alloc error");
}
