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

#include "LunAero.hpp"

void cb_framecheck() {
	printf("getting current frame\n");
	current_frame();
	LOST_COUNTER = frame_centroid(LOST_COUNTER);
	if (LOST_COUNTER == 30) {
		*val_ptr.ABORTaddr = 1;
	} else {
		LOST_COUNTER = 0;
	}
}

void cleanup () {
	// Placeholder in case we need to clean anything up on exit.
	std::cout << "killing run" << std::endl;
	kill_raspivid();
	usleep(1000000);
}

void kill_raspivid () {
	*val_ptr.STOP_DIRaddr == 3;
	
	// HACK - This is a filthy hack to get kill the raspivid instance.
	// Since fork interactions are hard, we will use this for now.
	char line[1024];
	FILE *cmd = popen("pidof raspivid", "r");
	fgets(line, 1024, cmd);
	pid_t pid = strtoul(line, NULL, 10);
	pclose(cmd);
	if (kill(pid, SIGINT) != 0) {
		std::cout << "ERROR: Unable to kill raspivid" << std::endl;
	}
}

void abort_code() {
	*val_ptr.ABORTaddr = 1;
}

void current_frame() {
	auto start = std::chrono::steady_clock::now();
	DISPMANX_DISPLAY_HANDLE_T   display;
	DISPMANX_MODEINFO_T         info;
	DISPMANX_RESOURCE_HANDLE_T  resource;
	VC_IMAGE_TYPE_T             type = VC_IMAGE_RGB888;
	DISPMANX_TRANSFORM_T	    transform = static_cast <DISPMANX_TRANSFORM_T> (0);
	VC_RECT_T			        rect;
	
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
	auto mid = std::chrono::steady_clock::now();
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
	std::string commandstring;
	commandstring = "convert /media/pi/MOON1/out.ppm -crop "
	+ std::to_string(info.width/2)
	+ "x"
	+ std::to_string(info.height/2)
	+ "+"
	+ std::to_string((info.width/2)-(info.width/4))
	+ "+"
	+ std::to_string(info.height/2)
	+ " -threshold 50% -depth 1 /media/pi/MOON1/out.ppm"; 
	system(commandstring.c_str());
	auto end = std::chrono::steady_clock::now();
	std::cout 
	<< "mid-start = "
	<< std::chrono::duration_cast<std::chrono::microseconds>(mid - start).count()
	<< " µs, end-mid = " 
	<< std::chrono::duration_cast<std::chrono::microseconds>(end - mid).count()
	<< " µs"
	<< std::endl;
}

