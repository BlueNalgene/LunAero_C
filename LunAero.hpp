
#ifndef LUNAERO_H
#define LUNAERO_H

// Standard C++ Includes
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>          // provides C++ chrono
#include <ctime>           // provides c time funcitons for chrono usage
//~ #include <sstream>

//Special RPi include
#include "bcm_host.h"      // provides DISPMANX et al.

// Module specific Includes
#include <assert.h>        // provides assert
#include <signal.h>        // provides kill signals
#include <stdlib.h>        // provides system
#include <stdio.h>         // provides popen
#include <sys/mman.h>      // provides mmap
#include <sys/stat.h>      // provides mkdir
#include <sys/types.h>     // provides kill
#include <sys/wait.h>      // provides wait
#include <time.h>          // provides time
#include <unistd.h>        // provides usleep


// User Includes
#include "gtk_LunAero.hpp"
#include "motors_LunAero.hpp"
#include "camera_LunAero.hpp"

// Globally Defined Constants
#define LOST_THRESH 30
// Observed duration is 2100s in video, but 1801 in timestamp
inline std::chrono::duration<double> RECORD_DURATION = (std::chrono::duration<double>) 30.;

// Global variables (Inline Requires C++17)
inline int LOST_COUNTER = 0;
inline int COUNTER = 0;
inline int WORK_HEIGHT = 0;
inline int WORK_WIDTH = 0;
inline int RASPI_PID = 0;
inline std::string FILEPATH;
inline std::string DEFAULT_FILEPATH = "/media/pi/MOON1/";
inline std::string TSBUFF;
inline std::chrono::time_point OLD_RECORD_TIME = std::chrono::system_clock::now();

// Struct of values used when writing labels (Inline Requires C++17)
inline struct val_addresses {
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
void cb_framecheck();
void cleanup();
void kill_raspivid();
void current_frame();
std::string current_time(int gmt);
int frame_centroid(int lost_counter);
void abort_code();

#endif
