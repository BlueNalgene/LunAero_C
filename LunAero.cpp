
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

/**
 * This function (called from gtk_LunAero.cpp) handles checking the frame during live operation and
 * processes the LOST_COUNTER
 *
 */
void cb_framecheck() {
	printf("getting current frame\n");
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Time in Milliseconds ="
		<< std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()
		<< std::endl;
		LOGGING.close();
	}
	current_frame();
	//~ frame_centroid();
	if (*val_ptr.LOST_COUNTERaddr == LOST_THRESH) {
		sem_wait(&LOCK);
		*val_ptr.ABORTaddr = 1;
		sem_post(&LOCK);
	}
}

/**
 * This function is called on exit to clean up the environment.  Primarily used to ensure that the
 * raspivid system call is killed properly.
 *
 *
 */
void cleanup () {
	// Placeholder in case we need to clean anything up on exit.
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "killing run" << std::endl;
		LOGGING.close();
	}
	kill_raspivid();
	usleep(1000000);
}

/**
 * This function kills raspivid.  It does this in an ugly way by finding the process ID of the raspivid
 * instance and killing that PID.
 *
 *
 */
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
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: Unable to kill raspivid" << std::endl;
			LOGGING.close();
		}
	}
}

/**
 * This funciton creates an ID file to be stored with the recorded video which includes information
 * about the unit the run worked on.  The processor id is stored using the cpuinfo unique to the 
 * Raspberry Pi.
 *
 * @return status
 */
int create_id_file() {
	std::ofstream idfile;
	char line[1024];
	FILE *cmd = popen("cat /proc/cpuinfo | grep Serial | cut -d ' ' -f 2", "r");
	fgets(line, 1024, cmd);
	pclose(cmd);
	
	std::string linestr(line);
	linestr.erase(std::remove(linestr.begin(), linestr.end(), '\n'), linestr.end());
	IDPATH = FILEPATH + "/" + linestr + ".txt";
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "LUID: " << linestr << std::endl
		<< "idpath: " << IDPATH << std::endl;
		LOGGING.close();
	}
	
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

/**
 * This is a helper funciton which updates the abort code to end the run across all forks.
 *
 *
 */
void abort_code() {
	sem_wait(&LOCK);
	*val_ptr.ABORTaddr = 1;
	sem_post(&LOCK);
}

/**
 * This function fetches the current time and formats it as a string.
 *
 * @param gmt plus or minus tmz gmt
 * @return str the current time formatted as a string
 */
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

	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< str << std::endl;
		LOGGING.close();
	}
	
	return str;
}

/**
 * This function captures the frame from the preview window (a complicated process involving capturing
 * the VC/DISPMANX screenshot, not the X window screensho), crops it to ignore the GTK,  and determines
 * how blurry the image is.  Blur is computed based on the standard method, running a Laplacian
 * operation on the cropped image and computing the variance of this result.
 *
 * @return blurval A floating point value representing the degree of variation in tbe blur
 */
