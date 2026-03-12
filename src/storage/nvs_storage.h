#ifndef STORAGE_NVS_STORAGE_H
#define STORAGE_NVS_STORAGE_H

#include <stdint.h>

/* Wrapper declarations for NVS-backed storage. The concrete functions
 * currently live in hw_glue.c:
 *
 *  - storage_read_physical(uint8_t *out_value)
 *  - storage_write_physical(uint8_t value)
 *  - storage_read_orientation(uint8_t *out_value)
 *  - storage_write_orientation(uint8_t value)
 */

int nvs_storage_read_physical(uint8_t *out_value);
int nvs_storage_write_physical(uint8_t value);
int nvs_storage_read_orientation(uint8_t *out_value);
int nvs_storage_write_orientation(uint8_t value);

/* Initialize GPIO/interrupt used to trigger saving current physical
 * state to NVS when a dedicated pin goes low.
 */
void nvs_storage_save_trigger_init(void);

#endif /* STORAGE_NVS_STORAGE_H */

