/*
 * BLE Switch - main entry
 * Initializes app logic, BLE, button, storage trigger; then idle.
 */
#include "ble_service.h"
#include "button.h"
#include "nvs_storage.h"
#include "led.h"
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
	led_init();
	ble_svc_init();
	button_init();
	nvs_storage_save_trigger_init();
	k_sleep(K_FOREVER);

	return 0;
}
