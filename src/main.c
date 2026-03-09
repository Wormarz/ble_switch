/*
 * BLE Switch - main entry
 * Initializes app logic, BLE, button, timer; then idle.
 */
#include "ble_svc.h"
#include "button.h"
#include "hw_glue.h"
#include "timer_glue.h"
#include <zephyr/init.h>
#include <zephyr/kernel.h>

extern void app_init(void);

static int ble_switch_init(void)
{
	app_init();
	return 0;
}

SYS_INIT(ble_switch_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

int main(void)
{
	ble_svc_init();
	button_init();
	timer_glue_init();
	hw_save_state_trigger_init();
	k_sleep(K_FOREVER);

	return 0;
}
