use crate::io::port::{outl, inl};

pub unsafe fn pci_cfg_read_word(bus: u8, slot: u8, function: u8, offset: u8) -> u16 {
    const CONFIG_ADDRESS_PORT: u16 = 0xCF8;
    const CONFIG_DATA_PORT: u16 = 0xCFC;

    let address: u32 = 0x8000_0000 |
        ((bus as u32) << 16) |
        ((slot as u32) << 11) |
        ((function as u32) << 8) |
        ((offset as u32) & 0xFC);

    outl(CONFIG_ADDRESS_PORT, address);
    (inl(CONFIG_DATA_PORT) >> ((offset & 2) * 8)) as u16
}
