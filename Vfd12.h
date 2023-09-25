//
// Created by shabaz on 22/09/2023.
//

#ifndef MAG_COUNTER_VFD12_H
#define MAG_COUNTER_VFD12_H
// defines
#include "pico/stdlib.h"
// class definition
class Vfd12
{
public:
    // Vfd12 object requres 4 pins for use as digital outputs
    Vfd12(uint din_pin, uint clk_pin, uint cs_pin, uint en_pin);
    // init() will enable the display, set 12 character mode, and set brightness
    void init();
    // show() might not be needed by the user, but is used by the library
    void show();
    // custprog() will program a custom character into the controller
    void custprog(unsigned char idx, const char *str);
    // printchar() will print a single character to the display, idx is the position (0-11)
    void printchar(unsigned char x, unsigned char chr);
    // print() will print a string to the display, idx is the position (0-11)
    void print(unsigned char idx, const char *str);
private:
    // write() will write a byte to the controller chip
    void write(uint8_t data);
    // cmd() will write a command byte to the controller chip
    void cmd(uint8_t command);
    // member variables
    uint _din_pin;
    uint _clk_pin;
    uint _cs_pin;
    uint _en_pin;
};

#endif //MAG_COUNTER_VFD12_H