int frame_centroid(int lost_cnt) {
	printf("framecentroid\n");
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
	unsigned char c;
	int i = 0;
	int j;
	int k;
	int width;
	int height;
	std::string charst = "";
	
	// Ignore P6 Header
	rewind(fp);
	c = fgetc(fp); //P
	c = fgetc(fp); //6
	c = fgetc(fp); //OxOa
	
	// Grab the image size from header
	while (i < 2) {
		c = fgetc(fp);
		if (((int)c > 47) && ((int)c < 58)) {
			charst += c;
		} else {
			if (i == 0) {
				width = std::stoi(charst);
			} else if (i == 1) {
				height = std::stoi(charst);
			}
			charst = "";
			i++;
		}
	}
	// Ignore netbpm max value (1)
	c = fgetc(fp); //1
	c = fgetc(fp); //OxOa
	
	// Create Matrix
	int matrix[height][width];
	
	
	// Put the pixels in the matrix
	for (k=0; k<height; k++) {
		for (j=0; j<width; j++) {
			// RGB has depth of 3 digits
			i = 0;
			while (i < 3) {
				c = fgetc(fp);
				if ((c == 0) || (c == 1)) {
					i++;
				}
			}
			matrix[k][j] = c;
		}
	}
	
	// Store sum of each edge
	int top_edge = 0;
	int bottom_edge = 0;
	int left_edge = 0;
	int right_edge = 0;
	
	for (j=0; j<width; j++) {
		if (matrix[0][j] == checkval) {
			top_edge++;
		}
		if (matrix[height-1][j] == checkval) {
			bottom_edge++;
		}
	}
	for (k=0; k<height; k++) {
		if (matrix[k][0] == checkval) {
			left_edge++;
		}
		if (matrix[k][width-1] == checkval) {
			right_edge++;
		}
	}
	
	
	// Calculate the centroid using sum.
	int sumx = 0;
	int sumy = 0;
	int mcnt = 0;
	
	for (k=0; k<height; k++) {
		for (j=0; j<width; j++) {
			if (matrix[k][j] == checkval) {
				sumx = sumx + j;
				sumy = sumy + k;
				mcnt++;
			}
		}
	}
	
	// If nothing is found, return an increment to the moon loss counter
	if ((sumx == 0) && (sumy == 0)) {
		lost_cnt = lost_cnt + 1;
		std::cout << "lost moon for " << lost_cnt <<  " cycles" << std::endl;
	} else {
		// something was found, reset moon loss counter
		lost_cnt = 0;
		std::cout << "Moon found centered at (" << (sumx/mcnt) << ", " << (sumy/mcnt) << ")\n" << std::endl;
		std::cout << "top:bottom::left:right " << top_edge << ":" << bottom_edge << "::" << left_edge 
		<< ":" << right_edge <<std::endl;
		// Report edges only
		if ((top_edge >= w_thresh) && (bottom_edge < w_thresh)) {
			std::cout << "+top edge" << std::endl;
		}
		if ((bottom_edge >= w_thresh) && (top_edge < w_thresh)) {
			std::cout << "+bottom edge" << std::endl;
		}
		if ((left_edge >= h_thresh) && (right_edge < h_thresh)) {
			std::cout << "+left edge" << std::endl;
		}
		if ((left_edge <= h_thresh) && (right_edge > h_thresh)) {
			std::cout << "+right edge" << std::endl;
		}
		
		
		
		
		// Check if bright spot is near the edge of the frame
		// If so, move away from that edge
		// If not or near both edges, use centroid
		if ((top_edge >= w_thresh) && (bottom_edge < w_thresh)) {
			std::cout << "detected light on top edge" << std::endl;
			mot_up_command();
		} else if ((bottom_edge >= w_thresh) && (top_edge < w_thresh)) {
			std::cout << "detected light on bottom edge" << std::endl;
			mot_down_command();
		} else {
			if (abs((sumy/mcnt)-(height/2)) > ((height/2)*0.2)) {
				std::cout << "M_y = " << (sumy/mcnt)-(height/2) << std::endl;
				if (((sumy/mcnt)-(height/2)) > 0) {
					std::cout << "centroid moving to down" << std::endl;
					mot_down_command();
				} else {
					std::cout << "centroid moving to up" << std::endl;
					mot_up_command();
				}
			} else {
				*val_ptr.STOP_DIRaddr = 2;
			}
		}
		
		if ((left_edge >= h_thresh) && (right_edge < h_thresh)) {
			std::cout << "detected light on left edge" << std::endl;
			mot_left_command();
		} else if ((left_edge <= h_thresh) && (right_edge > h_thresh)) {
			std::cout << "detected light on right edge" << std::endl;
			mot_right_command();
		} else {
			if (abs((sumx/mcnt)-(width/2)) > ((width/2)*0.4)) {
				std::cout << "M_x = " << (sumx/mcnt)-(width/2) << std::endl;
				if (((sumx/mcnt)-(width/2)) > 0) {
					std::cout << "centroid moving to right" << std::endl;
					mot_right_command();
				} else {
					std::cout << "centroid moving to left" << std::endl;
					mot_left_command();
				}
			} else {
				// In the case we already need to stop one direction, stop both
				if (*val_ptr.STOP_DIRaddr == 2) {
					*val_ptr.STOP_DIRaddr = 3;
				} else {
					*val_ptr.STOP_DIRaddr = 1;
				}
			}
		}
	}
	return lost_cnt;
}

