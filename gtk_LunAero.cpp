/*
 * C_LunAero/gtk_LunAero.cpp - GUI functions for LunAero_C
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

#include "gtk_LunAero.hpp"

gboolean bb_runner(gpointer data) {
	if (*val_ptr.ABORTaddr == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			if (blur_bright()) {
				std::cerr << "ERROR: Encountered error running blur_bright" << std::endl;
				sem_wait(&LOCK);
				*val_ptr.ABORTaddr = 1;
				sem_post(&LOCK);
			}
		}
	}
	return TRUE;
}

/**
 * This function refreshes the text boxes on the side of the GTK window.  During preview and manual motor
 * movement mode, this portion of the screen shows the value of the selected SHUTTER_VAL, ISO, and blur
 * value.  Note that this does not show the values of the video in the preview screen if the values have
 * been adjusted but not refreshed using the refresh camera button.  During normal operation, the box
 * just says "running".
 *
 * @param data gpointer to data from callback.  Not used here.
 * @return gboolean status
 */
gboolean refresh_text_boxes(gpointer data) {
	if (*val_ptr.ABORTaddr == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			// Capture blur and brightness float and bool, respectively.
			float local_blur;
			bool local_bright;
			sem_wait(&LOCK);
			local_blur = *val_ptr.BLURaddr;
			local_bright = *val_ptr.BRIGHTaddr;
			sem_post(&LOCK);
			// Remove extraneous decimal points from float
			std::string mod_blur;
			mod_blur = std::to_string((int)floor(local_blur));
			mod_blur += ".";
			mod_blur += std::to_string((int)floor((std::fmod(local_blur, 1)*100)));
			// Construct message
			std::string msg;
			msg = "Shutter: ";
			msg += std::to_string(*val_ptr.SHUTTER_VALaddr);
			msg += "\nISO: ";
			msg += std::to_string(*val_ptr.ISO_VALaddr);
			msg += "\nFOCUS: ";
			msg += mod_blur;
			if (local_bright) {
				msg += "\n";
			} else {
				msg += "\nTOO BRIGHT!";
			}
			gtk_label_set_text(GTK_LABEL(gtk_class::text_status), msg.c_str());
		} else {
			std::string msg;
			msg = "Running";
			gtk_label_set_text(GTK_LABEL(gtk_class::text_status), msg.c_str());
		}
	}
    return TRUE;
}

/**
 * This callback function is a local holder of the cb_framecheck function from LunAero.cpp
 *
 * @param data gpointer to data from callback.  Not used here.
 * @return gboolean status
 */
gboolean g_framecheck(gpointer data) {
	cb_framecheck();
	return TRUE;
}

/**
 * This funciton, called at program start, measures the available screen size the GTK window can occupy.
 * Several globals are defined here, including those of the RVD_ prototype, WORK_WIDTH, and WORK_HEIGHT.
 * If you are having strange behavior related to screen sizing, check here first.  Note that some of the
 * values do not always behave well with the VC/DISPMANX screen program, since GTK is based on the X
 * window environment and VC/DISPMANX is a unique entity.  If the screenshot related functions are
 * experiencing issues, check those functions first rather than starting here.
 *
 */
void screen_size () {
	gtk_init(0, NULL);
	GdkRectangle workarea = {0};
	gdk_monitor_get_workarea(gdk_display_get_primary_monitor(gdk_display_get_default()), &workarea);
	// Calculate the screen workarea
	WORK_HEIGHT = workarea.height;
	WORK_WIDTH = workarea.width;
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "W: " << WORK_WIDTH << " x H: " << WORK_HEIGHT << std::endl;
		LOGGING.close();
	}
	// Calculate the estimated size of a Raspivid window
	RVD_HEIGHT = WORK_HEIGHT/2;
	RVD_WIDTH = WORK_WIDTH/2;
	if ((RVD_WIDTH/RVD_HEIGHT) != (16/9)) {
		if ((RVD_WIDTH/RVD_HEIGHT) > (16/9)) {
			RVD_WIDTH = RVD_HEIGHT * 16/9;
		} else {
			RVD_HEIGHT = RVD_WIDTH * 9/16;
		}
	}
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Est Raspivid preview W: " << RVD_WIDTH << " x H: " << RVD_HEIGHT << std::endl;
		LOGGING.close();
	}
	// Calculate the Raspivid preview corner
	RVD_XCORN = (WORK_WIDTH/2)-(WORK_WIDTH/4);
	RVD_YCORN = (WORK_HEIGHT/2);
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Raspivid corner X: " << RVD_XCORN << " x Y: " << RVD_YCORN << std::endl;
		LOGGING.close();
	}
	return;
}

