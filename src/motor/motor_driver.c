#include "motor_driver.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motor_drv, LOG_LEVEL_INF);

/* Simple GPIO-based H-bridge style control using GPIO0 pins.
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

