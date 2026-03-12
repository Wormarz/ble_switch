#ifndef POWER_BATTERY_H
#define POWER_BATTERY_H

#include <stdint.h>

/* Battery level in percent (0..100). */
uint8_t battery_level_read_percent(void);

#endif /* POWER_BATTERY_H */

