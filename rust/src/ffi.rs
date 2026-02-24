//! C ABI: raw declarations and safe wrappers.
//! All `unsafe` for FFI is confined to this module; the rest of the crate uses only the safe APIs.

extern "C" {
    fn motor_move_to_on();
    fn motor_move_to_off();
    fn motor_stop();
    fn motor_power_enable(enable: u8);
    fn led_flash_pairing();
    fn led_flash_feedback();
    fn led_flash_error();
    fn storage_read(out_value: *mut u8) -> i32;
    fn storage_write(value: u8) -> i32;
    fn battery_read_percent() -> u8;
    fn timer_glue_start_motion_timeout_ms(ms: u32);
    fn timer_glue_stop_motion_timeout();
    fn hw_factory_reset();
}

// ---------- Safe wrappers (only place that calls the C functions) ----------

pub fn motor_move_to_on_safe() {
    unsafe { motor_move_to_on() }
}

pub fn motor_move_to_off_safe() {
    unsafe { motor_move_to_off() }
}

pub fn motor_stop_safe() {
    unsafe { motor_stop() }
}

pub fn motor_power_enable_safe(enable: bool) {
    unsafe { motor_power_enable(if enable { 1 } else { 0 }) }
}

pub fn led_flash_pairing_safe() {
    unsafe { led_flash_pairing() }
}

pub fn led_flash_feedback_safe() {
    unsafe { led_flash_feedback() }
}

pub fn led_flash_error_safe() {
    unsafe { led_flash_error() }
}

/// Returns `Some(value)` if read succeeded (value is 0 or 1), `None` on error or missing.
pub fn storage_read_safe() -> Option<u8> {
    let mut v: u8 = 0;
    let ok = unsafe { storage_read(&mut v) == 0 };
    if ok {
        Some(v & 1)
    } else {
        None
    }
}

/// Returns true if write succeeded.
pub fn storage_write_safe(value: u8) -> bool {
    unsafe { storage_write(value) == 0 }
}

pub fn battery_read_percent_safe() -> u8 {
    unsafe { battery_read_percent() }
}

pub fn timer_glue_start_motion_timeout_ms_safe(ms: u32) {
    unsafe { timer_glue_start_motion_timeout_ms(ms) }
}

pub fn timer_glue_stop_motion_timeout_safe() {
    unsafe { timer_glue_stop_motion_timeout() }
}

pub fn hw_factory_reset_safe() {
    unsafe { hw_factory_reset() }
}
