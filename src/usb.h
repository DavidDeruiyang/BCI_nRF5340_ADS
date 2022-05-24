#include "config.h"

typedef void (*usb_receive_data_handler_t) (uint8_t * data, int32_t length);

typedef struct usb_priv_t {
    usb_receive_data_handler_t data_receive_handler; 
} usb_priv_t;

void usb_init(usb_receive_data_handler_t data_handler);