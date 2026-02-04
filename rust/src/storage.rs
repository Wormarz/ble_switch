//! Persistence policy (throttling); actual read/write via ffi to C NVS.

// Throttling can be added here (e.g. write at most every N actions).
// For now state_machine calls ffi::storage_write directly.