/**
 * This function constructs a CSS string which is used to format the GTK window.  This is a hacky way to
 * generate the CSS code since we want our screen properties and font size to be dependent on the size
 * of the screen as determined by WORK_WIDTH and WORK_HEIGHT.  Therefore, a static CSS will not do.
 *
 * @return css_string the CSS formatting code formatted as a string
 */
std::string get_css_string() {
	screen_size();
	std::string css_string;
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "width: " << WORK_WIDTH << std::endl
		<< "height: " << WORK_HEIGHT << std::endl
		<< "font-size: " << ((WORK_WIDTH*FONT_MOD)/WORK_HEIGHT) << std::endl;
		LOGGING.close();
	}
	std::string font_size_string = std::to_string((WORK_WIDTH*FONT_MOD)/WORK_HEIGHT);
	css_string = "window { background-color: black; \
		 color: red; \
		 font-size: " + font_size_string + "px; \
		 font-weight: bolder; } \
		 .activebutton { background-image: image(dimgray); \
		 border-color: dimgray; \
		 text-shadow: 0 1px black; \
		 color: red; \
		 font-size: " + font_size_string + "px; \
		 font-weight: bolder; } \
		 .fakebutton {background-image: image(black); \
		 color: black; \
		 border-width: 0px; \
		 border-color: black; } \
		 .shadowbutton {background-image: image(dimgray); \
		 color: darkgray; \
		 font-size: " + font_size_string + "px; \
		 font-weight: bolder; \
		 border-width: 0px; \
		 border-color: black; }";
	return css_string;
}

/**
 * This function prepares the GTK window layout.  All widgets available to our GTK setup are defined here
 * before being activated.  The steps taken are 1) Define the window based on dynamic screen size 2)
 * create table layouts and apply to screen 3) define simple buttons 4) define button boxes to hold the
 * buttons 5) define exit button and box 6) define fake buttons (buttons used to prevent incorrect
 * keyboard focus) and grid 6) define directional button pad and grid 7) define refresh button 8)
 * define camera function buttons using a grid layout 9) define record button and box 10) define text
 * boxes 11) attach these elements to a grid (rudimentary image of layout is in the function comments)
 * 12) expand and align all boxes to fill space.
 *
 * @param *app Pointer value to the whole GtkApplication
 * @param local_val_ptr payload passed to this function (unused)
 */