int main (int argc, char **argv) {
	
	// Screensaver settings for the raspberry pi
	system("xset -dpms");
	system("xset s off");
	
	//~ GtkApplication *app;
	int status = 0;
	
	
	
	// Make folder for stuff
	TSBUFF = std::to_string((unsigned long)time(NULL));
	FILEPATH = DEFAULT_FILEPATH + TSBUFF;
	mkdir(FILEPATH.c_str(), 0700);
	std::cout << "time: " << TSBUFF << std::endl;
	std::cout << "path: " << FILEPATH << std::endl;
	
	// Get the screen size now.  Opens and kills an invisible GTK instance.
	//~ screen_size(argc, argv);
	
	// Keep these in order! Fork MUST be called AFTER the mmap ABORT code!!
	
	//~ if (memory_map_init() != 0) {
		//~ std::cout << "Failed to map values to memory" << std::endl;
		//~ return 1;
	//~ }
	
	
	
	// Memory values which influence camera commands. Defaults to 0.
	val_ptr.ISO_VALaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	val_ptr.SHUTTER_VALaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	val_ptr.RUN_MODEaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	val_ptr.REFRESH_CAMaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	
	// Memory values which influence motor commands.
	val_ptr.HORZ_DIRaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	val_ptr.VERT_DIRaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	val_ptr.STOP_DIRaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	val_ptr.DUTY_Aaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	val_ptr.DUTY_Baddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	
	// Memory value which tells the program to continue running.
	val_ptr.ABORTaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	
	// Assign initial values to mmap'd variables
	*val_ptr.ISO_VALaddr = 200;
	*val_ptr.SHUTTER_VALaddr = 10000;
	*val_ptr.RUN_MODEaddr = 0;
	*val_ptr.REFRESH_CAMaddr = 0;
	*val_ptr.HORZ_DIRaddr = 0;
	*val_ptr.VERT_DIRaddr = 0;
	*val_ptr.STOP_DIRaddr = 0;
	*val_ptr.DUTY_Aaddr = 100;
	*val_ptr.DUTY_Baddr = 100;
	*val_ptr.ABORTaddr = 0;
	
	int pid1 = fork();
	
	if (pid1 > 0) {
		// Parent process 1
		// Prep the GPIO
		gpio_pin_setup();
		// Handle motor commands
		while (*val_ptr.ABORTaddr == 0) {
			motor_handler();
			usleep(50000);
		}
		// Stop everything
		final_stop();

		std::cout << "waiting for child exit signal 2" << std::endl;
		wait(0);
		std::cout << "caught second wait" << std::endl;
	} else {
		// Child process 1
		int pid2 = fork();
		if (pid2 > 0) {
			// Parent Process 2
			std::cout << "started child proc 2" << std::endl;
			camera_preview();
			while ((*val_ptr.ABORTaddr == 0) && (*val_ptr.RUN_MODEaddr == 0)) {
				usleep(50);
			}
			while ((*val_ptr.ABORTaddr == 0) && (*val_ptr.RUN_MODEaddr == 1)) {
				auto current_time = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = current_time-OLD_RECORD_TIME;
				if (elapsed_seconds > RECORD_DURATION) {
					std::cout << "refreshing camera" << std::endl;
					OLD_RECORD_TIME = std::chrono::system_clock::now();
					reset_record();
				}
				// This doesn't have to be super accurate, so only do it every 5 seconds
				usleep(5000000);
			}
			std::cout << "caught abort code: " << *val_ptr.ABORTaddr << " run mode: " << *val_ptr.RUN_MODEaddr << std::endl;
			kill_raspivid();
			
			std::cout << "waiting for child exit signal 1" << std::endl;
			wait(0);
			std::cout << "caught first wait" << std::endl;
			
			std::cout << "SIGCHLD camera" << std::endl;
			exit(SIGCHLD);
		} else {
			// child process of 2
			// Init app
			std::cout << "preparing app" << std::endl;
			int LOST_COUNTER = 0;
			gtk_class::app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
			g_signal_connect(gtk_class::app, "activate", G_CALLBACK (activate), NULL);
			status = g_application_run(G_APPLICATION(gtk_class::app), argc, argv);

			// Cleanup GTK
			g_object_unref(gtk_class::app);
			
			std::cout << "SIGCHLD gtk" << std::endl;
			exit(SIGCHLD);
		}
	}
	
	std::cout << "closing program" << std::endl;
	//~ usleep(2000000);
	
	// Undo our screensaver settings
	system("xset +dpms");
	system("xset s on");
	
	return status;
}




















