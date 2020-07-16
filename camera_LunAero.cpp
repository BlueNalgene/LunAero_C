#include "camera_LunAero.hpp"

void camera_start () {
	std::cout << "\n\nNOW RECORDING\n\nPATH: " << FILEPATH << std::endl;
	// Call preview of camera
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
	+ "outA.h264 &";
	std::cout << "Using the command: " << commandstring << std::endl;
	system(commandstring.c_str());
}

void camera_preview() {
	std::string commandstring;
	commandstring = "killall raspivid";
	system(commandstring.c_str());
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
	+ " &";
	std::cout << "Using the command: " << commandstring << std::endl;
	system(commandstring.c_str());
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
