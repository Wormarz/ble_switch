#include "timer_glue.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(timer_glue, LOG_LEVEL_DBG);

extern void app_on_motion_complete(void);

static struct k_timer motion_timer;

static void motion_timeout_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	LOG_INF("Motion timeout fired");
	app_on_motion_complete();
}

void timer_glue_init(void)
{
	k_timer_init(&motion_timer, motion_timeout_handler, NULL);
	LOG_DBG("Motion timer initialized");
}

void timer_glue_start_motion_timeout_ms(uint32_t ms)
{
	LOG_DBG("Start motion timeout: %u ms", ms);
	k_timer_start(&motion_timer, K_MSEC(ms), K_FOREVER);
}

void timer_glue_stop_motion_timeout(void)
{
	LOG_DBG("Stop motion timeout");
	k_timer_stop(&motion_timer);
}
