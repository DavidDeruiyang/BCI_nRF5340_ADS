#pragma once

#include <stdint.h>
#include <stdio.h>

/* Configurations SPI */
#define CONFIG_SPI_NAME               "SPI_4"  // Using SPI 4, the only high speed spi
#define CONFIG_SPI_CS_CTRL_GPIO_DEV   "GPIO_1"
#define CONFIG_SPI_CS_CTRL_GPIO_PIN   12  // This has to match the dts overlay nrf5340dk_nrf5340_cpuapp.overlay
#define CONFIG_SPI_FREQ_HZ          200000
//#define CONFIG_SPI_FREQ_HZ           16000000


typedef struct spi_priv_t {
    bool is_initialized;
} spi_priv_t;

void spi_init(void);
void spi_test_send(void);
bool spi_is_initialized(void);
int spi_send_receive(uint8_t * send_buf, size_t send_length, uint8_t * recv_buf, size_t recv_length);