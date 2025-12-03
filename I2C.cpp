#include "I2C.hpp"
#include "log_vt100.h"
#include <cstdint>
#ifdef I2C_USE_FREERTOS
#include "I2C_freeRTOS.hpp"
#include "FreeRTOS.h"
#endif

I2C::I2C(i2c_inst_t *i2c_instance, uint sda_pin, uint scl_pin) {
    _i2c = i2c_instance;
    _sda = sda_pin;
    _scl = scl_pin;
    _rxBufferIndex = 0;
    _rxBufferLength = 0;
    _txBufferIndex = 0;
    _txBufferLength = 0;
    _transmitting = false;

    if(_i2c == i2c1) {
        _port = 1;
    }else{
        _port = 0;
    }

    LOG_INFO("[I2C] Inicializando I2C para o port %d", _port);

    #ifdef I2C_USE_FREERTOS
    LOG_DEBUG("[I2C] Inicializando semaforo para o port %d", _port);
    const BaseType_t result = initI2CSemaphore(_port);
    if (result != pdPASS) {
        LOG_WARN("[I2C] Failed to initialize semaphore for port %d", _port);
    } else {
        LOG_DEBUG("[I2C] Semaphore initialized for port %d", _port);
    }
    #endif
}

void I2C::begin() {
    i2c_init(_i2c, 100 * 1000); // Default to 100kHz
    gpio_set_function(_sda, GPIO_FUNC_I2C);
    gpio_set_function(_scl, GPIO_FUNC_I2C);
    gpio_pull_up(_sda);
    gpio_pull_up(_scl);
}

void I2C::end() {
    i2c_deinit(_i2c);
    gpio_set_function(_sda, GPIO_FUNC_NULL);
    gpio_set_function(_scl, GPIO_FUNC_NULL);
}

void I2C::setClock(uint frequency) {
    i2c_set_baudrate(_i2c, frequency);
}

/**
 * Start a transmission to the specified address.
 * @param address The 7-bit address of the slave device.
 * @param nostop If true, do not send a stop condition after the transmission.
 */
void I2C::beginTransmission(uint8_t address, bool nostop) {
    _txAddress = address;
    _txBufferIndex = 0;
    _txBufferLength = 0;
    _transmitting = true;
    _nostop = nostop;
    #ifdef I2C_USE_FREERTOS
    const BaseType_t result = takeI2C(_port, portMAX_DELAY);
    if (result != pdPASS) {
        LOG_WARN("[I2C.beginTransaction] Failed to take semaphore for port %d", _port);
    }else{
        LOG_DEBUG("[I2C.beginTransaction] Semaphore taken for port %d", _port);
    }
    #endif
}

/**
 * Write a single byte to the transmission buffer.
 * @param data The byte to write.
 * @return 1 if successful, 0 if the buffer is full or not in a transmission.
 */
size_t I2C::write(uint8_t data) {
    if (!_transmitting || _txBufferLength >= sizeof(_txBuffer)) {
        return 0;
    }
    _txBuffer[_txBufferLength++] = data;
    return 1;
}

/**
 * Write multiple bytes to the transmission buffer.
 * @param data Pointer to the data to write.
 * @param quantity Number of bytes to write.
 * @return The number of bytes written, or 0 if not in a transmission.
 */
size_t I2C::write(const uint8_t *data, size_t quantity) {
    if (!_transmitting) {
        return 0;
    }
    for (size_t i = 0; i < quantity; ++i) {
        if (!write(data[i])) {
            return i;
        }
    }
    return quantity;
}

/**
 * End a transmission and send the data to the slave device.
 * @return 0 on success, otherwise an error code.
 */
uint8_t I2C::endTransmission(void) {
    if (!_transmitting) {
        return 4; // Not in a transmission
    }
    int ret = i2c_write_blocking(_i2c, _txAddress, _txBuffer, _txBufferLength, _nostop);
    _transmitting = false;
    #ifdef I2C_USE_FREERTOS
    const BaseType_t result = releaseI2C(_port);
    if (result != pdPASS) {
        LOG_WARN("[I2C.endTransmission] Failed to release semaphore for port %d", _port);
    } else {
        LOG_DEBUG("[I2C.endTransmission] Semaphore released for port %d", _port);
    }
    #endif
    if (ret == PICO_ERROR_GENERIC) {
        return 2; // NACK on address
    } else if (ret < _txBufferLength) {
        return 3; // NACK on data
    }
    return 0; // Success
}

/**
 * Request data from a device on the I2C bus.
 * @param address The 7-bit address of the device to request data from.
 * @param quantity The number of bytes to request.
 * @param nostop If true, do not send a stop condition after the read.
 * @return The number of bytes actually read, or 0 on error.
 */
uint8_t I2C::requestFrom(uint8_t address, size_t quantity, bool nostop) {
    // Clamp quantity to the size of the buffer
    if (quantity > sizeof(_rxBuffer)) {
        quantity = sizeof(_rxBuffer);
    }

    // If there's anything in the transmit buffer, send it first.
    // This is typical for setting a register address before reading.
    if (_txBufferLength > 0) {
        int ret = i2c_write_blocking(_i2c, address, _txBuffer, _txBufferLength, _nostop); // nostop = true
        if (ret < _txBufferLength) {
            // Error during write, abort
            _rxBufferLength = 0;
            _rxBufferIndex = 0;
            return 0;
        }

        // Clear the transmit buffer after successful write
        _txBufferLength = 0;
    }

    // Now, read the data
    int bytes_read = i2c_read_blocking(_i2c, address, _rxBuffer, quantity, nostop);

    if (bytes_read < 0) {
        _rxBufferLength = 0; // Error
    } else {
        _rxBufferLength = bytes_read;
    }
    _rxBufferIndex = 0;
    return _rxBufferLength;
}

/**
 * Get the number of bytes available to read.
 * @return The number of bytes available in the receive buffer.
 */
int I2C::available(void) {
    return _rxBufferLength - _rxBufferIndex;
}

/**
 * Read one byte from the receive buffer.
 * @return The next byte in the buffer, or -1 if no bytes are available.
 */
int I2C::read(void) {
    if (_rxBufferIndex < _rxBufferLength) {
        return _rxBuffer[_rxBufferIndex++];
    }
    return -1;
}

void I2C::read(uint8_t * buffer, uint len) {
    for (uint i = 0; i < len && _rxBufferIndex < _rxBufferLength; i++) {
        buffer[i] = _rxBuffer[_rxBufferIndex++];
    }
}
