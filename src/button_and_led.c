#include "config.h"
#include "button_and_led.h"

int init_button_and_led(void)
{
	int err;

	err = dk_leds_init();
	if (err) {
		printk("Cannot init LEDs (err: %d)\n", err);
	}

	return err;
}
