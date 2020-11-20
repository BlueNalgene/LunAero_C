/*
 * C_LunAero/motors_LunAero.hpp - Motor headers for LunAero_C
 * Copyright (C) <2020>  <Wesley T. Honeycutt>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
/**
 * Duty cycle % range for motors
 */
#define DUTY 100


/**
 * Raspberry Pi GPIO pin for motor A Soft PWM.  BCM equivalent of 0 = 17  Customizable from settings.cfg.
 */
inline int APINP = 0;
/**
 * Raspberry Pi GPIO pin for motor A 1 pin.  BCM equivalent of 2 = 27  Customizable from settings.cfg.
 */
inline int APIN1 = 2;
/**
 * Raspberry Pi GPIO pin for motor A 2 pin.  BCM equivalent of 3 = 22  Customizable from settings.cfg.
 */
inline int APIN2 = 3;
/**
 * Raspberry Pi GPIO pin for motor B 1 pin.  BCM equivalent of 12 = 10  Customizable from settings.cfg.
 */
inline int BPIN1 = 12;
/**
 * Raspberry Pi GPIO pin for motor B 2 pin.  BCM equivalent of 13 = 9  Customizable from settings.cfg.
 */
inline int BPIN2 = 13;
/**
 * Raspberry Pi GPIO pin for motor A Soft PWM.  BCM equivalent of 14 = 11  Customizable from
 * settings.cfg.
 */
inline int BPINP = 14;
/**
 * Minimum allowable duty cycle.  Customizable from settings.cfg.
 */
inline int MIN_DUTY = 20;
/**
 * Maximumallowable duty cycle.  Customizable from settings.cfg.
 */
inline int MAX_DUTY = 75;
/**
 * Duty cycle threshold for braking during recording.  Customizable from settings.cfg.
 */
inline int BRAKE_DUTY = 10;
/**
 * PWM operation frequency in Hz.  Customizable from settings.cfg.
 */
inline int FREQ = 10000;
/**
 * Number of seconds to perform a loose wheel maneuver.  This can be customized in settings.cfg.
 */
inline std::chrono::duration<double> LOOSE_WHEEL_DURATION = (std::chrono::duration<double>)2.;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// Global Variables - Not "private" but not necessary to define for Doxygen
inline int OLD_DIR = 0;
inline int OLD_DUTY_A = 0;
inline int OLD_DUTY_B = 0;
inline int CNT_MOTOR_A = 0;
inline int CNT_MOTOR_B = 0;
inline std::chrono::time_point OLD_LOOSE_WHEEL_TIME = std::chrono::system_clock::now();

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

// Declare Functions
void gpio_pin_setup();
void motor_handler();
void loose_wheel(int wheel_dir);
void speed_up(int motor);
void final_stop();

#endif
