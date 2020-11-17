/*
 * C_LunAero/camera_LunAero.cpp - Camera control functions for LunAero_C
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

#include "camera_LunAero.hpp"

/**
 * This function confirms that there is enought space on the output drive for a new video to be saved.
 * While this is similar to LunAero.cpp/startup_disk_check, it only checks the available space rather
 * than the drive integrity.  And, it does not predictively check for low hard drive space for future
 * videos.  If the check fails, a positive status is returned and the program ends.
 *
 * @return status
 */
int confirm_filespace() {
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Confirming filespace" << std::endl;
		LOGGING.close();
	}
	namespace fs = std::filesystem;
	fs::space_info tmp = fs::space(DEFAULT_FILEPATH);
	if (tmp.available < (1000000 * std::chrono::duration<double>(RECORD_DURATION).count())) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: The space on this drive is too low with "
			<< tmp.available << " bytes remaining, exiting." << std::endl;
			LOGGING.close();
		}
		return 1;
	}
	return 0;
}

/**
 * This function checks the integrity of the MMAL camera connection.  This prevents improper restarts
 * of the raspivid program caused by quick successive stops and starts.  Originally, on these restarts,
 * the MMAL may fail for a few microseconds after a shutdown since something had not finished clearing
 * in the background (black magic).  This simply handles a few failures before deciding that the MMAL
 * device is not actually connected and ending the program.
 *
 * @return status
 */
int confirm_mmal_safety(int error_cnt) {
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "mmal safety count: " << error_cnt << std::endl;
		LOGGING.close();
	}
	// If the retry attempts are way too high, don't even bother
	if (error_cnt > 100) {
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING
			<< "ERROR: LunAero detected repeating MMAL problems.  Exiting" << std::endl;
			LOGGING.close();
		}
		kill_raspivid();
		sem_wait(&LOCK);
		*val_ptr.ABORTaddr = 1;
		sem_post(&LOCK);
		return 101;
	}
	// source_command == 1 for preview, 2 for recording
	usleep(1000000);
	std::string str_mmal = "mmal:";
	std::ifstream file("/tmp/raspivid.log");
	if (file.is_open()) {
		std::string line;
		while (std::getline(file, line)) {
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING
				<< line << std::endl;
				LOGGING.close();
			}
			if (line.find(str_mmal) != std::string::npos) {
				if (DEBUG_COUT) {
					LOGGING.open(LOGOUT, std::ios_base::app);
					LOGGING
					<< "WARNING: LunAero detected an MMAL problem with raspivid.  Retrying" << std::endl;
					LOGGING.close();
				}
				if (error_cnt > 100) {
					sem_wait(&LOCK);
					*val_ptr.ABORTaddr = 1;
					sem_post(&LOCK);
				} else {
					// Alternate kill method if "pidof" doesn't work right
					if (DEBUG_COUT) {
						LOGGING.open(LOGOUT, std::ios_base::app);
						LOGGING
						<< "attempting killall" << std::endl;
						LOGGING.close();
					}
					std::string commandstring = "killall raspivid";
					system(commandstring.c_str());
				}
				file.close();
				error_cnt += 1;
				return error_cnt;
			}
			
		}
		file.close();
	}
	
	write_video_id();
	
	return 0;
}

/**
 * This functions ties together multiple functions to 1) confirm_filespace 2) confirm_mmal_safety 3)
 * execute the command constructed by command_cam_start.
 *
 *
 */
void camera_start() {
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "\n\nNOW RECORDING\n\nPATH: " << FILEPATH << std::endl;
		LOGGING.close();
	}
	// Call preview of camera
	std::string commandstring = "";
	commandstring = command_cam_start();
	
	if (confirm_filespace()) {
		sem_wait(&LOCK);
		*val_ptr.ABORTaddr = 1;
		sem_post(&LOCK);
		usleep(1000000);
		return;
	}
	
	int mmal_safety_outcome = 1;
	
	while (mmal_safety_outcome) {
		system(commandstring.c_str());
		mmal_safety_outcome = confirm_mmal_safety(mmal_safety_outcome);
		usleep(1000000);
	}
	return;
}

/**
 * This command starts the preview screen using raspivid.  The command is constructed based on
 * command_cam_preview and the MMAL integrity is checked with mmal_safety_outcome.
 *
 *
 */
