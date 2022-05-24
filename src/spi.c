#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <logging/log.h>

#define LOG_MODULE_NAME         nordic_spi_c
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_ERR);

#include "spi.h"

///Private///
struct spi_priv_t spi_priv;

///SPI///
struct device * spi_dev;

struct spi_cs_control spi_cs = {
	.gpio_dev = NULL,
	.gpio_pin = CONFIG_SPI_CS_CTRL_GPIO_PIN,
	.gpio_dt_flags = GPIO_ACTIVE_LOW,
	.delay = 1,
};

static const struct spi_config spi_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
	.frequency = CONFIG_SPI_FREQ_HZ,
	.slave = 0,
    .cs = &spi_cs
};


void spi_init(void) {

	const char* const spiName = CONFIG_SPI_NAME;
	
	if (spi_priv.is_initialized) {
		// skip init if this was previously initialized
		return;
	}

	spi_dev = device_get_binding(spiName);

	if (spi_dev == NULL) {
		printk("Could not get %s device\n", spiName);
		spi_priv.is_initialized = false;
		return;
	}

	// The chip select GPIO needs to be obtained separate from SPI initialization.
	spi_cs.gpio_dev = device_get_binding(CONFIG_SPI_CS_CTRL_GPIO_DEV);

	if (spi_cs.gpio_dev == NULL) {
		printk("Could not get %s device\n", CONFIG_SPI_CS_CTRL_GPIO_DEV);
		spi_priv.is_initialized = false;
		return;
	}

	spi_priv.is_initialized = true;
}

bool spi_is_initialized(void) {
	return spi_priv.is_initialized;
}

/*
Send to SPI and also read back data at the same time.
*/
int spi_send_receive(uint8_t * send_buf, size_t send_length, uint8_t * recv_buf, size_t recv_length) {
	int err = 0;

	// Nordic SPI driver requires things to be converted to their special structs before sending/receiving
	struct spi_buf tx_buf = {
		.buf = send_buf,
		.len = send_length
	};
	struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	struct spi_buf rx_buf = {
		.buf = recv_buf,
		.len = recv_length,
	};
	struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	err = spi_transceive(spi_dev, &spi_cfg, &tx, &rx);

	if (err) {
		LOG_ERR("SPI error: %d\n", err);
	}
	else {
		/* Connect MISO to MOSI for loopback */
		LOG_DBG("TX sent: %x %x %x %x\n", send_buf[0], send_buf[1], send_buf[2], send_buf[3]);
		LOG_DBG("RX recv: %x %x %x %x\n", recv_buf[0], recv_buf[1], recv_buf[2], recv_buf[3]);
		//tx_buffer[0]++;
	}	

	return err;
}

/*
This function is only for testing SPI connection.
*/
void spi_test_send(void)
{
	int err;
	uint8_t tx_buffer[4] = {0xC0, 0xFF, 0x00, 0x00};
	uint8_t rx_buffer[4] = {0x00};

	const struct spi_buf tx_buf = {
		.buf = tx_buffer,
		.len = 4
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	const struct spi_buf rx_buf = {
		.buf = rx_buffer,
		.len = 4,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	err = spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
	if (err) {
		printk("SPI error: %d\n", err);
	} else {
		/* Connect MISO to MOSI for loopback */
		printk("TX sent: %x %x %x %x\n", tx_buffer[0], tx_buffer[1], tx_buffer[2], tx_buffer[3]);
		printk("RX recv: %x %x %x %x\n", rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3]);
		//tx_buffer[0]++;
	}	
}