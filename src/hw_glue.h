#ifndef HW_GLUE_H
#define HW_GLUE_H

#include <stdbool.h>
#include <stdint.h>

/* Motor (PWM/servo) - called from Rust */
void motor_move_to_on(void);
void motor_move_to_off(void);
void motor_stop(void);

/* Motor power enable - called from Rust */
void motor_power_enable(bool enable);

/* LED feedback - called from Rust */
void led_flash_pairing(void);
void led_flash_feedback(void);
void led_flash_error(void);

/* Factory reset helper - called from Rust UI code */
void hw_factory_reset(void);

/* Storage (NVS) - called from Rust. Physical state written only on save-state GPIO low. */
int storage_read_physical(uint8_t *out_value);
int storage_write_physical(uint8_t value);
int storage_read_orientation(uint8_t *out_value);
int storage_write_orientation(uint8_t value);

/* Call from main to enable save-state trigger: when GPIO goes low, physical state is written to NVS. */
void hw_save_state_trigger_init(void);

/* Battery - called from Rust; returns 0..100 */
uint8_t battery_read_percent(void);

#endif
