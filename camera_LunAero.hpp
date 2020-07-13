#ifndef CAMERA_LUNAERO_H
#define CAMERA_LUNAERO_H

// Standard C++ includes
#include <string>
#include <iostream>

// Module specific includes
#include <gtk/gtk.h>       // provides GTK3

// User Includes
#include "LunAero.hpp"

// Function Prototypes
void camera_preview();
void camera_start();
void iso_cycle();
void first_record();
void refresh_camera();
void shutter_up();
void shutter_down();
void shutter_up_up();
void shutter_down_down();
void reset_record();


#endif
