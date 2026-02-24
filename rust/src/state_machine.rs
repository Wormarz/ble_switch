//! Mechanical action state machine: Idle, MovingToOn, MovingToOff, Error.
//! All state lives in `app_state`; no `unsafe` in this module.

use crate::app_state;
use crate::ffi;
use crate::power;

pub use crate::app_state::State;

pub fn init() {
    app_state::with_state(|s| {
        s.machine_state = State::Idle;
        if let Some(v) = ffi::storage_read_safe() {
            s.logical_state = v;
        }
    });
}

pub fn handle_switch_control(value: u8) {
    match value {
        0 => request_off(),
        1 => request_on(),
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
                s.logical_state = 1;
                ffi::led_flash_feedback_safe();
                let _ = ffi::storage_write_safe(1);
            }
            State::MovingToOff => {
                s.logical_state = 0;
                ffi::led_flash_feedback_safe();
                let _ = ffi::storage_write_safe(0);
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

pub fn get_switch_state() -> u8 {
    app_state::with_state(|s| s.logical_state & 1)
}

/// Expose the raw state as a small integer for BLE status characteristic.
pub fn get_error_state() -> u8 {
    app_state::with_state(|s| s.machine_state as u8)
}
