/*
 * C_LunAero moontracking program
 * 
 * 
 * Requirements and Dependencies:
 *   - Raspberry Pi v4
 *   - wiringPi v2.52+ 
 *     While waiting for official release, use:
 *       cd /tmp
 *       wget https://project-downloads.drogon.net/wiringpi-latest.deb
 *       sudo dpkg -i wiringpi-latest.deb
 *   - bcm_host.h 
 *     (default with Raspberry Pi)
 *   - GTK3.0
 *     To compile, you need the dev files "libgtk-3-dev"
 *     The libraries to run should be already on the Raspberry Pi
 *   - pthread
 *     (default with Raspberry Pi)
 * 
 * 
 * Compilation instructions
 *   Open terminal and run the commands:
 *     cd /path/to/C_LunAero
 * 	   make
 * 
 * Run instructions
 *   You must have a drive installed at /media/pi/MOON1
 *   Then run
 *     ./LunAero_Moontracker
 */ 

#include <stdio.h>         // provides standard input output
#include <stdlib.h>        // provides exit
#include <assert.h>        // provides assert
#include <stdbool.h>       // provides TRUE/FALSE
#include <sys/mman.h>      // provides mmap
#include <sys/stat.h>      // provides mkdir
#include <string.h>        // provides strncat
#include <time.h>          // provides time
#include <unistd.h>        // provides usleep
#include <wiringPi.h>      // provides GPIO things
#include <softPwm.h>       // provides PWM for GPIO
#include <gtk/gtk.h>       // provides GTK3

#include "bcm_host.h"

// Global Defined Constants
#define DELAY 0.01        // Number of seconds to delay between sampling frames
#define MOVETHRESH 10     // % of frame height to warrant a move
#define DUTY 100          // Duty cycle % for motors
#define FREQ 10000        // Frequency in Hz to run PWM
#define APINP 0           // GPIO BCM pin definition 17
#define APIN1 2          // GPIO BCM pin definition 27
#define APIN2 3          // GPIO BCM pin definition 22
#define BPIN1 12          // GPIO BCM pin definition 10
#define BPIN2 13          // GPIO BCM pin definition 9
#define BPINP 14          // GPIO BCM pin definition 11

// Global variables
int WORK_HEIGHT = 0;
int WORK_WIDTH = 0;
int RASPI_PID = 0;
char FILEPATH[256];
char DEFAULT_FILEPATH[] = "/media/pi/MOON1/";
int OLD_DIR = 0;
int DUTY_A = 100;
int DUTY_B = 100;
char TSBUFF[8];

// Struct of values used when writing labels
struct val_addresses {
	int ** ISO_VALaddr;
	int ** SHUTTER_VALaddr;
	int ** RUN_MODEaddr;
	int ** ABORTaddr;
	int ** REFRESH_CAMaddr;
} val_ptr;

// Global GTK widgets
static GtkWidget *fakebutton;
static GtkWidget *text_status;
static GtkCssProvider *provider;

// Declare Functions
int main (int argc, char **argv);
static void screen_size(int argc, char *argv[]);
static void cleanup();
static void kill_raspivid();
static void activate (GtkApplication *app, gpointer user_data);
static void gpio_pin_setup();
static void mot_up();
static void mot_down();
static void mot_left();
static void mot_right();
static void mot_stop(int direct);
static gboolean key_event(GtkWidget *widget, GdkEventKey *event);
static void loose_wheel(int wheel_dir);
static void current_frame();
static int frame_centroid(int lost_counter);
static void camera_start();
static void speed_up(int motor);
static void shutter_up();
static void shutter_down();
static void shutter_up_up();
static void shutter_down_down();
static void iso_cycle();
static void first_record();
static void refresh_camera();
static void abort_code();
static gboolean abort_check(gpointer data);


static gboolean refresh_text_boxes(gpointer data) {
	char msg[256]={0};
	g_snprintf(msg, sizeof msg, "Shutter Speed: %d\nISO: %d", 
		*(int *)*(int *)val_ptr.SHUTTER_VALaddr, 
		*(int *)*(int *)val_ptr.ISO_VALaddr);
	gtk_label_set_text(GTK_LABEL(text_status), msg);
	
    return TRUE;
}

static void mot_stop(int direct) {
	printf("stopping\n");
	// Case Both motors are moving
	if (direct == 0) {
		while ((DUTY_A > 0) | (DUTY_B > 0)) {
			if (DUTY_A > 10) {
				DUTY_A = 10;
			} else {
				DUTY_A = DUTY_A - 1;
			}
			if (DUTY_B > 10) {
				DUTY_B = 10;
			} else {
				DUTY_B = DUTY_B - 1;
			}
			softPwmWrite(APINP, DUTY_A);
			softPwmWrite(BPINP, DUTY_B);
			usleep(5);
		}
		digitalWrite(APIN1, LOW);
		digitalWrite(APIN2, LOW);
		digitalWrite(BPIN1, LOW);
		digitalWrite(BPIN2, LOW);
	}
	// Case motor A is moving
	if (direct == 1) {
		while (DUTY_A > 0) {
			DUTY_A = DUTY_A - 1;
			softPwmWrite(APINP, DUTY_A);
			usleep(10);
		}
		digitalWrite(APIN1, LOW);
		digitalWrite(APIN2, LOW);
	}
	// Case motor B is moving
	if (direct == 2) {
		while (DUTY_B > 0) {
			DUTY_B = DUTY_B - 1;
			softPwmWrite(BPINP, DUTY_B);
			usleep(10);
		}
		digitalWrite(BPIN1, LOW);
		digitalWrite(BPIN2, LOW);
	}
}

