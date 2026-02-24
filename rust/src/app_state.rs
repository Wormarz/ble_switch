//! Single global application state; all mutable access through `with_state()`.
//! The only `unsafe` in this crate (besides ffi.rs) is below: init and access of `STATE`.
//! Safety: all Rust entry points (from C) run on the same Zephyr thread; no concurrent access.
#![allow(static_mut_refs)]

use core::cell::RefCell;

#[derive(Clone, Copy, PartialEq, Eq, Default)]
#[repr(u8)]
pub enum State {
    #[default]
    Idle = 0,
    MovingToOn = 1,
    MovingToOff = 2,
    Error = 3,
}

#[derive(Default)]
pub struct AppState {
    pub machine_state: State,
    /// 0 = off, 1 = on
    pub logical_state: u8,
    pub pairing_mode: bool,
}

static mut STATE: Option<RefCell<AppState>> = None;

/// Run a closure with exclusive mutable access to app state. No `unsafe` required by callers.
pub fn with_state<R, F>(f: F) -> R
where
    F: FnOnce(&mut AppState) -> R,
{
    unsafe {
        if STATE.is_none() {
            STATE = Some(RefCell::new(AppState::default()));
        }
        let cell = STATE.as_ref().unwrap();
        let mut borrow = cell.borrow_mut();
        f(&mut borrow)
    }
}
