use core::ffi::c_void;
use crate::io;
use core::fmt::{Display, Formatter, Write};
use core::ptr::slice_from_raw_parts;

#[repr(C)]
#[derive(Copy, Clone)]
pub struct BootData {
    pub signature: u64,
    pub video_info: VideoInfo,
    pub memory_info: MemoryInfo,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct VideoInfo {
    pub buffer_address: u64,
    pub buffer_size: u64,
    pub horizontal_resolution: u64,
    pub vertical_resolution: u64,
    pub pixels_per_scan_line: u64,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct MemoryInfo {
    pub count: u64,
    pub entries: *mut MemoryDescriptor,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct MemoryDescriptor {
    pub descriptor_type: u32,
    pub padding: u32,
    pub physical_address: u64,
    pub virtual_address: u64,
    pub number_of_pages: u64,
    pub flags: u64,
}

impl Display for MemoryDescriptor {
    fn fmt(&self, f: &mut Formatter<'_>) -> core::fmt::Result {
        write!(f, "type: {}, paddr: {:#016x}, vaddr: {:#016x}, pages: {}, flags: {:#x}",
               self.descriptor_type, self.physical_address, self.virtual_address,
               self.number_of_pages, self.flags
        )
    }
}

pub fn entry(boot_data: &BootData) {
    let framebuffer = boot_data.video_info.buffer_address as *mut u32;
    let mut writer = io::serial::SerialWriter::new(io::serial::com1());

    writeln!(&mut writer, "Hello, World!").unwrap();
    writeln!(&mut writer, "Hello, World!").unwrap();
    writeln!(&mut writer, "Hello, World!").unwrap();
    writeln!(&mut writer, "Hello, World!").unwrap();

    for y in 0..boot_data.video_info.vertical_resolution {
        for x in 0..boot_data.video_info.horizontal_resolution {
            let byte = (x + (y * boot_data.video_info.horizontal_resolution)) as isize;
            unsafe {
                core::ptr::write(framebuffer.offset(byte),0xffAAAAAA);
            }
        }
    }

    let entries = unsafe { slice_from_raw_parts(boot_data.memory_info.entries, boot_data.memory_info.count as usize) };
    for i in 0..boot_data.memory_info.count {
        let entry: MemoryDescriptor = unsafe { (*entries)[i as usize] };
        writeln!(&mut writer, "Entry {}: [{}]", i, entry).unwrap();
    }
}