static void mot_up () {
	printf("moving up\n");
	digitalWrite(APIN1, LOW);
	digitalWrite(APIN2, HIGH);
	if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
		DUTY_A = DUTY;
	} else {
		speed_up(1);
	}
	softPwmWrite(APINP, DUTY_A);
}

static void mot_down () {
	printf("moving down\n");
	digitalWrite(APIN1, HIGH);
	digitalWrite(APIN2, LOW);
	if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
		DUTY_A = DUTY;
	} else {
		speed_up(1);
	}
	softPwmWrite(APINP, DUTY_A);
}

static void mot_left() {
	printf("moving left\n");
	digitalWrite(BPIN1, LOW);
	digitalWrite(BPIN2, HIGH);
	if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
		DUTY_B = DUTY;
	} else {
		speed_up(2);
	}
	softPwmWrite(BPINP, DUTY_B);
	if (OLD_DIR == 2) {
		loose_wheel(1);
	}
	OLD_DIR = 1;
}

static void mot_right() {
	printf("moving right\n");
	digitalWrite(BPIN1, HIGH);
	digitalWrite(BPIN2, LOW);
	if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
		DUTY_B = DUTY;
	} else {
		speed_up(2);
	}
	softPwmWrite(BPINP, DUTY_B);
	if (OLD_DIR == 1) {
		loose_wheel(2);
	}
	OLD_DIR = 2;
}

static void loose_wheel(int wheel_dir) {
	/* When changing the left right motion direction, we need to
	 * compensate for some wiggle in the gear.  This is a hardware
	 * correction.
	 */
	if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 1) {
		int old_duty;
		
		old_duty = DUTY_B;
		DUTY_B = DUTY;
		if (wheel_dir == 1) {
			digitalWrite(BPIN1, LOW);
			digitalWrite(BPIN2, HIGH);
		} else {
			digitalWrite(BPIN1, HIGH);
			digitalWrite(BPIN2, LOW);
		}
		softPwmWrite(BPINP, DUTY_B);
		usleep(2000000);
		DUTY_B = old_duty;
	}
}

static void speed_up(int motor) {
	/* Increase the duty cycle of the motor called by this function.
	 * The duty cycle will never go below 20%
	 * 
	 * @param motor The motor we are modifying (1=vert, 2=horz)
	 */
	
	
	if (motor == 1) {
		if (DUTY_A < 20) {
			DUTY_A = 20;
		} else if (DUTY_A < 100) {
			DUTY_A = DUTY_A + 5;
		}
	} else if (motor == 2) {
		if (DUTY_B < 20) {
			DUTY_B = 20;
		} else if (DUTY_B < 100) {
			DUTY_B = DUTY_B + 5;
		}
	}
}

static void gpio_pin_setup () {
	int i;
	int pin_array[] = { APINP, APIN1, APIN2, BPIN1, BPIN2, BPINP };
	
	// Required init
	wiringPiSetup();
	// Set the output pins
	for( i = 0; i < 6; i = i + 1 ) {
		pinMode(pin_array[i], OUTPUT);
		// PWM pins go PWM, all else go HIGH
		if ((i == 0) | (i == 5)) {
			digitalWrite(pin_array[i], LOW);
			printf("Set pin %d LOW\n", pin_array[i]);
		} else {
			digitalWrite(pin_array[i], HIGH);
			printf("Set pin %d HIGH\n", pin_array[i]);
		}
	}
	// create soft PWM
	softPwmCreate(APINP, DUTY_A, DUTY);
	softPwmCreate(BPINP, DUTY_B, DUTY);
}

static void screen_size (int argc, char *argv[]) {
	gtk_init (&argc, &argv);
	GdkRectangle workarea = {0};
	gdk_monitor_get_workarea(gdk_display_get_primary_monitor(gdk_display_get_default()), &workarea);
	WORK_HEIGHT = workarea.height;
	WORK_WIDTH = workarea.width;
	printf ("W: %d x H:%d\n", WORK_WIDTH, WORK_HEIGHT);
}

static void cleanup () {
	// Placeholder in case we need to clean anything up on exit.
	printf("killing run\n");
	kill_raspivid();
	usleep(1000000);
}

