/************************************************************************
 * mag_counter
 * main.cpp
 * rev 1.0 
 ************************************************************************/

// ************* header files ******************
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "hardware/i2c.h"
#include "Vfd12.h"
#include "L6219.h"
#include "Keypad.h"

// ***************** defines *******************
// Pico-Eurocard used GPIO22 for the LED, otherwise use GPIO25
#define LED_PIN 25
#define BUTTON_PIN 27

//Board LED
#define PICO_LED_ON gpio_put(LED_PIN, 1)
#define PICO_LED_OFF gpio_put(LED_PIN, 0)
// Inputs
#define BUTTON_UNPRESSED (gpio_get(BUTTON_PIN)!=0)
#define BUTTON_PRESSED (gpio_get(BUTTON_PIN)==0)
// VFD pins
#define DIN_PIN 2
#define CLK_PIN 3
#define CS_PIN 4
#define EN_PIN 5
// stepper motor driver (L6219) pins
#define ph1_pin 0
#define ph2_pin 1
#define i01_pin 6
#define i11_pin 7
#define i02_pin 8
#define i12_pin 9
// stepper motor directions
#define DIR_CW 1
#define DIR_CCW 0
#define STOP 0
#define GO 1
// keypad pins
#define COL1_PIN 10
#define COL2_PIN 11
#define COL3_PIN 12
#define COL4_PIN 13
#define ROW1_PIN 14
#define ROW2_PIN 15
#define ROW3_PIN 16
#define ROW4_PIN 17
// keypad key states
#define KEY_PRESSED 1
#define KEY_UNPRESSED 0
// options
#define OPTION_NULL 0
#define OPTION_DIR 1
#define OPTION_ZEROIZE 2
#define OPTION_MAN_NORM 3
// VFD symbols
#define UP_ARROW 3
#define DOWN_ARROW 6
// misc
#define FOREVER 1

// ************ constants **********************
// Custom characters are 5 bytes long for 7x5 dot matrix,
// where the most significant bit (bit 7) is always zero.
// Each byte corresponds to one column of pixels,
// where the first byte is the leftmost column.
// The lowest pixel in a column is bit 6,
// and the highest pixel in a column is bit 0
const char CUST_CHAR[][5] = {
        {0x7f, 0x3e, 0x1c, 0x08, 0x00}, // 0: right-pointing triangle
        {0x00, 0x08, 0x1c, 0x3e, 0x7f}, // 1: left-pointing triangle
        {0x7f, 0x7f, 0x00, 0x7f, 0x7f}, // 2: pause symbol
        {0x04, 0x06, 0x7f, 0x06, 0x04}, // 3: up-arrow
        {0x07, 0x05, 0x07, 0x00, 0x00}, // 4: degree symbol
        {0x7f, 0x07, 0x04, 0x04, 0x07}, // 5: u (micro) symbol
        {0x10, 0x30, 0x7f, 0x30, 0x7f}, // 6: down-arrow
        {0x0c, 0x12, 0x7f, 0x12, 0x0c}, // 7: phi character
};

// ********** external variables ***************
extern L6219* stepper_instance[MAX_MOTORS];
extern int stepper_instance_count;
extern int request_integer;
extern Keypad keypad;

// ************ global variables ****************
Vfd12 vfd(DIN_PIN, CLK_PIN, CS_PIN, EN_PIN);
L6219 stepper_motor(ph1_pin, i01_pin, i11_pin, ph2_pin, i02_pin, i12_pin);
int cur_turns = 0; // current number of turns
absolute_time_t display_hold_time; // how long the VFD display is held for
int display_hold; // this gets set to 1 when it is desired to hold the display for a time
int option_selection; // this gets set to the option selected by the user ('C' keypad button)


// ************ function prototypes *************


// ********** functions *************************
void
print_title(void) {
    printf("\n\n");
    printf("Project built on %s %s\n", __DATE__, __TIME__);
    printf("\n");
}

