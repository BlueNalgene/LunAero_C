#ifndef MOTORS_LUNAERO_H
#define MOTORS_LUNAERO_H

// Standard C++ Includes
#include <string>
#include <iostream>


// Motor Specific Includes
#include <wiringPi.h>      // provides GPIO things
#include <softPwm.h>       // provides PWM for GPIO
#include <chrono>          // provides C++ chrono
#include <ctime>           // provides c time funcitons for chrono usage

// User includes
#include "LunAero.hpp"

// Global Defined Constants
#define DELAY 0.01              // Number of seconds to delay between sampling frames
#define MOVETHRESH 10           // % of frame height to warrant a move
#define DUTY 100                // Duty cycle % for motors
#define FREQ 10000              // Frequency in Hz to run PWM
#define APINP 0                 // GPIO BCM pin definition 17
#define APIN1 2                 // GPIO BCM pin definition 27
#define APIN2 3                 // GPIO BCM pin definition 22
#define BPIN1 12                // GPIO BCM pin definition 10
#define BPIN2 13                // GPIO BCM pin definition 9
#define BPINP 14                // GPIO BCM pin definition 11
#define MIN_DUTY 20             // Minimum allowable duty cycle
// Number of seconds to perform a loose wheel maneuver
inline std::chrono::duration<double> LOOSE_WHEEL_DURATION = (std::chrono::duration<double>)2.;

// Global Variables
inline int OLD_DIR = 0;
inline int OLD_DUTY_A = 0;
inline int OLD_DUTY_B = 0;
inline int CNT_MOTOR_A = 0;
inline int CNT_MOTOR_B = 0;
inline std::chrono::time_point OLD_LOOSE_WHEEL_TIME = std::chrono::system_clock::now();

// Declare Functions
void gpio_pin_setup();
void motor_handler();
//~ void mot_up();
//~ void mot_down();
//~ void mot_left();
//~ void mot_right();
//~ void mot_stop(int direct);
void loose_wheel(int wheel_dir);
void speed_up(int motor);
void final_stop();

#endif
