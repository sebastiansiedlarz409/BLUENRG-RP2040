#include "btle_driver.h"
#include "btle_callbacks.h"

#include "BlueNRG-2/bluenrg1_aci.h"
#include "BlueNRG-2/bluenrg1_hci_le.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

extern HANDLE mainCharTxHandle;
extern HANDLE mainCharRxHandle;

void BTLE_ConnectHandler(void){

}

void BTLE_DisconnectHandler(void){

}

void BTLE_AttributeModifiedCallback(uint16_t handle, uint8_t data_length, uint8_t *att_data){
	if(handle == mainCharRxHandle+1){
		printf("Write....\r\n");
	}
}

void BTLE_CharRead(uint16_t connHandle,uint16_t handle,uint16_t offset){
	if(handle == mainCharTxHandle+1){
		printf("Read....\r\n");
		uint8_t data[] = {'R','P','2','0','4','0'};
		uint32_t size = 6;
		aci_gatt_update_char_value(handle-2, handle-1, 0, size, data);
		aci_gatt_allow_read(connHandle);
	}
}
