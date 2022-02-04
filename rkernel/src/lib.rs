#![no_std]
#![feature(allocator_api)]
#![feature(alloc_error_handler)]

extern crate bit_field;
extern crate alloc;

mod boot;
mod io;
mod mm;
mod pci;
mod acpi;
mod x86_64;

use core::arch::asm;
use core::fmt::Write;
use alloc::vec::Vec;
use crate::boot::BootData;
use crate::x86_64::VirtAddr;

#[no_mangle]
pub unsafe extern "C" fn kernel_main(boot_data_ptr: u64) -> ! {
    let ptr = VirtAddr::new(boot_data_ptr);
    let boot_data: BootData = unsafe { *ptr.as_ptr() };

    // technically a redundant check because of bootstrap.c
    if boot_data.signature != 0x534B52554E4B4C59 {
        panic!("Invalid boot signature");
    }

    boot::entry(&boot_data);

    todo!();
    loop { asm!("hlt"); }
}

#[panic_handler]
unsafe fn panic(info: &core::panic::PanicInfo) -> ! {
    #[cfg(debug_assertions)] {
        let mut writer = io::serial::SerialWriter::new(io::serial::com1());
        write!(&mut writer, "\n\n!!! panic occurred: {} !!!", info).unwrap();
    }
    loop { asm!("hlt"); }
}

#[alloc_error_handler]
fn alloc_error(layout: alloc::alloc::Layout) -> ! {
    panic!("alloc error");
}
