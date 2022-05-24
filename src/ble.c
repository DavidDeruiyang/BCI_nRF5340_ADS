#include "ble.h"
#include "config.h"
#include <dk_buttons_and_leds.h>
#include <stdio.h>
#include <settings/settings.h>
#include <logging/log.h>

#define LOG_MODULE_NAME nordic_ble_client
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

static struct bt_conn *current_conn;
static struct bt_conn *auth_conn;

ble_priv_data_t ble_priv_data;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

void error(void)
{
	dk_set_leds_state(DK_ALL_LEDS_MSK, DK_NO_LEDS_MSK);

	while (true) {
		/* Spin for ever */
		k_sleep(K_MSEC(1000));
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected %s", log_strdup(addr));

	current_conn = bt_conn_ref(conn);

	dk_set_led_on(CON_STATUS_LED);

	// Reset our outgoing message counter when a client connects
	ble_priv_data.outgoing_msg_counter = 0;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", log_strdup(addr), reason);

	if (auth_conn) {
		bt_conn_unref(auth_conn);
		auth_conn = NULL;
	}

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
		dk_set_led_off(CON_STATUS_LED);
	}

	// Reset our outgoing message counter when client disconnects
	ble_priv_data.outgoing_msg_counter = 0;
}


BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected    = connected,
	.disconnected = disconnected
};


// This function is called when host (phone/computer) send data to Nordic
static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data,
			  uint16_t len)
{
	char addr[BT_ADDR_LE_STR_LEN] = {0};

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

	LOG_INF("Received data from: %s", log_strdup(addr));
	
	if (ble_priv_data.receive_data_handler) {
		ble_priv_data.receive_data_handler((uint8_t*)data, (size_t)len);
	}
}


static struct bt_nus_cb nus_cb = {
	.received = bt_receive_cb,
};


// Send message using Nordic's BLE stack. Do not call this function directly. It bypasses any crc 
// that we are building.
void ble_send(uint8_t * data, uint32_t len)
{
	if (bt_nus_send(NULL, data, (uint16_t) len)) {
		LOG_DBG("Failed to send data over BLE connection");
	}
}

// Send bytes to currently connected client. 
void ble_send_bytes(uint8_t * data, uint32_t len) {

	if (len > BLE_MAX_DATA_SIZE) {
		LOG_ERR("Send failed due to size too big. This function needs to be improved to handle big writes.");
	}

	//ble_data_t buf = {0}; // TODO: fix this to save stack space
	
	// Increment counter, prepend it to the data going out.
	// buf.data[0] = ble_priv_data.outgoing_msg_counter;
	// if (ble_priv_data.outgoing_msg_counter == 0xff) {
	// 	ble_priv_data.outgoing_msg_counter = 0;
	// }
	// ble_priv_data.outgoing_msg_counter += 1;
	

	// 0th byte is counter, data bytes come after.
	//memcpy(&(buf.data[1]), data, len);
	//buf.len = len + 1; // +1 here because we allocated extra byte as counter

	ble_send(data, len);
}

void ble_init(ble_receive_data_handler_t ble_receive_data_callback)
{
	int err = 0;

	LOG_INF("Bluetooth initializing");

	memset(&ble_priv_data, 0, sizeof(ble_priv_data_t));

	if (ble_receive_data_callback)
		ble_priv_data.receive_data_handler = ble_receive_data_callback;

	err = bt_enable(NULL);
	if (err) {
		error();
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_nus_init(&nus_cb);
	if (err) {
		LOG_ERR("Failed to initialize nus callback service (err: %d)", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
				  ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

}
