#pragma once

#include <dk_buttons_and_leds.h>
/* Configurations Buttons and GPIO */
#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2
#define RUN_LED_BLINK_INTERVAL  5000
#define USER_LED                DK_LED3
#define USER_BUTTON             DK_BTN1_MSK

/* Configuration for Main Application */
#define DEFAULT_SAMPLE_DELAY_US 1000  // This translates to 1000 samples per second = 1kS/sec

/* Configuration for Hostcomm */
#define HOSTCOMM_MAX_PACKET_PER_TRANSMISSION 64

/* Configuration for Intan */
#define INTAN_BUFFER_SIZE 64