void camera_preview() {
	std::string commandstring = "killall raspivid";
	system(commandstring.c_str());
	commandstring = command_cam_preview();
	
	int mmal_safety_outcome = 1;
	
	while (mmal_safety_outcome) {
		system(commandstring.c_str());
		mmal_safety_outcome = confirm_mmal_safety(mmal_safety_outcome);
		usleep(1000000);
	}
	return;
}

/**
 * This function constructs the command string to call a raspivid preview.  The size of the mini screen
 * determined by other functions and used to construct the window.
 *
 * @return commandstring the constructed command formatted as a string
 */
std::string command_cam_preview() {
	std::string commandstring;
	// Get the current unix timestamp as a string
	TSBUFF = std::to_string((unsigned long)time(NULL));
	commandstring = "raspivid -v -t 0 -w 1920 -h 1080 -fps 30 -b 8000000 -ISO " 
	+ std::to_string(*val_ptr.ISO_VALaddr)
	+ " -ss " 
	+ std::to_string(*val_ptr.SHUTTER_VALaddr)
	+ " --exposure auto "
	+ " -p "
	+ std::to_string(RVD_XCORN)
	+ ","
	+ std::to_string(RVD_YCORN)
	+ ","
	+ std::to_string(RVD_WIDTH)
	+ ","
	+ std::to_string(RVD_HEIGHT)
	+ " > /tmp/raspivid.log 2>&1 &";
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Using the command: " << commandstring << std::endl;
		LOGGING.close();
	}
	return commandstring;
}

/**
 * This function constructs the command string to call a raspivid recording and preview window.  The 
 * size of the mini screen determined by other functions and used to construct the preview window.  The
 * save location of the video is determined by the current timestamp.
 *
 * @return commandstring the constructed command formatted as a string
 */
std::string command_cam_start() {
	std::string commandstring;
	// Get the current unix timestamp as a string
	TSBUFF = current_time(0);
	commandstring = "raspivid -v -t 0 -w 1920 -h 1080 -fps 30 -b 8000000 -ISO " 
	+ std::to_string(*val_ptr.ISO_VALaddr)
	+ " -ss " 
	+ std::to_string(*val_ptr.SHUTTER_VALaddr)
	+ " --exposure auto "
	+ " -p "
	+ std::to_string(RVD_XCORN)
	+ ","
	+ std::to_string(RVD_YCORN)
	+ ","
	+ std::to_string(RVD_WIDTH)
	+ ","
	+ std::to_string(RVD_HEIGHT)
	+ " -o "
	+ FILEPATH
	+ "/"
	+ TSBUFF
	+ "outA.h264 > /tmp/raspivid.log 2>&1 &";
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Using the command: " << commandstring << std::endl;
		LOGGING.close();
	}
	
	return commandstring;
}

/**
 * This function creates a metadata file for each recording called by LunAero.  The metadata includes
 * the recording values set by the command constructed in command_cam_start.  The file is stored in the
 * same directory as the video.
 *
 *
 */
void write_video_id() {
	std::ofstream idfile;
	idfile.open(IDPATH, std::ios_base::app);
	idfile 
	<< "File: " 
	<< TSBUFF << "outA.h264"
	<< std::endl
	<< "    Width:         1920"
	<< std::endl
	<< "    Height:        1080"
	<< std::endl
	<< "    ISO:           "
	<< std::to_string(*val_ptr.ISO_VALaddr)
	<< std::endl
	<< "    Shutter Speed: "
	<< std::to_string(*val_ptr.SHUTTER_VALaddr)
	<< std::endl
	<< "    Bitrate:       8000000"
	<< std::endl
	<< "    Framerate:     30"
	<< std::endl;
	
	idfile.close();
}

/**
 * This function handles the first recording.  This is distinct as we need to kill the preview window
 * and ensure that the motors are in an initial state.  If we don't set the duty cycles for the motors
 * to the minimum here, the motors start too aggressive and lose the target.
 *
 *
 */
void first_record() {
	kill_raspivid();
	sem_wait(&LOCK);
	//~ *val_ptr.RUN_MODEaddr = 1;
	*val_ptr.DUTY_Aaddr = 20;
	*val_ptr.DUTY_Baddr = 20;
	sem_post(&LOCK);
	camera_start();
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "-----------------\nRECORDING STARTED\n-----------------\n" << std::endl;
		LOGGING.close();
	}
}

/**
 * This helper function kills raspivid and starts recording for each video restart subsequent to the
 * initial recording.
 *
 *
 */
void reset_record() {
	kill_raspivid();
	usleep(1000000);
	camera_start();
}

/**
 * This helper function is called by a GTK button and is used to kill the existing preview window and
 * replace it with a new window based on the latest ISO/Shutter values.
 *
 *
 */
