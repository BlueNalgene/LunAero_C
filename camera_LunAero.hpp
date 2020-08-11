#ifndef CAMERA_LUNAERO_H
#define CAMERA_LUNAERO_H

// Standard C++ includes
#include <string>
#include <iostream>

// Module specific includes
#include <gtk/gtk.h>       // provides GTK3
#include <fstream>         // provides ifstream

// User Includes
#include "LunAero.hpp"

// Function Prototypes
int confirm_mmal_safety(int error_cnt);
void camera_preview();
void camera_start();
std::string command_cam_start();
std::string command_cam_preview();
void write_video_id();
void iso_cycle();
void first_record();
void refresh_camera();
void shutter_up();
void shutter_down();
void shutter_up_up();
void shutter_down_down();
void reset_record();


#endif
