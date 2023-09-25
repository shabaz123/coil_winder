//
// Created by shabaz on 24/09/2023.
//

#include "Keypad.h"
#include "Vfd12.h"
#include <stdio.h>

Keypad keypad;

// Some keypads are different! Modify accordingly.
char keypad_chars[4][4] = {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'0','*','#','D'}
};

extern Vfd12 vfd;
extern absolute_time_t display_hold_time;
extern int display_hold;

// *****************************************************
// ************* interrupt capabilities ****************
// *****************************************************
repeating_timer_t keypad_timer;
int debounce_count;
int do_debounce;
int request_integer;
char stored_content[10];
int toggle=0;

void keypad_timer_init() {
    debounce_count = 0;
    do_debounce = 0;
    request_integer = 0;
    stored_content[0] = '\0';
}

void keypad_timer_start() {
    add_repeating_timer_ms(-10, keypad_callback, NULL, &keypad_timer);
}

void keypad_timer_stop() {
    cancel_repeating_timer(&keypad_timer);
}

int str2int(char *str) {
    int res = 0;
    int slen;
    int i;
    slen = strlen(str);
    for(i=0; i<slen; i++) {
        res = res * 10;
        res = res + (str[i] - '0');
    }
    return res;
}

// store a keypress from elsewhere. For use with integer request
void set_stored_keypress(char key) {
    stored_content[0] = key;
    stored_content[1] = '\0';
}

bool keypad_callback(repeating_timer_t *rt) {
    char key;
    int slen;
    int res;



    key = keypad.get_key_pressed();

    if (key > 0) {
        if (keypad.get_key_event() == key) {
            // key event is already recorded
        } else {
            if (do_debounce==0) {
                // record the key event
                keypad.set_key_event(key);
                do_debounce = 1;
            }
        }
    } else {
        // no key pressed
        if (do_debounce == 1) {
            // key was pressed, but now released
            debounce_count++;
            if (debounce_count >= DEBOUNCE_PERIOD) {
                debounce_count = 0;
                do_debounce = 0;
            }
        }
    }

    if ((request_integer == INTEGER_REQUEST) && (do_debounce == 0)) {
        slen = strlen(stored_content);
        key = keypad.get_key_event();
        if (key > 0) {
            // printf("got a key\n");
            keypad.clear_key_event();
            if (key == KEY_ENTER) {
                request_integer = INTEGER_COMPLETE;
                res = str2int(stored_content);
                keypad.set_integer(res);
                stored_content[0] = '\0';
            } else if (key == KEY_CANCEL) {
                request_integer = NO_INTEGER_REQUEST;
                keypad.set_integer(-1);
                stored_content[0] = '\0';
                vfd.print(0, "Cancelled OK");
                display_hold_time = make_timeout_time_ms(2000);
                display_hold = 1;
            } else if ((key >= '0') && (key <= '9')) {
                if (slen < 9) {
                    stored_content[slen] = key;
                    stored_content[slen + 1] = '\0';
                    slen++;
                    vfd.print(0, "            ");
                    vfd.print(0, stored_content);
                    display_hold_time = make_timeout_time_ms(2000);
                    display_hold = 1;
                } else {
                    // too long
                }
            } else {
                // unexpected key
                request_integer = 0;
                keypad.set_integer(-1);
                stored_content[0] = '\0';
            }
        } else {
            // no key pressed
        }
    }
    return true; // keep repeating
}

// *****************************************************
// *********** Keypad class implementation *************
// *****************************************************

// constructor
Keypad::Keypad() {
    // do nothing
}

