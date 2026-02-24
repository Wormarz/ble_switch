//! Battery level and motor power policy; ADC/GPIO in C.
//! Uses only safe ffi wrappers; no `unsafe` in this module.

use crate::ffi;

/// Threshold below which motor actions are disabled (in percent).
const LOW_BATTERY_THRESHOLD_PERCENT: u8 = 10;

pub fn get_battery_level() -> u8 {
    ffi::battery_read_percent_safe()
}

/// Return true if the battery is considered too low for motor movement.
pub fn is_battery_too_low_for_motion() -> bool {
    get_battery_level() <= LOW_BATTERY_THRESHOLD_PERCENT
}
