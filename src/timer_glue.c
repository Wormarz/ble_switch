#include "timer_glue.h"
#include <zephyr/kernel.h>

extern void rust_on_motion_complete(void);

static struct k_timer motion_timer;

static void motion_timeout_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	rust_on_motion_complete();
}

void timer_glue_init(void)
{
	k_timer_init(&motion_timer, motion_timeout_handler, NULL);
}

void timer_glue_start_motion_timeout_ms(uint32_t ms)
{
	k_timer_start(&motion_timer, K_MSEC(ms), K_FOREVER);
}

void timer_glue_stop_motion_timeout(void)
{
	k_timer_stop(&motion_timer);
}
