#ifndef ROTARY_H
#define ROTARY_H

#include <Wire.h>
#include <Arduino.h>
#include <stdint.h>

#define ROTARY_SDA_PIN 14
#define ROTARY_SCL_PIN 8

class RotaryEncoder {
public:
    // Constructor
    RotaryEncoder(uint8_t i2c_address = 0x36);
    
    // Initialize I2C communication with optional custom pins
    void begin(int sda_pin = ROTARY_SDA_PIN, int scl_pin = ROTARY_SCL_PIN);
    
    // Read raw angle (12-bit: 0-4095)
    uint16_t readRawAngle();
    
    // Read angle in degrees (0-360)
    float readAngleDegrees();
    
    // Read angle in radians (0-2π)
    float readAngleRadians();
    
    // Read status register
    uint8_t readStatus();

private:
    uint8_t i2c_address;

    // Register addresses for AS5600
    static const uint8_t REG_RAW_ANGLE_HIGH = 0x0E;
    static const uint8_t REG_RAW_ANGLE_LOW = 0x0F;
    static const uint8_t REG_STATUS = 0x0B;
    
    // I2C read helper
    uint8_t readRegister(uint8_t reg);
    uint16_t readRegister16(uint8_t reg_high, uint8_t reg_low);
};

using RotaryDirectionHandler = void (*)(int8_t direction);

void rotary_init(int sda_pin = ROTARY_SDA_PIN, int scl_pin = ROTARY_SCL_PIN, uint8_t ticksPerRevolution = 25);
void rotary_setDirectionHandler(RotaryDirectionHandler handler);
void rotary_update();

#endif // ROTARY_H
