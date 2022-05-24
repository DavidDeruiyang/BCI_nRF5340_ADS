
/*
This files contain functions related to the Intan RHS2116.

Datasheet: https://intantech.com/files/Intan_RHS2116_datasheet.pdf

SPI Command Sequence Developer Note (Pg24 of Datasheet)
The rate and timing of SPI commands sent to the chip
determines the ADC sampling rate; sample times are set by
the falling edge of CS. In most applications, all 16 amplifiers
on the chips will be sampled in round-robin fashion. This
can be accomplished by repeating the following command
sequence:
CONVERT(0)
CONVERT(1)
CONVERT(2)
…
CONVERT(14)
CONVERT(15)
If a per-channel sampling rate of R is desired, then SPI
commands are sent at a rate of 16R.
The problem with simply repeating 16 CONVERT
commands is that additional commands (e.g., to write to
registers to control stimulation) must be substituted for
regular CONVERT commands (which results in a missing
sample on one channel) or else the sequence must be
interrupted by an inserted command, which makes the perchannel sampling rate irregular.

The simplest solution to this problem is to always insert a
fixed number (typically 1-4) of extra “auxiliary” commands
into the round-robin command sequence:
CONVERT(0)
CONVERT(1)
CONVERT(2)
…
CONVERT(14)
CONVERT(15)
auxiliary command 1
auxiliary command 2
auxiliary command 3
auxiliary command 4
Now having a list of 20 commands, the SPI commands are
sent at a rate of 20R to achieve a per-channel sampling rate
of R. Extra commands (e.g., to control stimulation, to update
the impedance check DAC, etc.) may be inserted into one of
the auxiliary command “slots”, and these extra commands
will not interrupt the steady, constant-rate sampling of the
amplifiers on the chip. Dummy commands (e.g., reading a
ROM register) can be inserted into these slots as place
holders when no auxiliary actions are required.
*/

#include <stdint.h>
#include "config.h"
#include "spi.h"
#include "hostcomm.h"
#include "intan.h"
#include "intan_helper.h"
#include "thread_config.h"
#include <kernel.h>
#include <logging/log.h>
#include <zephyr.h>
#include <soc.h>


#define LOG_MODULE_NAME       intan_c
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

extern struct k_msgq hostcomm_msgq; 

intan_priv_t intan_priv;
K_MSGQ_DEFINE(intan_msgq, sizeof(intan_msg_t), 16, 4);

int intan_headstage_set_rate(uint32_t rate) {
    // TODO: Take in parameters from host and apply them here.
    return 0;
}

// enable stimulation for all channels
void intan_stim_enable(void) {
    intan_send_and_receive(INTAN_WRITE(REG_STIM_ENABLE_A, STIM_EN_A, 0, 0));
    intan_send_and_receive(INTAN_WRITE(REG_STIM_ENABLE_B, STIM_EN_B, 0, 0));
}

// disable stimulation for all channels
void intan_stim_disable(void) {
    intan_send_and_receive(INTAN_WRITE(REG_STIM_ENABLE_A, 0x0, 0, 0));
    intan_send_and_receive(INTAN_WRITE(REG_STIM_ENABLE_B, 0x0, 0, 0));
}


// turn on stimulation, each bit in stim_on_mask correspond to a channel
void intan_stim_on(uint16_t stim_on_mask) {
    
    intan_send_and_receive(INTAN_WRITE(REG_STIM_ON_TRGD, stim_on_mask, 1, 0));
}


// set stimulation polarity, setting bit = 1 means positive current for the associated channel
void intan_stim_polarity_set(uint16_t mask) {
    
    intan_send_and_receive(INTAN_WRITE(REG_STIM_POLARITY_TRGD, mask, 1, 0));

}


// batch send to host, will send every thing that is present in the Fifo queue
void intan_batch_send_to_host(void) {
    hostcomm_msg_t msg = {
        .message_id = HOSTCOMM_INTERNAL_INTAN_SEND_TO_HOST_MSG_ID,
        .optional_header = intan_priv.current_channel_mask,
        .data_len = intan_priv.current_batch_count * 2
    };

    // * 2 because each channel data is 2 bytes, TODO: this needs to be updated when channel masks change
    memcpy(msg.data_buf, intan_priv.channel_data_buffer, intan_priv.current_batch_count * 2);

    k_msgq_put(&hostcomm_msgq, &msg, K_NO_WAIT);

    // Reset our internal counter
    intan_priv.current_batch_count = 0;
}


