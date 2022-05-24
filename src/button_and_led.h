#pragma once
#include "config.h"
void app_led_cb(bool led_state);
bool app_button_cb(void);
void button_changed(uint32_t button_state, uint32_t has_changed);
int init_button_and_led(void);