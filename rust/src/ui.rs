//! When to flash LED (pairing / feedback / error); actual GPIO in C.

// Led flashes are triggered from state_machine via ffi::led_flash_*.
// This module can hold UI state (e.g. pairing mode) if needed.
