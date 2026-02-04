#include "button.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

extern void rust_on_button_short(void);
extern void rust_on_button_long(void);

#define LONG_PRESS_MS 2000

#if DT_NODE_EXISTS(DT_ALIAS(sw0))
#define SW0_NODE DT_ALIAS(sw0)
#else
#error "No sw0 alias (button) in devicetree"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static struct gpio_callback button_cb;
static struct k_timer long_press_timer;
static volatile bool long_press_fired;

static void long_press_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	long_press_fired = true;
	rust_on_button_long();
}

static void button_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	int val = gpio_pin_get_dt(&button);
	if (val == 0) {
		/* Pressed: start long-press timer */
		long_press_fired = false;
		k_timer_start(&long_press_timer, K_MSEC(LONG_PRESS_MS), K_NO_WAIT);
	} else {
		/* Released */
		k_timer_stop(&long_press_timer);
		if (!long_press_fired) {
			rust_on_button_short();
		}
	}
}

void button_init(void)
{
	if (!gpio_is_ready_dt(&button)) {
		return;
	}
	gpio_pin_configure_dt(&button, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
	k_timer_init(&long_press_timer, long_press_expiry, NULL);
	gpio_init_callback(&button_cb, button_handler, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb);
}
