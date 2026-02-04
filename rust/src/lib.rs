#![no_std]

mod ffi;
mod state_machine;
mod storage;
mod motor;
mod ui;
mod power;

/// Called from C at boot (SYS_INIT).
#[no_mangle]
pub extern "C" fn rust_app_init() {
    state_machine::init();
}

// ---------- C-callable Rust API (used by ble_svc.c, button.c, timer_glue.c) ----------

#[no_mangle]
pub extern "C" fn rust_handle_switch_control(value: u8) {
    state_machine::handle_switch_control(value);
}

#[no_mangle]
pub extern "C" fn rust_get_switch_state() -> u8 {
    state_machine::get_switch_state()
}

#[no_mangle]
pub extern "C" fn rust_get_battery_level() -> u8 {
    power::get_battery_level()
}

#[no_mangle]
pub extern "C" fn rust_on_button_short() {
    state_machine::on_button_short();
}

#[no_mangle]
pub extern "C" fn rust_on_button_long() {
    state_machine::on_button_long();
}

#[no_mangle]
pub extern "C" fn rust_on_motion_complete() {
    state_machine::on_motion_complete();
}

#[no_mangle]
pub extern "C" fn rust_clear_error() {
    state_machine::clear_error();
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