float blur_test() {
	float blurval = 0;
	
	// Run a system check to see if raspivid is running, if not, print a warning
	pid_t thepid = 0;
	FILE* fpidof = popen("pidof raspivid", "r");
	if (fpidof) {
		int p=0;
		if (fscanf(fpidof, "%d", &p)>0 && p>0) {
			thepid = (pid_t)p;
		} else {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "WARNING: cannot detect image focus if Raspivid is not running" << std::endl;
				LOGGING.close();
			}
			return 0;
		}
		pclose(fpidof);
	} else {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "WARNING: cannot detect image focus if Raspivid is not running" << std::endl;
			LOGGING.close();
		}
		return 0;
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
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to get display info" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}

	// This holds an image
	image = calloc( 1, info.width * 3 * info.height );
	if (!image) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed image assertion" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}

	// Create space based on the screen info
	resource = vc_dispmanx_resource_create( type, info.width, info.height, &vc_image_ptr);
	if (!resource) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to create VC Dispmanx Resource" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}

	// Take a snapshot of the screen (stored in resource)
	vc_dispmanx_snapshot(display, resource, transform);

	// Read the rectangular data from resource into the image calloc
	vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);
	vc_dispmanx_resource_read_data(resource, &rect, image, info.width*3);

	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< info.width << " x " << info.height << std::endl;
		LOGGING.close();
	}
	std::string imgstr(static_cast<char*>(image), info.width*3*info.height);
	
	int local_height = RVD_HEIGHT - 6;
	int local_width = RVD_WIDTH - 4;
	int local_xcorn = RVD_XCORN + 2;
	int local_ycorn = RVD_YCORN + 3;
	
	unsigned char * img_data_ptr = (unsigned char*) &image;
	Mat in_image(info.height, info.width, CV_8UC3, image);
	cvtColor(in_image.clone(), in_image, COLOR_RGB2GRAY);
	
	// Cleanup the VC resources
	if (vc_dispmanx_resource_delete(resource) != 0) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to delete vc resource" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}
	if (vc_dispmanx_display_close(display) != 0) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to close vc display" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}
	free(image);
	
	// Run laplacian on input image
	Mat ROI(in_image, Rect(local_xcorn, local_ycorn, local_width, local_height));
	Mat cropped_image;
	ROI.copyTo(cropped_image);
	imwrite("/home/pi/Documents/LunAero_C/test.png", cropped_image);
	
	Laplacian(cropped_image, in_image, CV_64F);
	Scalar mean, stddev; // 0:1st channel, 1:2nd channel and 2:3rd channel
	meanStdDev(in_image, mean, stddev, Mat());
	blurval = stddev.val[0] * stddev.val[0];
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "BLURVAL: " << blurval << std::endl;
		LOGGING.close();
	}
	
	return blurval;
}

/**
 * This function tests our image to determine if the moon appears too bright.  This is not a simple
 * test.  Since we are looking at a "white" moon, we can expect to see a lot of spots which are bright
 * on their own.  So we need to rely on another method to get the brightness saturation.  I am using
 * the nearest neighbors of each pixel by iterating a custom \f$ 3 \times 3\f$ kernel across the image.
 * Rather than using a simple nearest neighbor kernel like:
 * \f[
 * \boldsymbol{I_{neigh}} =
 * \begin{bmatrix}
 * 1 & 1 & 1 \\
 * 1 & 0 & 1 \\
 * 1 & 1 & 1
 * \end{bmatrix}
 * \f]
 * I used two separate \f$ 3 \times 3\f$ matrices split into a T and X form:
 * \f[
 * \boldsymbol{I_{t}} =
 * \begin{bmatrix}
 * 0 & 1 & 0 \\
 * 1 & 0 & 1 \\
 * 0 & 1 & 0
 * \end{bmatrix}
 * ,
 * \boldsymbol{I_{x}} =
 * \begin{bmatrix}
 * 1 & 0 & 1 \\
 * 0 & 0 & 0 \\
 * 1 & 0 & 1
 * \end{bmatrix}
 * \f]
 * I found empirically that this split more accurately scales with the level of glowing brightness
 * in images by creating sample over-bright images.  So my method, once the image has been extracted
 * from the VC/DISPMANX screenshotting tool, is to 1) find the area of the largest contour (which is
 * presumably the moon) 2) Convert the input image to the HSV color space and extract only the V values
 * (the brightness level) 3) Find the raw brightess (thresholded based on a user set value
 * RAW_BRIGHT_THRESH from settings.cfg)per area of moon 4) Create a test Mat of values by iterating
 * each kernel over the V channel image 5) Run an or test on the truth value of those Mat's and count
 * the number of "overbright" pixels and correct this based on the observe area of the moon.  If the
 * resulting percentages are less than the BRIGHT_THRESH, as declared in settings.cfg, then the moon is
 * not too bright to use.  Otherwise, a message is issued from the GTK loop.
 * 
 *
 * @return bright_outcome Result of comparing brightness values to BRIGHT_THRESH.  True = passed and
 * the image is not too bright, false = failed as the image was too bright.
 */