void Keypad::init(uint row1_pin, uint row2_pin, uint row3_pin, uint row4_pin, uint col1_pin, uint col2_pin, uint col3_pin, uint col4_pin)
{
    _row1_pin = row1_pin;
    _row2_pin = row2_pin;
    _row3_pin = row3_pin;
    _row4_pin = row4_pin;
    _col1_pin = col1_pin;
    _col2_pin = col2_pin;
    _col3_pin = col3_pin;
    _col4_pin = col4_pin;
    gpio_init(row1_pin);
    gpio_init(row2_pin);
    gpio_init(row3_pin);
    gpio_init(row4_pin);
    gpio_init(col1_pin);
    gpio_init(col2_pin);
    gpio_init(col3_pin);
    gpio_init(col4_pin);
    gpio_set_dir(row1_pin, GPIO_IN);
    gpio_set_dir(row2_pin, GPIO_IN);
    gpio_set_dir(row3_pin, GPIO_IN);
    gpio_set_dir(row4_pin, GPIO_IN);
    gpio_set_dir(col1_pin, GPIO_IN);
    gpio_set_dir(col2_pin, GPIO_IN);
    gpio_set_dir(col3_pin, GPIO_IN);
    gpio_set_dir(col4_pin, GPIO_IN);
    gpio_pull_down(col1_pin);
    gpio_pull_down(col2_pin);
    gpio_pull_down(col3_pin);
    gpio_pull_down(col4_pin);
    gpio_pull_down(row1_pin);
    gpio_pull_down(row2_pin);
    gpio_pull_down(row3_pin);
    gpio_pull_down(row4_pin);
    _stored_key_event = 0;
    _stored_integer = -1;
}



int Keypad::get_key_state(char keyname) {
    int row, col;
    int row_pin, col_pin;
    int retval = 0;

    for (row=0; row<4; row++) {
        for (col=0; col<4; col++) {
            if (keypad_chars[row][col] == keyname) {
                break;
            }
        }
        if (keypad_chars[row][col] == keyname) {
            break;
        }
    }
    if (row == 4 || col == 4) {
        // should never occur. Key not found!
        return -1;
    }

    switch (row) {
        case 0:
            row_pin = _row1_pin;
            break;
        case 1:
            row_pin = _row2_pin;
            break;
        case 2:
            row_pin = _row3_pin;
            break;
        case 3:
            row_pin = _row4_pin;
            break;
    }
    switch (col) {
        case 0:
            col_pin = _col1_pin;
            break;
        case 1:
            col_pin = _col2_pin;
            break;
        case 2:
            col_pin = _col3_pin;
            break;
        case 3:
            col_pin = _col4_pin;
            break;
    }
    // test the row/column
    gpio_set_dir(row_pin, GPIO_OUT);
    gpio_put(row_pin, 1);
    //sleep_us(2);
    busy_wait_us(2);
    if (gpio_get(col_pin) == 1) {
        retval = 1;
    } else {
        retval = 0;
    }
    // set back to input
    gpio_put(row_pin, 0);
    gpio_set_dir(row_pin, GPIO_IN);

    return(retval);
}

char Keypad::get_key_pressed() {
    int row, col;
    int row_pin, col_pin;
    char retval = 0;
    int found = 0;

    for (row=0; row<4; row++) {
        switch (row) {
            case 0:
                row_pin = _row1_pin;
                break;
            case 1:
                row_pin = _row2_pin;
                break;
            case 2:
                row_pin = _row3_pin;
                break;
            case 3:
                row_pin = _row4_pin;
                break;
        }
        gpio_set_dir(row_pin, GPIO_OUT);
        gpio_put(row_pin, 1);
        busy_wait_us(2); // sleep_us cannot be called from a timer callback!
        if (gpio_get(_col1_pin) == 1) {
            col = 0;
            found = 1;
            break;
        }
        if (gpio_get(_col2_pin) == 1) {
            col = 1;
            found = 1;
            break;
        }
        if (gpio_get(_col3_pin) == 1) {
            col = 2;
            found = 1;
            break;
        }
        if (gpio_get(_col4_pin) == 1) {
            col = 3;
            found = 1;
            break;
        }
        gpio_put(row_pin, 0);
        gpio_set_dir(row_pin, GPIO_IN);
    }
    if (found == 0) {
        // no pressed key found
        return(0);
    }
    gpio_put(row_pin, 0);
    gpio_set_dir(row_pin, GPIO_IN);
    return(keypad_chars[row][col]);
}

char Keypad::get_key_event() {
    return(_stored_key_event);
}

void Keypad::set_key_event(char keyname) {
    _stored_key_event = keyname;
}

void Keypad::clear_key_event() {
    _stored_key_event = 0;
}

void Keypad::set_integer(int val) {
    _stored_integer = val;
}

int Keypad::get_integer(void) {
    return(_stored_integer);
}

