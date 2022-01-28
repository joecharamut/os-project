use core::alloc;
use core::ptr::null_mut;

#[global_allocator]
static mut ALLOCATOR: KHeapAllocator = KHeapAllocator {};

struct KHeapAllocator {

}

unsafe impl alloc::GlobalAlloc for KHeapAllocator {
    unsafe fn alloc(&self, _layout: alloc::Layout) -> *mut u8 {
        null_mut()
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: alloc::Layout) {
        todo!()
    }
}