bool bright_test() {
	bool bright_outcome;
	// Create kernels
	float tdata[] = {0, 1, 0, 1, 0, 1, 0, 1, 0};
	Mat tkernel(3, 3, CV_32F, tdata);
	float xdata[] = {1, 0, 1, 0, 0, 0, 1, 0, 1};
	Mat xkernel(3, 3, CV_32F, xdata);
	
	// Run a system check to see if raspivid is running, if not, print a warning
	pid_t thepid = 0;
	FILE* fpidof = popen("pidof raspivid", "r");
	if (fpidof) {
		int p=0;
		if (fscanf(fpidof, "%d", &p)>0 && p>0) {
			thepid = (pid_t)p;
		} else {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "WARNING: cannot detect image focus if Raspivid is not running" << std::endl;
				LOGGING.close();
			}
			return 0;
		}
		pclose(fpidof);
	} else {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "WARNING: cannot detect image focus if Raspivid is not running" << std::endl;
			LOGGING.close();
		}
		return 0;
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
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to get display info" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}

	// This holds an image
	image = calloc( 1, info.width * 3 * info.height );
	if (!image) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed image assertion" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}

	// Create space based on the screen info
	resource = vc_dispmanx_resource_create( type, info.width, info.height, &vc_image_ptr);
	if (!resource) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to create VC Dispmanx Resource" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}

	// Take a snapshot of the screen (stored in resource)
	vc_dispmanx_snapshot(display, resource, transform);

	// Read the rectangular data from resource into the image calloc
	vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);
	vc_dispmanx_resource_read_data(resource, &rect, image, info.width*3);

	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< info.width << " x " << info.height << std::endl;
		LOGGING.close();
	}
	std::string imgstr(static_cast<char*>(image), info.width*3*info.height);
	
	int local_height = RVD_HEIGHT - 6;
	int local_width = RVD_WIDTH - 4;
	int local_xcorn = RVD_XCORN + 2;
	int local_ycorn = RVD_YCORN + 3;
	
	unsigned char * img_data_ptr = (unsigned char*) &image;
	Mat in_image(info.height, info.width, CV_8UC3, image);
	cvtColor(in_image.clone(), in_image, COLOR_RGB2GRAY);
	
	// Cleanup the VC resources
	if (vc_dispmanx_resource_delete(resource) != 0) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to delete vc resource" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}
	if (vc_dispmanx_display_close(display) != 0) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to close vc display" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}
	free(image);
	
	
	// Perform the brightness tests
	Mat alt_image(in_image.cols, in_image.rows, CV_8U);
	
	// Find largest contour
	cvtColor(in_image, alt_image, COLOR_BGR2GRAY);
	std::vector <std::vector <Point>> contours;
	std::vector<Vec4i> hierarchy;
	int big = 0;
	double area;
	findContours(alt_image, contours, hierarchy, RETR_TREE, CHAIN_APPROX_NONE);
	for( size_t i = 0; i< contours.size(); i++ ) {
		area = contourArea(contours[i]);
		if (area > big) {
			big = area;
		}
	}
	
	// Get just the brightness of the image
	cvtColor(in_image, alt_image, COLOR_BGR2HSV);
	Mat1b brights;
	extractChannel(in_image, brights, 2);
	
	int i, j;
	
	// Get corrected brightness value
	float cba = 0.;
	for (i = 0; i < brights.rows; ++i) {
		for (j = 0; j < brights.cols; j++) {
			if ((int)brights.at<uchar>(i, j) > (RAW_BRIGHT_THRESH)) {
				cba++;
			}
		}
	}
	cba = cba/big;
	
	
	// Filter based on each kernel
	int valsum1 = 0;
	int valsum2 = 0;
	Mat vals1(Size(alt_image.cols, alt_image.rows), CV_8U);
	Mat vals2(Size(alt_image.cols, alt_image.rows), CV_8U);
	filter2D(brights, alt_image, CV_16U, tkernel);
	for (i = 0; i < alt_image.rows; ++i) {
		for (j = 0; j < alt_image.cols; j++) {
			if ((int)alt_image.at<unsigned short>(i, j) > (4*RAW_BRIGHT_THRESH)) {
				vals1.at<uint8_t>(i, j) = 1;
				valsum1++;
			} else {
				vals1.at<uint8_t>(i, j) = 0;
			}
		}
	}
	filter2D(brights, alt_image, CV_16U, xkernel);
	for (i = 0; i < alt_image.rows; ++i) {
		for (j = 0; j < alt_image.cols; j++) {
			if (alt_image.at<unsigned short>(i, j) > (4*RAW_BRIGHT_THRESH)) {
				vals2.at<uint8_t>(i, j) = 1;
				valsum2++;
			} else {
				vals2.at<uint8_t>(i, j) = 0;
			}
		}
	}
	
	// Get corrected overbrightness value
	float coa = 0.;
	for (i = 0; i < vals1.rows; ++i) {
		for (j = 0; j < vals1.cols; j++) {
// 			std::cout << vals1.at<uchar>(i, j) << " " << vals2.at<uchar>(i, j) << std::endl;
			if ((vals1.at<uint8_t>(i, j) == 1) || (vals2.at<uint8_t>(i, j) == 1)) {
				coa++;
			}
		}
	}
	coa = coa/big;
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Brightness calcs: " << cba << ", " << coa << std::endl;
		LOGGING.close();
	}
	
	// Test if these values are below our threshold and return boolean result
	if ((cba < BRIGHT_THRESH) && (coa < BRIGHT_THRESH)) {
		return true;
	} else {
		return false;
	}
}

