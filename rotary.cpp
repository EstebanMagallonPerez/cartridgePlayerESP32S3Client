#include "rotary.h"

RotaryEncoder::RotaryEncoder(uint8_t i2c_address) : i2c_address(i2c_address) {
}

void RotaryEncoder::begin(int sda_pin, int scl_pin) {
    Serial.write("begin Encoder");
    if (sda_pin >= 0 && scl_pin >= 0) {
        Serial.write("wire begin");
        Wire.begin(sda_pin, scl_pin);
    } else {
        Serial.write("also being with no sda scl");
        Wire.begin();
    }
}

uint8_t RotaryEncoder::readRegister(uint8_t reg) {
    Wire.beginTransmission(i2c_address);
    Wire.write(reg);
    Wire.endTransmission();
    
    Wire.requestFrom(i2c_address, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

uint16_t RotaryEncoder::readRegister16(uint8_t reg_high, uint8_t reg_low) {
    uint8_t high = readRegister(reg_high);
    uint8_t low = readRegister(reg_low);
    return ((uint16_t)high << 8) | low;
}

uint16_t RotaryEncoder::readRawAngle() {
    // Read full 12-bit angle value from high and low registers
    return readRegister16(REG_RAW_ANGLE_HIGH, REG_RAW_ANGLE_LOW);
}

float RotaryEncoder::readAngleDegrees() {
    uint16_t raw = readRawAngle();
    // Convert 12-bit (0-4095) to degrees (0-360)
    return (raw / 4095.0) * 360.0;
}

uint8_t RotaryEncoder::readStatus() {
    return readRegister(REG_STATUS);
}

using RotaryDirectionHandler = void (*)(int8_t direction);

static RotaryEncoder encoder;
static uint16_t last_raw = 0xFFFF;
static int last_sector = -1;
static uint8_t numTicksPerRevolution = 1028;
static RotaryDirectionHandler directionHandler = nullptr;

void rotary_init(int sda_pin, int scl_pin, uint8_t ticksPerRevolution) {
    Serial.write("rotary_init");
    encoder.begin(sda_pin, scl_pin);
    numTicksPerRevolution = ticksPerRevolution;
    last_raw = 0xFFFF;
    last_sector = -1;
    directionHandler = nullptr;
}

void rotary_setDirectionHandler(RotaryDirectionHandler handler) {
    directionHandler = handler;
}

void rotary_update() {
    uint16_t raw = encoder.readRawAngle(); // 0–4095
    uint16_t sectorSize = 4096 / numTicksPerRevolution;

    if (last_raw == 0xFFFF) {
        last_raw = raw;
    }

    int diff = (int)raw - (int)last_raw;
    if (diff > 2048) diff -= 4096;
    else if (diff < -2048) diff += 4096;
    int sector = raw / sectorSize;

    if (sector != last_sector) {
        int8_t direction = (diff > 0) ? 1 : -1;
        if (directionHandler) {
            directionHandler(direction);
        }
        last_sector = sector;
    }

    last_raw = raw;
}
