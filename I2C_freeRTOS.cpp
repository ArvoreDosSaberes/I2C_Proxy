
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "portmacro.h"
#include "semphr.h"

#include "stdio.h"

#include "log_vt100.h"
#include "I2C_freeRTOS.hpp"

extern "C" {

static SemaphoreHandle_t i2cSemaphores[2] = { NULL, NULL };

BaseType_t initI2CSemaphore(uint8_t port) {
    if (port >= 2) {
        LOG_WARN("[I2C] Invalid port index %u for semaphore init", port);
        return pdFAIL;
    }

    if (i2cSemaphores[port] == NULL) {
        i2cSemaphores[port] = xSemaphoreCreateMutex();
    }

    if (i2cSemaphores[port] == NULL) {
        LOG_WARN("[I2C] Semaphore for port %u not initialized", port);
        return pdFAIL;
    }

    LOG_INFO("[I2C] Semaphore for port %u initialized", port);
    return pdPASS;
}

BaseType_t takeI2C(uint8_t port, TickType_t delay) {
    if (port >= 2) {
        LOG_WARN("[I2C] Invalid port index %u for take", port);
        return pdFAIL;
    }

    if (i2cSemaphores[port] == NULL) {
        LOG_WARN("[I2C] Semaphore for port %u not initialized", port);
        return pdFAIL;
    }

    LOG_DEBUG("[I2C] Semaphore for port %u taken", port);
    return xSemaphoreTake(i2cSemaphores[port], delay);
}

BaseType_t releaseI2C(uint8_t port) {
    if (port >= 2) {
        LOG_WARN("[I2C] Invalid port index %u for release", port);
        return pdFAIL;
    }

    if (i2cSemaphores[port] == NULL) {
        LOG_WARN("[I2C] Semaphore for port %u not initialized", port);
        return pdFAIL;
    }

    LOG_DEBUG("[I2C] Semaphore for port %u released", port);
    return xSemaphoreGive(i2cSemaphores[port]);
}

}
