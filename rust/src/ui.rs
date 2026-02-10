//! UI behaviour for pairing / factory reset.
//! Actual LED GPIO is handled in C via ffi::led_flash_*.

use crate::ffi;

static mut PAIRING_MODE: bool = false;

/// Enter pairing / factory-reset UI mode.
///
/// Currently this just flashes the pairing LED pattern and clears
/// stored logical state via a call into C. BLE pairing/bonding wipe
/// can be added later on the C side if needed.
pub fn enter_pairing_or_factory_reset() {
    unsafe {
        PAIRING_MODE = true;
        ffi::led_flash_pairing();
        ffi::hw_factory_reset();
    }
}

/// Check if we are in pairing mode.
pub fn is_pairing_mode() -> bool {
    unsafe { PAIRING_MODE }
}
