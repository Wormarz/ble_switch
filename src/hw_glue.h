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

/* Storage (NVS) - called from Rust */
int storage_read(uint8_t *out_value);
int storage_write(uint8_t value);

/* Battery - called from Rust; returns 0..100 */
uint8_t battery_read_percent(void);

#endif
