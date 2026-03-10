#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_logic, LOG_LEVEL_INF);

#include "hw_glue.h"
#include "timer_glue.h"

/* Application state and logic previously implemented in Rust,
 * now reimplemented in pure C.
 */

typedef enum {
	STATE_IDLE = 0,
	STATE_MOVING_TO_ON = 1,
	STATE_MOVING_TO_OFF = 2,
	STATE_ERROR = 3,
} machine_state_t;

struct app_state {
	machine_state_t machine_state;
	uint8_t physical_state; /* 0 = off position, 1 = on position */
	uint8_t orientation;    /* 0 = normal, 1 = inverted */
	bool pairing_mode;
};

static struct app_state g_state;

static uint8_t app_get_battery_level(void)
{
	return battery_read_percent();
}

static bool app_is_battery_too_low_for_motion(void)
{
	return app_get_battery_level() <= 10;
}

static uint8_t app_get_logical_state(void)
{
	uint8_t p = g_state.physical_state & 1U;
	uint8_t o = g_state.orientation & 1U;
	if (o == 0U) {
		return p;
	}
	return (uint8_t)(1U - p);
}

static void app_request_on(void);
static void app_request_off(void);

static void app_request_toggle(void)
{
	uint8_t logical = app_get_logical_state();
	if (logical != 0U) {
		app_request_off();
	} else {
		app_request_on();
	}
}

#define MOTION_TIMEOUT_MS 1500U

static void app_request_on(void)
{
	if (g_state.machine_state != STATE_IDLE) {
		LOG_DBG("request_on ignored, state=%d", g_state.machine_state);
		return;
	}
	if (app_is_battery_too_low_for_motion()) {
		LOG_WRN("Battery too low, cannot move to ON (level=%u%%)", app_get_battery_level());
		g_state.machine_state = STATE_ERROR;
		led_flash_error();
		return;
	}
	LOG_INF("Moving to ON (orientation=%u, physical=%u)", g_state.orientation, g_state.physical_state);
	g_state.machine_state = STATE_MOVING_TO_ON;
	motor_power_enable(true);
	motor_move_to_on();
	timer_glue_start_motion_timeout_ms(MOTION_TIMEOUT_MS);
}

static void app_request_off(void)
{
	if (g_state.machine_state != STATE_IDLE) {
		LOG_DBG("request_off ignored, state=%d", g_state.machine_state);
		return;
	}
	if (app_is_battery_too_low_for_motion()) {
		LOG_WRN("Battery too low, cannot move to OFF (level=%u%%)", app_get_battery_level());
		g_state.machine_state = STATE_ERROR;
		led_flash_error();
		return;
	}
	LOG_INF("Moving to OFF (orientation=%u, physical=%u)", g_state.orientation, g_state.physical_state);
	g_state.machine_state = STATE_MOVING_TO_OFF;
	motor_power_enable(true);
	motor_move_to_off();
	timer_glue_start_motion_timeout_ms(MOTION_TIMEOUT_MS);
}

/* -------- Public API: application logic entry points -------- */

/* Called from SYS_INIT hook in main.c. */
void app_init(void)
{
	uint8_t v;

	g_state.machine_state = STATE_IDLE;
	g_state.physical_state = 0U;
	g_state.orientation = 0U;
	g_state.pairing_mode = false;

	if (storage_read_physical(&v) == 0) {
		g_state.physical_state = v & 1U;
	}
	if (storage_read_orientation(&v) == 0) {
		g_state.orientation = v & 1U;
	}
	LOG_INF("App init: physical=%u, orientation=%u", g_state.physical_state, g_state.orientation);
}

void app_handle_switch_control(uint8_t value)
{
	LOG_INF("Switch control write: value=%u (0=off,1=on,2=toggle)", value);
	switch (value) {
	case 0: /* logical off */
		if (g_state.orientation == 0U) {
			app_request_off();
		} else {
			app_request_on();
		}
		break;
	case 1: /* logical on */
		if (g_state.orientation == 0U) {
			app_request_on();
		} else {
			app_request_off();
		}
		break;
	case 2: /* toggle */
		app_request_toggle();
		break;
	default:
		break;
	}
}

uint8_t app_get_switch_state(void)
{
	uint8_t logical = app_get_logical_state();
	LOG_DBG("Get switch state: logical=%u (physical=%u, orientation=%u)",
		logical, g_state.physical_state, g_state.orientation);
	return app_get_logical_state();
}

uint8_t app_get_orientation(void)
{
	return g_state.orientation & 1U;
}

void app_set_orientation(uint8_t value)
{
	uint8_t v = value & 1U;
	g_state.orientation = v;
	(void)storage_write_orientation(v);
	LOG_INF("Set orientation: %u", v);
}

uint8_t app_get_battery_level_api(void)
{
	uint8_t lvl = app_get_battery_level();
	LOG_DBG("Battery level: %u%%", lvl);
	return lvl;
}

uint8_t app_get_error_state(void)
{
	return (uint8_t)g_state.machine_state;
}

void app_on_button_short(void)
{
	LOG_INF("Button short press -> toggle");
	app_request_toggle();
}

void app_on_button_long(void)
{
	g_state.pairing_mode = true;
	LOG_INF("Button long press -> pairing/factory reset");
	led_flash_pairing();
	hw_factory_reset();
}

void app_on_motion_complete(void)
{
	LOG_INF("Motion complete, state before=%d", g_state.machine_state);
	timer_glue_stop_motion_timeout();
	motor_stop();
	motor_power_enable(false);

	switch (g_state.machine_state) {
	case STATE_MOVING_TO_ON:
		g_state.physical_state = 1U;
		led_flash_feedback();
		LOG_INF("Reached ON position, physical_state=%u", g_state.physical_state);
		break;
	case STATE_MOVING_TO_OFF:
		g_state.physical_state = 0U;
		led_flash_feedback();
		LOG_INF("Reached OFF position, physical_state=%u", g_state.physical_state);
		break;
	default:
		break;
	}

	g_state.machine_state = STATE_IDLE;
}

void app_clear_error(void)
{
	LOG_INF("Clear error, state from=%d to IDLE", g_state.machine_state);
	g_state.machine_state = STATE_IDLE;
}

/* Called from save-state GPIO work handler in hw_glue.c. */
void app_save_physical_state_to_nvs(void)
{
	uint8_t v = g_state.physical_state & 1U;
	int rc = storage_write_physical(v);
	if (rc == 0) {
		LOG_INF("Saved physical_state=%u to NVS", v);
	} else {
		LOG_WRN("Failed to save physical_state=%u to NVS, rc=%d", v, rc);
	}
}

