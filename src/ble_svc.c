/*
 * BLE GATT: Remote Mechanical Switch Service
 * - Switch Control: read/write, 1 byte 0=off, 1=on, 2=toggle
 */
#include "ble_svc.h"
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

/* Rust API */
extern uint8_t rust_get_switch_state(void);
extern void rust_handle_switch_control(uint8_t value);

/* Remote Mechanical Switch Service: 0x0001-... (custom 128-bit) */
#define BT_UUID_REMOTE_SWITCH_SVC_VAL \
	BT_UUID_128_ENCODE(0x00010000, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
/* Switch Control characteristic */
#define BT_UUID_SWITCH_CTRL_VAL \
	BT_UUID_128_ENCODE(0x00010000, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

static const struct bt_uuid_128 remote_switch_svc_uuid = BT_UUID_INIT_128(
	BT_UUID_REMOTE_SWITCH_SVC_VAL);
static const struct bt_uuid_128 switch_ctrl_uuid = BT_UUID_INIT_128(
	BT_UUID_SWITCH_CTRL_VAL);

static ssize_t read_switch_ctrl(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	uint8_t val = rust_get_switch_state();
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &val, sizeof(val));
}

static ssize_t write_switch_ctrl(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset,
				 uint8_t flags)
{
	if (offset != 0 || len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	uint8_t val = *(const uint8_t *)buf;
	rust_handle_switch_control(val);
	return len;
}

BT_GATT_SERVICE_DEFINE(remote_switch_svc,
	BT_GATT_PRIMARY_SERVICE(&remote_switch_svc_uuid),
	BT_GATT_CHARACTERISTIC(&switch_ctrl_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_switch_ctrl, write_switch_ctrl, NULL),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_REMOTE_SWITCH_SVC_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void bt_ready(int err)
{
	if (err) {
		return;
	}
	ble_svc_advertise_start();
}

void ble_svc_init(void)
{
	int err = bt_enable(bt_ready);
	if (err) {
		/* Log if CONFIG_LOG enabled */
		(void)err;
	}
}

int ble_svc_advertise_start(void)
{
	/* Use GAP recommended fast connectable advertising parameters */
	return bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad),
			       sd, ARRAY_SIZE(sd));
}
