#ifndef GTK_LUNAERO_H
#define GTK_LUNAERO_H


// Standard C++ includes
#include <string>
#include <iostream>

// Module specific includes
#include <gtk/gtk.h>       // provides GTK3

// User Includes
#include "LunAero.hpp"

// Global GTK widgets
static GtkWidget *fakebutton;
static GtkWidget *text_status;
static GtkCssProvider *provider;

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

class gtk_class {
	public:
		// Main Application
		inline static GtkApplication *app;
		// Main Window
		inline static GtkWidget *window;
		// Grids
		inline static GtkWidget *grid;
		inline static GtkWidget *dirgrid;
		inline static GtkWidget *funcgrid;
		inline static GtkWidget *fakegrid;
		// Button Containers
		inline static GtkWidget *button_box_quit;
		inline static GtkWidget *button_box_fakes;
		inline static GtkWidget *button_box_dirgrid;
		inline static GtkWidget *button_box_redeploy;
		inline static GtkWidget *button_box_funcs;
		inline static GtkWidget *button_box_activate;
		// Buttons
		inline static GtkWidget *exit_button;
		inline static GtkWidget *button_up;
		inline static GtkWidget *button_down;
		inline static GtkWidget *button_left;
		inline static GtkWidget *button_right;
		inline static GtkWidget *button_stop;
		inline static GtkWidget *button_camera_command;
		inline static GtkWidget *button_shutter_up;
		inline static GtkWidget *button_shutter_down;
		inline static GtkWidget *button_shutter_up_up;
		inline static GtkWidget *button_shutter_down_down;
		inline static GtkWidget *button_iso;
		inline static GtkWidget *button_record;
		// Text Holders
		inline static GtkWidget *text_status;
		inline static GtkWidget *text_shutter;
		inline static GtkWidget *text_description;
		// More fake buttons to shield the real fake button!
		inline static GtkWidget *fakebutton;
		inline static GtkWidget *fakebutton2;
		inline static GtkWidget *fakebutton3;
		inline static GtkWidget *fakebutton4;
		inline static GtkWidget *fakebutton5;
    
		inline static const std::string css_string = get_css_string();
		inline static gulong key_id;
	

};

#endif
