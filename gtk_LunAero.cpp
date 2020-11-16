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

gboolean refresh_text_boxes(gpointer data) {
	if (*val_ptr.ABORTaddr == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			float blur_val = blur_test();
			std::string msg;
			msg = "Shutter Speed:\n  ";
			msg += std::to_string(*val_ptr.SHUTTER_VALaddr);
			msg += "\nISO:\n  ";
			msg += std::to_string(*val_ptr.ISO_VALaddr);
			msg += "\nFOCUS VAL:\n";
			msg += std::to_string(floor(blur_val*100)/100);
			//~ std::cout << msg << std::endl;
			gtk_label_set_text(GTK_LABEL(gtk_class::text_status), msg.c_str());
		} else {
			std::string msg;
			msg = "Running";
			gtk_label_set_text(GTK_LABEL(gtk_class::text_status), msg.c_str());
		}
	}
    return TRUE;
}

gboolean g_framecheck(gpointer data) {
	cb_framecheck();
	return TRUE;
}

void screen_size () {
	gtk_init(0, NULL);
	GdkRectangle workarea = {0};
	gdk_monitor_get_workarea(gdk_display_get_primary_monitor(gdk_display_get_default()), &workarea);
	// Calculate the screen workarea
	WORK_HEIGHT = workarea.height;
	WORK_WIDTH = workarea.width;
	std::cout << "W: " << WORK_WIDTH << " x H: " << WORK_HEIGHT << std::endl;
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
	std::cout << "Est Raspivid preview W: " << RVD_WIDTH << " x H: " << RVD_HEIGHT << std::endl;
	// Calculate the Raspivid preview corner
	RVD_XCORN = (WORK_WIDTH/2)-(WORK_WIDTH/4);
	RVD_YCORN = (WORK_HEIGHT/2);
	std::cout << "Raspivid corner X: " << RVD_XCORN << " x Y: " << RVD_YCORN << std::endl;
	return;
}

