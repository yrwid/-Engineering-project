/* SPI Master example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_log.h"            // for log_write

static const char *MASTER_TAG = "MASTER_TAG";

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

#define GPIO_OUTPUT_PIN_SEL (1ULL << PIN_NUM_CS)

const uint8_t adc_single_ch0   = (0x08);     // ADC Single Channel 0
const uint8_t adc_single_ch1   = (0x09);     // ADC Single Channel 1
const uint8_t adc_single_ch2   = (0x0A);     // ADC Single Channel 2
const uint8_t adc_single_ch3   = (0x0B);     // ADC Single Channel 3
const uint8_t adc_single_ch4   = (0x0C);     // ADC Single Channel 4
const uint8_t adc_single_ch5   = (0x0D);     // ADC Single Channel 5
const uint8_t adc_single_ch6   = (0x0E);     // ADC Single Channel 6
const uint8_t adc_single_ch7   = (0x0F);     // ADC Single Channel 7

void cs_gpio_setting()
{
    gpio_config_t io_conf;

    //disable interrupt 
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;

    //set as output 
    io_conf.mode = GPIO_MODE_OUTPUT;

    //bit mask for the pins to set
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;

    //disable pull-down
    io_conf.pull_down_en = 0;

    //disable pull-dup
    io_conf.pull_up_en = 0;

    //configure GPIO
    gpio_config(&io_conf);

}

void cs_low()
{
    gpio_set_level(PIN_NUM_CS, 0);

}

void cs_high()
{
    gpio_set_level(PIN_NUM_CS, 1);
    
}

void setup_mcp3008()
{
    cs_low();
    cs_high();
}


bool send_cmd(spi_device_handle_t spi, const uint8_t cmddd, uint8_t *data)
{
    ESP_LOGI(MASTER_TAG,"SPI sending ");

    spi_transaction_t t;
    uint8_t a, b, c, d;
    esp_err_t ret;
    int requested = 0;

    uint8_t cmd[3] = {0x01, 0x80, 0x00 };
    //pull cs line to low 
    // cs_low();

    //send start bit 
    // uint8_t start = 0x01;
    memset(&t, 0, sizeof(t));
    t.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    t.user = (void*)requested;
    t.length = 3*8;
    t.tx_data[0] = 1;
    t.tx_data[1] = 0x80;
    ret = spi_device_transmit(spi, &t);
    assert(ret==ESP_OK);

    // a = t.rx_data[0];
    a = t.rx_data[0];
    b = t.rx_data[1] & 0x03;
    c = t.rx_data[2];
    
    ESP_LOGI(MASTER_TAG, "received data 1: %d, received data 2: %d, received3: %d ", a, b,  c );
    
    return true;
}

void app_main()
{
    ESP_LOGI(MASTER_TAG,"Entry point");

    esp_err_t ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg={
        .miso_io_num=PIN_NUM_MISO,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };

    spi_device_interface_config_t devcfg={
        .clock_speed_hz=1*1000*1000,               //Clock out at 2 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=7                          //We want to be able to queue 7 transactions at a time
    };

    uint8_t rx;

    //Initialize the SPI bus
    ret=spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    assert(ret==ESP_OK);

    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(VSPI_HOST, &devcfg, &spi);
    assert(ret==ESP_OK);

    ESP_LOGI(MASTER_TAG,"Enter initialization");

    int i;
    // cs_gpio_setting();
    // setup_mcp3008();
    for(i = 0; i<150; i++)
    {
        send_cmd(spi, (adc_single_ch0 << 4), &rx);
        vTaskDelay(200);
    }
    //close spi bus 
}
