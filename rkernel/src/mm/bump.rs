use core::alloc::{Allocator, AllocError, GlobalAlloc, Layout};
use core::ptr::NonNull;

struct SimpleBumpAllocator {
    ptr: u64,
}

#[global_allocator]
static mut ALLOCATOR: SimpleBumpAllocator = SimpleBumpAllocator {
    ptr: 0xA00000,
};

unsafe impl GlobalAlloc for SimpleBumpAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let ret = ALLOCATOR.ptr;
        ALLOCATOR.ptr += layout.size() as u64;
        ret as *mut u8
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        todo!()
    }
}
