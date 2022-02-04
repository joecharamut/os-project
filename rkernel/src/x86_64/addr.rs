use bit_field::BitField;

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct PhysAddr(u64);

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[repr(transparent)]
pub struct VirtAddr(u64);

#[derive(Debug)]
pub struct InvalidVirtAddr(u64);

#[allow(dead_code)]
impl VirtAddr {
    /// create a new canonical virtual address
    #[inline]
    pub fn new(addr: u64) -> VirtAddr {
        Self::try_new(addr).expect("Virtual address must be sign extended in bits 48 to 64")
    }

    /// try to create a new virtual address
    #[inline]
    pub fn try_new(addr: u64) -> Result<VirtAddr, InvalidVirtAddr> {
        // top 16 bits must be copies of bit 48
        match addr.get_bits(47..64) {
            0b0000_0000_0000_0000_0 | 0b1111_1111_1111_1111_1 => Ok(VirtAddr(addr)),
            other => Err(InvalidVirtAddr(other)),
        }
    }

    #[inline]
    pub const fn zero() -> VirtAddr {
        VirtAddr(0)
    }

    #[inline]
    pub const fn as_u64(self) -> u64 {
        self.0
    }

    #[inline]
    pub unsafe fn as_ptr<T>(self) -> *const T {
        self.as_u64() as *const T
    }

    #[inline]
    pub unsafe fn as_mut_ptr<T>(self) -> *mut T {
        self.as_u64() as *mut T
    }

    /// return the lower 12 bits -- page offset
    #[inline]
    pub const fn page_offset(self) -> u16 {
        (self.0 & 0xfff) as u16
    }

    /// return the index for the nth page level
    #[inline]
    pub const fn nth_level_table_index(self, level: u8) -> u16 {
        // shift out the 12 bit page offset, then 9 bits for each previous level
        ((self.0 >> 12 >> ((level - 1) * 9)) & 0x1ff) as u16
    }

    /// return the index for the first level page table (page table)
    #[inline]
    pub const fn p1_index(self) -> u16 {
        self.nth_level_table_index(1)
    }

    /// return the index for the second level page table (page directory)
    #[inline]
    pub const fn p2_index(self) -> u16 {
        self.nth_level_table_index(2)
    }

    /// return the index for the third level page table (page directory pointer table)
    #[inline]
    pub const fn p3_index(self) -> u16 {
        self.nth_level_table_index(3)
    }

    /// return the index for the fourth level page table (page map level 4)
    #[inline]
    pub const fn p4_index(self) -> u16 {
        self.nth_level_table_index(4)
    }
}

