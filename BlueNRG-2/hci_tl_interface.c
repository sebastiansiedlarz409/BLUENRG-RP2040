/**
  ******************************************************************************
  * @file    hci_tl_interface.c
  * @author  SRA Application Team
  * @brief   This file provides the implementation for all functions prototypes
  *          for the STM32 BlueNRG HCI Transport Layer interface.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "RTE_Components.h"

#include "hci_tl.h"

/* Defines -------------------------------------------------------------------*/

#define HEADER_SIZE       5U
#define MAX_BUFFER_SIZE   255U
#define TIMEOUT_DURATION  15U
#define TIMEOUT_IRQ_HIGH  1000U

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void HCI_TL_SPI_Enable_IRQ(void);
static void HCI_TL_SPI_Disable_IRQ(void);
static int32_t IsDataAvailable(void);

extern void init_spi_bt();
extern void reset_bt();
extern bool is_data_available_bt();
extern uint32_t sendrecv_spi_bt(uint8_t* txdata, uint8_t* rxdata, uint32_t length);
extern void enable_boot_int_bt(void(*handler)(uint, uint32_t));
extern void set_boot_int_bt(uint8_t enable);
extern int32_t gettick_bt();
extern uint32_t gettickms_bt();
extern void write_cs_pin_bt(uint8_t value);
extern uint8_t read_boot_pin();

/******************** IO Operation and BUS services ***************************/
/**
 * @brief  Enable SPI IRQ.
 * @param  None
 * @retval None
 */
static void HCI_TL_SPI_Enable_IRQ(void)
{
  set_boot_int_bt(1);
}

/**
 * @brief  Disable SPI IRQ.
 * @param  None
 * @retval None
 */
static void HCI_TL_SPI_Disable_IRQ(void)
{
  set_boot_int_bt(0);
}

/**
 * @brief  Initializes the peripherals communication with the BlueNRG
 *         Expansion Board (via SPI, I2C, USART, ...)
 *
 * @param  void* Pointer to configuration struct
 * @retval int32_t Status
 */
int32_t HCI_TL_SPI_Init(void* pConf)
{
  init_spi_bt();
  return 0;
}

/**
 * @brief  DeInitializes the peripherals communication with the BlueNRG
 *         Expansion Board (via SPI, I2C, USART, ...)
 *
 * @param  None
 * @retval int32_t 0
 */
int32_t HCI_TL_SPI_DeInit(void)
{
  return 0;
}

/**
 * @brief Reset BlueNRG module.
 *
 * @param  None
 * @retval int32_t 0
 */
int32_t HCI_TL_SPI_Reset(void)
{
  reset_bt();
  return 0;
}

/**
 * @brief  Reads from BlueNRG SPI buffer and store data into local buffer.
 *
 * @param  buffer : Buffer where data from SPI are stored
 * @param  size   : Buffer size
 * @retval int32_t: Number of read bytes
 */
int32_t HCI_TL_SPI_Receive(uint8_t* buffer, uint16_t size)
{
  uint16_t byte_count;
  uint8_t len = 0;
  uint8_t char_00 = 0x00;
  volatile uint8_t read_char;

  uint8_t header_master[HEADER_SIZE] = {0x0b, 0x00, 0x00, 0x00, 0x00};
  uint8_t header_slave[HEADER_SIZE];

  HCI_TL_SPI_Disable_IRQ();

  /* CS reset */
  write_cs_pin_bt(0);

  /* Read the header */
  sendrecv_spi_bt(header_master, header_slave, HEADER_SIZE);

  /* device is ready */
  byte_count = (header_slave[4] << 8)| header_slave[3];

  if(byte_count > 0)
  {

    /* avoid to read more data than the size of the buffer */
    if (byte_count > size)
    {
      byte_count = size;
    }

    for(len = 0; len < byte_count; len++)
    {
      sendrecv_spi_bt(&char_00, (uint8_t*)&read_char, 1);
      buffer[len] = read_char;
    }
  }

  /**
   * To be aligned to the SPI protocol.
   * Can bring to a delay inside the frame, due to the BlueNRG-2 that needs
   * to check if the header is received or not.
   */
  uint32_t tickstart = gettickms_bt();
  while ((gettickms_bt() - tickstart) < TIMEOUT_IRQ_HIGH) {
    if (read_boot_pin()==0) {
      break;
    }
  }
  HCI_TL_SPI_Enable_IRQ();

  /* Release CS line */
  write_cs_pin_bt(1);

  return len;
}

