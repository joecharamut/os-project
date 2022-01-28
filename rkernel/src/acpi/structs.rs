use core::ffi::c_void;
use core::intrinsics::transmute;
use crate::panic;

#[repr(C, packed)]
#[derive(Copy, Clone)]
pub struct AcpiRsdp {
    pub signature: [u8; 8],
    pub checksum: u8,
    pub oemid: [u8; 6],
    pub revision: u8,
    pub rsdt_address: u32,

    pub length: u32,
    pub xsdt_address: u64,
    pub extended_checksum: u8,
    _reserved: [u8; 3],
}

impl AcpiRsdp {
    pub unsafe fn get_xsdt(&self) -> *const ExtendedSystemDescriptionTable {
        let xsdt = self.xsdt_address as *const SystemDescriptionTableHeader;
        if (*xsdt).signature != "XSDT".as_bytes() {
            panic!("Invalid XSDT Header");
        }
        todo!()
    }
}

#[repr(C, packed)]
pub struct SystemDescriptionTableHeader {
    pub signature: [u8; 4],
    pub length: u32,
    pub revision: u8,
    pub checksum: u8,
    pub oemid: [u8; 6],
    pub oem_table_id: [u8; 8],
    pub oem_revision: u32,
    pub creator_id: u32,
    pub creator_revision: u32,
}

#[repr(C, packed)]
pub struct ExtendedSystemDescriptionTable {
    pub header: SystemDescriptionTableHeader,

    pub entries: [u64],
}

impl ExtendedSystemDescriptionTable {
    pub unsafe fn get_entry(&self, idx: u64) -> *const SystemDescriptionTableHeader {
        todo!()
    }
}

#[repr(u8)]
pub enum PowerManagementProfile {
    Unspecified = 0,
    Desktop = 1,
    Mobile = 2,
    Workstation = 3,
    EnterpriseServer = 4,
    SohoServer = 5,
    AppliancePc = 6,
    PerformanceServer = 7,
    Tablet = 8,
}

#[repr(C, packed)]
pub struct FixedACPIDescriptionTable {
    pub header: SystemDescriptionTableHeader,

    pub firmware_ctrl: u32,
    pub dsdt_address: u32,
    _reserved: u8,
    pub preferred_pm_profile: PowerManagementProfile,

    pub sci_interrupt_vector: u16,
    pub smi_command_port: u32,
    pub acpi_enable_value: u8,
    pub acpi_disable_value: u8,
    pub s4bios_request_value: u8,
    pub pstate_control_value: u8,

    pub pm1a_event_block_port: u32,
    pub pm1b_event_block_port: u32,

    pub pm1a_control_block_port: u32,
    pub pm1b_control_block_port: u32,
    pub pm2_control_block_port: u32,
    pub pm_timer_control_block_port: u32,


}
