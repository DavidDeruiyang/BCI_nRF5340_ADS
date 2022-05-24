#include <stdint.h>
#include "hostcomm.h"

typedef struct main_priv_t {
    int sample_delay_us;
    uint16_t channel_select_mask;
    outgoing_message_struct_t outgoing_msg;
} main_priv_t;