void gtk_global_setup(GtkApplication *app, gpointer local_val_ptr) {
	// Define Window, dynamic size for screen.
	gtk_class::window = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (gtk_class::window), "LunAero Moontracker");
	gtk_window_fullscreen (GTK_WINDOW (gtk_class::window));
	
	// Create table and put it in the window.
	gtk_class::grid = gtk_grid_new();
	gtk_container_add (GTK_CONTAINER (gtk_class::window), gtk_class::grid);
	
	// Define Button Boxes.
	gtk_class::button_box_quit = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_class::button_box_fakes = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_class::button_box_dirgrid = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_class::button_box_redeploy = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_class::button_box_funcs = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_class::button_box_activate = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	
	// Define Exit Button, put it in a box.
	gtk_class::exit_button = gtk_button_new_with_label("Exit");
	gtk_container_add(GTK_CONTAINER(gtk_class::button_box_quit), gtk_class::exit_button);
	
	// Define the Fake button, put it in a box
	// Shield this with other fake buttons so keypresses can't move it much
	// This doubles as a placeholder for the raspicam window
	gtk_class::fakebutton = gtk_button_new_with_label(" ");
	gtk_class::fakebutton2 = gtk_button_new_with_label(" ");
	gtk_class::fakebutton3 = gtk_button_new_with_label(" ");
	gtk_class::fakebutton4 = gtk_button_new_with_label(" ");
	gtk_class::fakebutton5 = gtk_button_new_with_label(" ");
	gtk_class::fakegrid = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(gtk_class::fakegrid), 10);
	gtk_grid_attach(GTK_GRID(gtk_class::fakegrid), gtk_class::fakebutton, 0, 0, 1, 1);
	gtk_grid_attach_next_to(GTK_GRID(gtk_class::fakegrid), gtk_class::fakebutton2, gtk_class::fakebutton, GTK_POS_TOP, 1, 1);
	gtk_grid_attach_next_to(GTK_GRID(gtk_class::fakegrid), gtk_class::fakebutton3, gtk_class::fakebutton, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to(GTK_GRID(gtk_class::fakegrid), gtk_class::fakebutton4, gtk_class::fakebutton, GTK_POS_LEFT, 1, 1);
	gtk_grid_attach_next_to(GTK_GRID(gtk_class::fakegrid), gtk_class::fakebutton5, gtk_class::fakebutton, GTK_POS_RIGHT, 1, 1);
	gtk_container_add(GTK_CONTAINER(gtk_class::button_box_fakes), gtk_class::fakegrid);
	
	// Define directional buttons...
	gtk_class::button_up = gtk_button_new_with_label("^");
	gtk_class::button_down = gtk_button_new_with_label("v");
	gtk_class::button_left = gtk_button_new_with_label("<");
	gtk_class::button_right = gtk_button_new_with_label(">");
	gtk_class::button_stop = gtk_button_new_with_label("stop");
	// Put these buttons in the special grid...
	gtk_class::dirgrid = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(gtk_class::dirgrid), 10);
	gtk_grid_attach(GTK_GRID(gtk_class::dirgrid), gtk_class::button_stop, 0, 0, 1, 1);
	gtk_grid_attach_next_to(GTK_GRID(gtk_class::dirgrid), gtk_class::button_up, gtk_class::button_stop, GTK_POS_TOP, 1, 1);
	gtk_grid_attach_next_to(GTK_GRID(gtk_class::dirgrid), gtk_class::button_down, gtk_class::button_stop, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to(GTK_GRID(gtk_class::dirgrid), gtk_class::button_left, gtk_class::button_stop, GTK_POS_LEFT, 1, 1);
	gtk_grid_attach_next_to(GTK_GRID(gtk_class::dirgrid), gtk_class::button_right, gtk_class::button_stop, GTK_POS_RIGHT, 1, 1);
	// ...Put it in a box
	gtk_container_add(GTK_CONTAINER (gtk_class::button_box_dirgrid), gtk_class::dirgrid);
	
	// Define redeploy button and put in a box
	gtk_class::button_camera_command = gtk_button_new_with_label("Reissue\nCamera\nCommand");
	gtk_container_add(GTK_CONTAINER(gtk_class::button_box_redeploy), gtk_class::button_camera_command);
	
	// Define camera function buttons, plus text, and put in a box
	gtk_class::button_shutter_up = gtk_button_new_with_label("+");
	gtk_class::button_shutter_down = gtk_button_new_with_label("-");
	gtk_class::button_shutter_up_up = gtk_button_new_with_label("++");
	gtk_class::button_shutter_down_down = gtk_button_new_with_label("--");
	gtk_class::button_iso = gtk_button_new_with_label("ISO");
	gtk_class::text_shutter = gtk_label_new("Shutter Speed");
	// Use a grid for all the buttons and text
	gtk_class::funcgrid = gtk_grid_new();
	gtk_grid_set_column_spacing (GTK_GRID (gtk_class::funcgrid), 10);
	gtk_grid_attach (GTK_GRID (gtk_class::funcgrid), gtk_class::button_shutter_up, 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (gtk_class::funcgrid), gtk_class::button_shutter_up_up, 1, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (gtk_class::funcgrid), gtk_class::text_shutter, 0, 1, 2, 1);
	gtk_grid_attach (GTK_GRID (gtk_class::funcgrid), gtk_class::button_shutter_down, 0, 2, 1, 1);
	gtk_grid_attach (GTK_GRID (gtk_class::funcgrid), gtk_class::button_shutter_down_down, 1, 2, 1, 1);
	gtk_grid_attach (GTK_GRID (gtk_class::funcgrid), gtk_class::button_iso, 2, 0, 1, 2);
	gtk_widget_set_valign(gtk_class::button_iso, GTK_ALIGN_FILL);
	// ...Put it in a box
	gtk_container_add(GTK_CONTAINER (gtk_class::button_box_funcs), gtk_class::funcgrid);
	
	// Define record button and put in a box
	gtk_class::button_record = gtk_button_new_with_label("▶️");
	gtk_container_add(GTK_CONTAINER (gtk_class::button_box_activate), gtk_class::button_record);
	
	// Define text description
	// This is an inelegant special padding element which adjusts based on screen size
	gtk_class::text_description = gtk_label_new("\n\n\n");
	
	// Define text status
	gtk_class::text_status = gtk_label_new("here");
	
	// Assemble: Attach elements to grid
	// To work with raspicam overlay, must be grid with 4 cols, 4 rows, equally spaced
	//
	// -----------------
	// | 1 | 2 |       |
	// |-------|   3   |
	// |   4   |       |
	// |---------------|
	// |   |       |   |
	// | 5 | video | 6 |
	// |   |       |   |
	// -----------------
	//
	// Grid cells shall be assigned functions bound to their size
	// Box 1: Quit Button
	// Box 2: Redeploy camera button
	// Box 3: Directional control pad
	// Box 4: Video property control buttons
	// Box 5: Video property text box
	// Box 6: Begin recording button/currently recording status
	// Video box: Include fake button(s) to prevent keyboard focus loss
	gtk_grid_attach (GTK_GRID (gtk_class::grid), gtk_class::button_box_quit, 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (gtk_class::grid), gtk_class::button_box_redeploy, 1, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (gtk_class::grid), gtk_class::button_box_dirgrid, 2, 0, 2, 2);
	gtk_grid_attach (GTK_GRID (gtk_class::grid), gtk_class::button_box_funcs, 0, 1, 2, 1);
	gtk_grid_attach (GTK_GRID (gtk_class::grid), gtk_class::text_status, 0, 2, 1, 2);
	gtk_grid_attach (GTK_GRID (gtk_class::grid), gtk_class::button_box_fakes, 1, 2, 2, 2);
	gtk_grid_attach (GTK_GRID (gtk_class::grid), gtk_class::button_box_activate, 3, 2, 1, 2);
	gtk_grid_attach (GTK_GRID (gtk_class::grid), gtk_class::text_description, 0, 4, 4, 1);

	// Expand all of the widget boxes on the display
	gtk_widget_set_vexpand (gtk_class::button_box_quit, TRUE);
	gtk_widget_set_hexpand (gtk_class::button_box_quit, TRUE);
	gtk_widget_set_vexpand (gtk_class::button_box_redeploy, TRUE);
	gtk_widget_set_hexpand (gtk_class::button_box_redeploy, TRUE);
	gtk_widget_set_vexpand (gtk_class::button_box_dirgrid, TRUE);
	gtk_widget_set_hexpand (gtk_class::button_box_dirgrid, TRUE);
	gtk_widget_set_vexpand (gtk_class::button_box_funcs, TRUE);
	gtk_widget_set_hexpand (gtk_class::button_box_funcs, TRUE);
	gtk_widget_set_vexpand (gtk_class::text_status, TRUE);
	gtk_widget_set_hexpand (gtk_class::text_status, TRUE);
	gtk_widget_set_vexpand (gtk_class::button_box_fakes, TRUE);
	gtk_widget_set_hexpand (gtk_class::button_box_fakes, TRUE);
	gtk_widget_set_vexpand (gtk_class::button_box_activate, TRUE);
	gtk_widget_set_hexpand (gtk_class::button_box_activate, TRUE);
	// Alignment of boxes
	gtk_widget_set_halign(gtk_class::button_box_quit, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(gtk_class::button_box_quit, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(gtk_class::button_box_redeploy, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(gtk_class::button_box_redeploy, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(gtk_class::button_box_dirgrid, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(gtk_class::button_box_dirgrid, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(gtk_class::button_box_funcs, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(gtk_class::button_box_funcs, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(gtk_class::text_status, GTK_ALIGN_START);
	gtk_widget_set_valign(gtk_class::text_status, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(gtk_class::button_box_fakes, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(gtk_class::button_box_fakes, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(gtk_class::button_box_activate, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(gtk_class::button_box_activate, GTK_ALIGN_CENTER);
	return;
}

/**
 * This function defines the stop button response and tells motors_LunAero.cpp to execute a stop on both
 * motors.
 *
 */
void mot_stop_command() {
	if (*val_ptr.RUN_MODEaddr == 0) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "mot stop command" << std::endl;
			LOGGING.close();
		}
	} else {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "mot stop auto" << std::endl;
			LOGGING.close();
		}
	}
	sem_wait(&LOCK);
	*val_ptr.STOP_DIRaddr = 3;
	sem_post(&LOCK);
}

/**
 * This function describes the up button response and sets the behavior value to be processed on the
 * next cycle of motors_LunAero.cpp/motor_handler.
 *
 */
void mot_up_command() {
	if (*val_ptr.RUN_MODEaddr == 0) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "mot up command" << std::endl;
			LOGGING.close();
		}
	} else {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "mot up auto" << std::endl;
			LOGGING.close();
		}
	}
	if (*val_ptr.STOP_DIRaddr == 3) {
		sem_wait(&LOCK);
		*val_ptr.STOP_DIRaddr = 1;
		sem_post(&LOCK);
	} else if (*val_ptr.STOP_DIRaddr == 2) {
		sem_wait(&LOCK);
		*val_ptr.STOP_DIRaddr = 0;
		sem_post(&LOCK);
	}
	sem_wait(&LOCK);
	*val_ptr.VERT_DIRaddr = 1;
	sem_post(&LOCK);
}

/**
 * This function describes the down button response and sets the behavior value to be processed on the
 * next cycle of motors_LunAero.cpp/motor_handler.
 *
 */
void mot_down_command() {
	if (*val_ptr.RUN_MODEaddr == 0) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "mot down command" << std::endl;
			LOGGING.close();
		}
	} else {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "mot down auto" << std::endl;
			LOGGING.close();
		}
	}
	if (*val_ptr.STOP_DIRaddr == 3) {
		sem_wait(&LOCK);
		*val_ptr.STOP_DIRaddr = 1;
		sem_post(&LOCK);
	} else if (*val_ptr.STOP_DIRaddr == 2) {
		sem_wait(&LOCK);
		*val_ptr.STOP_DIRaddr = 0;
		sem_post(&LOCK);
	}
	sem_wait(&LOCK);
	*val_ptr.VERT_DIRaddr = 2;
	sem_post(&LOCK);
}

/**
 * This function describes the left button response and sets the behavior value to be processed on the
 * next cycle of motors_LunAero.cpp/motor_handler.
 *
 */
void mot_left_command() {
	if (*val_ptr.RUN_MODEaddr == 0) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "mot left command" << std::endl;
			LOGGING.close();
		}
	} else {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "mot left auto" << std::endl;
			LOGGING.close();
		}
	}
	if (*val_ptr.STOP_DIRaddr == 3) {
		sem_wait(&LOCK);
		*val_ptr.STOP_DIRaddr = 2;
		sem_post(&LOCK);
	} else if (*val_ptr.STOP_DIRaddr == 1) {
		sem_wait(&LOCK);
		*val_ptr.STOP_DIRaddr = 0;
		sem_post(&LOCK);
	}
	sem_wait(&LOCK);
	*val_ptr.HORZ_DIRaddr = 1;
	sem_post(&LOCK);
}

