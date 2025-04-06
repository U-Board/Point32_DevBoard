#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define PIN_NUM_MISO 13
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12
#define PIN_NUM_CS   10
#define SPI_NUM    SPI2_HOST

#define DECODE_MODE_REG     0x09
#define INTENSITY_REG       0x0A
#define SCAN_LIMIT_REG      0x0B
#define SHUTDOWN_REG        0x0C
#define DISPLAY_TEST_REG    0x0F

spi_device_handle_t spi;

void spi_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len==0) return;             //no need to send anything
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=len*8;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=data;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    t.flags     = SPI_TRANS_USE_TXDATA,
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

void spi_cmd(spi_device_handle_t spi, const uint8_t cmd, bool keep_cs_active)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8;                     //Command is 8 bits
    t.tx_buffer=&cmd;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    if (keep_cs_active) {
      t.flags = SPI_TRANS_CS_KEEP_ACTIVE;   //Keep CS active after data transfer
    }
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

uint32_t spi_read(spi_device_handle_t spi) {
	spi_device_acquire_bus(spi, portMAX_DELAY);

	spi_transaction_t t;
	    memset(&t, 0, sizeof(t));
	    t.length=8*3;
	    t.flags = SPI_TRANS_USE_RXDATA;
	    t.user = (void*)1;

	    esp_err_t ret = spi_device_polling_transmit(spi, &t);
	    assert( ret == ESP_OK );

	    // Release bus
	    spi_device_release_bus(spi);

	    return *(uint32_t*)t.rx_data;
}

void app_main(void)
{
	esp_err_t ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=32
    };
    spi_device_interface_config_t devcfg={
            .clock_speed_hz = 1000000,  // 1 MHz
            .mode = 0,                  //SPI mode 0
            .spics_io_num = PIN_NUM_CS,
            .queue_size = 1,
//            .flags = SPI_DEVICE_HALFDUPLEX,
            .pre_cb = NULL,
            .post_cb = NULL,                        //We want to be able to queue 7 transactions at a time
    };

    //Initialize the SPI bus
        ret=spi_bus_initialize(SPI_NUM, &buscfg, SPI_DMA_CH_AUTO);
        ESP_ERROR_CHECK(ret);
        //Attach the LCD to the SPI bus
        ret=spi_bus_add_device(SPI_NUM, &devcfg, &spi);
        ESP_ERROR_CHECK(ret);

        spi_cmd(spi, SCAN_LIMIT_REG, false);
        spi_data(spi, 0x07, 1&0x1F);

        spi_cmd(spi, DECODE_MODE_REG, false);
        spi_data(spi, 0x00, 1);

        spi_cmd(spi, SHUTDOWN_REG, false);
        spi_data(spi, 0x01, 1);

        spi_cmd(spi, DISPLAY_TEST_REG, false);
        spi_data(spi, 0x00, 1);

        spi_cmd(spi, SCAN_LIMIT_REG, false);
        uint32_t test = spi_read(spi);
        printf("here test: %ld", test);

    while (true) {
        printf("Hello from app_main!\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
