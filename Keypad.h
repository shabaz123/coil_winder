//
// Created by shabaz on 24/09/2023.
//

#ifndef MAG_COUNTER_KEYPAD_H
#define MAG_COUNTER_KEYPAD_H

// includes
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "string.h"

// defines
// debounce period in tens of msec
#define DEBOUNCE_PERIOD 5
#define KEY_ENTER '#'
#define KEY_CANCEL '*'
// integer request
#define NO_INTEGER_REQUEST 0
#define INTEGER_REQUEST 1
#define INTEGER_COMPLETE 2

// class definition
class Keypad
{
public:
    Keypad();
    void init(uint row1_pin, uint row2_pin, uint row3_pin, uint row4_pin, uint col1_pin, uint col2_pin, uint col3_pin, uint col4_pin);
    char get_key_pressed();
    int get_key_state(char keyname);
    char get_key_event(); // debounced key events
    void set_key_event(char keyname); // debounced key events
    void clear_key_event(); // clear the event
    void set_integer(int val);
    int get_integer(void);

private:
    uint _row1_pin, _row2_pin, _row3_pin, _row4_pin, _col1_pin, _col2_pin, _col3_pin, _col4_pin;
    char _stored_key_event; // stores the button that was pressed and debounced
    int _stored_integer; // stores an integer if a number sequence is entered on the keypad

};

// function prototypes
void keypad_timer_init();
void keypad_timer_start();
void keypad_timer_stop();
bool keypad_callback(repeating_timer_t *rt);
int str2int(char *str);
void set_stored_keypress(char key); // stores the first digit for an integer about to be entered on the keypad

#endif //MAG_COUNTER_KEYPAD_H
