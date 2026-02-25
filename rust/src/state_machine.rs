//! Mechanical action state machine: Idle, MovingToOn, MovingToOff, Error.
//! All state lives in `app_state`; no `unsafe` in this module.

use crate::app_state;
use crate::ffi;
use crate::power;

pub use crate::app_state::State;

pub fn init() {
    app_state::with_state(|s| {
        s.machine_state = State::Idle;
        if let Some(v) = ffi::storage_read_physical_safe() {
            s.physical_state = v;
        }
        if let Some(v) = ffi::storage_read_orientation_safe() {
            s.orientation = v;
        }
    });
}

/// Logical "on" means physical 1 when orientation=0, physical 0 when orientation=1.
fn target_physical_for_logical_on() -> bool {
    app_state::with_state(|s| s.orientation == 0)
}

/// Logical "off" means physical 0 when orientation=0, physical 1 when orientation=1.
fn target_physical_for_logical_off() -> bool {
    app_state::with_state(|s| s.orientation != 0)
}

pub fn handle_switch_control(value: u8) {
    match value {
        0 => {
            if target_physical_for_logical_off() {
                request_off();
            } else {
                request_on();
            }
        }
        1 => {
            if target_physical_for_logical_on() {
                request_on();
            } else {
                request_off();
            }
        }
        2 => request_toggle(),
        _ => {}
    }
}

const MOTION_TIMEOUT_MS: u32 = 1500;

fn request_on() {
    app_state::with_state(|s| {
        if s.machine_state != State::Idle {
            return;
        }
        if power::is_battery_too_low_for_motion() {
            s.machine_state = State::Error;
            ffi::led_flash_error_safe();
            return;
        }
        s.machine_state = State::MovingToOn;
        ffi::motor_power_enable_safe(true);
        ffi::motor_move_to_on_safe();
        ffi::timer_glue_start_motion_timeout_ms_safe(MOTION_TIMEOUT_MS);
    });
}

fn request_off() {
    app_state::with_state(|s| {
        if s.machine_state != State::Idle {
            return;
        }
        if power::is_battery_too_low_for_motion() {
            s.machine_state = State::Error;
            ffi::led_flash_error_safe();
            return;
        }
        s.machine_state = State::MovingToOff;
        ffi::motor_power_enable_safe(true);
        ffi::motor_move_to_off_safe();
        ffi::timer_glue_start_motion_timeout_ms_safe(MOTION_TIMEOUT_MS);
    });
}

fn request_toggle() {
    let logical = get_switch_state();
    if logical != 0 {
        request_off();
    } else {
        request_on();
    }
}

pub fn on_button_short() {
    request_toggle();
}

pub fn on_button_long() {
    crate::ui::enter_pairing_or_factory_reset();
}

pub fn on_motion_complete() {
    app_state::with_state(|s| {
        ffi::timer_glue_stop_motion_timeout_safe();
        ffi::motor_stop_safe();
        ffi::motor_power_enable_safe(false);
        match s.machine_state {
            State::MovingToOn => {
                s.physical_state = 1;
                ffi::led_flash_feedback_safe();
            }
            State::MovingToOff => {
                s.physical_state = 0;
                ffi::led_flash_feedback_safe();
            }
            _ => {}
        }
        s.machine_state = State::Idle;
    });
}

pub fn clear_error() {
    app_state::with_state(|s| {
        s.machine_state = State::Idle;
    });
}

/// Logical state (user-visible on/off): derived from physical_state and orientation.
pub fn get_switch_state() -> u8 {
    app_state::with_state(|s| {
        let p = s.physical_state & 1;
        let o = s.orientation & 1;
        if o == 0 {
            p
        } else {
            1 - p
        }
    })
}

/// Logical↔physical mapping: 0 = normal, 1 = inverted. Persisted in NVS when set via BLE.
pub fn get_orientation() -> u8 {
    app_state::with_state(|s| s.orientation & 1)
}

/// Set logical↔physical mapping (0 or 1) and persist to NVS.
pub fn set_orientation(value: u8) {
    app_state::with_state(|s| {
        s.orientation = value & 1;
    });
    let _ = ffi::storage_write_orientation_safe(value & 1);
}

/// Called from C when save-state GPIO goes low: write current physical state to NVS (not on every change, to limit wear).
pub fn save_physical_state_to_nvs() {
    let physical = app_state::with_state(|s| s.physical_state & 1);
    let _ = ffi::storage_write_physical_safe(physical);
}

/// Expose the raw state as a small integer for BLE status characteristic.
pub fn get_error_state() -> u8 {
    app_state::with_state(|s| s.machine_state as u8)
}
