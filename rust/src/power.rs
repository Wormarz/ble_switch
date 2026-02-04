//! Battery level and motor power policy; ADC/GPIO in C.

use crate::ffi;

pub fn get_battery_level() -> u8 {
    unsafe { ffi::battery_read_percent() }
}