void intan_add_channel_data_to_batch_buffer(uint16_t data) {

    if (intan_priv.current_batch_count < INTAN_BUFFER_SIZE) {
        intan_priv.channel_data_buffer[intan_priv.current_batch_count] = data;
        intan_priv.current_batch_count += 1;
    }
    else {
        LOG_ERR("Out of space in channel data buffer, current batch count %d", intan_priv.current_batch_count);
    }
    
}

// send command data and also process the current data that is in rx_buf. 
void intan_send_and_receive(uint32_t command) {

    // Send command first
    int err = intan_send (command);
    uint32_t resp = 0;

    if (err) {
        // TODO: add error handling
        LOG_ERR("intan send failed for command 0x%x", command);
        return;
    }


    // Keep track of n, n-1 and n-2 commands so when the data comes back, we know what the data is responding to
    intan_priv.n_minus_two_command = intan_priv.n_minus_one_command;
    intan_priv.n_minus_one_command = intan_priv.nth_command;
    intan_priv.nth_command = command;

    // So the rx_buf contains the response to our n-2 command that was sent before
    // ignore the response if n-2 command was 0 (aka not valid)
    if (intan_priv.n_minus_two_command) {

        // Intan transmission gives most significant byte first.
        resp = (uint32_t) (intan_priv.rx_buf[0]<<24 | intan_priv.rx_buf[1] <<16 | intan_priv.rx_buf[2] << 8 | intan_priv.rx_buf[3]);
        
        switch (INTAN_RWC_COMMAND_HEADER_MASK & intan_priv.n_minus_two_command) {   
            case INTAN_CONVERT_HEADER: {
                
                unsigned channel_num = (intan_priv.n_minus_two_command >> INTAN_CONVERT_CHANNEL_OFFSET) & INTAN_CONVERT_CHANNEL_MASK;
                intan_convert_channel_data_t * data = (intan_convert_channel_data_t*) &resp;
                intan_priv.channel_data[channel_num] = *data;

                // Save this data for batch sending to host if this channel is in the channel mask
                if (channel_num & intan_priv.current_channel_mask) {
                    intan_add_channel_data_to_batch_buffer(data->ac_amp_data);
                }
                
                break;

            }
            case INTAN_READ_HEADER: {
                // TODO: add special handling
                LOG_DBG ("Read command 0x%x, Return Value 0x%x ", intan_priv.n_minus_two_command, resp);
                break;
            }
            case INTAN_WRITE_HEADER: {

                if (!intan_check_write_response(intan_priv.n_minus_two_command, resp)) {

                    // A write has failed. Show warning. Maybe harmless depending on the register that we were writing to
                    LOG_WRN ("Write command 0x%x failed, register has value 0x%x ", intan_priv.n_minus_two_command, resp);
                    
                }
                break;
            }
            default:
                break;
        }
     
    }
}

// continuously sample all 16 channels
void intan_continuous_sample(void) {
    
    for (int i=0; i < NUM_CHANNELS; i++) {
        // TODO: only sample if the channel is enabled in channel mask.
        intan_send_and_receive(INTAN_CONVERT(i, 0, 0, 1, 0));
    }

    // Shove in 4 additional commands.
    for (int i=0; i < 4; i++) {
        if (intan_priv.host_commands[i]) {
            intan_send_and_receive(intan_priv.host_commands[i]);
        }
        else {
            //dummy command
            intan_send_and_receive(INTAN_READ(RO_REG_CHIP_ID, 0, 0));
        }
    }
}


