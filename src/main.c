/*
 * BLE Switch - main entry
 * Initializes Rust app, BLE, button, timer; then idle.
 */
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include "ble_svc.h"
#include "button.h"
#include "timer_glue.h"

extern void rust_app_init(void);

static int ble_switch_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	rust_app_init();
	return 0;
}
SYS_INIT(ble_switch_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

void main(void)
{
	ble_svc_init();
	button_init();
	timer_glue_init();
	k_sleep(K_FOREVER);
}
