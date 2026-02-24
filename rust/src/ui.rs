//! UI behaviour for pairing / factory reset.
//! Actual LED GPIO is handled in C via safe ffi wrappers.

use crate::app_state;
use crate::ffi;

/// Enter pairing / factory-reset UI mode.
///
/// Flashes the pairing LED and clears stored logical state via C.
/// BLE pairing/bonding wipe can be added later on the C side if needed.
pub fn enter_pairing_or_factory_reset() {
    app_state::with_state(|s| {
        s.pairing_mode = true;
    });
    ffi::led_flash_pairing_safe();
    ffi::hw_factory_reset_safe();
}

/// Returns true if we are in pairing mode.
#[allow(dead_code)]
pub fn is_pairing_mode() -> bool {
    app_state::with_state(|s| s.pairing_mode)
}
