#ifndef BTLE_CALLBACKS
#define BTLE_CALLBACKS

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

void BTLE_ConnectHandler(void);
void BTLE_DisconnectHandler(void);
void BTLE_AttributeModifiedCallback(uint16_t handle, uint8_t data_length, uint8_t *att_data);

#endif //BTLE_CALLBACKS