void intan_step_up_stim(void) {

    static uint16_t counter = 0;
    int64_t curr_time = k_uptime_get();

    // we've done stim toggles before
    if (intan_priv.last_stim_toggle_time_ms != 0) {
        if (curr_time - intan_priv.last_stim_toggle_time_ms < 1000) {
            // Toggle once every 1000 ms, so if time elapsed < 1000, do nothing
            return;
        }
    }

    // If we get here, then time elapsed > 1000ms AND we have done stim before
    // OR it is our first stim 
    if (counter == 0) {
        // Negative stimulation
        intan_priv.host_commands[0] = INTAN_WRITE(REG_STIM_POLARITY_TRGD, 0x0, 1, 0);
        // Turn on stimulation for all channels
        intan_priv.host_commands[1] = INTAN_WRITE(REG_STIM_ON_TRGD, 0xffff, 1, 0);
    }
    else if (counter == 1) {
         // Positive stimulation
        intan_priv.host_commands[0] = INTAN_WRITE(REG_STIM_POLARITY_TRGD, 0xffff, 1, 0);
        // Turn on stimulation for all channels
        intan_priv.host_commands[1] = INTAN_WRITE(REG_STIM_ON_TRGD, 0xffff, 1, 0);
    }
    else if (counter == 2) {
        // Turn off stimulation for all channels
        intan_priv.host_commands[1] = INTAN_WRITE(REG_STIM_ON_TRGD, 0x0, 1, 0);
        counter = 0;
    }

    // Give 128 magnitude to channel 0 and 1
    intan_priv.host_commands[2] = INTAN_WRITE(REG_POS_STIM_CURRENT_MAG_TRGD_BASE, (0x8000 | 128), 1, 0);
    intan_priv.host_commands[3] = INTAN_WRITE((REG_POS_STIM_CURRENT_MAG_TRGD_BASE+1), (0x8000 | 128), 1, 0);

    counter+=1;
    intan_priv.last_stim_toggle_time_ms = k_uptime_get();

}


int intan_headstage_init(void) {

    if (!spi_is_initialized()) {
        spi_init();
    }

    // Always zero out our internal buffers during init.
    memset(&intan_priv, 0, sizeof(intan_priv_t));

    intan_priv.sample_delay_us = DEFAULT_SAMPLE_DELAY_US;
    intan_priv.current_channel_mask = 0;

    /*
    All the following commands are strictly follow Pg44 of Intan Datasheet
    */
    intan_send_and_receive(INTAN_READ(RO_REG_CHIP_ID, 0, 0));

    intan_stim_disable();

    //Power up all DC-coupled low-gain amplifiers to avoid excessive power consumption due to hardware bug.
    intan_send_and_receive(INTAN_WRITE(REG_IND_DC_AMP_POWER, 0xFFFF, 0, 0));

    intan_send_and_receive(INTAN_CLEAR);

    // Default = 0x00C7
    // Setting to (32 << 6) | (40) for <= 120kS sampling rate
    intan_send_and_receive(INTAN_WRITE(REG_SUPPLY_SENSOR_ADC_BUFF_BIAS_CURRENT, ((32 << 5) | (40)), 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_ADC_OUT_FORMAT_DSP_OFFSET_RM_AUX_DIG_OUT, 0x051A, 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_IMPEDENCE_CHECK_CONTROL, 0x0040, 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_IMPEDENCE_CHECK_DAC, 0x0080, 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_ONCHIP_AMP_BW_SEL_ONE, 0x0016, 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_ONCHIP_AMP_BW_SEL_TWO, 0x0017, 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_ONCHIP_AMP_BW_SEL_THREE, 0x00A8, 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_ONCHIP_AMP_BW_SEL_FOUR, 0x000A, 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_IND_AC_AMP_POWER, 0xFFFF, 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_AMP_FAST_SETTLE_TRGD, 0x0000, 1, 0));

    intan_send_and_receive(INTAN_WRITE(REG_AMP_LOWER_CUTOFF_FREQ_SEL_TRGD, 0xFFFF, 1, 0));

    intan_send_and_receive(INTAN_WRITE(REG_STIM_CURRENT_STEP_SIZE, 0x00E2, 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_STIM_BIAS_VOL, 0x00AA, 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_CHARGE_RECOVERY_TARGET, 0x0080, 0, 0));

    intan_send_and_receive(INTAN_WRITE(REG_CHARGE_RECOVERY_LIM, 0x4F00, 0, 0));

    intan_stim_on(0x0); // intan_send_and_receive(INTAN_WRITE(REG_STIM_ON_TRGD, 0x0000, 1, 0));

    intan_stim_polarity_set(0x0); //intan_send_and_receive(INTAN_WRITE(REG_STIM_POLARITY_TRGD, 0x0000, 1, 0));

    intan_send_and_receive(INTAN_WRITE(REG_CHARGE_RECOVERY_SWTICH_TRGD, 0x0000, 1, 0));

    intan_send_and_receive(INTAN_WRITE(REG_CHARGE_RECOVERY_ENABLE_TRGD, 0x0000, 1, 0));

    for (int i = REG_NEG_STIM_CURRENT_MAG_TRGD_BASE; i <= REG_NEG_STIM_CURRENT_MAG_TRGD_END; i++) {
        intan_send_and_receive(INTAN_WRITE(i, 0x8000, 1, 0));
    }

    for (int i = REG_POS_STIM_CURRENT_MAG_TRGD_BASE; i <= REG_POS_STIM_CURRENT_MAG_TRGD_END; i++) {
        intan_send_and_receive(INTAN_WRITE(i, 0x8000, 1, 0));
    }

    intan_stim_enable();

    intan_send_and_receive(INTAN_READ(RO_REG_CHIP_ID, 0, 1));

    intan_priv.initialized = true;

    return 0;

}


