/* FreeModbus Slave Example ESP32
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"

#include "esp_err.h"
#include "mbcontroller.h"       // for mbcontroller defines and api
#include "modbus_params.h"      // for modbus parameters structures
#include "esp_log.h"            // for log_write
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_log.h"            // for log_write


#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

#define GPIO_OUTPUT_PIN_SEL (1ULL << PIN_NUM_CS)

#define MB_PORT_NUM     (2)   // Number of UART port used for Modbus connection 
#define MB_SLAVE_ADDR   (1)      // The address of device in Modbus network
#define MB_DEV_SPEED    (115200)  // The communication speed of the UART
#define CONFIG_MB_COMM_MODE_RTU (0) // modbus RTU 
#define CONFIG_MB_COMM_MODE_ASCII (1) // define ASCII mode

// Note: Some pins on target chip cannot be assigned for UART communication.
// Please refer to documentation for selected board and target to configure pins using Kconfig.

// Defines below are used to define register start address for each type of Modbus registers
// Offests from first addres data struct holding etc..
#define MB_REG_DISCRETE_INPUT_START         (0x0000)
#define MB_REG_INPUT_START                  (0x0000)
#define MB_REG_HOLDING_START                (0x0000)
#define MB_REG_COILS_START                  (0x0000)

#define MB_PAR_INFO_GET_TOUT                (10) // Timeout for get parameter info
#define MB_CHAN_DATA_MAX_VAL                (6)
#define MB_CHAN_DATA_OFFSET                 (0.2f)
#define MB_READ_MASK                        (MB_EVENT_INPUT_REG_RD \
                                                | MB_EVENT_HOLDING_REG_RD \
                                                | MB_EVENT_DISCRETE_RD \
                                                | MB_EVENT_COILS_RD)
#define MB_WRITE_MASK                       (MB_EVENT_HOLDING_REG_WR \
                                                | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK                  (MB_READ_MASK | MB_WRITE_MASK)

static const char *SLAVE_TAG = "SLAVE_TEST";
static const char *MASTER_TAG = "MASTER_TAG";

// static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;

// Set register values into known state
static void setup_reg_data(void)
{
    // Define initial state of parameters
    discrete_reg_params.discrete_input1 = 1;
    discrete_reg_params.discrete_input3 = 1;
    discrete_reg_params.discrete_input5 = 1;
    discrete_reg_params.discrete_input7 = 1;

    holding_reg_params.holding_data0 = 2.55;
    holding_reg_params.holding_data1 = 3;
    holding_reg_params.holding_data2 = 4;
    holding_reg_params.holding_data3 = 5;

    coil_reg_params.coils_port0 = 0x55;
    coil_reg_params.coils_port1 = 0xAA;

    input_reg_params.input_data0 = 1.12;
    input_reg_params.input_data1 = 2.34;
    input_reg_params.input_data2 = 3.56;
    input_reg_params.input_data3 = 4.78;
}

void task_ModBus(void *pvParameters)
{
    ESP_LOGI(SLAVE_TAG,"sizeof float %u", sizeof(float));
    mb_param_info_t reg_info; // keeps the Modbus registers access information
    mb_communication_info_t comm_info; // Modbus communication parameters
    mb_register_area_descriptor_t reg_area; // Modbus register area descriptor structure

    // Set UART log level
    esp_log_level_set(SLAVE_TAG, ESP_LOG_INFO);
    void* mbc_slave_handler = NULL;

    ESP_ERROR_CHECK(mbc_slave_init(MB_PORT_SERIAL_SLAVE, &mbc_slave_handler)); // Initialization of Modbus controller

    // Setup communication parameters and start stack
#if CONFIG_MB_COMM_MODE_ASCII
    comm_info.mode = MB_MODE_ASCII,
#elif CONFIG_MB_COMM_MODE_RTU
    comm_info.mode = MB_MODE_RTU,
#endif
    comm_info.slave_addr = MB_SLAVE_ADDR;
    comm_info.port = MB_PORT_NUM;
    comm_info.baudrate = MB_DEV_SPEED;
    comm_info.parity = MB_PARITY_NONE;
    ESP_ERROR_CHECK(mbc_slave_setup((void*)&comm_info));

    // The code below initializes Modbus register area descriptors
    // for Modbus Holding Registers, Input Registers, Coils and Discrete Inputs
    // Initialization should be done for each supported Modbus register area according to register map.
    // When external master trying to access the register in the area that is not initialized
    // by mbc_slave_set_descriptor() API call then Modbus stack
    // will send exception response for this register area.
    reg_area.type = MB_PARAM_HOLDING; // Set type of register area
    reg_area.start_offset = MB_REG_HOLDING_START; // Offset of register area in Modbus protocol
    reg_area.address = (void*)&holding_reg_params; // Set pointer to storage instance
    reg_area.size = sizeof(holding_reg_params); // Set the size of register storage instance
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    // Initialization of Input Registers area
    reg_area.type = MB_PARAM_INPUT;
    reg_area.start_offset = MB_REG_INPUT_START;
    reg_area.address = (void*)&input_reg_params;
    reg_area.size = sizeof(input_reg_params);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    // Initialization of Coils register area
    reg_area.type = MB_PARAM_COIL;
    reg_area.start_offset = MB_REG_COILS_START;
    reg_area.address = (void*)&coil_reg_params;
    reg_area.size = sizeof(coil_reg_params);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    // Initialization of Discrete Inputs register area
    reg_area.type = MB_PARAM_DISCRETE;
    reg_area.start_offset = MB_REG_DISCRETE_INPUT_START;
    reg_area.address = (void*)&discrete_reg_params;
    reg_area.size = sizeof(discrete_reg_params);
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_area));

    setup_reg_data(); // Set values into known state

    // Starts of modbus controller and stack
    ESP_ERROR_CHECK(mbc_slave_start());
    
    // Set UART pin numbers
    ESP_ERROR_CHECK(uart_set_pin(MB_PORT_NUM, 17, 16, 4, 2));
                            
    // Set UART driver mode to Half Duplex
    ESP_ERROR_CHECK(uart_set_mode(MB_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX));
                                    
    ESP_LOGI(SLAVE_TAG, "Modbus slave stack initialized.");
    ESP_LOGI(SLAVE_TAG, "Start modbus test...");

    // The cycle below will be terminated when parameter holdingRegParams.dataChan0
    // incremented each access cycle reaches the CHAN_DATA_MAX_VAL value.
    for(;;) {
        // Check for read/write events of Modbus master for certain events
        mb_event_group_t event = mbc_slave_check_event(MB_READ_WRITE_MASK);
        const char* rw_str = (event & MB_READ_MASK) ? "READ" : "WRITE";

        // Filter events and process them accordingly
        if(event & (MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD)) {
            // Get parameter information from parameter queue
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(SLAVE_TAG, "HOLDING %s (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                    rw_str,
                    (uint32_t)reg_info.time_stamp,
                    (uint32_t)reg_info.mb_offset,
                    (uint32_t)reg_info.type,
                    (uint32_t)reg_info.address,
                    (uint32_t)reg_info.size);
            // if (reg_info.address == (uint8_t*)&holding_reg_params.holding_data0)
            // {
            //     portENTER_CRITICAL(&param_lock);
            //     holding_reg_params.holding_data0 += MB_CHAN_DATA_OFFSET;
            //     if (holding_reg_params.holding_data0 >= (MB_CHAN_DATA_MAX_VAL - MB_CHAN_DATA_OFFSET)) {
            //         coil_reg_params.coils_port1 = 0xFF;
            //     }
            //     portEXIT_CRITICAL(&param_lock);
            // }
        } else if (event & MB_EVENT_INPUT_REG_RD) {
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(SLAVE_TAG, "INPUT READ (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                    (uint32_t)reg_info.time_stamp,
                    (uint32_t)reg_info.mb_offset,
                    (uint32_t)reg_info.type,
                    (uint32_t)reg_info.address,
                    (uint32_t)reg_info.size);
        } else if (event & MB_EVENT_DISCRETE_RD) {
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(SLAVE_TAG, "DISCRETE READ (%u us): ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                                (uint32_t)reg_info.time_stamp,
                                (uint32_t)reg_info.mb_offset,
                                (uint32_t)reg_info.type,
                                (uint32_t)reg_info.address,
                                (uint32_t)reg_info.size);
        } else if (event & (MB_EVENT_COILS_RD | MB_EVENT_COILS_WR)) {
            ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
            ESP_LOGI(SLAVE_TAG, "COILS %s (%u us), ADDR:%u, TYPE:%u, INST_ADDR:0x%.4x, SIZE:%u",
                                rw_str,
                                (uint32_t)reg_info.time_stamp,
                                (uint32_t)reg_info.mb_offset,
                                (uint32_t)reg_info.type,
                                (uint32_t)reg_info.address,
                                (uint32_t)reg_info.size);
        }
    }
}

bool send_cmd(spi_device_handle_t spi, const uint8_t cmddd, uint8_t *data)
{
    ESP_LOGI(MASTER_TAG,"SPI sending ");

    spi_transaction_t t;
    uint8_t a, b, c;
    esp_err_t ret;
    int requested = 0;

    // uint8_t cmd[3] = {0x01, 0x80, 0x00 };
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

void task_SPI(void *pvParameters)
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

    for(;;)
    {
        send_cmd(spi, 0x80, &rx);
        vTaskDelay(200);
    }
}


void app_main(void)
{
    xTaskCreate(
                  &task_ModBus,       // Function that implements the task.
                  "ModBus",          // Text name for the task.
                  1024 * 10,      // Stack size in bytes, not words.
                  NULL,    // Parameter passed into the task.
                  10,               // Priority at which the task is created.
                  NULL);          // Array to use as the task's stack.

    xTaskCreate(
                  &task_SPI,       // Function that implements the task.
                  "SPI",          // Text name for the task.
                  1024 * 10,      // Stack size in bytes, not words.
                  ( void * ) 1,    // Parameter passed into the task.
                  5,               // Priority at which the task is created.
                  NULL);          // Array to use as the task's stack.


}