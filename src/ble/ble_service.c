/*
 * BLE service implementation for the remote mechanical switch.
 * This file contains the logic previously in src/ble_svc.c.
 */

#include "ble_service.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/services/bas.h>

LOG_MODULE_REGISTER(ble_svc, LOG_LEVEL_INF);

/* Application logic API */
extern uint8_t app_get_switch_state(void);
extern void app_handle_switch_control(uint8_t value);
extern uint8_t app_get_orientation(void);
extern void app_set_orientation(uint8_t value);
extern uint8_t app_get_battery_level_api(void);
extern uint8_t app_get_error_state(void);

/* LED control (advertising indication) */
extern void led_adv_blink_start(void);
extern void led_adv_blink_stop(void);

/* Remote Mechanical Switch Service: 0x0001-... (custom 128-bit) */
#define BT_UUID_REMOTE_SWITCH_SVC_VAL \
	BT_UUID_128_ENCODE(0x00010000, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
/* Switch Control characteristic */
#define BT_UUID_SWITCH_CTRL_VAL \
	BT_UUID_128_ENCODE(0x00010000, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)
/* Installation orientation: 0 = normal, 1 = inverted (read/write, persisted in NVS) */
#define BT_UUID_ORIENTATION_VAL \
	BT_UUID_128_ENCODE(0x00010000, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)

static const struct bt_uuid_128 remote_switch_svc_uuid = BT_UUID_INIT_128(
	BT_UUID_REMOTE_SWITCH_SVC_VAL);
static const struct bt_uuid_128 switch_ctrl_uuid = BT_UUID_INIT_128(
	BT_UUID_SWITCH_CTRL_VAL);
static const struct bt_uuid_128 orientation_uuid = BT_UUID_INIT_128(
	BT_UUID_ORIENTATION_VAL);

/* Battery Level Service (BAS), 0x180F, with a single read-only Battery Level characteristic. */
static ssize_t read_batt_level(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       void *buf, uint16_t len, uint16_t offset)
{
	uint8_t lvl = app_get_battery_level_api();
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &lvl, sizeof(lvl));
}

BT_GATT_SERVICE_DEFINE(bas_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_batt_level, NULL, NULL),
);

/* Application-provided fixed passkey.
 * For now this is hard-coded; in future it can be loaded from NVS or provided by MCUboot.
 */
#if defined(CONFIG_BT_APP_PASSKEY)
static uint32_t app_fixed_passkey = 123456;

static uint32_t auth_app_passkey(struct bt_conn *conn)
{
	ARG_UNUSED(conn);
	return app_fixed_passkey;
}

static const struct bt_conn_auth_cb auth_cb = {
	.app_passkey = auth_app_passkey,
};
#endif

/* Simple error/status characteristic:
 * 0 = Idle/OK, 1 = MovingToOn, 2 = MovingToOff, 3 = Error.
 */
static ssize_t read_error_state(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	uint8_t val = app_get_error_state();
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &val, sizeof(val));
}

BT_GATT_SERVICE_DEFINE(status_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0x180A)), /* reuse Device Info UUID namespace */
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0x2A3D), /* arbitrary 16-bit for status */
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_error_state, NULL, NULL),
);

static ssize_t read_switch_ctrl(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	uint8_t val = app_get_switch_state();
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
	switch (val) {
	case 0:
	case 1:
	case 2:
		break;
	default:
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	app_handle_switch_control(val);
	return len;
}

static ssize_t read_orientation(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	uint8_t val = app_get_orientation();
	LOG_DBG("GATT Orientation read: %u", val);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &val, sizeof(val));
}

static ssize_t write_orientation(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len, uint16_t offset,
				  uint8_t flags)
{
	if (offset != 0 || len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	uint8_t val = *(const uint8_t *)buf;
	switch (val) {
	case 0:
	case 1:
		break;
	default:
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	LOG_INF("GATT Orientation write: %u", val);
	app_set_orientation(val);
	return len;
}

BT_GATT_SERVICE_DEFINE(remote_switch_svc,
	BT_GATT_PRIMARY_SERVICE(&remote_switch_svc_uuid),
	BT_GATT_CHARACTERISTIC(&switch_ctrl_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_switch_ctrl, write_switch_ctrl, NULL),
	BT_GATT_CHARACTERISTIC(&orientation_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_orientation, write_orientation, NULL),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_REMOTE_SWITCH_SVC_VAL),
	/* Advertise standard Battery Service (0x180F) as 16-bit UUID */
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0F, 0x18),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static bool ble_any_bonded(void)
{
	bool has_bond = false;

	void foreach_bond(const struct bt_bond_info *info, void *user_data)
	{
		bool *flag = user_data;
		ARG_UNUSED(info);
		*flag = true;
	}

	bt_foreach_bond(BT_ID_DEFAULT, foreach_bond, &has_bond);
	return has_bond;
}

static void ble_conn_connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_WRN("Connection failed (err %u) to %s", err, addr);
		return;
	}

	LOG_INF("Connected: %s", addr);
	/* Stop advertising indicator once a connection is established. */
	led_adv_blink_stop();
}

static void ble_conn_disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

	int err = ble_svc_advertise_start();
	if (err) {
		LOG_ERR("Restart advertising after disconnect failed: %d", err);
		return;
	}

	/* If there is no existing bond yet, enable LED blink to indicate
	 * pairing/advertising; otherwise keep LED off.
	 */
	if (!ble_any_bonded()) {
		led_adv_blink_start();
	} else {
		led_adv_blink_stop();
	}
}

static struct bt_conn_cb ble_conn_cbs = {
	.connected = ble_conn_connected,
	.disconnected = ble_conn_disconnected,
};

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("bt_enable failed: %d", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	LOG_INF("bt_enable OK, starting advertising");
	err = ble_svc_advertise_start();
	if (err) {
		LOG_ERR("Advertising start failed: %d", err);
	} else {
		LOG_INF("Advertising started successfully");
		/* Only blink if there is no bond yet (first-time pairing). */
		if (!ble_any_bonded()) {
			led_adv_blink_start();
		} else {
			led_adv_blink_stop();
		}
	}
}

void ble_svc_init(void)
{
	int err;

#if defined(CONFIG_BT_APP_PASSKEY)
	bt_conn_auth_cb_register(&auth_cb);
#endif

	bt_conn_cb_register(&ble_conn_cbs);

	err = bt_enable(bt_ready);
	if (err) {
		LOG_ERR("bt_enable failed: %d", err);
	}
}

int ble_svc_advertise_start(void)
{
	/* Use GAP recommended fast connectable advertising parameters */
	return bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad),
			       sd, ARRAY_SIZE(sd));
}

