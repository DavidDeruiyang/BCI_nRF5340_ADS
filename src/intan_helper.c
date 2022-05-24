/*
This file is used for functions related to configuring the behavior of Intan Chip.

*/

#include <stdint.h>

#include "spi.h"
#include "intan.h"
#include "intan_helper.h"
#include <logging/log.h>


#define LOG_MODULE_NAME       intan_helper_c
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);


extern intan_priv_t intan_priv;

int intan_send_spi_command(uint32_t command) {

    int err = 0;

    // First we convert the 32bit command into a 4 byte array as required by SPI driver
    // Note that nrf5340 is little endian, but we need to send most sig. byte 1st in SPI protocol for Intan
    intan_priv.tx_buf[0] = command >> 24;
    intan_priv.tx_buf[1] = command >> 16;
    intan_priv.tx_buf[2] = command >> 8;
    intan_priv.tx_buf[3] = command;

    err = spi_send_receive(intan_priv.tx_buf, sizeof(intan_priv.tx_buf), intan_priv.rx_buf, sizeof(intan_priv.rx_buf));

    return err;
}


int intan_send(uint32_t command) {
    return intan_send_spi_command(command);
}


void intan_add_host_command(uint32_t command) {
    intan_priv.host_commands[0] = command;
}


bool intan_check_write_response(uint32_t write_command, uint32_t resp) {

    // In the intan protocol, the write command as the write value in the bottom 2 bytes
    uint16_t original_write_val = write_command & 0xFFFF;

    // In the response of a write command, the top 2 bytes must be 0xFFFF
    if ((resp >> 16) != 0xFFFF) {
        return false;
    }

    // The bottom 2 bytes of response must = bottom 2 bytes of write command
    if ((resp & 0xFFFF) != original_write_val) {
        return false;
    }
    return true;
}

// For better printing to console, log only a few channels
void intan_dump_channel_data(void) {

    // Only dumping channel 0 for tesing.
    for (int i = 0; i < 1; i++) {
        LOG_INF("chan: %d, dc: %d, ac: %d 0s %d\n", i, intan_priv.channel_data[i].dc_amp_data, 
                                                        intan_priv.channel_data[i].ac_amp_data, intan_priv.channel_data[i].zeros);
    }
}

intan_convert_channel_data_t intan_get_channel_data(uint8_t channel_num) {
    return intan_priv.channel_data[channel_num];
}

uint16_t intan_get_channel_ac_data(uint8_t channel_num) {
    intan_convert_channel_data_t tmp = intan_get_channel_data(channel_num);
    return tmp.ac_amp_data;
}