// general-purpose long delay timer if required
void
sleep_sec(uint32_t s) {
    sleep_ms(s * 1000);
}


// board initialisation
void
board_init(void) {
    int i;
    // wait for devices to come out of power-on reset
    sleep_ms(100);
    // LED on Pico board
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // button for input
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_set_pulls(BUTTON_PIN, true, false); // pullup enabled

    // VFD initialisation done in main function since we need it earlier.
    // vfd.init();
    // set up the custom characters
    for (i=0; i<8; i++) {
        vfd.custprog(i, CUST_CHAR[i]);
    }

    // stepper motor initialisation
    stepper_motor.init();

    // stepper timer system initialisation
    stepper_timers_init();

    // keypad initialisation
    keypad.init(ROW1_PIN, ROW2_PIN, ROW3_PIN, ROW4_PIN,COL1_PIN, COL2_PIN, COL3_PIN, COL4_PIN);

}

// VFD helper functions
void spinning_bar(int pos, int sec) {
    int i, j;
    for (i=0; i<sec; i++) {
        for (j=0; j<2; j++) {
            vfd.printchar(pos, '|');
            sleep_ms(125);
            vfd.printchar(pos, '/');
            sleep_ms(125);
            vfd.printchar(pos, '-');
            sleep_ms(125);
            vfd.printchar(pos, '\\');
            sleep_ms(125);
        }
    }
}

void vfd_cls(void) {
    vfd.print(0, "            ");
}

// options functions
void do_opt_dir(int* direction, int done = 0) {
    if (*direction == DIR_CCW) {
        if (done) {
            vfd.print(0, "Dir: CCW  OK");
            stepper_motor.set_direction(DIR_CCW);
        } else {
            vfd.print(0, "Dir: CCW   ");
            vfd.printchar(11, UP_ARROW);
        }
    } else {
        if (done) {
            vfd.print(0, "Dir: CW   OK");
            stepper_motor.set_direction(DIR_CW);
        } else {
            vfd.print(0, "Dir: CW   ");
            vfd.printchar(11, UP_ARROW);
        }
    }
}

void do_opt_zero(int* zero_sel, int done = 0) {
    if (*zero_sel == 0) {
        if (done) {
            vfd.print(0, "Aborted   OK");
        } else {
            vfd.print(0, "Zero: No   ");
            vfd.printchar(11, UP_ARROW);
        }
    } else {
        if (done) {
            vfd.print(0, "Zeroized  OK");
            stepper_motor.set_turns(0);
        } else {
            vfd.print(0, "Zero: Yes  ");
            vfd.printchar(11, UP_ARROW);
        }
    }
}

