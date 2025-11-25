#pragma once

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "portmacro.h"
#include "semphr.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

BaseType_t initI2CSemaphore(uint8_t port);
BaseType_t takeI2C(uint8_t port, TickType_t delay);
BaseType_t releaseI2C(uint8_t port);

#ifdef __cplusplus
}
#endif
