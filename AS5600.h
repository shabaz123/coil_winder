//
// Created by shabaz on 22/09/2023.
//

#ifndef MAG_COUNTER_AS5600_H
#define MAG_COUNTER_AS5600_H

// includes
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// defines
#define AS5600_ADDR 0x36
// registers
#define AS5600_REG_CONF 0x07
#define AS5600_REG_RAW_ANGLE 0x0C
#define AS5600_REG_ANGLE 0x0E
#define AS5600_REG_ZPOS 0x01
#define AS5600_REG_MPOS 0x03

// class definition
// class definition
class AS5600
{
public:
    AS5600(uint sda_pin, uint scl_pin);
    void init();
    int32_t getAngle();

private:
    uint8_t read8(uint8_t reg);
    uint16_t read16(uint8_t reg);
    void write8(uint8_t reg, uint8_t data);
    void write16(uint8_t reg, uint16_t data);

    // member variables
    uint _sda_pin;
    uint _scl_pin;
    i2c_inst_t* _i2c_port;
};

#endif //MAG_COUNTER_AS5600_H