static void kill_raspivid () {
	mot_stop(0);
	
	// HACK - This is a filthy hack to get kill the raspivid instance.
	// Since fork interactions are hard, we will use this for now.
	char line[1024];
	FILE *cmd = popen("pidof raspivid", "r");
	fgets(line, 1024, cmd);
	pid_t pid = strtoul(line, NULL, 10);
	pclose(cmd);
	if (pid) {
		printf("attempting to kill pid:%d\n", pid);
		system("killall -s INT raspivid");
	} else {
		// This else is left in for debug
		//printf("WARNING: no proc to kill with pid:%d\n", pid);
	}
}

static void camera_start () {
	printf("\n\nNOW RECORDING\n\nPATH: %s\n", FILEPATH);
	// Call preview of camera
	char commandstring[1024];
	// Get the current unix timestamp as a string
	sprintf(TSBUFF, "%lu", (unsigned long)time(NULL));
	snprintf(commandstring, 1024, 
		"raspivid -v -t 0 -w 1920 -h 1080 -fps 30 -b 8000000 -ISO %d -ss % d -p %d,%d,%d,%d -o %s%soutA.h264 &", 
		*(int *)*(int *)val_ptr.SHUTTER_VALaddr, *(int *)*(int *)val_ptr.ISO_VALaddr,
		(WORK_WIDTH/2)-(WORK_WIDTH/4), WORK_HEIGHT/2, 
		WORK_WIDTH/2, WORK_HEIGHT/2, FILEPATH, TSBUFF);
	printf("Using the command: %s\n", commandstring);
	system(commandstring);
}

static void camera_preview() {
	char commandstring[256];
	// Get the current unix timestamp as a string
	sprintf(TSBUFF, "%lu", (unsigned long)time(NULL));
	snprintf(commandstring, 256, 
		"raspivid -v -t 0 -w 1920 -h 1080 -fps 30 -b 8000000 -ISO %d -ss %d -p %d,%d,%d,%d &",
		*(int *)*(int *)val_ptr.SHUTTER_VALaddr, *(int *)*(int *)val_ptr.ISO_VALaddr,
		(WORK_WIDTH/2)-(WORK_WIDTH/4), WORK_HEIGHT/2, 
		WORK_WIDTH/2, WORK_HEIGHT/2);
	printf("Using the command: %s\n", commandstring);
	system(commandstring);
}

static void first_record() {
	*(int *)*(int *)val_ptr.RUN_MODEaddr = 1;
	kill_raspivid();
	camera_start();
}

static void reset_record() {
	kill_raspivid();
	usleep(1000000);
	camera_start();
}

static void refresh_camera() {
	kill_raspivid();
	*(int *)*(int *)val_ptr.REFRESH_CAMaddr = 1;
}

