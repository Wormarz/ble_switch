#ifndef TIMER_GLUE_H
#define TIMER_GLUE_H

void timer_glue_init(void);
void timer_glue_start_motion_timeout_ms(uint32_t ms);
void timer_glue_stop_motion_timeout(void);

#endif
