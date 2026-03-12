/* LED indicator control.
 * High-level LED policies for pairing, feedback, error, and advertising
 * indication.
 */

#include "led.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(ui_led, LOG_LEVEL_INF);

#if DT_NODE_EXISTS(DT_ALIAS(led0))
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static struct k_timer led_off_timer;
static struct k_timer led_adv_blink_timer;
static bool led_timer_init_done;
static bool led_adv_blinking;

static void led_off_handler(struct k_timer *t)
{
	ARG_UNUSED(t);
	gpio_pin_set_dt(&led, 0);
}

static void led_flash_ms(uint32_t ms)
{
	if (!gpio_is_ready_dt(&led)) {
		return;
	}
	if (!led_timer_init_done) {
		k_timer_init(&led_off_timer, led_off_handler, NULL);
		led_timer_init_done = true;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	gpio_pin_set_dt(&led, 1);
	k_timer_start(&led_off_timer, K_MSEC(ms), K_NO_WAIT);
}

static void led_adv_blink_handler(struct k_timer *t)
{
	ARG_UNUSED(t);
	if (!led_adv_blinking) {
		return;
	}
	int val = gpio_pin_get_dt(&led);
	gpio_pin_set_dt(&led, !val);
}

void led_flash_pairing(void)
{
	LOG_INF("LED pairing flash");
	led_flash_ms(100);
}

void led_flash_feedback(void)
{
	led_flash_ms(80);
}

void led_flash_error(void)
{
	led_flash_ms(200);
}

void led_adv_blink_start(void)
{
	if (!gpio_is_ready_dt(&led)) {
		return;
	}
	if (!led_timer_init_done) {
		k_timer_init(&led_off_timer, led_off_handler, NULL);
		led_timer_init_done = true;
	}
	k_timer_init(&led_adv_blink_timer, led_adv_blink_handler, NULL);
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	led_adv_blinking = true;
	/* Blink at ~2 Hz: toggle every 250 ms */
	k_timer_start(&led_adv_blink_timer, K_NO_WAIT, K_MSEC(250));
}

void led_adv_blink_stop(void)
{
	if (!gpio_is_ready_dt(&led)) {
		return;
	}
	led_adv_blinking = false;
	k_timer_stop(&led_adv_blink_timer);
	gpio_pin_set_dt(&led, 0);
}

#else /* !DT_ALIAS(led0) */

static void led_flash_ms(uint32_t ms) { ARG_UNUSED(ms); }
void led_flash_pairing(void) { }
void led_flash_feedback(void) { }
void led_flash_error(void) { }
void led_adv_blink_start(void) { }
void led_adv_blink_stop(void) { }

#endif

