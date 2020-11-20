/*
 * C_LunAero/gtk_LunAero.hpp - GUI Headers for LunAero_C
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

#ifndef GTK_LUNAERO_H
#define GTK_LUNAERO_H


// Standard C++ includes
#include <string>
#include <iostream>

// Module specific includes
#include <gtk/gtk.h>       // provides GTK3

// User Includes
#include "LunAero.hpp"

// Declare Functions
void screen_size();
void activate(GtkApplication *app, gpointer user_data);
gboolean key_event_preview(GtkWidget *widget, GdkEventKey *event);
gboolean key_event_running(GtkWidget *widget, GdkEventKey *event);
gboolean abort_check(GtkWidget* data);
gboolean g_framecheck(gpointer data);
gboolean key_event(GtkWidget *widget, GdkEventKey *event);
std::string get_css_string();
void first_record_killer(GtkWidget* data);
gboolean cb_subsequent(GtkWidget* data);
void mot_stop_command();
void mot_up_command();
void mot_down_command();
void mot_left_command();
void mot_right_command();

/**
 * Modifier value for determining the font size.  Customizable from settings.cfg
 */
inline int FONT_MOD = 20;
/**
 * Frequency in milliseconds to check the frame for moon centering.  Customizable from settings.cfg
 */
inline int FRAMECHECK_FREQ = 50;
/**
 * Keybinding for quit command
 * Customizable from settings.cfg
 */
inline std::string KV_QUIT = "q";
/**
 * Keybinding for left manual motor movement.
 * Customizable from settings.cfg
 */
inline std::string KV_LEFT = "Left";
/**
 * Keybinding for right manual motor movement.
 * Customizable from settings.cfg
 */
inline std::string KV_RIGHT = "Right";
/**
 * Keybinding for up manual motor movement.
 * Customizable from settings.cfg
 */
inline std::string KV_UP = "Up";
/**
 * Keybinding for down manual motor movement.
 * Customizable from settings.cfg
 */
inline std::string KV_DOWN = "Down";
/**
 * Keybinding for motor stop command.
 * Customizable from settings.cfg
 */
inline std::string KV_STOP = "space";
/**
 * Keybinding for raspivid refresh command.
 * Customizable from settings.cfg
 */
inline std::string KV_REFRESH = "z";
/**
 * Keybinding for greatly increasing the shutter speed.
 * Customizable from settings.cfg
 */
inline std::string KV_S_UP_UP = "g";
/**
 * Keybinding for greatly decreasing the shutter speed.
 * Customizable from settings.cfg
 */
inline std::string KV_S_DOWN_DOWN = "b";
/**
 * Keybinding for increasing the shutter speed.
 * Customizable from settings.cfg
 */
inline std::string KV_S_UP = "h";
/**
 * Keybinding for decreasing the shutter speed.
 * Customizable from settings.cfg
 */
inline std::string KV_S_DOWN = "n";
/**
 * Keybinding for cycling the ISO value.
 * Customizable from settings.cfg
 */
inline std::string KV_ISO = "i";
/**
 * Keybinding for beginning the recording/entering automatic mode
 * Customizable from settings.cfg
 */
inline std::string KV_RUN = "Return";


/**
 * This is a class to hold the various GTK widgets and whatnots.
 */
class gtk_class {
	public:
		/**
		 * GTK Main Application
		 */
		inline static GtkApplication *app;
		/**
		 * Main GTK window
		 */
		inline static GtkWidget *window;
		/**
		 * GTK CSS provider
		 */
		inline static GtkCssProvider *provider;
		
		// Grids
		
		/**
		 * Primary grid for window layout.
		 */
		inline static GtkWidget *grid;
		/**
		 * Grid to hold the directional movement pad
		 */
		inline static GtkWidget *dirgrid;
		/**
		 * Grid to hold function buttons
		 */
		inline static GtkWidget *funcgrid;
		/**
		 * Grid to hold our fakebuttons
		 */
		inline static GtkWidget *fakegrid;
		
		// Button Containers
		
		/**
		 * Quit button container
		 */
		inline static GtkWidget *button_box_quit;
		/**
		 * Container for all of the fake buttons
		 */
		inline static GtkWidget *button_box_fakes;
		/**
		 * Container for all of the directional pad buttons
		 */
		inline static GtkWidget *button_box_dirgrid;
		/**
		 * Container for the camera refresh button
		 */
		inline static GtkWidget *button_box_redeploy;
		/**
		 * Container for all of the ISO/Shutter editing buttons
		 */
		inline static GtkWidget *button_box_funcs;
		/**
		 * Container for the "start recording" button
		 */
		inline static GtkWidget *button_box_activate;
		
		// Buttons
		
		/**
		 * exit button
		 */
		inline static GtkWidget *exit_button;
		/**
		 * movement up button
		 */
		inline static GtkWidget *button_up;
		/**
		 * movement down button
		 */
		inline static GtkWidget *button_down;
		/**
		 * movement left button
		 */
		inline static GtkWidget *button_left;
		/**
		 * movement right button
		 */
		inline static GtkWidget *button_right;
		/**
		 * movement stop button
		 */
		inline static GtkWidget *button_stop;
		/**
		 * refresh preview camera button
		 */
		inline static GtkWidget *button_camera_command;
		/**
		 * increase shutter value button
		 */
		inline static GtkWidget *button_shutter_up;
		/**
		 * decrease shutter value button
		 */
		inline static GtkWidget *button_shutter_down;
		/**
		 * greatly increase shutter button
		 */
		inline static GtkWidget *button_shutter_up_up;
		/**
		 * greately decrease shutter button
		 */
		inline static GtkWidget *button_shutter_down_down;
		/**
		 * cycle ISO button
		 */
		inline static GtkWidget *button_iso;
		/**
		 * begin recording button
		 */
		inline static GtkWidget *button_record;
		
		// Text Holders
		
		/**
		 * Container to hold the status (ISO/Shutter/Blur and Running)
		 */
		inline static GtkWidget *text_status;
		/**
		 * Container for shutter speed label
		 */
		inline static GtkWidget *text_shutter;
		/**
		 * Container for some blank lines that help arrange the grid
		 */
		inline static GtkWidget *text_description;
		
		// More fake buttons to shield the real fake button!
		
		/**
		 * fake button - does nothing
		 */
		inline static GtkWidget *fakebutton;
		/**
		 * fake button - does nothing
		 */
		inline static GtkWidget *fakebutton2;
		/**
		 * fake button - does nothing
		 */
		inline static GtkWidget *fakebutton3;
		/**
		 * fake button - does nothing
		 */
		inline static GtkWidget *fakebutton4;
		/**
		 * fake button - does nothing
		 */
		inline static GtkWidget *fakebutton5;
		
		/**
		 * Holds the CSS string.  Not an actual widtet!
		 */
		inline static const std::string css_string = get_css_string();
		/**
		 * Holds the key_id of a button pressed on the user's keyboard.
		 */
		inline static gulong key_id;
	

};

#endif
