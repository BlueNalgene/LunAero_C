
/*
 * C_LunAero/LunAero.hpp - Primary header for robotic moon tracking scope
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


#ifndef LUNAERO_H
#define LUNAERO_H

// Standard C++ Includes
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>         // provides c++ version of getenv
#include <chrono>          // provides C++ chrono
#include <ctime>           // provides c time funcitons for chrono usage
#include <vector>
using std::vector;
//~ #include <sstream>

//Special RPi include
#include "bcm_host.h"      // provides DISPMANX et al.
#include "opencv2/opencv.hpp"
using namespace cv;

// Module specific Includes
#include <signal.h>        // provides kill signals
#include <semaphore.h>     // provides sem_init
#include <stdlib.h>        // provides system
#include <stdio.h>         // provides popen
#include <sys/mman.h>      // provides mmap
#include <sys/stat.h>      // provides mkdir
#include <sys/types.h>     // provides kill
#include <sys/wait.h>      // provides wait
#include <time.h>          // provides time
#include <unistd.h>        // provides usleep
#include <filesystem>      // provides filesystem space info
#include <libnotify/notify.h> // provides notify functions

// User Includes
#include "gtk_LunAero.hpp"
#include "motors_LunAero.hpp"
#include "camera_LunAero.hpp"


/*
 * Recording duration in seconds.  Customizable from settings.cfg.  Default 1800.
 */
inline std::chrono::duration<double> RECORD_DURATION = (std::chrono::duration<double>) 1800.;

// Global variables (Inline Requires C++17)
/**
 * The height of the usable GTK window.
 */
inline int WORK_HEIGHT = 0;
/**
 * The width of the usable GTK window.
 */
inline int WORK_WIDTH = 0;
/**
 * Relative half screen height of the window.  Altered to meet the 16:9 ratio of the Raspberry Pi camera.
 */
inline int RVD_HEIGHT = 0;
/**
 * Relative half screen width of the window.  Altered to meet the 16:9 ratio of the Raspberry Pi camera.
 */
inline int RVD_WIDTH = 0;
/**
 * X value of top left corner for preview window application.
 */
inline int RVD_XCORN = 0;
/**
 * Y value for top left corner for preview window application
 */
inline int RVD_YCORN = 0;
/**
 * Divisor for the number pixels on the top and bottom edges to warrant a move.  Customizable from
 * settings.cfg.
 */
inline int EDGE_DIVISOR_W = 20;
/**
 * Divisor for the number pixels on the left and right edges to warrant a move.  Customizable from
 * settings.cfg.
 */
inline int EDGE_DIVISOR_H = 20;
/**
 * Brightness value between 0-255 to act as the threshold for raw brightness tests.  Customizable from
 * settings.cfg.
 */
inline int RAW_BRIGHT_THRESH = 240;
/**
 * Threshold value for the brightness tests.  Outcome of the brightness tests must be below this value,
 * otherwise the image is deemed "too bright" because the birds might get hidden by the lunar albedo.
 * Customizable from settings.cfg
 */
inline float BRIGHT_THRESH = 0.001;
/**
 * Drive name given to the external video storage drive.  Customizable from settings.cfg.
 */
inline std::string DRIVE_NAME = "MOON1";
/**
 * Number of cycles the moon is lost for before stopping LunAero.  Customizable from settings.cfg.
 */
inline int LOST_THRESH = 30;
/**
 * Milliseconds an emergency messeage should remain on the desktop in event of a crash.  Customizable
 * from settings.cfg
 */
inline int EMG_DUR = 10000;
/**
 * Should LunAero save a ppm file of the current raspivid screenshot?
 */
inline bool SAVE_DEBUG_IMAGE = false;


inline vector <float> BLUR_BRIGHT;

inline std::string FILEPATH;
inline std::string DEFAULT_FILEPATH = "";
inline std::string TSBUFF;
inline std::string IDPATH = "";
inline std::chrono::time_point OLD_RECORD_TIME = std::chrono::system_clock::now();
inline bool DEBUG_COUT = false;
inline std::string DEBUG_LOG;
inline vector <std::string> DISK_OUTPUT;
inline std::string LOGOUT;
inline std::ofstream LOGGING;

/*
 * Semaphore int to lock processes across forks.  Do not touch.
 */
inline sem_t LOCK;

/**
 * Struct of addresses used across forks to store important values.  Call these values with
 * the prototype: *val_ptr.EXAMPLEaddr.  These are declared inline across cpp files, requiring C++17.
 */
inline struct val_addresses {
	/**
	 * Counter of the number of cycles the moon has been lost.
	 */
	volatile int * LOST_COUNTERaddr;
	/**
	 * Value of ISO selected by the user.  Valid values (100, 200, 400, 800)
	 */
	volatile int * ISO_VALaddr;
	/**
	 * Value of the shutter speed selected by the user.  Minimum and maximum values are determined by the
	 * hardware and limited further by code.
	 */
	volatile int * SHUTTER_VALaddr;
	/**
	 * Value of the current run mode.  Valid values are 0 for preview/manual mode and 1 for
	 * recording/automatic mode
	 */
	volatile int * RUN_MODEaddr;
	/**
	 * Flag to sync abort functions across code forks.  If 0, run.  If 1, abort.
	 */
	volatile int * ABORTaddr;
	/**
	 * Flag to sync camera refreshes across forks.  If 0, do nothing.  If 1, refresh.
	 */
	volatile int * REFRESH_CAMaddr;
	/**
	 * Horizontal motion to be applied to motor B. Values: 0 = none, 1 = left, 2 = right
	 */
	volatile int * HORZ_DIRaddr;
	/**
	 * Vertical motion to be applied to motor A.  Values: 0 = none, 1 = up, 2 = down
	 */
	volatile int * VERT_DIRaddr;
	/**
	 * Stop motors selected by this flag.  Values: 0 = none, 1 = horizontal only, 2 = vertical only,
	 * 3 = both motors.
	 */
	volatile int * STOP_DIRaddr;
	/**
	 * Current duty cycle of motor A.  Valid values 0-100.
	 */
	volatile int * DUTY_Aaddr;
	/**
	 * Current duty cycle of motor B.  Valid values 0-100.
	 */
	volatile int * DUTY_Baddr;
	/**
	 * Flag for telling the raspivid refresh algorithm if this the original run or subsequent runs being
	 * refreshed to elicit appropriate behavior.
	 */
	volatile int * SUBSaddr;
} val_ptr;

// Declare Function Prototypes
int main (int argc, char **argv);
int startup_disk_check();
void cb_framecheck();
void cleanup();
void kill_raspivid();
void current_frame();
int create_id_file();
std::string current_time(int gmt);
//void frame_centroid();
void abort_code();
int notify_handler(std::string input1, std::string input2);
int parse_checklist(std::string name, std::string value);
int create_default_config();

#endif