/**
 * This function describes the right button response and sets the behavior value to be processed on the
 * next cycle of motors_LunAero.cpp/motor_handler.
 *
 */
void mot_right_command() {
	if (*val_ptr.RUN_MODEaddr == 0) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "mot right command" << std::endl;
			LOGGING.close();
		}
	} else {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "mot right auto" << std::endl;
			LOGGING.close();
		}
	}
	if (*val_ptr.STOP_DIRaddr == 3) {
		sem_wait(&LOCK);
		*val_ptr.STOP_DIRaddr = 2;
		sem_post(&LOCK);
	} else if (*val_ptr.STOP_DIRaddr == 1) {
		sem_wait(&LOCK);
		*val_ptr.STOP_DIRaddr = 0;
		sem_post(&LOCK);
	}
	sem_wait(&LOCK);
	*val_ptr.HORZ_DIRaddr = 2;
	sem_post(&LOCK);
}

/**
 * This function connects the buttons available during preview/manual mode to the appropriate functions
 * on button behaviors.
 *
 */
void gtk_buttons_preview() {
	// Connect signals to buttons
	g_signal_connect_swapped(gtk_class::exit_button, "clicked", G_CALLBACK (abort_code), NULL);
	g_signal_connect_swapped(gtk_class::button_stop, "clicked", G_CALLBACK (mot_stop_command), NULL);
	g_signal_connect_swapped(gtk_class::button_up, "clicked", G_CALLBACK (mot_up_command), NULL);
	g_signal_connect_swapped(gtk_class::button_down, "clicked", G_CALLBACK (mot_down_command), NULL);
	g_signal_connect_swapped(gtk_class::button_left, "clicked", G_CALLBACK (mot_left_command), NULL);
	g_signal_connect_swapped(gtk_class::button_right, "clicked", G_CALLBACK (mot_right_command), NULL);
	g_signal_connect_swapped(gtk_class::button_shutter_up, "clicked", G_CALLBACK (shutter_up), NULL);
	g_signal_connect_swapped(gtk_class::button_shutter_down, "clicked", G_CALLBACK (shutter_down), NULL);
	g_signal_connect_swapped(gtk_class::button_shutter_up_up, "clicked", G_CALLBACK (shutter_up_up), NULL);
	g_signal_connect_swapped(gtk_class::button_shutter_down_down, "clicked", G_CALLBACK (shutter_down_down), NULL);
	g_signal_connect_swapped(gtk_class::button_iso, "clicked", G_CALLBACK (iso_cycle), NULL);
	g_signal_connect_swapped(gtk_class::button_camera_command, "clicked", G_CALLBACK (refresh_camera), NULL);
	g_signal_connect_swapped(gtk_class::button_record, "clicked", G_CALLBACK (first_record_killer), NULL);
	
	// Capture Key Events
	gtk_class::key_id = g_signal_connect(gtk_class::window, "key-release-event", G_CALLBACK(key_event_preview), NULL);
	return;
}

