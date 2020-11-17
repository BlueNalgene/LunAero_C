
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

// Globally Defined Constants
#define LOST_THRESH 30
#define SAVE_DEBUG_IMAGE 0
// Observed duration is 2100s in video, but 1801 in timestamp
inline std::chrono::duration<double> RECORD_DURATION = (std::chrono::duration<double>) 1800.;

// Global variables (Inline Requires C++17)
inline int COUNTER = 0;
inline int WORK_HEIGHT = 0;
inline int WORK_WIDTH = 0;
inline int RVD_HEIGHT = 0;
inline int RVD_WIDTH = 0;
inline int RVD_XCORN = 0;
inline int RVD_YCORN = 0;
inline int BLUR_THRESH = 100;
//~ inline int RASPI_PID = 0;
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

inline sem_t LOCK;

// Struct of values used when writing labels (Inline Requires C++17)
inline struct val_addresses {
	volatile int * LOST_COUNTERaddr;
	volatile int * ISO_VALaddr;
	volatile int * SHUTTER_VALaddr;
	volatile int * RUN_MODEaddr;
	volatile int * ABORTaddr;
	volatile int * REFRESH_CAMaddr;
	/* Horizontal motion
	 * 0 = none
	 * 1 = left
	 * 2 = right
	 */
	volatile int * HORZ_DIRaddr;
	/* Vertical motion
	 * 0 = none
	 * 1 = up
	 * 2 = down
	 */
	volatile int * VERT_DIRaddr;
	/* Stop motor
	 * 0 = none
	 * 1 = horizontal
	 * 2 = vertical
	 * 3 = both
	 */
	volatile int * STOP_DIRaddr;
	volatile int * DUTY_Aaddr;
	volatile int * DUTY_Baddr;
	volatile int * SUBSaddr;
} val_ptr;

// Declare Function Prototypes
int main (int argc, char **argv);
int startup_disk_check();
float blur_test();
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

#endif
