#pragma once
#include "intan_helper.h"

typedef enum {
    HOSTCOMM_HOST_MSG_SET_RATE = 1,
    HOSTCOMM_HOST_MSG_INTAN_MSG_SET_STIM_EN_MASK,
    HOSTCOMM_HOST_MSG_INTAN_MSG_SET_STIM_POS_MAG,
    HOSTCOMM_HOST_MSG_INTAN_MSG_SET_REC_MASK,
} hostcomm_external_msg_id_t;

#define HOST_CODE_SET_RATE      1
#define HOST_CODE_SET_COMM_ONE  2
#define HOST_CODE_SET_COMM_TWO  3

#define HOST_MESSAGE_CHANNEL_MASK_UPPER 0
#define HOST_MESSAGE_CHANNEL_MASK_LOWER 1
#define HOST_MESSAGE_CRC                2

typedef enum {
    HOSTCOMM_INTERNAL_INTAN_SEND_TO_HOST_MSG_ID = 1,
} hostcomm_internal_msg_id_t;

typedef struct __attribute__ ((__packed__)) {
    uint8_t crc;
    uint16_t channel_mask;
    uint16_t channel_data[HOSTCOMM_MAX_PACKET_PER_TRANSMISSION]; //always sending AC
} outgoing_message_struct_t;


typedef struct {
    hostcomm_internal_msg_id_t message_id;
    uint16_t optional_header; 
    uint16_t data_len;
    uint16_t data_buf[HOSTCOMM_MAX_PACKET_PER_TRANSMISSION];
} hostcomm_msg_t;

typedef struct {
    uint8_t crc;
} hostcomm_priv_t;
