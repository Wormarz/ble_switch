#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdbool.h>

void motor_move_to_on(void);
void motor_move_to_off(void);
void motor_stop(void);
void motor_power_enable(bool enable);

#endif /* MOTOR_DRIVER_H */


