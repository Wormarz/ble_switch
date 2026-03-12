/*
 * Battery measurement for the BLE switch.
 * Logic was previously implemented in hw_glue.c.
 */

#include "battery.h"

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>

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

uint8_t battery_level_read_percent(void)
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
uint8_t battery_level_read_percent(void)
{
	/* No ADC config: report dummy 100%. */
	return 100;
}
#endif