// ************ main function *******************
int main(void) {
    char tbuf[10];
    int turns;
    char key;
    int tval;
    absolute_time_t now;

    // active configuration
    int initial_period = 20;
    int target_period = 2;
    int desired_turns = 0;

    // options configuration
    int direction = DIR_CCW;
    int zeroize = 0;
    int manual_normal = 0;
    int candidate = 0;
    option_selection = OPTION_NULL;

    stdio_init_all();
    // show spinning bar for 3 seconds, to give time for the user to connect to the serial port
    // if desired, for debugging.
    sleep_ms(100);
    display_hold_time = get_absolute_time(); // initialize to any past value
    display_hold = 0;
    vfd.init();
    spinning_bar(0, 3);

    // print welcome message on the USB UART or Serial UART (selected in CMakelists.txt)
    print_title();
    board_init(); // GPIO initialisation
    PICO_LED_ON;

    keypad_timer_init();
    keypad_timer_start();
    keypad.clear_key_event();

    vfd.print(0, "Turns: 0");

    // set up the stepper motor rotation configuration
    stepper_instance[0] = &stepper_motor;
    stepper_instance_count = 1; // we have only one stepper motor
    stepper_motor.set_period(20); // multiples of 100usec, period between sequence steps
    stepper_motor.ramp_up(2); // ramp the speed up to the target period (multiples of 100usec)
    stepper_motor.state(STOP); // begin with the motor in the STOP state until the user presses a button

    // start the stepper motor timer
    stepper_timers_start();

    while (FOREVER) {
        PICO_LED_OFF;
        sleep_ms(100);
        PICO_LED_ON;
        sleep_ms(100);
        //stepper_motor.seq_step(DIR_CW);
        turns = stepper_motor.get_turns();
        // display the number of turns on the display
        now = get_absolute_time();
        if (absolute_time_diff_us(display_hold_time, now) > 0) {
            if ((turns != cur_turns) || (display_hold == 1)) {
                if (display_hold == 1) {
                    vfd_cls();
                    vfd.print(0, "Turns: ");
                    display_hold = 0;
                }
                cur_turns = turns;
                sprintf(tbuf, "%d", cur_turns);
                vfd.print(7, "      "); // clear the old value
                vfd.print(7, tbuf);
            }
        }
        if (request_integer == INTEGER_REQUEST) {
            // no need to get any key event, let the integer request handle it
            key = 0;
        } else {
            key = keypad.get_key_event();
        }
        switch(key) {
            case 'D': // STOP/GO button
                vfd_cls();
                if (stepper_motor.state() == STOP) {
                    stepper_motor.set_period(initial_period);
                    stepper_motor.ramp_up(target_period);
                    stepper_motor.desired_turns(desired_turns);
                    stepper_motor.state(GO);
                    vfd.print(0, "GO");
                } else {
                    stepper_motor.state(STOP);
                    vfd.print(0, "STOP");
                }
                display_hold = 1;
                display_hold_time = make_timeout_time_ms(2000);
                keypad.clear_key_event();
                break;
            case 'A': // increase speed button or up-arrow
                if (option_selection==OPTION_NULL) {
                    switch (target_period) {
                        case 2: // max, don't increase further
                            tval = 4; // displayed speed
                            break;
                        case 4:
                            target_period = 2;
                            tval = 4; // displayed speed
                            break;
                        case 8:
                            target_period = 4;
                            tval = 3;
                            break;
                        case 16:
                            target_period = 8;
                            tval = 2;
                            break;
                        default:
                            break;
                    }
                    if (stepper_motor.state() == STOP) {
                        stepper_motor.ramp_up(target_period);
                    } else {
                        //stepper_motor.set_period(target_period);
                        stepper_motor.ramp_up(target_period);
                    }
                    vfd_cls();
                    sprintf(tbuf, "Speed: %d", tval);
                    vfd.print(0, tbuf);
                    display_hold_time = make_timeout_time_ms(2000);
                    display_hold = 1;
                    keypad.clear_key_event();
                } else if (option_selection==OPTION_DIR) {
                    // toggle through the options each time the user presses A (up-arrow)
                    if (direction == DIR_CCW) {
                        direction = DIR_CW;
                    } else {
                        direction = DIR_CCW;
                    }
                    vfd_cls();
                    do_opt_dir(&direction); // display the new direction
                    display_hold_time = make_timeout_time_ms(10000);
                    display_hold = 1;
                    keypad.clear_key_event();
                } else if (option_selection==OPTION_ZEROIZE) {
                    // toggle through the options each time the user presses A (up-arrow)
                    if (zeroize == 0) {
                        zeroize = 1;
                    } else {
                        zeroize = 0;
                    }
                    vfd_cls();
                    do_opt_zero(&zeroize); // dizplay the zeroize option
                    display_hold_time = make_timeout_time_ms(10000);
                    display_hold = 1;
                    keypad.clear_key_event();
                }
                break;
            case 'B': // decrease speed button
                if (option_selection==OPTION_NULL) {
                    switch (target_period) {
                        case 2:
                            target_period = 4;
                            tval = 3; // displayed speed
                            break;
                        case 4:
                            target_period = 8;
                            tval = 2;
                            break;
                        case 8:
                            target_period = 16;
                            tval = 1;
                            break;
                        case 16: // min, don't decrease further
                            tval = 1; // displayed speed
                        default:
                            break;
                    }
                    if (stepper_motor.state() == STOP) {
                        stepper_motor.ramp_up(target_period);
                    } else {
                        stepper_motor.set_period(target_period);
                    }
                    vfd_cls();
                    sprintf(tbuf, "Speed: %d", tval);
                    vfd.print(0, tbuf);
                    display_hold_time = make_timeout_time_ms(2000);
                    display_hold = 1;
                    keypad.clear_key_event();
                } else {
                    // we are in a menu. This button isn't used, so silently swallow the key event.
                    keypad.clear_key_event();
                }
                break;
            case '#':
                // this is used as an OK button
                if (option_selection==OPTION_NULL) {
                    // do nothing
                    keypad.clear_key_event();
                } else if (option_selection==OPTION_DIR) {
                    vfd_cls();
                    do_opt_dir(&direction, 1); // activate and display the selection that was OK'd
                    display_hold_time = make_timeout_time_ms(2000);
                    display_hold = 1;
                    option_selection = OPTION_NULL;
                    keypad.clear_key_event();
                } else if (option_selection==OPTION_ZEROIZE) {
                    vfd_cls();
                    do_opt_zero(&zeroize, 1); // activate and display the selection that was OK'd
                    display_hold_time = make_timeout_time_ms(2000);
                    display_hold = 1;
                    option_selection = OPTION_NULL;
                    keypad.clear_key_event();
                }
                break;
            case '*':
                // this is used as a cancel button
                if (option_selection==OPTION_NULL) {
                    // do nothing
                    keypad.clear_key_event();
                } else {
                    // abort the menu selection
                    option_selection = OPTION_NULL;
                    vfd_cls();
                    vfd.print(0, "Cancelled OK");
                    display_hold_time = make_timeout_time_ms(2000);
                    display_hold = 1;
                    keypad.clear_key_event();
                }
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
                if (1) { // in future, if there are menus, then check for menu mode here
                    if (request_integer==NO_INTEGER_REQUEST) {
                        keypad.clear_key_event();
                        keypad.set_integer(-1);
                        set_stored_keypress(key); // store this first keypress
                        vfd_cls();
                        vfd.printchar(0, key);
                        request_integer = INTEGER_REQUEST;
                        display_hold_time = make_timeout_time_ms(3000);
                        display_hold = 1;
                    } else {
                        keypad.clear_key_event();
                    }
                } else {
                    keypad.clear_key_event();
                }
                break;
            case 'C':
                // used for the options menu
                // Each time C is pressed, increment through the menu options
                option_selection++;
                if (option_selection>2) { // only two menu items so far; Direction, and Zeroize
                    option_selection = 1;
                }
                vfd_cls();
                switch(option_selection) {
                    case OPTION_DIR:
                        do_opt_dir(&direction);
                        break;
                    case OPTION_ZEROIZE:
                        do_opt_zero(&zeroize);
                        break;
                    case OPTION_MAN_NORM:
                        // not implemented
                        //do_opt_man_norm(&man_norm_sel);
                        break;
                    default:
                        break;
                }
                // hold the menu on the display for quite a long time
                display_hold_time = make_timeout_time_ms(20000);
                display_hold = 1;
                keypad.clear_key_event();
                break;
            default:
                // no key pressed
                break;
        }
        if (request_integer==INTEGER_COMPLETE) {
            if (1) {
                desired_turns = keypad.get_integer();
                vfd_cls();
                stepper_motor.desired_turns(desired_turns);
                sprintf(tbuf, "Dest:  %d", desired_turns);
                vfd.print(0, tbuf);
                display_hold_time = make_timeout_time_ms(2000);
                display_hold = 1;
                request_integer = NO_INTEGER_REQUEST;
            }
        }
    }
}


