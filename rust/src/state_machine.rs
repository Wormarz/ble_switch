//! Mechanical action state machine: Idle, MovingToOn, MovingToOff, Error.

use crate::ffi;

#[derive(Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum State {
    Idle = 0,
    MovingToOn = 1,
    MovingToOff = 2,
    Error = 3,
}

static mut MACHINE_STATE: State = State::Idle;
static mut LOGICAL_STATE: u8 = 0; // 0 = off, 1 = on

pub fn init() {
    unsafe {
        MACHINE_STATE = State::Idle;
        let mut v: u8 = 0;
        if ffi::storage_read(&mut v) == 0 {
            LOGICAL_STATE = v & 1;
        }
    }
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
    unsafe {
        if MACHINE_STATE == State::Idle {
            MACHINE_STATE = State::MovingToOn;
            ffi::motor_power_enable(1);
            ffi::motor_move_to_on();
            ffi::timer_glue_start_motion_timeout_ms(MOTION_TIMEOUT_MS);
        }
    }
}

fn request_off() {
    unsafe {
        if MACHINE_STATE == State::Idle {
            MACHINE_STATE = State::MovingToOff;
            ffi::motor_power_enable(1);
            ffi::motor_move_to_off();
            ffi::timer_glue_start_motion_timeout_ms(MOTION_TIMEOUT_MS);
        }
    }
}

fn request_toggle() {
    unsafe {
        if LOGICAL_STATE != 0 {
            request_off();
        } else {
            request_on();
        }
    }
}

pub fn on_button_short() {
    request_toggle();
}

pub fn on_button_long() {
    unsafe {
        ffi::led_flash_pairing();
        // C side will switch to pairing / factory reset mode
    }
}

pub fn on_motion_complete() {
    unsafe {
        ffi::timer_glue_stop_motion_timeout();
        ffi::motor_stop();
        ffi::motor_power_enable(0);
        match MACHINE_STATE {
            State::MovingToOn => {
                LOGICAL_STATE = 1;
                ffi::led_flash_feedback();
                let _ = ffi::storage_write(1);
            }
            State::MovingToOff => {
                LOGICAL_STATE = 0;
                ffi::led_flash_feedback();
                let _ = ffi::storage_write(0);
            }
            _ => {}
        }
        MACHINE_STATE = State::Idle;
    }
}

pub fn clear_error() {
    unsafe {
        MACHINE_STATE = State::Idle;
    }
}

pub fn get_switch_state() -> u8 {
    unsafe { LOGICAL_STATE & 1 }
}