/**
 * This function applies the CSS style to the elements of the preview/manual mode GTK window.
 *
 */
void gtk_css_preview() {
	// CSS stylesheet without using CSS
	//~ std::string css_string = get_css_string();
	gtk_class::provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(gtk_class::provider, gtk_class::css_string.c_str(), -1, NULL);
	// Apply CSS style to window
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::window), 
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	// Apply activebutton class to buttons
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::exit_button), 
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_up),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_down),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_left),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_right),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_stop),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_camera_command),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_shutter_up),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_shutter_down),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_shutter_up_up),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_shutter_down_down),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_iso),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_record),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::exit_button), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_up), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_down), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_left), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_right), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_stop), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_camera_command), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_shutter_up), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_shutter_down), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_shutter_up_up), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_shutter_down_down), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_iso), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_record), "activebutton");
	
	// Add fakebuttons to special button class
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::fakebutton),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::fakebutton2),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::fakebutton3),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::fakebutton4),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::fakebutton5),
		GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::fakebutton), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::fakebutton2), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::fakebutton3), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::fakebutton4), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::fakebutton5), "fakebutton");
	
	return;
}

/**
 * This is the main function of the GTK code, per the GTK usage guide.  The elements are set up from
 * global definitions, buttons are connected, and the CSS is applied.  Timeouts are applied which, after
 * a set number of microseconds (or maybe cycles?) the function in G_SOURCE_FUNC is called.  Keyboard
 * focus is set to one of the fake buttons to prevent accidental button presses.  Finally, the actual
 * window is activated.
 *
 * @param *app Pointer value to the whole GtkApplication
 * @param local_val_ptr payload passed to this function (unused but passed on)
 */