void refresh_camera() {
	kill_raspivid();
	sem_wait(&LOCK);
	*val_ptr.REFRESH_CAMaddr = 1;
	sem_post(&LOCK);
	camera_preview();
}

/**
 * This function called from gtk_LunAero handles a request to increase the shutter value.  The maximum
 * shutter value is locked in here and prevents a shutter value from extending beyond this limit.
 *
 *
 */
void shutter_up() {
	if (*val_ptr.SHUTTER_VALaddr < 32901) {
		sem_wait(&LOCK);
		*val_ptr.SHUTTER_VALaddr = *val_ptr.SHUTTER_VALaddr + 100;
		sem_post(&LOCK);
	} else {
		sem_wait(&LOCK);
		*val_ptr.SHUTTER_VALaddr = 33000;
		sem_post(&LOCK);
	}
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "SHUTTER_VAL: " << *val_ptr.SHUTTER_VALaddr << std::endl;
		LOGGING.close();
	}
}

/**
 * This function called from gtk_LunAero handles a request to decrease the shutter value.  The minimum
 * shutter value is locked in here and prevents a shutter value from extending below this limit.
 *
 *
 */
void shutter_down() {
	if (*val_ptr.SHUTTER_VALaddr > 110) {
		sem_wait(&LOCK);
		*val_ptr.SHUTTER_VALaddr = *val_ptr.SHUTTER_VALaddr - 100;
		sem_post(&LOCK);
	} else {
		sem_wait(&LOCK);
		*val_ptr.SHUTTER_VALaddr = 10;
		sem_post(&LOCK);
	}
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "SHUTTER_VAL: " << *val_ptr.SHUTTER_VALaddr << std::endl;
		LOGGING.close();
	}
}

/**
 * This function called from gtk_LunAero handles a request to greatly increase the shutter value.  The
 * maximum shutter value is locked in here and prevents a shutter value from extending beyond this limit.
 *
 *
 */
void shutter_up_up() {
	if (*val_ptr.SHUTTER_VALaddr < 32001) {
		sem_wait(&LOCK);
		*val_ptr.SHUTTER_VALaddr = *val_ptr.SHUTTER_VALaddr + 1000;
		sem_post(&LOCK);
	} else {
		sem_wait(&LOCK);
		*val_ptr.SHUTTER_VALaddr = 33000;
		sem_post(&LOCK);
	}
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "SHUTTER_VAL: \n" << *val_ptr.SHUTTER_VALaddr << std::endl;
		LOGGING.close();
	}
}

/**
 * This function called from gtk_LunAero handles a request to greatly decrease the shutter value.  The
 * minimum shutter value is locked in here and prevents a shutter value from extending below this limit.
 *
 *
 */
void shutter_down_down() {
	if (*val_ptr.SHUTTER_VALaddr > 1010) {
		sem_wait(&LOCK);
		*val_ptr.SHUTTER_VALaddr = *val_ptr.SHUTTER_VALaddr - 1000;
		sem_post(&LOCK);
	} else {
		sem_wait(&LOCK);
		*val_ptr.SHUTTER_VALaddr = 10;
		sem_post(&LOCK);
	}
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "SHUTTER_VAL: " << *val_ptr.SHUTTER_VALaddr << std::endl;
		LOGGING.close();
	}
}

/**
 * This function handles the ISO cycle from gtk_LunAero.  Since there are only 4 valid ISO values (100,
 * 200, 400, 800) for the Raspberry Pi camera, it cycles through these values.
 *
 *
 */
void iso_cycle() {
	if (*val_ptr.ISO_VALaddr == 200) {
		sem_wait(&LOCK);
		*val_ptr.ISO_VALaddr = 400;
		sem_post(&LOCK);
	} else if (*val_ptr.ISO_VALaddr == 400) {
		sem_wait(&LOCK);
		*val_ptr.ISO_VALaddr = 800;
		sem_post(&LOCK);
	} else if (*val_ptr.ISO_VALaddr == 800) {
		sem_wait(&LOCK);
		*val_ptr.ISO_VALaddr = 100;
		sem_post(&LOCK);
	} else if (*val_ptr.ISO_VALaddr == 100) {
		sem_wait(&LOCK);
		*val_ptr.ISO_VALaddr = 200;
		sem_post(&LOCK);
	}
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "ISO_VAL: " << *val_ptr.ISO_VALaddr << std::endl;
		LOGGING.close();
	}
}
