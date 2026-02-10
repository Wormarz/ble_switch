#include "hw_glue.h"
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/drivers/adc.h>

/* Motor: simple GPIO-based H-bridge style control using GPIO0 pins.
 * Two pins (IN1/IN2) drive ON/OFF direction.
 */

#define MOTOR_IN1_PIN 13
#define MOTOR_IN2_PIN 14

static const struct device *motor_port = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static bool motor_pins_inited;

static void motor_init_pins(void)
{
	if (motor_pins_inited) {
		return;
	}
	if (!device_is_ready(motor_port)) {
		return;
	}
	gpio_pin_configure(motor_port, MOTOR_IN1_PIN, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure(motor_port, MOTOR_IN2_PIN, GPIO_OUTPUT_INACTIVE);
	motor_pins_inited = true;
}

void motor_move_to_on(void)
{
	motor_init_pins();
	if (!motor_pins_inited) {
		return;
	}
	/* Drive ON direction: IN1=1, IN2=0 */
	gpio_pin_set(motor_port, MOTOR_IN1_PIN, 1);
	gpio_pin_set(motor_port, MOTOR_IN2_PIN, 0);
}

void motor_move_to_off(void)
{
	motor_init_pins();
	if (!motor_pins_inited) {
		return;
	}
	/* Drive OFF direction: IN1=0, IN2=1 */
	gpio_pin_set(motor_port, MOTOR_IN1_PIN, 0);
	gpio_pin_set(motor_port, MOTOR_IN2_PIN, 1);
}

void motor_stop(void)
{
	motor_init_pins();
	if (!motor_pins_inited) {
		return;
	}
	/* Brake / stop: both low */
	gpio_pin_set(motor_port, MOTOR_IN1_PIN, 0);
	gpio_pin_set(motor_port, MOTOR_IN2_PIN, 0);
}

void motor_power_enable(bool enable)
{
	/* For simple two-pin control, power enable just maps to stop/idle. */
	if (!enable) {
		motor_stop();
	}
}

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

/* Simple NVS-backed storage for logical switch state.
 * Uses the fixed-partition labeled 'storage' (node label storage_partition).
 */
static struct nvs_fs nvs;
static bool nvs_inited;

static int storage_nvs_init(void)
{
	int rc;
	const struct flash_area *fa;

	if (nvs_inited) {
		return 0;
	}

	rc = flash_area_open(FIXED_PARTITION_ID(storage_partition), &fa);
	if (rc) {
		return rc;
	}

	nvs.offset = fa->fa_off;
	nvs.sector_size = fa->fa_size;
	nvs.sector_count = 1;
	nvs.flash_device = fa->fa_dev;

	rc = nvs_mount(&nvs);
	if (rc) {
		return rc;
	}

	nvs_inited = true;
	return 0;
}

/* Use a fixed NVS ID for logical state. */
#define NVS_ID_LOGICAL_STATE 1

int storage_read(uint8_t *out_value)
{
	int rc;
	uint8_t val;

	rc = storage_nvs_init();
	if (rc) {
		/* Fallback default: off */
		if (out_value) {
			*out_value = 0;
		}
		return rc;
	}

	rc = nvs_read(&nvs, NVS_ID_LOGICAL_STATE, &val, sizeof(val));
	if (rc < 0) {
		/* Not found or error: treat as 0/off. */
		if (out_value) {
			*out_value = 0;
		}
		return rc;
	}

	if (out_value) {
		*out_value = val;
	}
	return 0;
}

int storage_write(uint8_t value)
{
	int rc = storage_nvs_init();
	if (rc) {
		return rc;
	}

	rc = nvs_write(&nvs, NVS_ID_LOGICAL_STATE, &value, sizeof(value));
	return rc < 0 ? rc : 0;
}

/* Simple factory reset: clear stored logical state by deleting NVS entry. */
void hw_factory_reset(void)
{
	int rc;

	rc = storage_nvs_init();
	if (rc == 0) {
		(void)nvs_delete(&nvs, NVS_ID_LOGICAL_STATE);
	}
}

/* Battery measurement via ADC using zephyr,user io-channels[0]. */

#if DT_NODE_EXISTS(DT_PATH(zephyr_user)) && DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
static const struct adc_dt_spec batt_adc_chan =
	ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static int battery_adc_init(void)
{
	int err;

	if (!adc_is_ready_dt(&batt_adc_chan)) {
		return -ENODEV;
	}

	err = adc_channel_setup_dt(&batt_adc_chan);
	if (err < 0) {
		return err;
	}

	return 0;
}

uint8_t battery_read_percent(void)
{
	int err;
	struct adc_sequence seq = {
		.buffer_size = sizeof(uint16_t),
	};
	uint16_t sample = 0;
	int32_t mv;

	err = battery_adc_init();
	if (err < 0) {
		return 100;
	}

	seq.buffer = &sample;
	adc_sequence_init_dt(&batt_adc_chan, &seq);

	err = adc_read_dt(&batt_adc_chan, &seq);
	if (err < 0) {
		return 100;
	}

	mv = sample;
	err = adc_raw_to_millivolts_dt(&batt_adc_chan, &mv);
	if (err < 0) {
		/* Fallback: treat raw as millivolts-ish. */
		mv = sample;
	}

	/* Simple linear mapping: 3.0V -> 0%, 4.2V -> 100% (typical Li-ion) */
	const int32_t min_mv = 3000;
	const int32_t max_mv = 4200;

	if (mv <= min_mv) {
		return 0;
	}
	if (mv >= max_mv) {
		return 100;
	}
	return (uint8_t)(((mv - min_mv) * 100) / (max_mv - min_mv));
}
#else
uint8_t battery_read_percent(void)
{
	/* No ADC config: report dummy 100%. */
	return 100;
}
#endif

