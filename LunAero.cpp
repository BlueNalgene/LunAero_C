
/*
 * C_LunAero/LunAero.cpp - Primary file for robotic moon tracking scope
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
	std::cout << "Time in Milliseconds ="
	 << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()
	<< std::endl;
	current_frame();
	//~ frame_centroid();
	if (*val_ptr.LOST_COUNTERaddr == 30) {
		sem_wait(&LOCK);
		*val_ptr.ABORTaddr = 1;
		sem_post(&LOCK);
	}
}

void cleanup () {
	// Placeholder in case we need to clean anything up on exit.
	std::cout << "killing run" << std::endl;
	kill_raspivid();
	usleep(1000000);
}

void kill_raspivid () {
	sem_wait(&LOCK);
	*val_ptr.STOP_DIRaddr = 3;
	sem_post(&LOCK);
	
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

int create_id_file() {
	std::ofstream idfile;
	char line[1024];
	FILE *cmd = popen("cat /proc/cpuinfo | grep Serial | cut -d ' ' -f 2", "r");
	fgets(line, 1024, cmd);
	pclose(cmd);
	
	std::string linestr(line);
	linestr.erase(std::remove(linestr.begin(), linestr.end(), '\n'), linestr.end());
	IDPATH = FILEPATH + "/" + linestr + ".txt";
	
	std::cout << "LUID: " << linestr << std::endl;
	std::cout << "idpath: " << IDPATH << std::endl;
	
	std::string gmt = current_time(1);
	
	idfile.open(IDPATH);
	idfile 
	<< "LUID: " 
	<< linestr 
	<< std::endl
	<< "UTC : "
	<< gmt
	<< std::endl;
	idfile.close();
	
	return 0;
}

void abort_code() {
	sem_wait(&LOCK);
	*val_ptr.ABORTaddr = 1;
	sem_post(&LOCK);
}

std::string current_time(int gmt) {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time(&rawtime);
	if (gmt > 0) {
		timeinfo = gmtime(&rawtime);
	} else {
		timeinfo = localtime(&rawtime);
	}

	strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", timeinfo);
	std::string str(buffer);

	std::cout << str << std::endl;
	
	return str;
}

void current_frame() {
	
	// Don't bother if we have already told the program to abort
	if (*val_ptr.ABORTaddr == 1) {
		return;
	}
	
	// Run a system check to see if raspivid is running, if not, print a warning
	pid_t thepid = 0;
	FILE* fpidof = popen("pidof raspivid", "r");
	if (fpidof) {
		int p=0;
		if (fscanf(fpidof, "%d", &p)>0 && p>0) {
			thepid = (pid_t)p;
		} else {
			int local_cnt = *val_ptr.LOST_COUNTERaddr;
			local_cnt = local_cnt + 1;
			sem_wait(&LOCK);
			*val_ptr.LOST_COUNTERaddr = local_cnt;
			sem_post(&LOCK);
			std::cout << "WARNING: lost moon counter increased to" 
			<< *val_ptr.LOST_COUNTERaddr 
			<<  " cycles due to failure to find raspivid" 
			<< std::endl;
			pclose(fpidof);
			return;
		}
		pclose(fpidof);
	} else {
		int local_cnt = *val_ptr.LOST_COUNTERaddr;
		local_cnt = local_cnt + 1;
		sem_wait(&LOCK);
		*val_ptr.LOST_COUNTERaddr = local_cnt;
		sem_post(&LOCK);
		std::cout << "WARNING: lost moon counter increased to " 
		<< *val_ptr.LOST_COUNTERaddr 
		<<  " cycles due to failure to find raspivid" 
		<< std::endl;
		return;
	}
	
	DISPMANX_DISPLAY_HANDLE_T   display = 0;
	DISPMANX_MODEINFO_T         info;
	DISPMANX_RESOURCE_HANDLE_T  resource = 0;
	VC_IMAGE_TYPE_T             type = VC_IMAGE_RGB888;
	DISPMANX_TRANSFORM_T	    transform = static_cast <DISPMANX_TRANSFORM_T> (0);
	VC_RECT_T			        rect;
	
	void *image;
	uint32_t vc_image_ptr;
	uint32_t screen = 0;

	bcm_host_init();

	// Get display info for the screen we are using.
	display = vc_dispmanx_display_open( screen );
	if (vc_dispmanx_display_get_info(display, &info) != 0) {
		std::cout << "ERROR: failed to get display info" << std::endl;
		*val_ptr.ABORTaddr = 1;
	}

	// This holds an image
	image = calloc( 1, info.width * 3 * info.height );
	if (!image) {
		std::cout << "ERROR: failed image assertion" << std::endl;
		*val_ptr.ABORTaddr = 1;
	}

	// Create space based on the screen info
	resource = vc_dispmanx_resource_create( type, info.width, info.height, &vc_image_ptr);
	if (!resource) {
		std::cout << "ERROR: failed to create VC Dispmanx Resource" << std::endl;
		*val_ptr.ABORTaddr = 1;
	}

	// Take a snapshot of the screen (stored in resource)
	vc_dispmanx_snapshot(display, resource, transform);

	// Read the rectangular data from resource into the image calloc
	vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);
	vc_dispmanx_resource_read_data(resource, &rect, image, info.width*3);
	// Store the image in a .ppm format file on the hard drive
	// TODO - assert that the drive is plugged in
	// TODO - Make this an mmap stored image.

	std::cout << info.width << " x " << info.height << std::endl;
	std::string imgstr(static_cast<char*>(image), info.width*3*info.height);
	
	int local_height = RVD_HEIGHT - 6;
	int local_width = RVD_WIDTH - 4;
	int local_xcorn = RVD_XCORN + 2;
	int local_ycorn = RVD_YCORN + 3;
	
	int matrix[local_height][local_width];
	int wcnt = 0;
	int hcnt = 0;
	int wcnt_prime = 0;
	int hcnt_prime = 0;
	for (unsigned int i=0; i<imgstr.size(); i=i+3) {
		if ((wcnt > (local_xcorn)) && (wcnt < (local_xcorn + local_width + 1))) {
			if ((hcnt > (local_ycorn - 1)) && (hcnt < (local_ycorn + local_height))) {
				int out;
				out = 0.30*(int)imgstr[i] + 0.59*(int)imgstr[i+1] + 0.11*(int)imgstr[i+2];
				if (out > 25) { // 10% threshold
					out = 0;
				} else {
					out = 1;
				}
				matrix[hcnt_prime][wcnt_prime] = out;
				wcnt_prime += 1;
				if (wcnt_prime == local_width) {
					wcnt_prime = 0;
					hcnt_prime += 1;
				}
			}
		}
		wcnt += 1;
		if (wcnt == info.width) {
			wcnt = 0;
			hcnt += 1;
		}
	}
	
	// Optionally, save the image to a file on the disk so we can check that it makes sense
	if (SAVE_DEBUG_IMAGE) {
		FILE *fp = fopen("/media/pi/MOON1/out.pbm", "wb");
		//~ fprintf(fp, "P1\n%d %d\n1\n", frmwidth, frmheight);
		fprintf(fp, "P1\n%d %d\n1\n", local_width, local_height);
		for (int i=0; i<local_height; i++) {
			for(int j=0; j<local_width; j++) {
				if (j == local_width-1) {
					fprintf(fp, "\n");
				}
				fprintf(fp, "%d", matrix[i][j]);
			}
		}
		fclose(fp);
	}
	
	// Cleanup the VC resources
	if (vc_dispmanx_resource_delete(resource) != 0) {
		std::cout << "ERROR: failed to delete vc resource" << std::endl;
		*val_ptr.ABORTaddr = 1;
	}
	if (vc_dispmanx_display_close(display) != 0) {
		std::cout << "ERROR: failed to close vc display" << std::endl;
		*val_ptr.ABORTaddr = 1;
	}
	free(image);
	
	//Check for bright or dark spots?
	// Note the weird inversion when we use .pbm format
	// Bright spots == 0
	// dark spots == 1
	int checkval = 0;
	
	// Number of points to be "on edge" is 10% of edge
	int w_thresh = WORK_WIDTH/20;
	int h_thresh = WORK_HEIGHT/20;

	// Store sum of each edge
	int top_edge = 0;
	int bottom_edge = 0;
	int left_edge = 0;
	int right_edge = 0;
	
	for (int j=0; j<local_width; j++) {
		if (matrix[0][j] == checkval) {
			top_edge++;
		}
		if (matrix[local_height-1][j] == checkval) {
			bottom_edge++;
		}
	}
	
	for (int k=0; k<local_height; k++) {
		if (matrix[k][0] == checkval) {
			left_edge++;
		}
		if (matrix[k][local_width-1] == checkval) {
			right_edge++;
		}
	}
	
	// Calculate the centroid using sum.
	int sumx = 0;
	int sumy = 0;
	int mcnt = 0;
	
	for (int k=0; k<local_height; k++) {
		for (int j=0; j<local_width; j++) {
			if (matrix[k][j] == checkval) {
				sumx = sumx + j;
				sumy = sumy + k;
				mcnt++;
			}
		}
	}
	std::cout << "sumx " << sumx << " sumy " << sumy << std::endl;
	
	// If nothing is found, return an increment to the moon loss counter
	if ((sumx == 0) && (sumy == 0)) {
		int local_cnt = *val_ptr.LOST_COUNTERaddr;
		local_cnt = local_cnt + 1;
		sem_wait(&LOCK);
		*val_ptr.LOST_COUNTERaddr = local_cnt;
		sem_post(&LOCK);
		std::cout << "lost moon for " << *val_ptr.LOST_COUNTERaddr <<  " cycles" << std::endl;
	} else {
		// something was found, reset moon loss counter
		sem_wait(&LOCK);
		*val_ptr.LOST_COUNTERaddr = 0;
		sem_post(&LOCK);
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
			if (abs((sumy/mcnt)-(local_height/2)) > ((local_height/2)*0.2)) {
				std::cout << "M_y = " << (sumy/mcnt)-(local_height/2) << std::endl;
				if (((sumy/mcnt)-(local_height/2)) > 0) {
					std::cout << "centroid moving to down" << std::endl;
					mot_down_command();
				} else {
					std::cout << "centroid moving to up" << std::endl;
					mot_up_command();
				}
			} else {
				sem_wait(&LOCK);
				*val_ptr.STOP_DIRaddr = 2;
				sem_post(&LOCK);
			}
		}
		
		if ((left_edge >= h_thresh) && (right_edge < h_thresh)) {
			std::cout << "detected light on left edge" << std::endl;
			mot_left_command();
		} else if ((left_edge <= h_thresh) && (right_edge > h_thresh)) {
			std::cout << "detected light on right edge" << std::endl;
			mot_right_command();
		} else {
			if (abs((sumx/mcnt)-(local_width/2)) > ((local_width/2)*0.4)) {
				std::cout << "M_x = " << (sumx/mcnt)-(local_width/2) << std::endl;
				if (((sumx/mcnt)-(local_width/2)) > 0) {
					std::cout << "centroid moving to right" << std::endl;
					mot_right_command();
				} else {
					std::cout << "centroid moving to left" << std::endl;
					mot_left_command();
				}
			} else {
				// In the case we already need to stop one direction, stop both
				if (*val_ptr.STOP_DIRaddr == 2) {
					sem_wait(&LOCK);
					*val_ptr.STOP_DIRaddr = 3;
					sem_post(&LOCK);
				} else {
					sem_wait(&LOCK);
					*val_ptr.STOP_DIRaddr = 1;
					sem_post(&LOCK);
				}
			}
		}
	}
	
	return;
}

int startup_disk_check() {
	std::string userenv = std::getenv("USER");
	std::string local_path = "/media/" + userenv + "/MOON1";
	DEFAULT_FILEPATH = local_path + "/";
	
	std::ifstream mountsfile("/proc/mounts", std::ifstream::in);
	if (!mountsfile.good()) {
		std::cout << "ERROR: Input stream to /proc/mounts is not valid" << std::endl;
		return 1;
	}
	
	int found_mount = 0;
	std::string line;
	
	while (std::getline(mountsfile, line)) {
		if (line.find(local_path) != std::string::npos) {
			found_mount = 1;
			break;
		}
	}
	mountsfile.close();
	
	if (found_mount) {
		namespace fs = std::filesystem;
		fs::space_info tmp = fs::space(local_path);
		if (tmp.available < (1000000 * std::chrono::duration<double>(RECORD_DURATION).count())) {
			std::cout << "ERROR: The space on this drive is too low with "
			<< tmp.available << " bytes remaining" << std::endl
			<< "...    the run will likely not be successful, exiting." << std::endl;
		} else if (tmp.available < (10 * 1000000 * std::chrono::duration<double>(RECORD_DURATION).count())) {
			std::cout << "WARNING: This drive only has " << tmp.available
			<< " bytes of space remaining," << std::endl 
			<< "...     you may run out of during this run" << std::endl;
		}
		std::cout << "Free space: " << tmp.free << std::endl 
		<< "Available space: " << tmp.available << std::endl;
		return 0;
	} else {
		std::cout << "ERROR: Could not find a drive mounted at " << DEFAULT_FILEPATH
		<< std::endl << "...    check that your external drive is connected properly"
		<< std::endl;
		return 1;
	}
}

int main (int argc, char **argv) {
	
	if (startup_disk_check()) {
		return 1;
	}
	
	// Screensaver settings for the raspberry pi
	system("xset -dpms");
	system("xset s off");
	
	int status = 0;
	
	// init SUBS semaphore
	sem_init(&LOCK, 1, 1);
	
	// Make folder for stuff
	TSBUFF = current_time(0);
	FILEPATH = DEFAULT_FILEPATH + TSBUFF;
	mkdir(FILEPATH.c_str(), 0700);
	std::cout << "time: " << TSBUFF << std::endl;
	std::cout << "path: " << FILEPATH << std::endl;
	
	// Make ID file
	if (create_id_file()) {
		std::cout << "ERROR: Failed to create ID file" << std::endl;
	}
	
	
	// Get the screen size now.  Opens and kills an invisible GTK instance.
	//~ screen_size(argc, argv);
	
	// Keep these in order! Fork MUST be called AFTER the mmap ABORT code!!
	
	//~ if (memory_map_init() != 0) {
		//~ std::cout << "Failed to map values to memory" << std::endl;
		//~ return 1;
	//~ }
	
	
	
	// Memory values which influence camera commands. Defaults to 0.
	val_ptr.LOST_COUNTERaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
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
	val_ptr.SUBSaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	
	// Memory value which tells the program to continue running.
	val_ptr.ABORTaddr = (int *)(mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	
	// Assign initial values to mmap'd variables
	*val_ptr.LOST_COUNTERaddr = 0;
	*val_ptr.ISO_VALaddr = 200;
	*val_ptr.SHUTTER_VALaddr = 10000;
	*val_ptr.RUN_MODEaddr = 0;
	*val_ptr.REFRESH_CAMaddr = 0;
	*val_ptr.HORZ_DIRaddr = 0;
	*val_ptr.VERT_DIRaddr = 0;
	*val_ptr.STOP_DIRaddr = 0;
	*val_ptr.DUTY_Aaddr = 100;
	*val_ptr.DUTY_Baddr = 100;
	*val_ptr.SUBSaddr = 0;
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
			sem_wait(&LOCK);
			*val_ptr.RUN_MODEaddr = 0;
			sem_post(&LOCK);
			while ((*val_ptr.ABORTaddr == 0) && (*val_ptr.RUN_MODEaddr == 0)) {
				usleep(50);
			}
			sem_wait(&LOCK);
			*val_ptr.RUN_MODEaddr = 1;
			sem_post(&LOCK);
			while ((*val_ptr.ABORTaddr == 0) && (*val_ptr.RUN_MODEaddr == 1)) {
				auto current_time = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = current_time-OLD_RECORD_TIME;
				if (elapsed_seconds > RECORD_DURATION) {
					std::cout << "refreshing camera" << std::endl;
					sem_wait(&LOCK);
					OLD_RECORD_TIME = std::chrono::system_clock::now();
					*val_ptr.SUBSaddr = 2;
					sem_post(&LOCK);
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
			//~ int LOST_COUNTER = 0;
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
