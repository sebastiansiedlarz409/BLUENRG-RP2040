#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/spi.h"

#include "btle_driver.h"

#include <stdio.h>
#include <stdint.h>

#define BT_SPI spi0

//spi pinout
#define BT_CLK 18
#define BT_MISO 16
#define BT_MOSI 19

//other bt pins
#define BT_BOOT 22
#define BT_CS 17    //21
#define BT_RST 20

//debug uart
#define UART_ID uart0
#define BAUD_RATE 9600
#define TX_PIN 0

void(*bootCallback)(uint, uint32_t);

void init_gpio_bt(){
    gpio_init(BT_RST);
    gpio_init(BT_CS);
    gpio_set_dir(BT_RST, true);
    gpio_set_dir(BT_CS, true);
    gpio_set_slew_rate(BT_RST, GPIO_SLEW_RATE_SLOW);
    gpio_set_slew_rate(BT_CS, GPIO_SLEW_RATE_SLOW);
}

void init_spi_bt(){
    //init spi for bt
    printf("SPI init %d\r\n", spi_init(BT_SPI, 1000 * 1000)); //0.5mhz = 0.5mbs
    spi_set_format(BT_SPI, 8, 0, 1, SPI_MSB_FIRST);
    gpio_set_function(BT_MISO, GPIO_FUNC_SPI);
    gpio_set_function(BT_CLK, GPIO_FUNC_SPI);
    gpio_set_function(BT_MOSI, GPIO_FUNC_SPI);
    init_gpio_bt();
    gpio_put(BT_CS, 1);
}

void deinit_spi_bt(){
    spi_deinit(BT_SPI);
}

void reset_bt(){
    gpio_put(BT_CS, 1);

    gpio_put(BT_RST, 0);
    sleep_ms(5);
    gpio_put(BT_RST, 1);
    sleep_ms(5);
}

bool is_data_available_bt(){
    return gpio_get(BT_BOOT);
}

uint32_t sendrecv_spi_bt(uint8_t* txdata, uint8_t* rxdata, uint32_t length){
    uint32_t bytes = spi_write_read_blocking(BT_SPI, txdata, rxdata, length);
    return bytes;
}

void enable_boot_int_bt(void(*handler)(uint, uint32_t)){
    gpio_init(BT_BOOT);
    gpio_set_dir(BT_BOOT, false);
    gpio_pull_down(BT_RST);
    bootCallback = handler;
    gpio_set_irq_enabled_with_callback(BT_BOOT, GPIO_IRQ_EDGE_RISE, true, handler);
}

void set_boot_int_bt(uint8_t enable){
    gpio_set_irq_enabled_with_callback(BT_BOOT, GPIO_IRQ_EDGE_RISE, enable, bootCallback);
}

int32_t gettick_bt(){
    return (int32_t)to_ms_since_boot(get_absolute_time());
}

uint32_t gettickms_bt(){
    return to_ms_since_boot(get_absolute_time());
}

void write_cs_pin_bt(uint8_t value){
    gpio_put(BT_CS, value);
}

uint8_t read_boot_pin(){
    return gpio_get(BT_BOOT);
}

int main() {
    stdio_init_all();

    //init debug uart
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);

    printf("Device started!\r\n");
    
    BTLE_Init();
    
    while(1){
        BTLE_Process();
    }
}
