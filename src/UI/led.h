#ifndef UI_LED_H
#define UI_LED_H

void led_init(void);
void led_flash_pairing(void);
void led_flash_feedback(void);
void led_flash_error(void);
void led_adv_blink_start(void);
void led_adv_blink_stop(void);

#endif /* UI_LED_H */

