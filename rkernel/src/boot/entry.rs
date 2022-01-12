use core::ffi::c_void;
use crate::io;

#[repr(C)]
pub struct VideoInfo {
    pub buffer_address: u64,
    pub buffer_size: u64,
    pub horizontal_resolution: u64,
    pub vertical_resolution: u64,
    pub pixels_per_scan_line: u64,
}

#[repr(C)]
pub struct MemoryInfo {
    pub count: u64,
    pub entries: *mut c_void,
}

#[repr(C)]
pub struct BootData {
    pub signature: u64,
    pub video_info: VideoInfo,
    pub memory_info: MemoryInfo,
}

pub fn entry(boot_data: &BootData) {
    let framebuffer = boot_data.video_info.buffer_address as *mut u32;

    let port = io::serial::com1();
    port.write_string("Hello, World!");

    for y in 0..boot_data.video_info.vertical_resolution {
        for x in 0..boot_data.video_info.horizontal_resolution {
            let byte = (x + (y * boot_data.video_info.horizontal_resolution)) as isize;
            unsafe {
                core::ptr::write(framebuffer.offset(byte),0x0000FF00);
            }
        }
    }
}
