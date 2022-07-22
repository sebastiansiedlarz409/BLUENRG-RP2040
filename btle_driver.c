#include "btle_driver.h"
#include "btle_callbacks.h"
#include "BlueNRG-2/bluenrg1_aci.h"
#include "BlueNRG-2/bluenrg1_hci_le.h"
#include "BlueNRG-2/bluenrg1_events.h"
#include "BlueNRG-2/hci_tl.h"
#include "BlueNRG-2/hci.h"

#include "pico/stdlib.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

HANDLE mainServiceTxHandle;
HANDLE mainCharTxHandle;

HANDLE mainServiceRxHandle;
HANDLE mainCharRxHandle;

HANDLE connectionHandle;

uint8_t role;

uint8_t connected = 0;
uint8_t discoverable = 0;

void BTLE_DBG(const char* fmt, ...);

//this is event callback for connection override
void hci_le_connection_complete_event(uint8_t Status, uint16_t Connection_Handle, uint8_t Role, uint8_t Peer_Address_Type, uint8_t Peer_Address[6], uint16_t Conn_Interval, uint16_t Conn_Latency, uint16_t Supervision_Timeout, uint8_t Master_Clock_Accuracy){
  connectionHandle = Connection_Handle;
  connected = 1;

  /* store device role */
  role = Role;

  BTLE_DBG("BTLE Connection with peer Addr: ");
  for (uint8_t i=5; i>0; i--)
  {
	 BTLE_DBG("0x%02X-", Peer_Address[i]);
  }
  BTLE_DBG("0x%02X\r\n", Peer_Address[0]);

  BTLE_ConnectHandler();

}

//this is event callback for disconnection override
void hci_disconnection_complete_event(uint8_t Status, uint16_t Connection_Handle, uint8_t Reason){
	connected = 0;
	discoverable = 0;

	BTLE_DisconnectHandler();

	BTLE_DBG("BTLE Remote peer disconnected...\r\n");
}

//this is event callback for read/write override
void aci_gatt_attribute_modified_event(uint16_t Connection_Handle, uint16_t Attr_Handle, uint16_t Offset, uint16_t Attr_Data_Length, uint8_t Attr_Data[]){
	//call main callback
	BTLE_AttributeModifiedCallback(Attr_Handle, Attr_Data_Length, Attr_Data);
}

void BTLE_DBG(const char* fmt, ...){
	char* buffer = (char*)malloc(100);
	memset(buffer, 0, 100);

	va_list args;
	va_start(args, fmt);

	vsprintf(buffer, fmt, args);

#if DEBUG == 1
	printf(buffer);
#endif

	va_end(args);
	free(buffer);
}

void BTLE_UserEvtRx(void* pData){
	uint32_t i;

	hci_spi_pckt *hci_pckt = (hci_spi_pckt *)pData;

	if(hci_pckt->type == HCI_EVENT_PKT)
	{
		hci_event_pckt *event_pckt = (hci_event_pckt*)hci_pckt->data;

		if(event_pckt->evt == EVT_LE_META_EVENT)
		{
		  evt_le_meta_event *evt = (void *)event_pckt->data;

		  for (i = 0; i < (sizeof(hci_le_meta_events_table)/sizeof(hci_le_meta_events_table_type)); i++)
		  {
			if (evt->subevent == hci_le_meta_events_table[i].evt_code)
			{
			  hci_le_meta_events_table[i].process((void *)evt->data);
			}
		  }
		}
		else if(event_pckt->evt == EVT_VENDOR)
		{
		  evt_blue_aci *blue_evt = (void*)event_pckt->data;

		  for (i = 0; i < (sizeof(hci_vendor_specific_events_table)/sizeof(hci_vendor_specific_events_table_type)); i++)
		  {
			if (blue_evt->ecode == hci_vendor_specific_events_table[i].evt_code)
			{
			  hci_vendor_specific_events_table[i].process((void *)blue_evt->data);
			}
		  }
		}
		else
		{
		  for (i = 0; i < (sizeof(hci_events_table)/sizeof(hci_events_table_type)); i++)
		  {
			if (event_pckt->evt == hci_events_table[i].evt_code)
			{
			  hci_events_table[i].process((void *)event_pckt->data);
			}
		  }
		}
	}
}