void activate(GtkApplication *app, gpointer local_val_ptr) {
	gtk_global_setup(app, local_val_ptr);
	gtk_buttons_preview();
	gtk_css_preview();
	
	// Create timeout to refresh
	g_timeout_add(100, G_SOURCE_FUNC(refresh_text_boxes), NULL);
	g_timeout_add(50, G_SOURCE_FUNC(abort_check), NULL);
	g_timeout_add(60, G_SOURCE_FUNC(cb_subsequent), app);
	g_timeout_add(700, G_SOURCE_FUNC(bb_runner), NULL);
	
	//Activate!
	//~ refresh_text_boxes(NULL);
	gtk_widget_grab_focus(gtk_class::fakebutton);
	gtk_widget_show_all(gtk_class::window);
}

/**
 * This function defines keyboard button press events for the preview/manual mode.  The code here
 * captures button pressed on the user's keyboard and issues the appropriate command.
 *
 * @param *widget Pointer value to the associated GTK widget
 * @param *event payload contents of button press event passed to this function.  Holds keyvalue.
 * @return gboolean status
 */
gboolean key_event_preview(GtkWidget *widget, GdkEventKey *event) {

	gchar* val = gdk_keyval_name (event->keyval);
	if (strcmp(val, KV_LEFT.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			mot_left_command();
		}
	} else if (strcmp(val, KV_RIGHT.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			mot_right_command();
		}
	} else if (strcmp(val, KV_UP.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			mot_up_command();
		}
	} else if (strcmp(val, KV_DOWN.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			mot_down_command();
		}
	} else if (strcmp(val, KV_STOP.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			mot_stop_command();
		}
	} else if (strcmp(val, KV_REFRESH.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			refresh_camera();
		}
	} else if (strcmp(val, KV_QUIT.c_str()) == 0) {
		*val_ptr.ABORTaddr = 1;
	} else if (strcmp(val, KV_S_UP_UP.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			shutter_up_up();
		}
	} else if (strcmp(val, KV_S_DOWN_DOWN.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			shutter_down_down();
		}
	} else if (strcmp(val, KV_S_UP.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			shutter_up();
		}
	} else if (strcmp(val, KV_S_DOWN.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			shutter_down();
		}
	} else if (strcmp(val, KV_ISO.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			iso_cycle();
		}
	} else if (strcmp(val, KV_RUN.c_str()) == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			first_record_killer(NULL);
		}
	} else {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "keyval: \"" << val << "\" not used here\n" << std::endl;
			LOGGING.close();
		}
	}
	
	// Always return the keyboard focus back to our fake button
	gtk_widget_grab_focus(gtk_class::fakebutton);
	
	return 0;
}

