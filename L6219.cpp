//
// Created by shabaz on 24/09/2023.
//

#include "L6219.h"

// *****************************************************
// ************* interrupt capabilities ****************
// *****************************************************
L6219* stepper_instance[MAX_MOTORS];
int stepper_instance_count = 0;
int usec100_count[MAX_MOTORS];
repeating_timer_t stepper_timer;

void stepper_timers_init() {
    int i;
    for (i=0; i<MAX_MOTORS; i++) {
        stepper_instance[i] = NULL;
        usec100_count[i] = 0;
    }
}

void stepper_timers_start() {
    add_repeating_timer_us(-100, stepper_callback, NULL, &stepper_timer);
}

void stepper_timers_stop() {
    cancel_repeating_timer(&stepper_timer);
}

bool stepper_callback(repeating_timer_t *rt) {
    int i;
    int ramp;
    int desired_turns, current_turns;
    for (i=0; i<stepper_instance_count; i++) {
        if (stepper_instance[i]->state()==0) { // motor is stopped so we don't rotate.
            continue;
        }
        desired_turns = stepper_instance[i]->desired_turns();
        current_turns = stepper_instance[i]->get_turns();
        if (desired_turns == 0) {
            // we want to turn indefinitely
        } else if (desired_turns > current_turns) {
            if (desired_turns-current_turns <40) {
                // we are close to the desired number of turns, so we slow down
                stepper_instance[i]->ramp_down(15);
            }
        } else {
            // we have reached the desired number of turns, so we stop
            stepper_instance[i]->state(0);
            continue;
        }
        usec100_count[i]++;
        if (usec100_count[i] >= stepper_instance[i]->get_period()) {
            stepper_instance[i]->seq_step(stepper_instance[i]->get_direction()); // turn the motor!
            usec100_count[i] = 0;
            ramp = stepper_instance[i]->get_ramp();
            if (ramp == 0) {
                // no ramping
            } else if (ramp>0){
                // ramping up, so decrement the period
                if (stepper_instance[i]->get_target_period() < stepper_instance[i]->get_period()) {
                    if (stepper_instance[i]->ready_for_ramp_change() ==1) {
                        stepper_instance[i]->set_period(
                                stepper_instance[i]->get_period() - stepper_instance[i]->get_ramp());
                    }
                } else {
                    // ramping up complete
                    stepper_instance[i]->no_ramp();
                }
            } else if (ramp<0) {
                // ramping down, so increment the period
                if (stepper_instance[i]->get_target_period() > stepper_instance[i]->get_period()) {
                    if (stepper_instance[i]->ready_for_ramp_change() ==1) {
                        stepper_instance[i]->set_period(
                                stepper_instance[i]->get_period() - stepper_instance[i]->get_ramp());
                    }
                } else {
                    // ramping down complete
                    stepper_instance[i]->no_ramp();
                }
            }
        }
    }
    return true; // keep repeating
}



// *****************************************************
// ************ L6219 class implementation *************
// *****************************************************
L6219::L6219(uint ph1_pin, uint i01_pin, uint i11_pin, uint ph2_pin, uint i02_pin, uint i12_pin)
        : _ph1_pin(ph1_pin), _i01_pin(i01_pin), _i11_pin(i11_pin), _ph2_pin(ph2_pin), _i02_pin(i02_pin), _i12_pin(i12_pin)
{
    gpio_init(ph1_pin);
    gpio_init(i01_pin);
    gpio_init(i11_pin);
    gpio_init(ph2_pin);
    gpio_init(i02_pin);
    gpio_init(i12_pin);
    gpio_set_dir(ph1_pin, GPIO_OUT);
    gpio_set_dir(i01_pin, GPIO_OUT);
    gpio_set_dir(i11_pin, GPIO_OUT);
    gpio_set_dir(ph2_pin, GPIO_OUT);
    gpio_set_dir(i02_pin, GPIO_OUT);
    gpio_set_dir(i12_pin, GPIO_OUT);
    gpio_put(ph1_pin, 0);
    gpio_put(ph2_pin, 0);
    CURRENT_BOTH_OFF;
}

