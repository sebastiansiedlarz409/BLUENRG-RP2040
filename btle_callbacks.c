#include "btle_driver.h"
#include "btle_callbacks.h"

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

void BTLE_CommandsHandler(uint8_t size, uint8_t *buffer){
	if(buffer[0] == 0)
		return;


	memset(buffer, 0, size);
}

void BTLE_AttributeModifiedCallback(uint16_t handle, uint8_t data_length, uint8_t *att_data){
	if(handle == mainCharRxHandle+1){
		BTLE_CommandsHandler(data_length, att_data);
	}
}
