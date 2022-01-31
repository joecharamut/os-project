#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct PhysAddr(u64);

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct VirtAddr(u64);

#[derive(Debug)]
pub struct InvalidVirtAddr(u64);

impl VirtAddr {
    /// create a new canonical virtual address
    pub fn new(addr: u64) -> VirtAddr {
        Self::try_new(addr).expect("Virtual address must be sign extended in bits 48 to 64")
    }

    /// try to create a new virtual address
    pub fn try_new(addr: u64) -> Result<VirtAddr, InvalidVirtAddr> {
        // top 16 bits must be copies of bit 48
        match addr & 0b1111_1111_1111_1111 {
            0b0000_0000_0000_0000 | 0b1111_1111_1111_1111 => Ok(VirtAddr(addr)),
            other => Err(InvalidVirtAddr(other)),
        }
    }
}