void L6219::init() {
    CURRENT_BOTH_OFF;
    _sequence_state = 0;
    _num_steps = 0;
    _turns = 0;
    _ramp = 0;
    _direction = 0;
    _period = 5;
    _target_period = 5;
    _ramp_internal_counter = 0;
    _desired_turns = 0;
}

int L6219::get_turns() {
    return _turns;
}

void L6219::set_turns(int turns) {
    _turns = turns;
}

int L6219::get_period() {
    return _period;
}

void L6219::set_period(int period) {
    _period = period;
}

void L6219::ramp_up(int target_period) {
    _target_period = target_period;
    _ramp = 1;
}

void L6219::ramp_down(int target_period) {
    _target_period = target_period;
    _ramp = -1;
}

void L6219::no_ramp() {
    _ramp = 0;
}

int L6219::get_target_period() {
    return _target_period;
}

int L6219::get_ramp() {
    return _ramp;
}

int L6219::get_direction() {
    return _direction;
}

int L6219::state() {
    return _motion_state;
}

void L6219::state(int motion_state) {
    _motion_state = motion_state;
    if (motion_state == 0) {
        CURRENT_BOTH_OFF;
    }
}

void L6219::desired_turns(int turns) {
    _desired_turns = turns;
}

int L6219::desired_turns() {
    return _desired_turns;
}

int L6219::ready_for_ramp_change() {
    float cur_period = (float)_period;
    float target_period = (float)_target_period;
    float desired_delay;
    int retval = 0;
    if (_ramp > 0) {
        desired_delay = (cur_period - target_period);
    } else if (_ramp < 0) {
        desired_delay = (target_period - cur_period);
    } else {
        // should not occur!
        return 1;
    }
    desired_delay = 1/desired_delay;
    if (_ramp<0) {
        desired_delay = desired_delay * 1000;
        desired_delay = 600 - desired_delay;
    } else {
        desired_delay = desired_delay * 2000;
    }
    if (desired_delay < 20.0) desired_delay = 20.0;
    _ramp_internal_counter++;
    if (_ramp_internal_counter >= (int)desired_delay) {
        _ramp_internal_counter = 0;
        retval = 1;
    }
    return retval;
}

void L6219::set_direction(int direction) {
    _direction = direction;
}

void L6219::seq_step(uint direction) {
    switch(direction) {
        case 0:
            _sequence_state++;
            if (_sequence_state > 7) _sequence_state = 0;
            _num_steps++;
            if (_num_steps >= STEPS_PER_TURN) {
                _num_steps = 0;
                _turns++;
            }
            break;
        case 1:
            _sequence_state--;
            if (_sequence_state < 0) _sequence_state = 7;
            _num_steps--;
            if (_num_steps <= 0) {
                _num_steps = STEPS_PER_TURN;
                //_turns--;
                _turns++; // we count positive turns in either direction
            }
            break;
        default:
            // should not occur
            break;
    }
    // briefly set the current off before changing the sequence
    CURRENT_BOTH_OFF;
    // the sequence is 0=A+B, 1=B, 2=*A+B, 3=*A, 4=*A+*B, 5=*B, 6=A+*B, 7=A
    // current is 67% when two phases are on, and 100% when one phase is on
    switch(_sequence_state) {
        case 0:
            PHASE_A;
            PHASE_B;
            CURRENT_BOTH_67PCT;
            break;
        case 1:
            PHASE_B;
            CURRENT_B_FULL;
            break;
        case 2:
            PHASE_A_BAR;
            PHASE_B;
            CURRENT_BOTH_67PCT;
            break;
        case 3:
            PHASE_A_BAR;
            CURRENT_A_FULL;
            break;
        case 4:
            PHASE_A_BAR;
            PHASE_B_BAR;
            CURRENT_BOTH_67PCT;
            break;
        case 5:
            PHASE_B_BAR;
            CURRENT_B_FULL;
            break;
        case 6:
            PHASE_A;
            PHASE_B_BAR;
            CURRENT_BOTH_67PCT;
            break;
        case 7:
            PHASE_A;
            CURRENT_A_FULL;
            break;
        default:
            // should not occur
            break;
    }
}

