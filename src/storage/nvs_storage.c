/*
 * NVS-backed storage for physical motor position and orientation mapping.
 * This logic was previously implemented in hw_glue.c.
 */

#include "nvs_storage.h"

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(nvs_storage, LOG_LEVEL_INF);

/* Uses the fixed-partition labeled 'storage' (node label storage_partition). */
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

/* NVS IDs: physical motor position (0/1) and logical↔physical mapping (0=normal, 1=inverted).
 * Physical state is written only when save-state GPIO goes low (to limit NVS wear).
 */
#define NVS_ID_PHYSICAL_STATE 1
#define NVS_ID_ORIENTATION    2

int nvs_storage_read_physical(uint8_t *out_value)
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

	rc = nvs_read(&nvs, NVS_ID_PHYSICAL_STATE, &val, sizeof(val));
	if (rc < 0) {
		if (out_value) {
			*out_value = 0;
		}
		return rc;
	}

	if (out_value) {
		*out_value = val & 1U;
	}
	return 0;
}

int nvs_storage_write_physical(uint8_t value)
{
	int rc = storage_nvs_init();
	if (rc) {
		return rc;
	}
	value &= 1U;
	rc = nvs_write(&nvs, NVS_ID_PHYSICAL_STATE, &value, sizeof(value)) < 0 ? -1 : 0;
	if (rc == 0) {
		LOG_INF("NVS write physical_state=%u", value);
	} else {
		LOG_WRN("NVS write physical_state failed, rc=%d", rc);
	}
	return rc;
}

int nvs_storage_read_orientation(uint8_t *out_value)
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
		*out_value = val & 1U;
	}
	return 0;
}

int nvs_storage_write_orientation(uint8_t value)
{
	int rc = storage_nvs_init();
	if (rc) {
		return rc;
	}
	value &= 1U;
	rc = nvs_write(&nvs, NVS_ID_ORIENTATION, &value, sizeof(value)) < 0 ? -1 : 0;
	if (rc == 0) {
		LOG_INF("NVS write orientation=%u", value);
	} else {
		LOG_WRN("NVS write orientation failed, rc=%d", rc);
	}
	return rc;
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

static struct gpio_callback save_state_cb;
static bool save_state_trigger_inited;

static void save_state_gpio_callback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	LOG_INF("Save-state GPIO falling edge");
	k_work_submit(&save_state_work);
}

void nvs_storage_save_trigger_init(void)
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


