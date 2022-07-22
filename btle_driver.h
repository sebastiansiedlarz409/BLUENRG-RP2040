#ifndef BTLE_DRIVER
#define BTLE_DRIVER

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define DEBUG 1

#define HANDLE uint16_t
#define CHAR_VALUE_LENGTH 63

void BTLE_Init(void);
void BTLE_Process(void);

#endif //BTLE_DRIVER

