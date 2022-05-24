/*
This file contain all function that is responsible for talking to Host (PC/Mobile).
The functions in this file are not aware of how the communication with Host is performed (BLE/USB).
The functions only need to understand and parse the message and do the correct actions.
*/

#include <logging/log.h>
#include <settings/settings.h>
#include <stdio.h>
#include <zephyr.h>
#include "ble.h"
#include "config.h"
#include "hostcomm.h"
#include "intan_helper.h"
#include "thread_config.h"


#define LOG_MODULE_NAME bci_hostcomm
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_DBG);

hostcomm_priv_t hostcomm_priv;
K_MSGQ_DEFINE(hostcomm_msgq, sizeof(hostcomm_msg_t), 16, 4);
extern struct k_msgq intan_msgq;  

// This function will be called when Host sends a message to Nordic.
// Only send message to hostcomm's message queue. DO NOT process the message here.
void host_message_receive_handler(uint8_t * data, size_t length) {

	// In the agreed upon format, the 1st byte is always code -> a value defined by us to signal what message is contained in the next byte
	if (length < 2) {
		return;
	}

    switch (data[0])
    {
        case HOSTCOMM_HOST_MSG_INTAN_MSG_SET_STIM_EN_MASK:
        case HOSTCOMM_HOST_MSG_INTAN_MSG_SET_STIM_POS_MAG:
        case HOSTCOMM_HOST_MSG_INTAN_MSG_SET_REC_MASK: {
            // Message intended for Intan, give it to Intan, this thread doesn't do anything.
            intan_msg_t intan_msg = {
                .length = length,
                .msg_id = data[0]
            };

            memcpy(intan_msg.data, data, length);

            k_msgq_put(&intan_msgq, &intan_msg, K_NO_WAIT);

            break;
        }
        default:
            LOG_ERR("Unsupported message type from host.");
            break;
    }
}


void hostcomm_thread_func(void * param1, void * param2, void * param3){

    // Start to initialize BLE
    ble_init(host_message_receive_handler);

    while(1) {
        hostcomm_msg_t hostcomm_msg;
        k_msgq_get(&hostcomm_msgq, &hostcomm_msg, K_FOREVER);
        
        //LOG_DBG("Received message, id %d ", hostcomm_msg.message_id);
        if (hostcomm_msg.message_id == HOSTCOMM_INTERNAL_INTAN_SEND_TO_HOST_MSG_ID) {
            /*
            Messages to host has the following format:
            byte 1 = crc
            byte 2&3 = channel mask
            byte 4&5 = channel X data
            byte 6&7 = channel Y data 
            and etc.

            The values of X and Y is decided by bits set in the mask
            */
            outgoing_message_struct_t msg = {0};
            msg.channel_mask = hostcomm_msg.optional_header;
            msg.crc = hostcomm_priv.crc;
            memcpy(msg.channel_data, hostcomm_msg.data_buf, hostcomm_msg.data_len);

            ble_send_bytes((uint8_t * ) &msg, sizeof(msg.crc) + sizeof(msg.channel_mask) + hostcomm_msg.data_len);
            if (hostcomm_priv.crc == 0xff) {
                hostcomm_priv.crc = 0;
            }
            else {
                hostcomm_priv.crc += 1;
            }
            
        }
        else {
            LOG_WRN("Unknown message ID %d ", hostcomm_msg.message_id);
        }
        
    }

}

K_THREAD_DEFINE(hostcomm_thread_id, HOSTCOMM_THREAD_STACK_SIZE, hostcomm_thread_func, NULL, NULL, NULL, HOSTCOMM_THREAD_PRIORITY, 0, 1500);