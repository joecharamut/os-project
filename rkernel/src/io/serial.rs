use core::fmt::Write;
use ioport::{outb, inb};

pub struct SerialPort {
    port_base: u16,
}

pub struct SerialWriter {
    port: &'static SerialPort,
}

struct Ports {
    com1: SerialPort,
}

static SERIAL_PORTS: Ports = Ports {
    com1: SerialPort { port_base: 0x3f8 },
};

pub fn com1() -> &'static SerialPort {
    &SERIAL_PORTS.com1
}

impl SerialPort {
    pub fn init_serial(&self) -> bool {
        unsafe {
            outb(self.port_base + 1, 0x00);    // Disable all interrupts
            outb(self.port_base + 3, 0x80);    // Enable DLAB (set baud rate divisor)
            outb(self.port_base + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
            outb(self.port_base + 1, 0x00);    //                  (hi byte)
            outb(self.port_base + 3, 0x03);    // 8 bits, no parity, one stop bit
            outb(self.port_base + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
            outb(self.port_base + 4, 0x0B);    // IRQs enabled, RTS/DSR set
            outb(self.port_base + 4, 0x1E);    // Set in loopback mode, test the serial chip
            outb(self.port_base + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

            // Check if serial is faulty (i.e: not same byte as sent)
            if inb(self.port_base + 0) != 0xAE {
                return false;
            }

            // If serial is not faulty set it in normal operation mode
            // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
            outb(self.port_base + 4, 0x0F);
            return true;
        }
    }

    pub fn write_char(&self, c: char) {
        unsafe {
            outb(self.port_base, c as u8);
        }
    }

    pub fn write_string(&self, s: &str) {
        for c in s.chars() {
            self.write_char(c);
        }
    }
}

impl SerialWriter {
    pub fn new(port: &'static SerialPort) -> SerialWriter {
        SerialWriter { port }
    }
}

impl Write for SerialWriter {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        self.port.write_string(s);
        Ok(())
    }
}