/**
 * This function captures the frame from the preview window (a complicated process involving capturing
 * the VC/DISPMANX screenshot, not the X window screensho), crops it to ignore the GTK, and determines 
 * how well the moon is centered in the cropped frame.  Priority is given to checking whether the moon
 * is touching the side of the cropped image.  If the edge is not being touched, threshold limited
 * brightness is used to find the center of mass of the bright spot and comparing it to the target
 * location.
 *
 *
 */
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
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "WARNING: lost moon counter increased to" 
				<< *val_ptr.LOST_COUNTERaddr 
				<<  " cycles due to failure to find raspivid" 
				<< std::endl;
				LOGGING.close();
			}
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
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "WARNING: lost moon counter increased to " 
			<< *val_ptr.LOST_COUNTERaddr 
			<<  " cycles due to failure to find raspivid" 
			<< std::endl;
			LOGGING.close();
		}
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
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to get display info" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}

	// This holds an image
	image = calloc( 1, info.width * 3 * info.height );
	if (!image) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed image assertion" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}

	// Create space based on the screen info
	resource = vc_dispmanx_resource_create( type, info.width, info.height, &vc_image_ptr);
	if (!resource) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to create VC Dispmanx Resource" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}

	// Take a snapshot of the screen (stored in resource)
	vc_dispmanx_snapshot(display, resource, transform);

	// Read the rectangular data from resource into the image calloc
	vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);
	vc_dispmanx_resource_read_data(resource, &rect, image, info.width*3);
	// TODO - Make this an mmap stored image.

	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< info.width << " x " << info.height << std::endl;
		LOGGING.close();
	}
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
		std::string userenv = std::getenv("USER");
		std::string filestr = "/media/" + userenv + "/" + DRIVE_NAME + "/out.pbm";
		FILE *fp = fopen(filestr.c_str(), "wb");
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
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to delete vc resource" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}
	if (vc_dispmanx_display_close(display) != 0) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: failed to close vc display" << std::endl;
			LOGGING.close();
		}
		*val_ptr.ABORTaddr = 1;
	}
	free(image);
	
	//Check for bright or dark spots?
	// Note the weird inversion when we use .pbm format
	// Bright spots == 0
	// dark spots == 1
	int checkval = 0;
	
	// Number of points to be "on edge" is 10% of edge
	int w_thresh = WORK_WIDTH/EDGE_DIVISOR_W;
	int h_thresh = WORK_HEIGHT/EDGE_DIVISOR_H;

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
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "sumx " << sumx << " sumy " << sumy << std::endl;
		LOGGING.close();
	}
	
	// If nothing is found, return an increment to the moon loss counter
	if ((sumx == 0) && (sumy == 0)) {
		int local_cnt = *val_ptr.LOST_COUNTERaddr;
		local_cnt = local_cnt + 1;
		sem_wait(&LOCK);
		*val_ptr.LOST_COUNTERaddr = local_cnt;
		sem_post(&LOCK);
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "lost moon for " << *val_ptr.LOST_COUNTERaddr <<  " cycles" << std::endl;
			LOGGING.close();
		}
	} else {
		// something was found, reset moon loss counter
		sem_wait(&LOCK);
		*val_ptr.LOST_COUNTERaddr = 0;
		sem_post(&LOCK);
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "Moon found centered at (" << (sumx/mcnt) << ", " << (sumy/mcnt) << ")\n" << std::endl
			<< "top:bottom::left:right " << top_edge << ":" << bottom_edge << "::" << left_edge
			<< ":" << right_edge <<std::endl;
			LOGGING.close();
		}
		// Report edges only
		if ((top_edge >= w_thresh) && (bottom_edge < w_thresh)) {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "+top edge" << std::endl;
				LOGGING.close();
			}
		}
		if ((bottom_edge >= w_thresh) && (top_edge < w_thresh)) {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "+bottom edge" << std::endl;
				LOGGING.close();
			}
		}
		if ((left_edge >= h_thresh) && (right_edge < h_thresh)) {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "+left edge" << std::endl;
				LOGGING.close();
			}
		}
		if ((left_edge <= h_thresh) && (right_edge > h_thresh)) {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "+right edge" << std::endl;
				LOGGING.close();
			}
		}
		
		
		
		
		// Check if bright spot is near the edge of the frame
		// If so, move away from that edge
		// If not or near both edges, use centroid
		if ((top_edge >= w_thresh) && (bottom_edge < w_thresh)) {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "detected light on top edge" << std::endl;
				LOGGING.close();
			}
			mot_up_command();
		} else if ((bottom_edge >= w_thresh) && (top_edge < w_thresh)) {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "detected light on bottom edge" << std::endl;
				LOGGING.close();
			}
			mot_down_command();
		} else {
			if (abs((sumy/mcnt)-(local_height/2)) > ((local_height/2)*0.2)) {
				if (DEBUG_COUT) {
					LOGGING.open(LOGOUT, std::ios_base::app);
					LOGGING
					<< "M_y = " << (sumy/mcnt)-(local_height/2) << std::endl;
					LOGGING.close();
				}
				if (((sumy/mcnt)-(local_height/2)) > 0) {
					if (DEBUG_COUT) {
						LOGGING.open(LOGOUT, std::ios_base::app);
						LOGGING
						<< "centroid moving to down" << std::endl;
						LOGGING.close();
					}
					mot_down_command();
				} else {
					if (DEBUG_COUT) {
						LOGGING.open(LOGOUT, std::ios_base::app);
						LOGGING
						<< "centroid moving to up" << std::endl;
						LOGGING.close();
					}
					mot_up_command();
				}
			} else {
				sem_wait(&LOCK);
				*val_ptr.STOP_DIRaddr = 2;
				sem_post(&LOCK);
			}
		}
		
		if ((left_edge >= h_thresh) && (right_edge < h_thresh)) {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "detected light on left edge" << std::endl;
				LOGGING.close();
			}
			mot_left_command();
		} else if ((left_edge <= h_thresh) && (right_edge > h_thresh)) {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "detected light on right edge" << std::endl;
				LOGGING.close();
			}
			mot_right_command();
		} else {
			if (abs((sumx/mcnt)-(local_width/2)) > ((local_width/2)*0.4)) {
				if (DEBUG_COUT) {
					LOGGING.open(LOGOUT, std::ios_base::app);
					LOGGING
					<< "M_x = " << (sumx/mcnt)-(local_width/2) << std::endl;
					LOGGING.close();
				}
				if (((sumx/mcnt)-(local_width/2)) > 0) {
					if (DEBUG_COUT) {
						LOGGING.open(LOGOUT, std::ios_base::app);
						LOGGING
						<< "centroid moving to right" << std::endl;
						LOGGING.close();
					}
					mot_right_command();
				} else {
					if (DEBUG_COUT) {
						LOGGING.open(LOGOUT, std::ios_base::app);
						LOGGING
						<< "centroid moving to left" << std::endl;
						LOGGING.close();
					}
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

/**
 * This function takes the two input strings and uses them to issue a notificaiton alert to the
 * Raspian desktop.  This is a variation of the linux command ``notify-send``, and it requires
 * libnotify-dev installed on the system.
 *
 * @param input1 The string which should be at the top of the notification
 * @param input2 The string's bottomtext, more descriptive.
 * @return status
 */
int notify_handler(std::string input1, std::string input2) {
	notify_init("LunAero");
	NotifyNotification* n = notify_notification_new (input1.c_str(), input2.c_str(), 0);
	notify_notification_set_timeout(n, EMG_DUR); // 10 seconds
	if (!notify_notification_show(n, 0)) {
		std::cerr << "Libnotify failed.  I hope you have terminal open!" << std::endl;
		return -1;
	}	
	return 0;
}

/**
 * This function checks the save location properties to determine if the location is acceptable for
 * saving video.  This only has limited capacity to determine the fitness of the drive.  The quality
 * of the save location is determined based on 1) drive is findable 2) valid /proc/mounts file 3)
 * remaining space left on the drive.  Remaining space is calculated based on 1000000*t_s where t_s
 * is the seconds a video will be recorded based on the settings.cfg line RECORD_DURATION.  If the
 * available space is less than this, the program stops with an error.  If it is less than 10 times this
 * value, a warning issued, but the program continues.  Reported free space and available space on the
 * drive is recorded in the log.
 *
 * @return status
 */
int startup_disk_check() {
	
	std::string userenv = std::getenv("USER");
	std::string local_path = "/media/" + userenv + "/" + DRIVE_NAME;
	DEFAULT_FILEPATH = local_path + "/";
	
	std::ifstream mountsfile("/proc/mounts", std::ifstream::in);
	if (!mountsfile.good()) {
		notify_handler("LunAero Error", "Input stream to /proc/mounts is not valid");
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
			notify_handler("LunAero Error", "The space on this drive is too low with "
			+ std::to_string(tmp.available)
			+ " bytes remaining");
			return 2;
		} else if (tmp.available < (10 * 1000000 * std::chrono::duration<double>(RECORD_DURATION).count())) {
			notify_handler("LunAero Warning", "This drive only has "
			+ std::to_string(tmp.available)
			+ " bytes of space remaining.");
		}
		DISK_OUTPUT.push_back("Free space: " + tmp.free);
		DISK_OUTPUT.push_back("Available space: " + tmp.available);
		return 0;
	} else {
		notify_handler("LunAero Error", "Could not find a drive mounted at " + DEFAULT_FILEPATH);
		return 3;
	}
}

/**
 * This function handles the strings and values parsed from the settings.cfg file and assigns them
 * to the global values.
 *
 * @param name String obtained while parsing the settings.cfg file
 * @param value The value associated with name from settings.cfg file
 * @return status
 */
int parse_checklist(std::string name, std::string value) {
	// Boolean cases
	if (name == "DEBUG_COUT"
 		|| name == "SAVE_DEBUG_IMAGE"
		) {
		// Define booleans
		bool result;
		if (value == "true" || value == "True" || value == "TRUE") {
			result = true;
		} else if (value == "false" || value == "False" || value == "FALSE") {
			result = false;
		} else {
			std::cerr << "Invalid boolean value in settings.cfg item: " << name << std::endl;
			return 1;
		}
		
		if (name == "DEBUG_COUT") {
			DEBUG_COUT = result;
		} else if (name == "SAVE_DEBUG_IMAGE") {
			SAVE_DEBUG_IMAGE = result;
		}
	}
	// Int cases
	else if (
		name == "FONT_MOD"
		|| name == "EDGE_DIVISOR_W"
		|| name == "EDGE_DIVISOR_H"
		|| name == "FRAMECHECK_FREQ"
		|| name == "MMAL_ERROR_THRESH"
		|| name == "RPI_FPS"
		|| name == "RPI_BR"
		|| name == "SHUT_JUMP"
		|| name == "SHUT_JUMP_BIG"
		|| name == "FREQ"
		|| name == "MIN_DUTY"
		|| name == "MAX_DUTY"
		|| name == "BRAKE_DUTY"
		|| name == "APINP"
		|| name == "APIN1"
		|| name == "APIN2"
		|| name == "BPIN1"
		|| name == "BPIN2"
		|| name == "BPINP"
		|| name == "EMG_DUR"
		|| name == "LOST_THRESH"
		|| name == "RAW_BRIGHT_THRESH"
		) {
		int result = std::stoi(value);
		if (name == "FONT_MOD") {
			FONT_MOD = result;
		} else if (name == "EDGE_DIVISOR_W") {
			EDGE_DIVISOR_W = result;
		} else if (name == "EDGE_DIVISOR_H") {
			EDGE_DIVISOR_H = result;
		} else if (name == "FRAMECHECK_FREQ") {
			FRAMECHECK_FREQ = result;
		} else if (name == "MMAL_ERROR_THRESH") {
			MMAL_ERROR_THRESH = result;
		} else if (name == "RPI_FPS") {
			RPI_FPS = result;
		} else if (name == "RPI_BR") {
			RPI_BR = result;
		} else if (name == "SHUT_JUMP") {
			SHUT_JUMP = result;
		} else if (name == "SHUT_JUMP_BIG") {
			SHUT_JUMP_BIG = result;
		} else if (name == "FREQ") {
			FREQ = result;
		} else if (name == "MIN_DUTY") {
			MIN_DUTY = result;
		} else if (name == "MAX_DUTY") {
			MAX_DUTY = result;
		} else if (name == "BRAKE_DUTY") {
			BRAKE_DUTY = result;
		} else if (name == "APINP") {
			APINP = result;
		} else if (name == "APIN1") {
			APIN1 = result;
		} else if (name == "APIN2") {
			APIN2 = result;
		} else if (name == "BPIN1") {
			BPIN1 = result;
		} else if (name == "BPIN2") {
			BPIN2 = result;
		} else if (name == "BPINP") {
			BPINP = result;
		} else if (name == "EMG_DUR") {
			EMG_DUR = result;
		} else if (name == "LOST_THRESH") {
			LOST_THRESH = result;
		} else if (name == "RAW_BRIGHT_THRESH") {
			RAW_BRIGHT_THRESH = result;
		}
	}
	// Double cases
	else if (
		name == "RECORD_DURATION"
		|| name == "LOOSE_WHEEL_DURATION"
		) {
		double result = std::stod(value);
		if (name == "RECORD_DURATION") {
			RECORD_DURATION = (std::chrono::duration<double>) result;
		} else if (name == "LOOSE_WHEEL_DURATION") {
			LOOSE_WHEEL_DURATION = (std::chrono::duration<double>) result;
		}
	}
	// Float cases
	else if (
		name == "BRIGHT_THRESH"
		 ) {
		float result = std::stof(value);
		if (name == "BRIGHT_THRESH") {
			BRIGHT_THRESH = result;
		}
	}
	// String cases
	else if (
		name == "KV_QUIT"
		|| name == "KV_RUN"
		|| name == "KV_LEFT"
		|| name == "KV_RIGHT"
		|| name == "KV_UP"
		|| name == "KV_DOWN"
		|| name == "KV_STOP"
		|| name == "KV_REFRESH"
		|| name == "KV_S_UP_UP"
		|| name == "KV_S_DOWN_DOWN"
		|| name == "KV_S_UP"
		|| name == "KV_S_DOWN"
		|| name == "KV_ISO"
		|| name == "RPI_EX"
		|| name == "DRIVE_NAME"
		) {
		if (name == "KV_QUIT") {
			KV_QUIT = value;
		} else if (name == "KV_RUN") {
			KV_RUN = value;
		} else if (name == "KV_LEFT") {
			KV_LEFT = value;
		} else if (name == "KV_RIGHT") {
			KV_RIGHT = value;
		} else if (name == "KV_UP") {
			KV_UP = value;
		} else if (name == "KV_DOWN") {
			KV_DOWN = value;
		} else if (name == "KV_STOP") {
			KV_STOP = value;
		} else if (name == "KV_REFRESH") {
			KV_REFRESH = value;
		} else if (name == "KV_S_UP_UP") {
			KV_S_UP_UP = value;
		} else if (name == "KV_S_DOWN_DOWN") {
			KV_S_DOWN_DOWN = value;
		} else if (name == "KV_S_UP") {
			KV_S_UP = value;
		} else if (name == "KV_S_DOWN") {
			KV_S_DOWN = value;
		} else if (name == "KV_ISO") {
			KV_ISO = value;
		} else if (name == "RPI_EX") {
			RPI_EX = value;
		} else if (name == "DRIVE_NAME") {
			DRIVE_NAME = value;
		}
	} else {
		std::cerr << "Did not recognize entry " << name << " in config file, skipping" << std::endl;
	}
	return 0;
}

/**
 * Main function
 *
 * @return status
 */
int main (int argc, char **argv) {
	
	if (startup_disk_check()) {
		return 1;
	}
	
	// Parse config file
	std::string config_file = "./settings.cfg";
	std::ifstream cFile (config_file);
	if (cFile.is_open()) {
		std::string line;
		while(getline(cFile, line)){
			line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
			if(line[0] == '#' || line.empty()) {
				continue;
			}
			auto delimiter_pos = line.find("=");
			std::string name = line.substr(0, delimiter_pos);
			std::string value = line.substr(delimiter_pos + 1);
			if (parse_checklist(name, value)) {
				return 1;
			}
		}
	}
	else {
		notify_handler("LunAero Error", "Couldn't open config file for reading.");
		return 1;
	}
	
	
	// Make folder for stuff
	TSBUFF = current_time(0);
	FILEPATH = DEFAULT_FILEPATH + TSBUFF;
	mkdir(FILEPATH.c_str(), 0700);
	
	if (DEBUG_COUT) {
		LOGOUT = FILEPATH + "log.log";
		LOGGING.open(LOGOUT);
		LOGGING.close();
	}
	
	// Add startup disk check messages to log file
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< DISK_OUTPUT[0]
		<< std::endl
		<< DISK_OUTPUT[1]
		<< std::endl;
		LOGGING.close();
	}
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "time: " << TSBUFF << std::endl
		<< "path: " << FILEPATH << std::endl;
		LOGGING.close();
	}
		
	// Screensaver settings for the raspberry pi
	system("xset -dpms");
	system("xset s off");
	
	int status = 0;
	
	// init SUBS semaphore
	sem_init(&LOCK, 1, 1);
	
	// Make ID file
	if (create_id_file()) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: Failed to create ID file" << std::endl;
			LOGGING.close();
		}
	}
	
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

		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "waiting for child exit signal 2" << std::endl;
			LOGGING.close();
		}
		wait(0);
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "caught second wait" << std::endl;
			LOGGING.close();
		}
	} else {
		// Child process 1
		int pid2 = fork();
		if (pid2 > 0) {
			// Parent Process 2
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "started child proc 2" << std::endl;
				LOGGING.close();
			}
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
					if (DEBUG_COUT) {
						LOGGING.open(LOGOUT, std::ios_base::app);
						LOGGING
						<< "refreshing camera" << std::endl;
						LOGGING.close();
					}
					sem_wait(&LOCK);
					OLD_RECORD_TIME = std::chrono::system_clock::now();
					*val_ptr.SUBSaddr = 2;
					sem_post(&LOCK);
				}
				// This doesn't have to be super accurate, so only do it every 5 seconds
				usleep(5000000);
			}
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "caught abort code: "
				<< *val_ptr.ABORTaddr
				<< " run mode: "
				<< *val_ptr.RUN_MODEaddr
				<< std::endl;
				LOGGING.close();
			}
			kill_raspivid();
			
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "waiting for child exit signal 1" << std::endl;
				LOGGING.close();
			}
			wait(0);
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "caught first wait" << std::endl;
				LOGGING.close();
			}
			
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "SIGCHLD camera" << std::endl;
				LOGGING.close();
			}
			exit(SIGCHLD);
		} else {
			// child process of 2
			// Init app
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "preparing app" << std::endl;
				LOGGING.close();
			}
			//~ int LOST_COUNTER = 0;
			gtk_class::app = gtk_application_new("org.gtk.example", G_APPLICATION_FLAGS_NONE);
			g_signal_connect(gtk_class::app, "activate", G_CALLBACK (activate), NULL);
			status = g_application_run(G_APPLICATION(gtk_class::app), argc, argv);

			// Cleanup GTK
			g_object_unref(gtk_class::app);
			
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< "SIGCHLD gtk" << std::endl;
				LOGGING.close();
			}
			exit(SIGCHLD);
		}
	}
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "closing program" << std::endl;
		LOGGING.close();
	}
	//~ usleep(2000000);
	
	// Undo our screensaver settings
	system("xset +dpms");
	system("xset s on");
	
	return status;
}
