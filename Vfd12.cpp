//
// Created by shabaz on 22/09/2023.
// rev 1 - Sep 2023 - shabaz
// based on https://github.com/hexesdesu/ESP32VFDClock/blob/master/ESP32VFDClock/ESP32VFDClock.ino
// VFD controller is PT6312
//

#include "Vfd12.h"
#include "pico/stdlib.h"

// defines
#define digitalWrite gpio_put
#define LOW 0
#define HIGH 1

// *****************************************************
// ************ Vfd12 class implementation *************
// *****************************************************
Vfd12::Vfd12(uint din_pin, uint clk_pin, uint cs_pin, uint en_pin)
        : _din_pin(din_pin), _clk_pin(clk_pin), _cs_pin(cs_pin), _en_pin(en_pin)
{
    gpio_init(_din_pin);
    gpio_set_dir(_din_pin, GPIO_OUT);
    gpio_init(_clk_pin);
    gpio_set_dir(_clk_pin, GPIO_OUT);
    gpio_init(_cs_pin);
    gpio_set_dir(_cs_pin, GPIO_OUT);
    gpio_init(_en_pin);
    gpio_set_dir(_en_pin, GPIO_OUT);

    digitalWrite(_cs_pin, HIGH);
    digitalWrite(_en_pin, HIGH);
    digitalWrite(_din_pin, LOW);
    digitalWrite(_clk_pin, LOW);
}

void Vfd12::init()
{
    sleep_ms(10); // 10msec delay
    digitalWrite(_en_pin, HIGH);
    sleep_ms(10);

    digitalWrite(_cs_pin, LOW);
    write(0xe0);
    sleep_us(5);
    write(11); // set to 11 for 12 character display
    sleep_us(5);
    digitalWrite(_cs_pin, HIGH);
    sleep_us(5);

    digitalWrite(_cs_pin, LOW);
    write(0xe4);
    sleep_us(5);
    write(0x33); //set brightness
    sleep_us(5);
    digitalWrite(_cs_pin, HIGH);
    sleep_us(5);
}

void Vfd12::write(uint8_t data)
{
    unsigned char i;
    for (i = 0; i < 8; i++)
    {
        digitalWrite(_clk_pin, LOW);
        if (data & 0x01)
            digitalWrite(_din_pin, HIGH);
        else
            digitalWrite(_din_pin, LOW);
        data >>= 1;
        //sleep_us(5);
        busy_wait_us(5);
        digitalWrite(_clk_pin, HIGH);
        //sleep_us(5);
        busy_wait_us(5);
    }
}

void Vfd12::cmd(uint8_t command)
{
    digitalWrite(_cs_pin, LOW);
    write(command);
    digitalWrite(_cs_pin, HIGH);
    sleep_us(5);
}

void Vfd12::show()
{
    digitalWrite(_cs_pin, LOW);
    write(0xe8);
    digitalWrite(_cs_pin, HIGH);
}

void Vfd12::custprog(unsigned char idx, const char *str)
{
    unsigned char i;
    digitalWrite(_cs_pin, LOW);
    write(0x40 + idx);
    for (i = 0; i < 5; i++)
    {
        write(*str);
        str++;
    }
    digitalWrite(_cs_pin, HIGH);
    show();
}

void Vfd12::printchar(unsigned char x, unsigned char chr)
{
    digitalWrite(_cs_pin, LOW);
    write(0x20 + x);
    write(chr);
    digitalWrite(_cs_pin, HIGH);
    show();
}

void Vfd12::print(unsigned char idx, const char *str)
{
    digitalWrite(_cs_pin, LOW);  // start transfer
    write(0x20 + idx); // address reg start position
    while (*str)
    {
        write(*str); //ascii convert with corresponding character table
        str++;
    }
    digitalWrite(_cs_pin, HIGH); // stop transmission
    show();
}