static void activate (GtkApplication *app, gpointer local_val_ptr) {
	GtkWidget *window;
	// Grids
	GtkWidget *grid;
	GtkWidget *dirgrid;
	GtkWidget *funcgrid;
	GtkWidget *fakegrid;
	// Button Containers
	GtkWidget *button_box_quit;
	GtkWidget *button_box_fakes;
	GtkWidget *button_box_dirgrid;
	GtkWidget *button_box_redeploy;
	GtkWidget *button_box_funcs;
	GtkWidget *button_box_activate;
	// Buttons
	GtkWidget *exit_button;
	GtkWidget *button_up;
	GtkWidget *button_down;
	GtkWidget *button_left;
	GtkWidget *button_right;
	GtkWidget *button_stop;
	GtkWidget *button_camera_command;
	GtkWidget *button_shutter_up;
	GtkWidget *button_shutter_down;
	GtkWidget *button_shutter_up_up;
	GtkWidget *button_shutter_down_down;
	GtkWidget *button_iso;
	GtkWidget *button_record;
	// Text Holders
	GtkWidget *text_shutter;
	GtkWidget *text_description;
	// More fake buttons to shield the real fake button!
	GtkWidget *fakebutton2;
	GtkWidget *fakebutton3;
	GtkWidget *fakebutton4;
	GtkWidget *fakebutton5;
	
	// Define Window, dynamic size for screen.
	window = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (window), "LunAero Moontracker");
	gtk_window_fullscreen (GTK_WINDOW (window));
	//gtk_window_maximize (GTK_WINDOW (window));
	
	// Create table and put it in the window.
	grid = gtk_grid_new ();
	gtk_container_add (GTK_CONTAINER (window), grid);
	
	// Define Button Boxes.
	button_box_quit = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	button_box_fakes = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	button_box_dirgrid = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	button_box_redeploy = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	button_box_funcs = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	button_box_activate = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	
	// Define Exit Button, put it in a box.
	exit_button = gtk_button_new_with_label ("Exit");
	gtk_container_add(GTK_CONTAINER (button_box_quit), exit_button);
	
	// Define the Fake button, put it in a box
	// Shield this with other fake buttons so keypresses can't move it much
	// This doubles as a placeholder for the raspicam window
	fakebutton = gtk_button_new_with_label(" ");
	fakebutton2 = gtk_button_new_with_label(" ");
	fakebutton3 = gtk_button_new_with_label(" ");
	fakebutton4 = gtk_button_new_with_label(" ");
	fakebutton5 = gtk_button_new_with_label(" ");
	fakegrid = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (fakegrid), 10);
	gtk_grid_attach (GTK_GRID (fakegrid), fakebutton, 0, 0, 1, 1);
	gtk_grid_attach_next_to (GTK_GRID (fakegrid), fakebutton2, fakebutton, GTK_POS_TOP, 1, 1);
	gtk_grid_attach_next_to (GTK_GRID (fakegrid), fakebutton3, fakebutton, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to (GTK_GRID (fakegrid), fakebutton4, fakebutton, GTK_POS_LEFT, 1, 1);
	gtk_grid_attach_next_to (GTK_GRID (fakegrid), fakebutton5, fakebutton, GTK_POS_RIGHT, 1, 1);
	gtk_container_add(GTK_CONTAINER (button_box_fakes), fakegrid);
	
	// Define directional buttons...
	button_up = gtk_button_new_with_label("^");
	button_down = gtk_button_new_with_label("v");
	button_left = gtk_button_new_with_label("<");
	button_right = gtk_button_new_with_label(">");
	button_stop = gtk_button_new_with_label("stop");
	// Put these buttons in the special grid...
	dirgrid = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (dirgrid), 10);
	gtk_grid_attach (GTK_GRID (dirgrid), button_stop, 0, 0, 1, 1);
	gtk_grid_attach_next_to (GTK_GRID (dirgrid), button_up, button_stop, GTK_POS_TOP, 1, 1);
	gtk_grid_attach_next_to (GTK_GRID (dirgrid), button_down, button_stop, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to (GTK_GRID (dirgrid), button_left, button_stop, GTK_POS_LEFT, 1, 1);
	gtk_grid_attach_next_to (GTK_GRID (dirgrid), button_right, button_stop, GTK_POS_RIGHT, 1, 1);
	// ...Put it in a box
	gtk_container_add (GTK_CONTAINER (button_box_dirgrid), dirgrid);
	
	// Define redeploy button and put in a box
	button_camera_command = gtk_button_new_with_label("Reissue\nCamera\nCommand");
	gtk_container_add(GTK_CONTAINER (button_box_redeploy), button_camera_command);
	
	// Define camera function buttons, plus text, and put in a box
	button_shutter_up = gtk_button_new_with_label("+");
	button_shutter_down = gtk_button_new_with_label("-");
	button_shutter_up_up = gtk_button_new_with_label("++");
	button_shutter_down_down = gtk_button_new_with_label("--");
	button_iso = gtk_button_new_with_label("ISO");
	text_shutter = gtk_label_new("Shutter Speed");
	// Use a grid for all the buttons and text
	funcgrid = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (funcgrid), 10);
	gtk_grid_attach (GTK_GRID (funcgrid), button_shutter_up, 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (funcgrid), button_shutter_up_up, 1, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (funcgrid), text_shutter, 0, 1, 2, 1);
	gtk_grid_attach (GTK_GRID (funcgrid), button_shutter_down, 0, 2, 1, 1);
	gtk_grid_attach (GTK_GRID (funcgrid), button_shutter_down_down, 1, 2, 1, 1);
	gtk_grid_attach (GTK_GRID (funcgrid), button_iso, 2, 0, 1, 2);
	gtk_widget_set_valign(button_iso, GTK_ALIGN_FILL);
	// ...Put it in a box
	gtk_container_add(GTK_CONTAINER (button_box_funcs), funcgrid);
	
	// Define record button and put in a box
	button_record = gtk_button_new_with_label("▶️");
	gtk_container_add(GTK_CONTAINER (button_box_activate), button_record);
	
	// Connect signals to buttons
	g_signal_connect_swapped (exit_button, "clicked", G_CALLBACK (abort_code), NULL);
	g_signal_connect_swapped (button_stop, "clicked", G_CALLBACK (mot_stop), 0);
	g_signal_connect_swapped (button_up, "clicked", G_CALLBACK (mot_up), NULL);
	g_signal_connect_swapped (button_down, "clicked", G_CALLBACK (mot_down), NULL);
	g_signal_connect_swapped (button_left, "clicked", G_CALLBACK (mot_left), NULL);
	g_signal_connect_swapped (button_right, "clicked", G_CALLBACK (mot_right), NULL);
	g_signal_connect_swapped (button_shutter_up, "clicked", G_CALLBACK (shutter_up), NULL);
	g_signal_connect_swapped (button_shutter_down, "clicked", G_CALLBACK (shutter_down), NULL);
	g_signal_connect_swapped (button_shutter_up_up, "clicked", G_CALLBACK (shutter_up_up), NULL);
	g_signal_connect_swapped (button_shutter_down_down, "clicked", G_CALLBACK (shutter_down_down), NULL);
	g_signal_connect_swapped (button_iso, "clicked", G_CALLBACK (iso_cycle), NULL);
	g_signal_connect_swapped (button_camera_command, "clicked", G_CALLBACK (refresh_camera), NULL);
	g_signal_connect_swapped (button_record, "clicked", G_CALLBACK (first_record), NULL);
	
	// Define text description
	// This is an inelegant special padding element which adjusts based on screen size
	text_description = gtk_label_new("\n\n\n");
	
	// Define text status
	text_status = gtk_label_new(NULL);
	
	// Capture Key Events
	g_signal_connect(window, "key-release-event", G_CALLBACK(key_event), NULL);
	
	
	
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
	gtk_grid_attach (GTK_GRID (grid), button_box_quit, 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), button_box_redeploy, 1, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), button_box_dirgrid, 2, 0, 2, 2);
	gtk_grid_attach (GTK_GRID (grid), button_box_funcs, 0, 1, 2, 1);
	gtk_grid_attach (GTK_GRID (grid), text_status, 0, 2, 1, 2);
	gtk_grid_attach (GTK_GRID (grid), button_box_fakes, 1, 2, 2, 2);
	gtk_grid_attach (GTK_GRID (grid), button_box_activate, 3, 2, 1, 2);
	gtk_grid_attach (GTK_GRID (grid), text_description, 0, 4, 4, 1);

	// Expand all of the widget boxes on the display
	gtk_widget_set_vexpand (button_box_quit, TRUE);
	gtk_widget_set_hexpand (button_box_quit, TRUE);
	gtk_widget_set_vexpand (button_box_redeploy, TRUE);
	gtk_widget_set_hexpand (button_box_redeploy, TRUE);
	gtk_widget_set_vexpand (button_box_dirgrid, TRUE);
	gtk_widget_set_hexpand (button_box_dirgrid, TRUE);
	gtk_widget_set_vexpand (button_box_funcs, TRUE);
	gtk_widget_set_hexpand (button_box_funcs, TRUE);
	gtk_widget_set_vexpand (text_status, TRUE);
	gtk_widget_set_hexpand (text_status, TRUE);
	gtk_widget_set_vexpand (button_box_fakes, TRUE);
	gtk_widget_set_hexpand (button_box_fakes, TRUE);
	gtk_widget_set_vexpand (button_box_activate, TRUE);
	gtk_widget_set_hexpand (button_box_activate, TRUE);
	// Alignment of boxes
	gtk_widget_set_halign(button_box_quit, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(button_box_quit, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(button_box_redeploy, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(button_box_redeploy, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(button_box_dirgrid, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(button_box_dirgrid, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(button_box_funcs, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(button_box_funcs, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(text_status, GTK_ALIGN_START);
	gtk_widget_set_valign(text_status, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(button_box_fakes, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(button_box_fakes, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(button_box_activate, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(button_box_activate, GTK_ALIGN_CENTER);

	// CSS stylesheet without using CSS
	char css_string[1024]={0};
	printf("width: %d\n", WORK_WIDTH);
	printf("height: %d\n", WORK_HEIGHT);
	printf("font-size: %d\n", ((WORK_WIDTH*20)/WORK_HEIGHT));
	g_snprintf(css_string, sizeof css_string,
		"window { background-color: black; \
		 color: red; font-size: %dpx; \
		 font-weight: bolder; } \
		 button { background-image: image(dimgray); \
		 border-color: dimgray; \
		 text-shadow: 0 1px black; } \
		 .fakebutton {background-image: image(black);  \
		 border-width: 0px; \
		 border-color: black; }",
		((WORK_WIDTH*20)/WORK_HEIGHT));
		//
	provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data (provider, css_string, -1, NULL);
	// Apply CSS style to elements
	gtk_style_context_add_provider(gtk_widget_get_style_context(window), 
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(exit_button), 
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_up),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_down),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_left),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_right),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_stop),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_camera_command),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_shutter_up),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_shutter_down),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_shutter_up_up),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_shutter_down_down),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_iso),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(button_record),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	// Add fakebuttons to special button class
	gtk_style_context_add_class (gtk_widget_get_style_context (fakebutton), "fakebutton");
	gtk_style_context_add_class (gtk_widget_get_style_context (fakebutton2), "fakebutton");
	gtk_style_context_add_class (gtk_widget_get_style_context (fakebutton3), "fakebutton");
	gtk_style_context_add_class (gtk_widget_get_style_context (fakebutton4), "fakebutton");
	gtk_style_context_add_class (gtk_widget_get_style_context (fakebutton5), "fakebutton");
	gtk_style_context_add_provider(gtk_widget_get_style_context(fakebutton),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(fakebutton2),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(fakebutton3),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(fakebutton4),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_add_provider(gtk_widget_get_style_context(fakebutton5),
		GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	
	// Create timeout to refresh
	g_timeout_add(500, G_SOURCE_FUNC(refresh_text_boxes), NULL);
	g_timeout_add(50, G_SOURCE_FUNC(abort_check), window);
	
	//Activate!
	refresh_text_boxes(NULL);
	gtk_widget_grab_focus(fakebutton);
	gtk_widget_show_all (window);
}

static void abort_code() {
	*(int *)*(int *)val_ptr.ABORTaddr = 1;
}

static gboolean abort_check(gpointer data) {
	if (*(int *)*(int *)val_ptr.ABORTaddr == 1) {
		gtk_widget_destroy(data);
	}
	
    return TRUE;
}

static void shutter_up() {
	if (*(int *)*(int *)val_ptr.SHUTTER_VALaddr < 32901) {
		*(int *)*(int *)val_ptr.SHUTTER_VALaddr = *(int *)*(int *)val_ptr.SHUTTER_VALaddr + 100;
	} else {
		*(int *)*(int *)val_ptr.SHUTTER_VALaddr = 33000;
	}
	printf("SHUTTER_VAL: %d\n", *(int *)*(int *)val_ptr.SHUTTER_VALaddr);
}

static void shutter_down() {
	if (*(int *)*(int *)val_ptr.SHUTTER_VALaddr > 110) {
		*(int *)*(int *)val_ptr.SHUTTER_VALaddr = *(int *)*(int *)val_ptr.SHUTTER_VALaddr - 100;
	} else {
		*(int *)*(int *)val_ptr.SHUTTER_VALaddr = 10;
	}
	printf("SHUTTER_VAL: %d\n", *(int *)*(int *)val_ptr.SHUTTER_VALaddr);
}

static void shutter_up_up() {
	if (*(int *)*(int *)val_ptr.SHUTTER_VALaddr < 32001) {
		*(int *)*(int *)val_ptr.SHUTTER_VALaddr = *(int *)*(int *)val_ptr.SHUTTER_VALaddr + 1000;
	} else {
		*(int *)*(int *)val_ptr.SHUTTER_VALaddr = 33000;
	}
	printf("SHUTTER_VAL: %d\n", *(int *)*(int *)val_ptr.SHUTTER_VALaddr);
}

static void shutter_down_down() {
	if (*(int *)*(int *)val_ptr.SHUTTER_VALaddr > 1010) {
		*(int *)*(int *)val_ptr.SHUTTER_VALaddr = *(int *)*(int *)val_ptr.SHUTTER_VALaddr - 1000;
	} else {
		*(int *)*(int *)val_ptr.SHUTTER_VALaddr = 10;
	}
	printf("SHUTTER_VAL: %d\n", *(int *)*(int *)val_ptr.SHUTTER_VALaddr);
}

static void iso_cycle() {
	if (*(int *)*(int *)val_ptr.ISO_VALaddr == 200) {
		*(int *)*(int *)val_ptr.ISO_VALaddr = 400;
	} else if (*(int *)*(int *)val_ptr.ISO_VALaddr == 400) {
		*(int *)*(int *)val_ptr.ISO_VALaddr = 800;
	} else if (*(int *)*(int *)val_ptr.ISO_VALaddr == 800) {
		*(int *)*(int *)val_ptr.ISO_VALaddr = 100;
	} else if (*(int *)*(int *)val_ptr.ISO_VALaddr == 100) {
		*(int *)*(int *)val_ptr.ISO_VALaddr = 200;
	}
	printf("ISO_VAL: %d\n", *(int *)*(int *)val_ptr.ISO_VALaddr);
}

static void current_frame() {
	
	DISPMANX_DISPLAY_HANDLE_T   display;
	DISPMANX_MODEINFO_T         info;
	DISPMANX_RESOURCE_HANDLE_T  resource;
	VC_IMAGE_TYPE_T type = VC_IMAGE_RGB888;
	VC_IMAGE_TRANSFORM_T	transform = 0;
	VC_RECT_T			rect;
	
	void *image;
	uint32_t vc_image_ptr;
	int ret;
	uint32_t screen = 0;

	bcm_host_init();

	// Get display info for the screen we are using.
	display = vc_dispmanx_display_open( screen );
	ret = vc_dispmanx_display_get_info(display, &info);
	assert(ret == 0);

	// This holds an image
	image = calloc( 1, info.width * 3 * info.height );
	assert(image);

	// Create space based on the screen info
	resource = vc_dispmanx_resource_create( type, info.width, info.height, &vc_image_ptr );
	
	// Take a snapshot of the screen (stored in resource)
	vc_dispmanx_snapshot(display, resource, transform);

	// Read the rectangular data from resource into the image calloc
	vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);
	vc_dispmanx_resource_read_data(resource, &rect, image, info.width*3);

	// Store the image in a .ppm format file on the hard drive
	// TODO - assert that the drive is plugged in
	// TODO - Make this an mmap stored image.
	FILE *fp = fopen("/media/pi/MOON1/out.ppm", "wb");
	// This is the requisite .ppm header
	fprintf(fp, "P6\n%d %d\n255\n", info.width, info.height);
	fwrite(image, info.width*3*info.height, 1, fp);
	fclose(fp);
	
	// Cleanup the VC resources
	ret = vc_dispmanx_resource_delete(resource);
	assert(ret == 0);
	ret = vc_dispmanx_display_close(display);
	assert(ret == 0);
	
	// This command crops the full screenshot and converts to a 1bit BW at 50% thresh.
	char commandstring[256];
	snprintf(commandstring, 256, 
		"convert /media/pi/MOON1/out.ppm -crop %dx%d+%d+%d -threshold 50%% -depth 1 /media/pi/MOON1/out.ppm", 
		info.width/2, info.height/2, (info.width/2)-(info.width/4), info.height/2);
	system(commandstring);
}

static int frame_centroid (int lost_counter) {

	// TODO - Change to mmap when possible
	FILE *fp = fopen("/media/pi/MOON1/out.ppm", "rb");
	
	//Check for bright or dark spots?
	// Bright spots == 1
	// dark spots == 0
	int checkval = 1;
	
	
	// Number of points to be "on edge" is 10% of edge
	int w_thresh = WORK_WIDTH/20;
	int h_thresh = WORK_HEIGHT/20;
	
	unsigned char linebreak = 0x0a;
	int matrix[WORK_WIDTH/2][WORK_HEIGHT/2];
	unsigned char c;
	int i = 0;
	int j;
	int k;
	
	rewind(fp);
	c = fgetc(fp);
	
	// Ignore the header
	while (i < 3) {
		c = fgetc(fp);
		if (c == linebreak) {
			i++;
		}
	}
	
	// Put the pixels in the matrix
	for (j=0; j<WORK_WIDTH/2; j++) {
		for (k=0; k<WORK_HEIGHT/2; k++) {
			matrix[j][k] = fgetc(fp);
		}
	}
	
	// Store sum of each edge
	int top_edge = 0;
	int bottom_edge = 0;
	int left_edge = 0;
	int right_edge = 0;
	
	for (j=0; j<WORK_WIDTH/2; j++) {
		if (matrix[j][0] == checkval) {
			top_edge++;
		}
		if (matrix[j][WORK_HEIGHT/2] == checkval) {
			bottom_edge++;
		}
	}
	for (k=0; k<WORK_HEIGHT/2; k++) {
		if (matrix[0][k] == checkval) {
			left_edge++;
		}
		if (matrix[WORK_HEIGHT/2][k] == checkval) {
			right_edge++;
		}
	}
	
	
	// Calculate the centroid using sum.
	int sumx = 0;
	int sumy = 0;
	int cnt = 0;
	for (j=0; j<WORK_WIDTH/2; j++) {
		for (k=0; k<WORK_HEIGHT/2; k++) {
			
			if (matrix[j][k] == checkval) {
				sumx = sumx + j;
				sumy = sumy + k;
				cnt++;
			}
		}
	}
	
	// If nothing is found, return an increment to the moon loss counter
	if ((sumx == 0) & (sumy == 0)) {
		lost_counter = lost_counter + 1;
	} else {
		// something was found, reset moon loss counter
		lost_counter = 0;
		
		// Check if bright spot is near the edge of the frame
		// If so, move away from that edge
		// If not or near both edges, use centroid
		if ((top_edge >= w_thresh) & (bottom_edge < w_thresh)) {
			mot_down();
		} else if ((bottom_edge >= w_thresh) & (top_edge < w_thresh)) {
			mot_up();
		} else {
			if (abs((sumy/cnt)-(WORK_HEIGHT/4)) > ((WORK_HEIGHT/4)*0.05)) {
				if (((sumy/cnt)-(WORK_HEIGHT/4)) > 0) {
					mot_down();
				} else {
					mot_up();
				}
			} else {
				mot_stop(1);
			}
		}
		
		if ((left_edge >= h_thresh) & (right_edge < h_thresh)) {
			mot_right();
		} else if ((left_edge <= h_thresh) & (right_edge > h_thresh)) {
			mot_left();
		} else {
			if (abs((sumx/cnt)-(WORK_WIDTH/4)) > ((WORK_WIDTH/4)*0.2)) {
				if (((sumx/cnt)-(WORK_WIDTH/4)) > 0) {
					mot_right();
				} else {
					mot_left();
				}
			} else {
				mot_stop(2);
			}
		}
	}
	return lost_counter;
}

static gboolean key_event(GtkWidget *widget, GdkEventKey *event) {
	//g_printerr("%s\n", gdk_keyval_name (event->keyval));
	
	
	gchar* val = gdk_keyval_name (event->keyval);
	if (strcmp(val, "Left") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			mot_left();
		}
	} else if (strcmp(val, "Right") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			mot_right();
		}
	} else if (strcmp(val, "Up") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			mot_up();
		}
	} else if (strcmp(val, "Down") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			mot_down();
		}
	} else if (strcmp(val, "space") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			mot_stop(0);
		}
	} else if (strcmp(val, "q") == 0) {
		*(int *)*(int *)val_ptr.ABORTaddr = 1;
	} else if (strcmp(val, "g") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			shutter_up_up();
		}
	} else if (strcmp(val, "b") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			shutter_down_down();
		}
	} else if (strcmp(val, "h") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			shutter_up();
		}
	} else if (strcmp(val, "n") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			shutter_down();
		}
	} else if (strcmp(val, "i") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			iso_cycle();
		}
	} else if (strcmp(val, "p") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			
		}
	} else if (strcmp(val, "Return") == 0) {
		if (*(int *)*(int *)val_ptr.RUN_MODEaddr == 0) {
			reset_record();
		}
	} else {
		printf("keyval: \"%s\" not used here\n", val);
	}
	
	// Always return the keyboard focus back to our fake button
	gtk_widget_grab_focus(fakebutton);
	
	return 0;
}

