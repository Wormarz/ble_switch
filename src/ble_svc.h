#ifndef BLE_SVC_H
#define BLE_SVC_H

/* Initialize BLE stack and register GATT service. Call once after bt_enable. */
void ble_svc_init(void);

/* Start advertising. Call from bt_ready callback. */
int ble_svc_advertise_start(void);

#endif
