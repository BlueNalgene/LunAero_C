/*
 * C_LunAero/motors_LunAero.cpp - Motor controller functions for LunAero_C
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

#include "motors_LunAero.hpp"

void motor_handler() {
	// Handle stopping
	if (*val_ptr.STOP_DIRaddr == 3) {
		//~ std::cout << "stopping both motors" << std::endl;
		sem_wait(&LOCK);
		*val_ptr.HORZ_DIRaddr = 0;
		*val_ptr.VERT_DIRaddr = 0;
		sem_post(&LOCK);
		while ((*val_ptr.DUTY_Aaddr > 0) || (*val_ptr.DUTY_Baddr > 0)) {
			if (*val_ptr.DUTY_Aaddr > 10) {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Aaddr = 10;
				sem_post(&LOCK);
			} else {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Aaddr = *val_ptr.DUTY_Aaddr - 1;
				sem_post(&LOCK);
			}
			if (*val_ptr.DUTY_Baddr > 10) {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Baddr = 10;
				sem_post(&LOCK);
			} else {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Baddr = *val_ptr.DUTY_Baddr - 1;
				sem_post(&LOCK);
			}
			softPwmWrite(APINP, *val_ptr.DUTY_Aaddr);
			softPwmWrite(BPINP, *val_ptr.DUTY_Baddr);
			usleep(5);
		}
		digitalWrite(APIN1, HIGH);
		digitalWrite(APIN2, HIGH);
		digitalWrite(BPIN1, HIGH);
		digitalWrite(BPIN2, HIGH);
		softPwmWrite(APINP, *val_ptr.DUTY_Aaddr);
		softPwmWrite(BPINP, *val_ptr.DUTY_Baddr);
		// When stopped, reset code
		//~ val_ptr.STOP_DIRaddr = 0;
	} else if (*val_ptr.STOP_DIRaddr == 2) {
		sem_wait(&LOCK);
		*val_ptr.VERT_DIRaddr = 0;
		sem_post(&LOCK);
		//~ std::cout << "stopping vertical motor (A)" << std::endl;
		while (*val_ptr.DUTY_Aaddr > 0) {
			sem_wait(&LOCK);
			*val_ptr.DUTY_Aaddr = *val_ptr.DUTY_Aaddr - 1;
			sem_post(&LOCK);
			softPwmWrite(APINP, *val_ptr.DUTY_Aaddr);
			usleep(10);
		}
		digitalWrite(APIN1, HIGH);
		digitalWrite(APIN2, HIGH);
		softPwmWrite(APINP, *val_ptr.DUTY_Aaddr);
		// When stopped, reset code
		//~ val_ptr.STOP_DIRaddr = 0;
	} else if (*val_ptr.STOP_DIRaddr == 1) {
		sem_wait(&LOCK);
		*val_ptr.HORZ_DIRaddr = 0;
		sem_post(&LOCK);
		//~ std::cout << "stopping horizontal motor (B)" << std::endl;
		while (*val_ptr.DUTY_Baddr > 0) {
			sem_wait(&LOCK);
			*val_ptr.DUTY_Baddr = *val_ptr.DUTY_Baddr - 1;
			sem_post(&LOCK);
			softPwmWrite(BPINP, *val_ptr.DUTY_Baddr);
			usleep(10);
		}
		digitalWrite(BPIN1, HIGH);
		digitalWrite(BPIN2, HIGH);
		softPwmWrite(BPINP, *val_ptr.DUTY_Baddr);
		// When stopped, reset code
		//~ val_ptr.STOP_DIRaddr = 0;
	}
	if (*val_ptr.STOP_DIRaddr > 0) {
		sem_wait(&LOCK);
		*val_ptr.STOP_DIRaddr = 0;
		sem_post(&LOCK);
	}
	// Handle Vertical Motion
	if (*val_ptr.VERT_DIRaddr > 0) {
		if (*val_ptr.VERT_DIRaddr == 1) {
			// Motor UP
			OLD_DUTY_A = *val_ptr.DUTY_Aaddr;
			//~ std::cout << "moving up" << std::endl;
			digitalWrite(APIN1, LOW);
			digitalWrite(APIN2, HIGH);
			if (*val_ptr.RUN_MODEaddr == 0) {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Aaddr = DUTY;
				sem_post(&LOCK);
			} else {
				speed_up(1);
			}
			if (*val_ptr.DUTY_Aaddr != OLD_DUTY_A) {
				if (DEBUG_COUT) {
					LOGGING.open(LOGOUT, std::ios_base::app);
					LOGGING
					<< "setting motor B duty cycle to: " << *val_ptr.DUTY_Aaddr << std::endl;
					LOGGING.close();
				}
			}
			softPwmWrite(APINP, *val_ptr.DUTY_Aaddr);
		} else {
			// Motor DOWN
			OLD_DUTY_A = *val_ptr.DUTY_Aaddr;
			//~ std::cout << "moving down" << std::endl;
			digitalWrite(APIN1, HIGH);
			digitalWrite(APIN2, LOW);
			if (*val_ptr.RUN_MODEaddr == 0) {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Aaddr = DUTY;
				sem_post(&LOCK);
			} else {
				speed_up(1);
			}
			if (*val_ptr.DUTY_Aaddr != OLD_DUTY_A) {
				if (DEBUG_COUT) {
					LOGGING.open(LOGOUT, std::ios_base::app);
					LOGGING
					<< "setting motor B duty cycle to: " << *val_ptr.DUTY_Aaddr << std::endl;
					LOGGING.close();
				}
			}
			softPwmWrite(APINP, *val_ptr.DUTY_Aaddr);
		}
	}
	// Handle Horizontal Motion
	if (*val_ptr.HORZ_DIRaddr > 0) {
		if (*val_ptr.HORZ_DIRaddr == 1) {
			// Motor LEFT
			OLD_DUTY_B = *val_ptr.DUTY_Baddr;
			//~ std::cout << "moving left" << std::endl;
			digitalWrite(BPIN1, LOW);
			digitalWrite(BPIN2, HIGH);
			if (*val_ptr.RUN_MODEaddr == 0) {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Baddr = DUTY;
				sem_post(&LOCK);
			} else {
				speed_up(2);
			}
			if ((*val_ptr.DUTY_Baddr != OLD_DUTY_B) && (OLD_DIR == 1)) {
				if (DEBUG_COUT) {
					LOGGING.open(LOGOUT, std::ios_base::app);
					LOGGING
					<< "setting motor B duty cycle to: " << *val_ptr.DUTY_Baddr << std::endl;
					LOGGING.close();
				}
			}
			softPwmWrite(BPINP, *val_ptr.DUTY_Baddr);
			if (OLD_DIR == 2) {
				// Loose Wheel protocol
				auto current_time = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = current_time-OLD_LOOSE_WHEEL_TIME;
				sem_wait(&LOCK);
				*val_ptr.DUTY_Baddr = DUTY;
				sem_post(&LOCK);
				if (elapsed_seconds > LOOSE_WHEEL_DURATION) {
					sem_wait(&LOCK);
					*val_ptr.DUTY_Baddr = MIN_DUTY;
					sem_post(&LOCK);
					if (DEBUG_COUT) {
						LOGGING.open(LOGOUT, std::ios_base::app);
						LOGGING
						<< "Loose Wheel maneuver complete" << std::endl;
						LOGGING.close();
					}
					OLD_DIR = 1;
				} else {
					if (DEBUG_COUT) {
						LOGGING.open(LOGOUT, std::ios_base::app);
						LOGGING
						<< "running in Loose Wheel mode" << std::endl;
						LOGGING.close();
					}
					OLD_DIR = 2;
				}
				if (DEBUG_COUT) {
					LOGGING.open(LOGOUT, std::ios_base::app);
					LOGGING
					<< "setting motor B duty cycle to: " << *val_ptr.DUTY_Baddr << std::endl;
					LOGGING.close();
				}
			} else {
				OLD_DIR = 1;
				softPwmWrite(BPINP, *val_ptr.DUTY_Baddr);
			}
		} else {
			// Motor RIGHT
			OLD_DUTY_B = *val_ptr.DUTY_Baddr;
			//~ std::cout << "moving right" << std::endl;
			digitalWrite(BPIN1, HIGH);
			digitalWrite(BPIN2, LOW);
			if (*val_ptr.RUN_MODEaddr == 0) {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Baddr = DUTY;
				sem_post(&LOCK);
			} else {
				speed_up(2);
			}
			if ((*val_ptr.DUTY_Baddr != OLD_DUTY_B) && (OLD_DIR == 2)) {
				if (DEBUG_COUT) {
					LOGGING.open(LOGOUT, std::ios_base::app);
					LOGGING
					<< "setting motor B duty cycle to: " << *val_ptr.DUTY_Baddr << std::endl;
					LOGGING.close();
				}
			}
			if (OLD_DIR == 1) {
				auto current_time = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = current_time-OLD_LOOSE_WHEEL_TIME;
				sem_wait(&LOCK);
				*val_ptr.DUTY_Baddr = DUTY;
				sem_post(&LOCK);
				if (elapsed_seconds > LOOSE_WHEEL_DURATION) {
					*val_ptr.DUTY_Baddr = MIN_DUTY;
					if (DEBUG_COUT) {
						LOGGING.open(LOGOUT, std::ios_base::app);
						LOGGING
						<< "Loose Wheel maneuver complete" << std::endl;
						LOGGING.close();
					}
					OLD_DIR = 2;
				} else {
					if (DEBUG_COUT) {
						LOGGING.open(LOGOUT, std::ios_base::app);
						LOGGING
						<< "running in Loose Wheel mode" << std::endl;
						LOGGING.close();
					}
					OLD_DIR = 1;
				}
				if (DEBUG_COUT) {
					LOGGING.open(LOGOUT, std::ios_base::app);
					LOGGING
					<< "setting motor B duty cycle to: " << *val_ptr.DUTY_Baddr << std::endl;
					LOGGING.close();
				}
				softPwmWrite(BPINP, *val_ptr.DUTY_Baddr);
			} else {
				OLD_DIR = 2;
				softPwmWrite(BPINP, *val_ptr.DUTY_Baddr);
			}
		}
	}
}

void speed_up(int motor) {
	/* Increase the duty cycle of the motor called by this function.
	 * The duty cycle will never go below 20%
	 * 
	 * @param motor The motor we are modifying (1=vert, 2=horz)
	 */
	
	
	if (motor == 1) {
		if (CNT_MOTOR_A == 2) {
			CNT_MOTOR_A = 0;
			if (*val_ptr.DUTY_Aaddr < 20) {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Aaddr = 20;
				sem_post(&LOCK);
			} else if (*val_ptr.DUTY_Aaddr < 75) {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Aaddr = *val_ptr.DUTY_Aaddr + 1;
				sem_post(&LOCK);
			}
		} else {
			CNT_MOTOR_A += 1;
		}
	} else if (motor == 2) {
		if (CNT_MOTOR_B == 2) {
			CNT_MOTOR_B = 0;
			if (*val_ptr.DUTY_Baddr < 20) {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Baddr = 20;
				sem_post(&LOCK);
			} else if (*val_ptr.DUTY_Baddr < 75) {
				sem_wait(&LOCK);
				*val_ptr.DUTY_Baddr = *val_ptr.DUTY_Baddr + 1;
				sem_post(&LOCK);
			}
		} else {
			CNT_MOTOR_B += 1;
		}
	}
	return;
}

