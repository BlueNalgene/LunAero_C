#include "camera_LunAero.hpp"

int confirm_mmal_safety(int error_cnt) {
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
	return 0;
}

void camera_start() {
	std::cout << "\n\nNOW RECORDING\n\nPATH: " << FILEPATH << std::endl;
	// Call preview of camera
	std::string commandstring = "";
	commandstring = command_cam_start();
	
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

std::string command_cam_start() {
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

std::string command_cam_preview() {
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
