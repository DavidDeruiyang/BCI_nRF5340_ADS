/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Zephyr and Nordic Includes */
#include <device.h>
#include <devicetree.h>
#include <errno.h>
#include <logging/log.h>
#include <nrfx_nvmc.h>
#include <nrfx_clock.h>
#include <soc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>

#include <settings/settings.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr/types.h>
#include <zephyr.h>

/* Our own header files */
#include "ble.h"
#include "button_and_led.h"
#include "config.h"
#include "intan.h"
#include "intan_helper.h"
#include "main.h"
#include "spi.h"
#include "usb.h"

#define LOG_MODULE_NAME bci_main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

main_priv_t main_priv;
extern struct k_msgq hostcomm_msgq;

void main(void)
{
	int blink_status = 0;
	int err;
	
	printk("Starting Nordic nRF5340 Main\n");

	// TODO: create a new initialization function for this.
	memset(&main_priv, 0, sizeof(main_priv_t));
	main_priv.sample_delay_us = DEFAULT_SAMPLE_DELAY_US;

	// Due to hardware bug, CPU needs to run at 128mhz for high speed SPI. TODO: Remove this because this is only needed on older revisions of hardware.
	// nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);

	// LEDs in this application are only used so we (observers) know the program is actually running.
	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return;
	}

    spi_init();
	//intan_headstage_init();

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_USEC(main_priv.sample_delay_us));
	}
}