/**
 * This function defines keyboard button press events for the recording/automatic mode.  The code here
 * captures button pressed on the user's keyboard and issues the appropriate command.
 *
 * @param *widget Pointer value to the associated GTK widget
 * @param *event payload contents of button press event passed to this function.  Holds keyvalue.
 * @return gboolean status
 */
gboolean key_event_running(GtkWidget *widget, GdkEventKey *event) {
	//g_printerr("%s\n", gdk_keyval_name (event->keyval));
	
	
	gchar* val = gdk_keyval_name (event->keyval);

	if (strcmp(val, KV_QUIT.c_str()) == 0) {
		sem_wait(&LOCK);
		*val_ptr.ABORTaddr = 1;
		sem_post(&LOCK);
	} else {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "keyval: \"" << val << "\" not used here\n" << std::endl;
			LOGGING.close();
		}
	}
	
	// Always return the keyboard focus back to our fake button
	gtk_widget_grab_focus(gtk_class::fakebutton);
	
	return 0;
}

/**
 * This function checks whether the ABORT flag has been set elsewhere in the code.  If it is found, the
 * appropriate action is taken.  Next, the code checks if the LOST_COUNTER has passed the LOST_THRESH.
 * If this value is breached, the moon has been lost by LunAero and a shutdown is initiated.
 *
 * @param data gpointer to data from callback.  Not used here.
 * @return gboolean status
 */
