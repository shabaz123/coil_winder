//
// Created by shabaz on 24/09/2023.
//

#ifndef MAG_COUNTER_L6219_H
#define MAG_COUNTER_L6219_H

// includes
#include "pico/stdlib.h"
#include "pico/util/queue.h"

// defines
// STEPS_PER_TURN is (360 degrees divided by the angle per step) * 2 (for half steps)
// for a 1.8 degree stepper motor, this is (360/1.8)*2 = 200*2 = 400
#define STEPS_PER_TURN (200*2)
#define CURRENT_BOTH_OFF gpio_put(_i01_pin, 1);gpio_put(_i11_pin, 1);gpio_put(_i02_pin, 1);gpio_put(_i12_pin, 1);
#define CURRENT_BOTH_67PCT gpio_put(_i01_pin, 1);gpio_put(_i11_pin, 0);gpio_put(_i02_pin, 1);gpio_put(_i12_pin, 0);
#define CURRENT_BOTH_FULL gpio_put(_i01_pin, 0);gpio_put(_i11_pin, 0);gpio_put(_i02_pin, 0);gpio_put(_i12_pin, 0);
#define CURRENT_A_67PCT gpio_put(_i01_pin, 1);gpio_put(_i11_pin, 0);
#define CURRENT_A_FULL gpio_put(_i01_pin, 0);gpio_put(_i11_pin, 0);
#define CURRENT_B_67PCT gpio_put(_i02_pin, 1);gpio_put(_i12_pin, 0);
#define CURRENT_B_FULL gpio_put(_i02_pin, 0);gpio_put(_i12_pin, 0);
#define PHASE_A gpio_put(_ph1_pin, 1);
#define PHASE_A_BAR gpio_put(_ph1_pin, 0);
#define PHASE_B gpio_put(_ph2_pin, 1);
#define PHASE_B_BAR gpio_put(_ph2_pin, 0);
#define MAX_MOTORS 4
#define RAMP_INTERNAL_COUNT_MAX 10

// function prototypes
bool stepper_callback(repeating_timer_t *rt);
void stepper_timers_init();
void stepper_timers_start();
void stepper_timers_stop();

// class definition
class L6219
{
public:
    L6219(uint ph1_pin, uint i01_pin, uint i11_pin, uint ph2_pin, uint i02_pin, uint i12_pin);
    void init();
    int get_turns();
    void set_turns(int turns);
    void seq_step(uint direction); // perform one step in the sequence (one eighth of a full step)
    int get_target_period();
    int get_ramp();
    int get_period();
    int get_direction();
    void ramp_up(int target_period);
    void ramp_down(int target_period);
    void no_ramp();
    void set_ramp(int ramp);
    void set_direction(int direction);
    void set_period(int period);
    int ready_for_ramp_change();
    int state(); // returns 1 if the motor is supposed to be rotating
    void state(int motion_state); // set the desired motion state (1=GO, 0=STOP)
    void desired_turns(int turns);
    int desired_turns();

private:


    // member variables
    uint _ph1_pin, _i01_pin, _i11_pin, _ph2_pin, _i02_pin, _i12_pin; // pins
    int _sequence_state; // 0 to 7 for half-step mode (total 8 steps)
    int _num_steps; // number of steps
    int _turns; // number of turns
    int _period; // period in 100 microseconds increments per step
    int _direction; // direction of rotation (0 or 1)
    int _ramp; // 0=no ramp, -1=slow down to target_period, +1=speed up to target_period
    int _target_period; // target period in 100 microseconds increments per step
    int _ramp_internal_counter; // internal counter for ramping
    int _motion_state; // 0=STOP, 1=GO
    int _desired_turns; // desired number of turns. Set to zero for unlimited turns until stopped
};


#endif //MAG_COUNTER_L6219_H
