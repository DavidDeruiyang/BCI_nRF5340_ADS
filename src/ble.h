#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/services/nus.h>

#define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
#define PRIORITY 7
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)
#define BLE_MAX_DATA_SIZE 2048
#define BLE_HEADER_SIZE  4
#define BLE_MAX_TRANSFER_SIZE (BLE_MAX_DATA_SIZE + BLE_HEADER_SIZE)  // Allowing 4 bytes of header, and 1024 bytes of data in 1 single send operation

typedef void (*ble_receive_data_handler_t) (uint8_t * data, size_t length);

typedef struct ble_data_t {
	uint8_t data[BLE_MAX_TRANSFER_SIZE];
	uint16_t len;
}ble_data_t;

typedef struct ble_priv_data_t {
	ble_receive_data_handler_t receive_data_handler;
	ble_data_t scrap_data_buf;
	uint8_t outgoing_msg_counter;
	uint8_t incoming_msg_counter; // not used for now
} ble_priv_data_t;

void ble_init(ble_receive_data_handler_t ble_receive_data_callback);
void ble_send(uint8_t * data, uint32_t len);
void ble_send_bytes(uint8_t * data, uint32_t len);
