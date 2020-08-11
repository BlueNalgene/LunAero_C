/*
 * C_LunAero/camera_LunAero.hpp - Camera control headers for LunAero_C
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