/**
 * @brief  Writes data from local buffer to SPI.
 *
 * @param  buffer : data buffer to be written
 * @param  size   : size of first data buffer to be written
 * @retval int32_t: Number of read bytes
 */
int32_t HCI_TL_SPI_Send(uint8_t* buffer, uint16_t size)
{
  int32_t result;
  uint16_t rx_bytes;

  uint8_t header_master[HEADER_SIZE] = {0x0a, 0x00, 0x00, 0x00, 0x00};
  uint8_t header_slave[HEADER_SIZE];

  static uint8_t read_char_buf[MAX_BUFFER_SIZE];
  uint32_t tickstart = gettickms_bt();

  HCI_TL_SPI_Disable_IRQ();

  do
  {
    uint32_t tickstart_data_available = gettickms_bt();

    result = 0;

    /* CS reset */
    write_cs_pin_bt(0);

    /*
     * Wait until BlueNRG-2 is ready.
     * When ready it will raise the IRQ pin.
     */
    while(!IsDataAvailable())
    {
      if((gettickms_bt() - tickstart_data_available) > TIMEOUT_DURATION)
      {
        result = -3;
        break;
      }
    }
    if(result == -3)
    {
      /* The break causes the exiting from the "while", so the CS line must be released */
      write_cs_pin_bt(1);
      break;
    }

    /* Read header */
    sendrecv_spi_bt(header_master, header_slave, HEADER_SIZE);

    rx_bytes = (((uint16_t)header_slave[2])<<8) | ((uint16_t)header_slave[1]);

    if(rx_bytes >= size)
    {
      /* Buffer is big enough */
      sendrecv_spi_bt(buffer, read_char_buf, size);
    }
    else
    {
      /* Buffer is too small */
      result = -2;
    }

    /* Release CS line */
    write_cs_pin_bt(1);

    if((gettickms_bt() - tickstart) > TIMEOUT_DURATION)
    {
      result = -3;
      break;
    }
  } while(result < 0);

  /**
   * To be aligned to the SPI protocol.
   * Can bring to a delay inside the frame, due to the BlueNRG-2 that needs
   * to check if the header is received or not.
   */
  tickstart = gettickms_bt();
  while ((gettickms_bt() - tickstart) < TIMEOUT_IRQ_HIGH) {
    if (read_boot_pin()==0) {
      break;
    }
  }
  HCI_TL_SPI_Enable_IRQ();

  return result;
}

/**
 * @brief  Reports if the BlueNRG has data for the host micro.
 *
 * @param  None
 * @retval int32_t: 1 if data are present, 0 otherwise
 */
static int32_t IsDataAvailable(void)
{
  return is_data_available_bt();
}

//wrapper for hci_tl_lowlevel_isr below
void hci_tl_lowlevel_isr_wrapper(uint gpio, uint32_t event_mask){
  hci_tl_lowlevel_isr();
}

/***************************** hci_tl_interface main functions *****************************/
/**
 * @brief  Register hci_tl_interface IO bus services
 *
 * @param  None
 * @retval None
 */
void hci_tl_lowlevel_init(void)
{
  tHciIO fops;

  /* Register IO bus services */
  fops.Init    = HCI_TL_SPI_Init;
  fops.DeInit  = HCI_TL_SPI_DeInit;
  fops.Send    = HCI_TL_SPI_Send;
  fops.Receive = HCI_TL_SPI_Receive;
  fops.Reset   = HCI_TL_SPI_Reset;
  fops.GetTick = gettick_bt;

  hci_register_io_bus (&fops);

  /* Register event irq handler */
  enable_boot_int_bt(hci_tl_lowlevel_isr_wrapper);
}

/**
  * @brief HCI Transport Layer Low Level Interrupt Service Routine
  *
  * @param  None
  * @retval None
  */
void hci_tl_lowlevel_isr(void)
{
  /* Call hci_notify_asynch_evt() */
  while(IsDataAvailable())
  {
    if (hci_notify_asynch_evt(NULL))
    {
      return;
    }
  }
}
