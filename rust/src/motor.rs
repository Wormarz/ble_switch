//! Motor action decisions; actual PWM in C (hw_glue).

// State machine drives motor via ffi::motor_move_to_on/off/stop.
// This module can hold config (e.g. motion timeout ms) if needed.