uint8_t BTLE_AddServices(void){
	uint8_t ret;

	uint8_t max_attribute_records = 1+3+2;

	//UUIDs
	//53454241-B19E-11E2-9E96-0800200C9A66 (main service TX)
	//53454241-B29E-11E2-9E96-0800200C9A66 (main chr TX)
	//53454241-B39E-11E2-9E96-0800200C9A66 (main service RX)
	//53454241-B49E-11E2-9E96-0800200C9A66 (main chr RX)

	const uint8_t mainServiceTX[16] = {0x66,0x9a,0x0c,0x20,0x00,0x08,0x96,0x9e,0xe2,0x11,0x9e,0xb1,0x41,0x42,0x45,0x53};
	const uint8_t mainCharTX[16] = {0x66,0x9a,0x0c,0x20,0x00,0x08,0x96,0x9e,0xe2,0x11,0x9e,0xb2,0x41,0x42,0x45,0x53};

	const uint8_t mainServiceRX[16] = {0x76,0x9a,0x0c,0x20,0x00,0x08,0x96,0x9e,0xe2,0x11,0x9e,0xb3,0x41,0x42,0x45,0x53};
	const uint8_t mainCharRX[16] = {0x76,0x9a,0x0c,0x20,0x00,0x08,0x96,0x9e,0xe2,0x11,0x9e,0xb4,0x41,0x42,0x45,0x53};

	Service_UUID_t service_uuidTx;
	Char_UUID_t chr_uuidTx;
	Service_UUID_t service_uuidRx;
	Char_UUID_t chr_uuidRx;

	BLUENRG_memcpy(&service_uuidRx.Service_UUID_128, mainServiceRX, 16);
	ret = aci_gatt_add_service(UUID_TYPE_128, &service_uuidRx, PRIMARY_SERVICE, max_attribute_records, &mainServiceRxHandle);
	if (ret != BLE_STATUS_SUCCESS) goto fail;
	BTLE_DBG("BTLE Add Service: %u\r\n", mainServiceRxHandle);

	BLUENRG_memcpy(&chr_uuidRx.Char_UUID_128, mainCharRX, 16);
	ret =  aci_gatt_add_char(mainServiceRxHandle, UUID_TYPE_128, &chr_uuidRx, CHAR_VALUE_LENGTH, CHAR_PROP_WRITE|CHAR_PROP_WRITE_WITHOUT_RESP, ATTR_PERMISSION_NONE, GATT_NOTIFY_ATTRIBUTE_WRITE,16, 1, &mainCharRxHandle);
	if (ret != BLE_STATUS_SUCCESS) goto fail;
	BTLE_DBG("BTLE Add Char: %u\r\n", mainCharRxHandle);

	BLUENRG_memcpy(&service_uuidTx.Service_UUID_128, mainServiceTX, 16);
	ret = aci_gatt_add_service(UUID_TYPE_128, &service_uuidTx, PRIMARY_SERVICE, max_attribute_records, &mainServiceTxHandle);
	if (ret != BLE_STATUS_SUCCESS) goto fail;
	BTLE_DBG("BTLE Add Service: %u\r\n", mainServiceTxHandle);

	BLUENRG_memcpy(&chr_uuidTx.Char_UUID_128, mainCharTX, 16);
	ret =  aci_gatt_add_char(mainServiceTxHandle, UUID_TYPE_128, &chr_uuidTx, CHAR_VALUE_LENGTH, CHAR_PROP_READ, ATTR_PERMISSION_NONE, GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP,16, 1, &mainCharTxHandle);
	if (ret != BLE_STATUS_SUCCESS) goto fail;
	BTLE_DBG("BTLE Add Char: %u\r\n", mainCharTxHandle);

	return 0;

	fail:
	BTLE_DBG("BTLE Adding Service/Characteristic Failed: %u\r\n", ret);
	return 255;
}

uint8_t BTLE_StackInit(void){
	uint8_t ret;

	HANDLE service_handle;
	HANDLE device_name_char_handle;
	HANDLE appearance_char_handle;

	ret = hci_reset();
	if(ret != BLE_STATUS_SUCCESS){
		BTLE_DBG("BTLE HCI Reset Failed: %u\r\n", ret);
		return 1;
	}
	sleep_ms(2000);

	//setup device address
	uint8_t bdaddr[] = {0x3C, 0x53, 0x45, 0x42, 0x41, 0x3E};
	ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bdaddr);
	if(ret != BLE_STATUS_SUCCESS){
		BTLE_DBG("BTLE Setting Addr Failed: %u\r\n", ret);
		return 1;
	}
	else{
		BTLE_DBG("BTLE Addr: ");
		for(uint8_t i = 0;i<5;i++){
			BTLE_DBG("0x%02X-", bdaddr[i]);
		}
		BTLE_DBG("0x%02X\r\n", bdaddr[5]);
	}

	//set the TX power to -2 dBm
	aci_hal_set_tx_power_level(1, 4);

	//init GATT
	ret = aci_gatt_init();
	if(ret != BLE_STATUS_SUCCESS)
	{
		BTLE_DBG("BTLE Init GATT Failed: %u\r\n", ret);
		return 2;
	}

	//init GAP
	//setup role
	ret = aci_gap_init(GAP_PERIPHERAL_ROLE,0x0,0x07, &service_handle,
	                     &device_name_char_handle, &appearance_char_handle);
	if(ret != BLE_STATUS_SUCCESS)
	{
		BTLE_DBG("BTLE Init GAP Failed: %u\r\n", ret);
		return 3;
	}

	//init GATT
	ret = BTLE_AddServices();
	if(ret != BLE_STATUS_SUCCESS)
	{
		BTLE_DBG("BTLE Adding Service Failed: %u\r\n", ret);
		return 4;
	}

	return 0;
}

void BTLE_Init(void){
	uint8_t ret;

	hci_init(BTLE_UserEvtRx, NULL);

	BTLE_DBG("HCI Init... OK\r\n");

	ret = BTLE_StackInit();
	if(ret){
		BTLE_DBG("BTLE Stack Init Failed: %u\r\n", ret);
		while(1){};
	}

	BTLE_DBG("BTLE Stack Init... OK\r\n");

}

void BTLE_SetConnectable(void){
	uint8_t ret;
	char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME, 'B','T','L','E','-','V','I','T','B','E','R','G'};

	ret = hci_le_set_scan_response_data(0, NULL);
	if(ret != BLE_STATUS_SUCCESS) goto fail;

	ret = aci_gap_set_discoverable(ADV_IND, 0, 0, PUBLIC_ADDR, NO_WHITE_LIST_USE, sizeof(local_name), (uint8_t*)local_name, 0, NULL, 0, 0);
	if(ret != BLE_STATUS_SUCCESS) goto fail;

	BTLE_DBG("BTLE Discoverable... OK\r\n");
	discoverable = 1;
	return;

	fail:
	BTLE_DBG("BTLE Discoverable Failed: %u\r\n", ret);
	while(1){};
}

void BTLE_Process(void){
	if(!discoverable){
		BTLE_SetConnectable();
	}

	hci_user_evt_proc();
}
