use crate::x86_64::{PhysAddr, VirtAddr};

pub struct Pager;
pub struct PageMapError;

impl Pager {
    pub unsafe fn add_mapping(paddr: PhysAddr, vaddr: VirtAddr) -> Result<(), PageMapError> {
        todo!()
    }
}
