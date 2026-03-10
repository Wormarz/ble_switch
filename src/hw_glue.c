#include "hw_glue.h"
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hw_glue, LOG_LEVEL_INF);

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
	LOG_INF("Motor move_to_on");
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
	LOG_INF("Motor move_to_off");
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
	LOG_DBG("Motor stop");
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

/* LED: short flash if led0 exists (e.g. nRF5340 DK LED1) */
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
	LOG_INF("LED pairing flash");
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
	nvs.sector_size = fa->fa_size / 2;
	nvs.sector_count = 2;
	nvs.flash_device = fa->fa_dev;

	rc = nvs_mount(&nvs);
	if (rc) {
		return rc;
	}

	nvs_inited = true;
	return 0;
}

/* NVS: physical motor position (0/1) and logical↔physical mapping (0=normal, 1=inverted).
 * Physical state is written only when save-state GPIO goes low (to limit NVS wear). */
#define NVS_ID_PHYSICAL_STATE 1
#define NVS_ID_ORIENTATION    2

int storage_read_physical(uint8_t *out_value)
{
	int rc;
	uint8_t val;

	rc = storage_nvs_init();
	if (rc) {
		if (out_value) *out_value = 0;
		return rc;
	}
	rc = nvs_read(&nvs, NVS_ID_PHYSICAL_STATE, &val, sizeof(val));
	if (rc < 0) {
		if (out_value) *out_value = 0;
		return rc;
	}
	if (out_value) *out_value = val & 1;
	return 0;
}

int storage_write_physical(uint8_t value)
{
	int rc = storage_nvs_init();
	if (rc) return rc;
	value &= 1;
	rc = nvs_write(&nvs, NVS_ID_PHYSICAL_STATE, &value, sizeof(value)) < 0 ? -1 : 0;
	if (rc == 0) {
		LOG_INF("NVS write physical_state=%u", value);
	} else {
		LOG_WRN("NVS write physical_state failed, rc=%d", rc);
	}
	return rc;
}

int storage_read_orientation(uint8_t *out_value)
{
	int rc;
	uint8_t val;

	rc = storage_nvs_init();
	if (rc) {
		if (out_value) {
			*out_value = 0;
		}
		return rc;
	}

	rc = nvs_read(&nvs, NVS_ID_ORIENTATION, &val, sizeof(val));
	if (rc < 0) {
		if (out_value) {
			*out_value = 0;
		}
		return rc;
	}

	if (out_value) {
		*out_value = val & 1;
	}
	return 0;
}

int storage_write_orientation(uint8_t value)
{
	int rc = storage_nvs_init();
	if (rc) {
		return rc;
	}
	value &= 1;
	rc = nvs_write(&nvs, NVS_ID_ORIENTATION, &value, sizeof(value)) < 0 ? -1 : 0;
	if (rc == 0) {
		LOG_INF("NVS write orientation=%u", value);
	} else {
		LOG_WRN("NVS write orientation failed, rc=%d", rc);
	}
	return rc;
}

/* Factory reset: clear NVS state and all BLE bonds. */
void hw_factory_reset(void)
{
	int rc = storage_nvs_init();
	if (rc == 0) {
		(void)nvs_delete(&nvs, NVS_ID_PHYSICAL_STATE);
		(void)nvs_delete(&nvs, NVS_ID_ORIENTATION);
	}
	LOG_INF("Factory reset: cleared NVS and BLE bonds");
	(void)bt_unpair(BT_ID_DEFAULT, NULL);
}

/* Save-state trigger: when this GPIO goes low, current physical state is written to NVS (via work, to limit wear). */
#define SAVE_STATE_TRIGGER_PIN 15

extern void app_save_physical_state_to_nvs(void);

static struct k_work save_state_work;
static void save_state_work_handler(struct k_work *w)
{
	ARG_UNUSED(w);
	app_save_physical_state_to_nvs();
}

static void save_state_gpio_callback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	LOG_INF("Save-state GPIO falling edge");
	k_work_submit(&save_state_work);
}

static struct gpio_callback save_state_cb;
static bool save_state_trigger_inited;

void hw_save_state_trigger_init(void)
{
	const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0) || save_state_trigger_inited) {
		return;
	}
	k_work_init(&save_state_work, save_state_work_handler);
	int ret = gpio_pin_configure(gpio0, SAVE_STATE_TRIGGER_PIN,
				     GPIO_INPUT | GPIO_PULL_UP);
	if (ret) {
		return;
	}
	gpio_init_callback(&save_state_cb, save_state_gpio_callback,
			   BIT(SAVE_STATE_TRIGGER_PIN));
	ret = gpio_add_callback(gpio0, &save_state_cb);
	if (ret) {
		return;
	}
	ret = gpio_pin_interrupt_configure(gpio0, SAVE_STATE_TRIGGER_PIN,
					   GPIO_INT_EDGE_FALLING);
	if (ret) {
		return;
	}
	save_state_trigger_inited = true;
	LOG_INF("Save-state trigger initialized on P0.%d", SAVE_STATE_TRIGGER_PIN);
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

