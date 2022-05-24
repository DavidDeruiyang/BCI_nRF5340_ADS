#pragma once
#include <stdio.h>
#include <stdint.h>

/* High Level Spec */
#define NUM_CHANNELS  16
#define CHIP_ID       0x20

/* Data Formatting Related */
typedef struct __attribute__ ((__packed__)) {
    unsigned dc_amp_data : 10;
    unsigned zeros : 6;
    unsigned ac_amp_data : 16;
} intan_convert_channel_data_t;


typedef struct intan_msg_t {
    uint32_t msg_id;
    uint8_t data[32]; // TODO: don't hardcode
    uint32_t length;
} intan_msg_t;


// Struct for storing Intan related private information
typedef struct intan_priv_t {
    uint32_t sample_delay_us; // This is configured by host application

    bool initialized;

    // Both of these buffers only have room for 1 transaction (1 SPI back and forth).
    // It is up to the caller (in this case the main thread in main.c) to handle additional storage/buffering
    uint8_t tx_buf[4]; // tx_buf will store data going to Intan Chip
    uint8_t rx_buf[4]; // rx_buf will store data coming from Intan Chip
    
    // Buffers to store sample data. These buffers are separate from rx_buf above, because not all SPI commands return INTAN chip samples
    intan_convert_channel_data_t  channel_data[NUM_CHANNELS];

    // Due to the nature of Intan chip responding 2 SPI transaction cycles later, we need to store our previous 2 commands
    uint32_t nth_command;
    uint32_t n_minus_one_command;
    uint32_t n_minus_two_command;

    // Allocate room for commands from host
    uint32_t host_commands[4]; // only support up to 4 host commands

    int64_t last_stim_toggle_time_ms;

    uint16_t current_channel_mask;
    uint16_t current_batch_count;
    uint16_t channel_data_buffer[64];

} intan_priv_t;


int intan_send(uint32_t command);
void intan_add_host_command(uint32_t command);
bool intan_check_write_response(uint32_t write_command, uint32_t resp);
void intan_dump_channel_data(void);
intan_convert_channel_data_t intan_get_channel_data(uint8_t channel_num);
uint16_t intan_get_channel_ac_data(uint8_t channel_num);