gboolean abort_check(GtkWidget* data) {
	if (*val_ptr.ABORTaddr == 1) {
		if (*val_ptr.LOST_COUNTERaddr > LOST_THRESH) {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "lost moon, shutting down" << std::endl;
				LOGGING.close();
			}
		} else {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "recieved shutdown command from user" << std::endl;
				LOGGING.close();
			}
		}
		gtk_window_close(GTK_WINDOW(gtk_class::window));
		g_application_quit(G_APPLICATION(gtk_class::app));
		cleanup();
	}
	
    return TRUE;
}

/**
 * This function is called when the mode is switched from preview/manual mode to recording/automatic
 * mode.  The code here transitions between the modes by refreshing elements of the screen to be kept
 * and removing functionality from buttons that no longer have function in recording/automatic mode.
 * Keyboard bindings from preview/manual mode are disconnected and the new bindings are set.  Finally,
 * a new timeout is added to call g_framecheck and begin testing frames for moon centering.  The mode
 * flag RUN_MODE is incremented here.
 *
 * @param data gpointer to data from callback.  Not used here.
 */
void first_record_killer(GtkWidget* data) {
	sem_wait(&LOCK);
	
	//~ GList   *listrunner;
	//~ gint    *value;
	
	//~ listrunner = g_list_first(gtk_style_context_list_classes (gtk_widget_get_style_context(gtk_class::button_up)));
	//~ while (listrunner) {
		//~ value = (gint *)listrunner->data;
		//~ printf("first %d\n", *value);
		//~ listrunner = g_list_next(listrunner);
	//~ }
	
	gtk_style_context_remove_class(gtk_widget_get_style_context(gtk_class::button_up), "activebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_up), "fakebutton");
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_up), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	
	
	//~ // Assign fakebutton context to each
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_down), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_left), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_right), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_stop), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_camera_command), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_shutter_up), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_shutter_down), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_shutter_up_up), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_shutter_down_down), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_iso), "fakebutton");
	gtk_style_context_add_class(gtk_widget_get_style_context(gtk_class::button_record), "fakebutton");
	
	//~ // Re-add the providers
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_down), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_left), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_right), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_stop), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_camera_command), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_shutter_up), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_shutter_down), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_shutter_up_up), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_shutter_down_down), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_iso), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(gtk_class::button_record), GTK_STYLE_PROVIDER(gtk_class::provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	
	gtk_label_set_text(GTK_LABEL(gtk_class::text_shutter), "");
	g_signal_handler_disconnect(gtk_class::window, gtk_class::key_id);
	g_signal_connect(gtk_class::window, "key-release-event", G_CALLBACK(key_event_running), NULL);
	g_timeout_add(FRAMECHECK_FREQ, G_SOURCE_FUNC(g_framecheck), NULL);
	
	gtk_widget_queue_draw(gtk_class::window);
	
	while (g_main_context_pending(NULL)) {
		g_main_context_iteration(NULL,FALSE);
	}
	
	sem_post(&LOCK);
	
	first_record();
	sem_wait(&LOCK);
	*val_ptr.RUN_MODEaddr = 1;
	sem_post(&LOCK);
}

/**
 * This callback function handles resetting video recording subsequent to the first recording event,
 * which is manually called by the user on mode switch.
 *
 * @param data gpointer to data from callback.  Not used here.
 * @return gboolean status
 */
gboolean cb_subsequent(GtkWidget* data) {
	if (*val_ptr.SUBSaddr == 2) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "cb sub 2 "
			<< std::endl;
			LOGGING.close();
		}
		reset_record();
		sem_wait(&LOCK);
		*val_ptr.SUBSaddr = 0;
		sem_post(&LOCK);
	//~ } else {
		//~ std::cout << "No subs" << std::endl;
	}
	return TRUE;
}
