//! C ABI: declarations of C functions called from Rust (implemented in hw_glue.c / timer_glue.c).

extern "C" {
    pub fn motor_move_to_on();
    pub fn motor_move_to_off();
    pub fn motor_stop();
    pub fn motor_power_enable(enable: u8);
    pub fn led_flash_pairing();
    pub fn led_flash_feedback();
    pub fn led_flash_error();
    pub fn storage_read(out_value: *mut u8) -> i32;
    pub fn storage_write(value: u8) -> i32;
    pub fn battery_read_percent() -> u8;
    pub fn timer_glue_start_motion_timeout_ms(ms: u32);
    pub fn timer_glue_stop_motion_timeout();
}
