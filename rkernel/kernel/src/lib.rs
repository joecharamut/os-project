#![no_std]
#![no_main]

use core::ffi::c_void;


#[repr(C)]
struct VideoInfo {
    buffer_address: u64,
    buffer_size: u64,
    horizontal_resolution: u64,
    vertical_resolution: u64,
    pixels_per_scan_line: u64,
}

#[repr(C)]
struct MemoryInfo {
    count: u64,
    entries: *mut c_void,
}

#[repr(C)]
struct BootData {
    signature: u64,
    video_info: VideoInfo,
    memory_info: MemoryInfo,
}

#[no_mangle]
pub extern "C" fn kernel_main(boot_data_ptr: *mut c_void) {
    let boot_data = unsafe { &*(boot_data_ptr as *mut BootData) };
    let framebuffer = boot_data.video_info.buffer_address as *mut u32;

    for y in 0..boot_data.video_info.vertical_resolution {
        for x in 0..boot_data.video_info.horizontal_resolution {
            unsafe {
                core::ptr::write(
                    framebuffer
                        .add(x as usize)
                        .add((y * boot_data.video_info.pixels_per_scan_line) as usize),
                    0x0000FF00
                );
            }
        }
    }

    loop {}
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
