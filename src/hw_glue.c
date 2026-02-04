#include "hw_glue.h"
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

/* Motor: stubs until PWM/servo wired */
void motor_move_to_on(void) { (void)0; }
void motor_move_to_off(void) { (void)0; }
void motor_stop(void) { (void)0; }
void motor_power_enable(bool enable) { (void)enable; }

/* LED: short flash if led0 exists (e.g. nRF52840 DK LED1) */
#if DT_NODE_EXISTS(DT_ALIAS(led0))
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static struct k_timer led_off_timer;
static bool led_timer_init_done;
static void led_off_handler(struct k_timer *t)
{
	ARG_UNUSED(t);
	gpio_pin_set_dt(&led, 0);
}
static void led_flash_ms(uint32_t ms)
{
	if (!gpio_is_ready_dt(&led)) return;
	if (!led_timer_init_done) {
		k_timer_init(&led_off_timer, led_off_handler, NULL);
		led_timer_init_done = true;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_set_dt(&led, 1);
	k_timer_start(&led_off_timer, K_MSEC(ms), K_NO_WAIT);
}
#else
static void led_flash_ms(uint32_t ms) { (void)ms; }
#endif

void led_flash_pairing(void)
{
	led_flash_ms(100);
}

void led_flash_feedback(void) { led_flash_ms(80); }
void led_flash_error(void) { led_flash_ms(200); }

int storage_read(uint8_t *out_value)
{
	if (out_value) *out_value = 0;
	return 0;
}
int storage_write(uint8_t value) { (void)value; return 0; }

uint8_t battery_read_percent(void) { return 100; }