int main (int argc, char **argv) {
	GtkApplication *app;
	int status = 0;
	int switched_modes = 0;
	
	// Prep the GPIO
	gpio_pin_setup();
	
	// Get the screen size now.  Opens and kills an invisible GTK instance.
	
	// Make folder for stuff
	char slash[] = "/";
	sprintf(TSBUFF, "%lu", (unsigned long)time(NULL));
	snprintf(FILEPATH, sizeof FILEPATH, "%s%s%s", DEFAULT_FILEPATH, TSBUFF, slash);
	//strncat(FILEPATH, TSBUFF, 10);
	//strncat(FILEPATH, slash, 1);
	mkdir(FILEPATH, 0700);
	printf("time: %s\n", TSBUFF);
	printf("path: %s\n", FILEPATH);
		
	// DO NOT PUT BEFORE strncat FUNCTION!!!
	// Doing so non-deterministically rewrites WORK_WIDTH with a new value
	// BAD MAGIC
	screen_size(argc, argv);
	
	// Keep these in order! Fork MUST be called AFTER the mmap ABORT code!!
	
	// Memory values which influence camera commands. Defaults to 0.
	int *ISO_VAL = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	int *SHUTTER_VAL = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	int *RUN_MODE = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	int *REFRESH_CAM = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	
	// Memory value which tells the program to continue running.
	int *ABORT = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	
	// Assign pointers to mmap objects
	val_ptr.ISO_VALaddr = &ISO_VAL;
	val_ptr.SHUTTER_VALaddr = &SHUTTER_VAL;
	val_ptr.RUN_MODEaddr = &RUN_MODE;
	val_ptr.REFRESH_CAMaddr = &REFRESH_CAM;
	val_ptr.ABORTaddr = &ABORT;
	
	// Assign initial values to mmap'd variables
	*ISO_VAL = 200;
	*SHUTTER_VAL = 10000;
	*RUN_MODE = 0;
	*REFRESH_CAM = 0;
	*ABORT = 0;
	
	int pid = fork();
	
	// Run our GTK Application window while using Raspivid to get camera data
	// This runs as a child from the fork (pid=0).
	if (pid == 0) {
		
		int lost_counter = 0;
		
		while (!*ABORT) {
			int counter = 0;
			
			if (*RUN_MODE == 0) {
				camera_preview();
			}
			
			// Wait for the preview and manual adjustment mode to finish.
			while (*RUN_MODE == 0) {
				// If the user requests we refresh the preview from GTK, do it here
				if (*REFRESH_CAM == 1) {
					printf("refreshing camera \n");
					camera_preview();
					*REFRESH_CAM = 0;
				}
				// This counter is required to prevent race condition zombies.
				usleep(500000);
			}
			
			if (switched_modes == 0) {
				// Set duty cycles to low end once automatic mode is started
				// Do this only once.
				DUTY_A = 20;
				DUTY_B = 20;
				switched_moedes = 1;
			}
			
			// This usleep counter is not temporally stable due to the forced mmap check every sec.
			// Empirically, appears to capture 1.66s of video/1000000 usleep microseconds.
			while ((counter < 3600) & (!*ABORT)) {
				usleep(400000); //FIXME Significantly reduce this value when possible
				current_frame();
				lost_counter = frame_centroid(lost_counter);
				if (lost_counter == 30) {
					*ABORT = 1;
				} else {
					//check_move();
					counter++;
				}
			}
			// If the task is still running and the time says we might pass 2gb, restart vid.
			if (!*ABORT) {
				reset_record();
			}
			
			
			//// If everything is commented out in this while, leave this usleep
			//// It prevents a race condition leaving a zombie process on abort
			//usleep(1000000);
		}
		// Exit the fork.
		exit(0);
	} else {
		// PID 0 is the main application, and runs the GTK window
		app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
		g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
		status = g_application_run (G_APPLICATION (app), argc, argv);
		// Cleanup GTK
		g_object_unref (app);
		// Write the abort code to mmap even though it should already be set to 1
		*ABORT = 1;
	}
	// Doubletap/Mozambique.  We have to make sure we don't leave a zombie process.
	*ABORT = 1;
	cleanup();
	
	return status;
}
