#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/spi.h"

#include "btle_driver.h"

#include <stdio.h>
#include <stdint.h>

//spi pinout
#define BT_CLK 18
#define BT_MISO 17
#define BT_MOSI 19

//other bt pins
#define BT_BOOT 22
#define BT_CS 21
#define BT_RST 20

//debug uart
#define UART_ID uart0
#define BAUD_RATE 9600
#define TX_PIN 0

#define BT_SPI spi0

void(*bootCallback)(uint, uint32_t);

void init_spi_bt(){
    //init spi for bt
    spi_init(BT_SPI, 1000 * 1000); //1mhz = 1mbs
    gpio_set_function(BT_MISO, GPIO_FUNC_SPI);
    gpio_set_function(BT_CLK, GPIO_FUNC_SPI);
    gpio_set_function(BT_MOSI, GPIO_FUNC_SPI);
}

void reset_bt(){
    gpio_put(BT_CS, 1);

    gpio_put(BT_RST, 0);
    sleep_ms(5);
    gpio_put(BT_RST, 1);
    sleep_ms(5);
}

bool is_data_available_bt(){
    return spi_is_readable(BT_SPI);
}

uint32_t sendrecv_spi_bt(uint8_t* txdata, uint8_t* rxdata, uint32_t length){
    return spi_write_read_blocking(BT_SPI, txdata, rxdata, length);
}

void enable_boot_int_bt(void(*handler)(uint, uint32_t)){
    bootCallback = handler;
    gpio_set_irq_enabled_with_callback(BT_BOOT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, handler);
}

void set_boot_int_bt(uint8_t enable){
    gpio_set_irq_enabled_with_callback(BT_BOOT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, enable, bootCallback);
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

uint32_t __get_PRIMASK(void)
{
  uint32_t result;

  __asm volatile ("MRS %0, primask" : "=r" (result) :: "memory");
  return(result);
}

void __set_PRIMASK(uint32_t priMask)
{
  __asm volatile ("MSR primask, %0" : : "r" (priMask) : "memory");
}

void __disable_irq(void)
{
  __asm volatile ("cpsid i" : : : "memory");
}

int main() {
    stdio_init_all();

    //init debug uart
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);

    printf("Device started!\r\n");
    BTLE_Init();
    
    while(1){

    }
}
