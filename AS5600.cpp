//
// Created by shabaz on 22/09/2023.
//

#include "AS5600.h"

// *****************************************************
// ************ AS5600 class implementation *************
// *****************************************************
AS5600::AS5600(uint sda_pin, uint scl_pin)
        : _sda_pin(sda_pin), _scl_pin(scl_pin) {
    switch (sda_pin) {
        case 0:
        case 4:
        case 8:
        case 12:
        case 16:
        case 20:
            _i2c_port = i2c0;
            break;
        case 2:
        case 6:
        case 10:
        case 14:
        case 18:
            _i2c_port = i2c1;
            break;
        default:
            _i2c_port = i2c0; // should not occur
            break;
    }
    i2c_init (_i2c_port, 100000);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);

}

uint8_t AS5600::read8(uint8_t reg)
{
    uint8_t data;
    // write register address:
    i2c_write_blocking(_i2c_port, AS5600_ADDR, &reg, 1, false);
    // read data:
    i2c_read_blocking(_i2c_port, AS5600_ADDR, &data, 1, false);
    return data;
}

uint16_t AS5600::read16(uint8_t reg)
{
    uint16_t data;
    // write register address:
    i2c_write_blocking(_i2c_port, AS5600_ADDR, &reg, 1, false);
    // read data:
    i2c_read_blocking(_i2c_port, AS5600_ADDR, (uint8_t*)&data, 2, false);
    return data;
}

void AS5600::write8(uint8_t reg, uint8_t data)
{
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = data;
    i2c_write_blocking(_i2c_port, AS5600_ADDR, buf, 2, false);
}

void AS5600::write16(uint8_t reg, uint16_t data)
{
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = data >> 8;
    buf[2] = data & 0xff;
    i2c_write_blocking(_i2c_port, AS5600_ADDR, buf, 3, false);
}

int32_t AS5600::getAngle()
{
    int i;
    uint32_t data;
    uint32_t tot = 0;
    for (i=0; i<16; i++) {
        tot += (read16(AS5600_REG_ANGLE) & 0x0fff);
    }
    data = tot >> 4;
    //data = data & 0x0fff;
    return data;
}

void AS5600::init()
{
    //write16(AS5600_REG_ZPOS, 0x0000);
    //write16(AS5600_REG_MPOS, 0x0fff);
    write8(AS5600_REG_CONF, 0x00 /*0x1c*/); // fast filter FTH = 7, 10 LSB
    write8(AS5600_REG_CONF+1, 0x0c); // get out of low-power mode, set hysteresis to max
}
