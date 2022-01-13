#![no_std]

mod boot;
mod io;

extern crate rlibc;

use core::arch::asm;
use crate::boot::BootData;
use core::fmt::Write;

#[no_mangle]
pub unsafe extern "C" fn kernel_main(boot_data_ptr: *mut BootData) -> ! {
    let boot_data = &*boot_data_ptr;
    boot::entry(boot_data);

    unimplemented!();
    loop { asm!("pause"); }
}

#[panic_handler]
unsafe fn panic(info: &core::panic::PanicInfo) -> ! {
    #[cfg(debug_assertions)] {
        let mut writer = io::serial::SerialWriter::new(io::serial::com1());

        write!(&mut writer, "\n\n!!! panic occurred: {} -- in file '{}' on line {} !!!",
               info.payload().downcast_ref::<&str>().unwrap_or(&"no message"),
               info.location().map_or("???", |l| l.file()),
               info.location().map_or(0, |l| l.line())
        ).unwrap();
    }
    loop { asm!("pause"); }
}
