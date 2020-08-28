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

int confirm_filespace() {
	std::cout << "Confirming filespace" << std::endl;
	namespace fs = std::filesystem;
	fs::space_info tmp = fs::space(DEFAULT_FILEPATH);
	if (tmp.available < (1000000 * std::chrono::duration<double>(RECORD_DURATION).count())) {
		std::cout << "ERROR: The space on this drive is too low with "
		<< tmp.available << " bytes remaining, exiting." << std::endl;
		return 1;
	}
	return 0;
}

int confirm_mmal_safety(int error_cnt) {
	std::cout << "mmal safety count: " << error_cnt << std::endl;
	// If the retry attempts are way too high, don't even bother
	if (error_cnt > 100) {
		std::cout << "ERROR: LunAero detected repeating MMAL problems.  Exiting" << std::endl;
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
			std::cout << line << std::endl;
			if (line.find(str_mmal) != std::string::npos) {
				std::cout << "WARNING: LunAero detected an MMAL problem with raspivid.  Retrying" << std::endl;
				kill_raspivid();
				if (error_cnt > 100) {
					sem_wait(&LOCK);
					*val_ptr.ABORTaddr = 1;
					sem_post(&LOCK);
				} else {
					// Alternate kill method if "pidof" doesn't work right
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

void camera_start() {
	std::cout << "\n\nNOW RECORDING\n\nPATH: " << FILEPATH << std::endl;
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
	+ std::to_string((WORK_WIDTH/2)-(WORK_WIDTH/4))
	+ ","
	+ std::to_string(WORK_HEIGHT/2)
	+ ","
	+ std::to_string(WORK_WIDTH/2)
	+ ","
	+ std::to_string(WORK_HEIGHT/2)
	+ " > /tmp/raspivid.log 2>&1 &";
	std::cout << "Using the command: " << commandstring << std::endl;
	return commandstring;
}

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
	+ std::to_string((WORK_WIDTH/2)-(WORK_WIDTH/4))
	+ ","
	+ std::to_string(WORK_HEIGHT/2)
	+ ","
	+ std::to_string(WORK_WIDTH/2)
	+ ","
	+ std::to_string(WORK_HEIGHT/2)
	+ " -o "
	+ FILEPATH
	+ "/"
	+ TSBUFF
	+ "outA.h264 > /tmp/raspivid.log 2>&1 &";
	std::cout << "Using the command: " << commandstring << std::endl;
	
	return commandstring;
}

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

void first_record() {
	kill_raspivid();
	sem_wait(&LOCK);
	//~ *val_ptr.RUN_MODEaddr = 1;
	*val_ptr.DUTY_Aaddr = 20;
	*val_ptr.DUTY_Baddr = 20;
	sem_post(&LOCK);
	camera_start();
	std::cout << "-----------------\nRECORDING STARTED\n-----------------\n" << std::endl;
}

void reset_record() {
	kill_raspivid();
	usleep(1000000);
	camera_start();
}

void refresh_camera() {
	kill_raspivid();
	sem_wait(&LOCK);
	*val_ptr.REFRESH_CAMaddr = 1;
	sem_post(&LOCK);
	camera_preview();
}

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
	std::cout << "SHUTTER_VAL: " << *val_ptr.SHUTTER_VALaddr << std::endl;
}

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
	std::cout << "SHUTTER_VAL: " << *val_ptr.SHUTTER_VALaddr << std::endl;
}

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
	std::cout << "SHUTTER_VAL: %d\n" << *val_ptr.SHUTTER_VALaddr << std::endl;
}

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
	std::cout << "SHUTTER_VAL: " << *val_ptr.SHUTTER_VALaddr << std::endl;
}

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
	std::cout << "ISO_VAL: " << *val_ptr.ISO_VALaddr << std::endl;
}