void intan_process_host_message(void) {
    // Clear all host commands
    for (int i = 0; i < 4; i++) {
        intan_priv.host_commands[i] = 0;
    }

    // Intan can only process up to 4 messages at a time. TODO: don't hardcode the 4 here
    for (int i = 0; i < 4; i++) {
        // Read from message queue to see if there is something waiting for us to process
        intan_msg_t msg;
        if (k_msgq_get(&intan_msgq, &msg, K_NO_WAIT) != 0) {
            break; // no message, break
        }

        // there was a msg, process it
        switch (msg.data[0])
        {
            case HOSTCOMM_HOST_MSG_INTAN_MSG_SET_STIM_EN_MASK: {
                // Message from host is little endian, aka least significant byte first.
                uint16_t mask = msg.data[2] << 8 | msg.data[1]; // TODO: clean up hard coded indices
                LOG_DBG("Setting stimulation enable mask to 0x%x", mask);
                intan_priv.host_commands[i] = INTAN_WRITE(REG_STIM_ON_TRGD, mask, 1, 0);
                break;
            }
            case HOSTCOMM_HOST_MSG_INTAN_MSG_SET_STIM_POS_MAG: {
                // Message from host is little endian, aka least significant byte first.
                uint16_t channel = msg.data[2] << 8 | msg.data[1]; // TODO: clean up hard coded indices
                uint16_t mag = msg.data[3] << 8 | msg.data[4]; // TODO: clean up hard coded indices
                LOG_DBG("Setting stimulation magnitude to %d for channel %d", mag, channel);

                intan_priv.host_commands[i] = INTAN_WRITE((REG_POS_STIM_CURRENT_MAG_TRGD_BASE + channel), (0x8000 | mag), 1, 0); // TODO: remove hardcoded
                break;
            }
            case HOSTCOMM_HOST_MSG_INTAN_MSG_SET_REC_MASK: {
                uint16_t mask = msg.data[2] << 8 | msg.data[1]; // TODO: clean up hard coded indices
                LOG_INF("Setting recording channel mask to 0x%x", mask);
                intan_priv.current_channel_mask = mask;

                // After changing the mask, we must drain currently buffered data
                intan_batch_send_to_host();
                break;
            }
            default:
                break;
        }

    }
}



void intan_thread_func(void * param1, void * param2, void * param3) {

    while(1) {
        if (!intan_priv.initialized) {
            // When the Intan thread starts, it is possible the main application hasn't finished initialization yet, so block here
            k_sleep(K_MSEC(2000));
            continue;
        }

        // Process host message first before sampling/recording
        intan_process_host_message();
        intan_continuous_sample();
        intan_step_up_stim();

        // TODO: fix this minus 16 logic, basically we are sending early in case the next iteration of sample runs over
        if (intan_priv.current_batch_count >= (INTAN_BUFFER_SIZE - NUM_CHANNELS)) {
            intan_batch_send_to_host();
        }

        k_sleep(K_USEC(intan_priv.sample_delay_us));
    }

}

K_THREAD_DEFINE(intan_thread_id, INTAN_THREAD_STACK_SIZE, intan_thread_func, NULL, NULL, NULL, INTAN_THREAD_PRIORITY, 0, 0);