void gpio_pin_setup () {
	int i;
	int pin_array[] = { APINP, APIN1, APIN2, BPIN1, BPIN2, BPINP };
	
	// Required init
	wiringPiSetup();
	// Set the output pins
	for( i = 0; i < 6; i = i + 1 ) {
		pinMode(pin_array[i], OUTPUT);
		// PWM pins go PWM, all else go HIGH
		if ((i == 0) | (i == 5)) {
			digitalWrite(pin_array[i], LOW);
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "Set pin " << pin_array[i] << " LOW" << std::endl;
				LOGGING.close();
			}
		} else {
			digitalWrite(pin_array[i], HIGH);
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "Set pin " << pin_array[i] << " HIGH" << std::endl;
				LOGGING.close();
			}
		}
	}
	// create soft PWM
	softPwmCreate(APINP, 0, DUTY);
	softPwmCreate(BPINP, 0, DUTY);
}

void final_stop() {
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "stopping motors to end program" << std::endl;
		LOGGING.close();
	}
	softPwmWrite(APINP, 0);
	softPwmWrite(BPINP, 0);
	digitalWrite(APIN1, LOW);
	digitalWrite(APIN2, LOW);
	digitalWrite(BPIN1, LOW);
	digitalWrite(BPIN2, LOW);
}