//~ while (g_application_run (G_APPLICATION (app), argc, argv)) {
		//~ printf("running\n");
		//~ // This usleep counter is not temporally stable due to the forced mmap check every sec.
		//~ // Empirically, appears to capture 1.66s of video/1000000 usleep microseconds.
		//~ while ((counter < 3600) && (!*ABORT)) {
			//~ usleep(400000); //FIXME Significantly reduce this value when possible
			//~ printf("getting current frame\n");
			//~ current_frame();
			//~ LOST_COUNTER = frame_centroid(LOST_COUNTER);
			//~ if (LOST_COUNTER == 30) {
				//~ *ABORT = 1;
			//~ } else {
				//~ //check_move();
				//~ counter++;
			//~ }
		//~ }
		//~ // If the task is still running and the time says we might pass 2gb, restart vid.
		//~ if (!*ABORT) {
			//~ reset_record();
		//~ } else {
			//~ break;
		//~ }
	//~ }
	
	//~ while (!*ABORT) {
		//~ std::cout << "entered" << std::endl;
		
		
		
		//~ // Wait for the preview and manual adjustment mode to finish.
		//~ while (*RUN_MODE == 0) {
			//~ // If the user requests we refresh the preview from GTK, do it here
			//~ if (*REFRESH_CAM == 1) {
				//~ std::cout << "refreshing camera" << std::endl;
				//~ camera_preview();
				//~ *REFRESH_CAM = 0;
			//~ }
			//~ // This counter is required to prevent race condition zombies.
			//~ usleep(500000);
		//~ }
			
			
		//~ if (switched_modes == 0) {
			//~ // Set duty cycles to low end once automatic mode is started
			//~ // Do this only once.
			//~ DUTY_A = 20;
			//~ DUTY_B = 20;
			//~ switched_modes = 1;
		//~ }
		
		//~ // This usleep counter is not temporally stable due to the forced mmap check every sec.
		//~ // Empirically, appears to capture 1.66s of video/1000000 usleep microseconds.
		//~ while ((counter < 3600) & (!*ABORT)) {
			//~ usleep(400000); //FIXME Significantly reduce this value when possible
			//~ current_frame();
			//~ lost_counter = frame_centroid(lost_counter);
			//~ if (lost_counter == 30) {
				//~ *ABORT = 1;
			//~ } else {
				//~ //check_move();
				//~ counter++;
			//~ }
		//~ }
		//~ // If the task is still running and the time says we might pass 2gb, restart vid.
		//~ if (!*ABORT) {
			//~ reset_record();
		//~ }
		
		
		//~ //// If everything is commented out in this while, leave this usleep
		//~ //// It prevents a race condition leaving a zombie process on abort
		//~ //usleep(1000000);
	//~ }
	

	
	
	
	
	
	//~ int pid = fork();
	
	//~ // Run our GTK Application window while using Raspivid to get camera data
	//~ // This runs as a child from the fork (pid=0).
	//~ if (pid > 0) {
		
		
		
		//~ while (!*ABORT) {
			
			
			
			
			//~ if (switched_modes == 0) {
				//~ // Set duty cycles to low end once automatic mode is started
				//~ // Do this only once.
				//~ DUTY_A = 20;
				//~ DUTY_B = 20;
				//~ switched_modes = 1;
			//~ }
			
			//~ // This usleep counter is not temporally stable due to the forced mmap check every sec.
			//~ // Empirically, appears to capture 1.66s of video/1000000 usleep microseconds.
			//~ while ((counter < 3600) & (!*ABORT)) {
				//~ usleep(400000); //FIXME Significantly reduce this value when possible
				//~ current_frame();
				//~ lost_counter = frame_centroid(lost_counter);
				//~ if (lost_counter == 30) {
					//~ *ABORT = 1;
				//~ } else {
					//~ //check_move();
					//~ counter++;
				//~ }
			//~ }
			//~ // If the task is still running and the time says we might pass 2gb, restart vid.
			//~ if (!*ABORT) {
				//~ reset_record();
			//~ }
			
			
			//~ //// If everything is commented out in this while, leave this usleep
			//~ //// It prevents a race condition leaving a zombie process on abort
			//~ //usleep(1000000);
		//~ }
		//~ // Exit the fork.
		//~ exit(0);
	//~ } else {
		//~ // PID 0 is the main application, and runs the GTK window
		//~ app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
		//~ g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
		//~ status = g_application_run (G_APPLICATION (app), argc, argv);
		//~ // Cleanup GTK
		//~ g_object_unref (app);
		//~ // Write the abort code to mmap even though it should already be set to 1
		//~ *ABORT = 1;
	//~ }