std::string get_css_string() {
	screen_size();
	std::string css_string;
	std::cout << "width: " << WORK_WIDTH << std::endl;
	std::cout << "height: " << WORK_HEIGHT << std::endl;
	std::cout << "font-size: " << ((WORK_WIDTH*20)/WORK_HEIGHT) << std::endl;
	std::string font_size_string = std::to_string((WORK_WIDTH*20)/WORK_HEIGHT);
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

void mot_stop_command() {
	if (*val_ptr.RUN_MODEaddr == 0) {
		std::cout << "mot stop command" << std::endl;
	} else {
		std::cout << "mot stop auto" << std::endl;
	}
	sem_wait(&LOCK);
	*val_ptr.STOP_DIRaddr = 3;
	sem_post(&LOCK);
}

void mot_up_command() {
	if (*val_ptr.RUN_MODEaddr == 0) {
		std::cout << "mot up command" << std::endl;
	} else {
		std::cout << "mot up auto" << std::endl;
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

void mot_down_command() {
	if (*val_ptr.RUN_MODEaddr == 0) {
		std::cout << "mot down command" << std::endl;
	} else {
		std::cout << "mot down auto" << std::endl;
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

void mot_left_command() {
	if (*val_ptr.RUN_MODEaddr == 0) {
		std::cout << "mot left command" << std::endl;
	} else {
		std::cout << "mot left auto" << std::endl;
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

void mot_right_command() {
	if (*val_ptr.RUN_MODEaddr == 0) {
		std::cout << "mot right command" << std::endl;
	} else {
		std::cout << "mot right auto" << std::endl;
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

void gtk_buttons_preview() {
	// Connect signals to buttons
	g_signal_connect_swapped (gtk_class::exit_button, "clicked", G_CALLBACK (abort_code), NULL);
	g_signal_connect_swapped (gtk_class::button_stop, "clicked", G_CALLBACK (mot_stop_command), NULL);
	g_signal_connect_swapped (gtk_class::button_up, "clicked", G_CALLBACK (mot_up_command), NULL);
	g_signal_connect_swapped (gtk_class::button_down, "clicked", G_CALLBACK (mot_down_command), NULL);
	g_signal_connect_swapped (gtk_class::button_left, "clicked", G_CALLBACK (mot_left_command), NULL);
	g_signal_connect_swapped (gtk_class::button_right, "clicked", G_CALLBACK (mot_right_command), NULL);
	g_signal_connect_swapped (gtk_class::button_shutter_up, "clicked", G_CALLBACK (shutter_up), NULL);
	g_signal_connect_swapped (gtk_class::button_shutter_down, "clicked", G_CALLBACK (shutter_down), NULL);
	g_signal_connect_swapped (gtk_class::button_shutter_up_up, "clicked", G_CALLBACK (shutter_up_up), NULL);
	g_signal_connect_swapped (gtk_class::button_shutter_down_down, "clicked", G_CALLBACK (shutter_down_down), NULL);
	g_signal_connect_swapped (gtk_class::button_iso, "clicked", G_CALLBACK (iso_cycle), NULL);
	g_signal_connect_swapped (gtk_class::button_camera_command, "clicked", G_CALLBACK (refresh_camera), NULL);
	g_signal_connect_swapped (gtk_class::button_record, "clicked", G_CALLBACK (first_record_killer), NULL);
	
	// Capture Key Events
	gtk_class::key_id = g_signal_connect(gtk_class::window, "key-release-event", G_CALLBACK(key_event_preview), NULL);
	return;
}

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

void activate(GtkApplication *app, gpointer local_val_ptr) {
	gtk_global_setup(app, local_val_ptr);
	gtk_buttons_preview();
	gtk_css_preview();
	
	// Create timeout to refresh
	g_timeout_add(500, G_SOURCE_FUNC(refresh_text_boxes), NULL);
	g_timeout_add(50, G_SOURCE_FUNC(abort_check), NULL);
	g_timeout_add(60, G_SOURCE_FUNC(cb_subsequent), app);
	
	//Activate!
	//~ refresh_text_boxes(NULL);
	gtk_widget_grab_focus(gtk_class::fakebutton);
	gtk_widget_show_all(gtk_class::window);
}

gboolean key_event_preview(GtkWidget *widget, GdkEventKey *event) {

	gchar* val = gdk_keyval_name (event->keyval);
	if (strcmp(val, "Left") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			mot_left_command();
		}
	} else if (strcmp(val, "Right") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			mot_right_command();
		}
	} else if (strcmp(val, "Up") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			mot_up_command();
		}
	} else if (strcmp(val, "Down") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			mot_down_command();
		}
	} else if (strcmp(val, "space") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			mot_stop_command();
		}
	} else if (strcmp(val, "z") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			refresh_camera();
		}
	} else if (strcmp(val, "q") == 0) {
		*val_ptr.ABORTaddr = 1;
	} else if (strcmp(val, "g") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			shutter_up_up();
		}
	} else if (strcmp(val, "b") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			shutter_down_down();
		}
	} else if (strcmp(val, "h") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			shutter_up();
		}
	} else if (strcmp(val, "n") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			shutter_down();
		}
	} else if (strcmp(val, "i") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			iso_cycle();
		}
	//~ } else if (strcmp(val, "p") == 0) {
		//~ if (*val_ptr.RUN_MODEaddr == 0) {
			
		//~ }
	} else if (strcmp(val, "Return") == 0) {
		if (*val_ptr.RUN_MODEaddr == 0) {
			first_record_killer(NULL);
		}
	} else {
		std::cout << "keyval: \"" << val << "\" not used here\n" << std::endl;
	}
	
	// Always return the keyboard focus back to our fake button
	gtk_widget_grab_focus(gtk_class::fakebutton);
	
	return 0;
}

gboolean key_event_running(GtkWidget *widget, GdkEventKey *event) {
	//g_printerr("%s\n", gdk_keyval_name (event->keyval));
	
	
	gchar* val = gdk_keyval_name (event->keyval);

	if (strcmp(val, "q") == 0) {
		sem_wait(&LOCK);
		*val_ptr.ABORTaddr = 1;
		sem_post(&LOCK);
	} else {
		std::cout << "keyval: \"" << val << "\" not used here\n" << std::endl;
	}
	
	// Always return the keyboard focus back to our fake button
	gtk_widget_grab_focus(gtk_class::fakebutton);
	
	return 0;
}

gboolean abort_check(GtkWidget* data) {
	if (*val_ptr.ABORTaddr == 1) {
		if (*val_ptr.LOST_COUNTERaddr > LOST_THRESH) {
			std::cout << "lost moon, shutting down" << std::endl;
		} else {
			std::cout << "recieved shutdown command from user" << std::endl;
		}
		//~ gtk_widget_destroy(data);
		gtk_window_close(GTK_WINDOW(gtk_class::window));
		g_application_quit(G_APPLICATION(gtk_class::app));
		cleanup();
	}
	
    return TRUE;
}

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
	g_timeout_add(50, G_SOURCE_FUNC(g_framecheck), NULL);
	
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

gboolean cb_subsequent(GtkWidget* data) {
	if (*val_ptr.SUBSaddr == 2) {
		std::cout << "cb sub 2 " << std::endl;
		reset_record();
		sem_wait(&LOCK);
		*val_ptr.SUBSaddr = 0;
		sem_post(&LOCK);
	//~ } else {
		//~ std::cout << "No subs" << std::endl;
	}
	return TRUE;
}
