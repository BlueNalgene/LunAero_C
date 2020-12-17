# LunAero_C/focus.py - Script to preview the camera output for focus and brightness adjustment
# Copyright (C) <2020>  <Wesley T. Honeycutt>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


##@file focus.py
##@brief Script to preview the camera output for focus and brightness adjustment prior to LunAero run.
##
##This script uses pygame to create a simple previewing environment for the LunAero hardware output.  A
##fullscreen window is created, calculating the display size of the unit's monitor.  Instructions are
##printed on an initial view to inform the user of the functions available to them and the associated key
##bindings.  Upon confirmation, the window updates, removing the full instructions, leaving only a limited
##version of the instructions.  This window leaves much more space for the video.  A raspberry pi camera
##preview window is called and placed over this screen.  A red rectangle in the back makes it easier for the
##user to recognize the edges of this window in dark conditions.  Motor commands, camera ISO settings, and
##shutter speed settings are available to the user.  The user is encouraged to focus his or her scope
##during this phase.  Pressing q exits.
##
##@section example_focus Usage Example
##@verbatim
##python3 ./focus.py
##@endverbatim
##
##@section libraries_focus Libraries/Modules
##- pygame (tested with version 1.9.4.post1)
##- picamera (tested with version 1.13)
##- RPi.GPIO (tested with version 0.7.0)

# Standard imports
import time

# Non-standard imports
import pygame

# Third Party imports
import picamera
import RPi.GPIO as GPIO

"""!
Set this debugging output switch to True to enable verbose debugging information to be printed to STDOUT.
Setting it to False is much quieter.
"""
DEBUG = True

"""!
PWM operation frequency in Hz
"""
FREQ = 10000

"""!
Duty cycle of Motor A (vertical motion).  Initially set to 0%
"""
DCA = 0

"""!
Duty cycle of Motor B (horizontal motion).  Initially set to 0%
"""
DCB = 0

"""!
RGB tuple for black
"""
BLACK = (0, 0, 0)

"""!
RGB tuple for red
"""
RED = (255, 0, 0)


"""!
Raspberry Pi GPIO pin for motor A Soft PWM.  Wiring Pi equivalent of 17 = 0
"""
APINP = 17

"""!
Raspberry Pi GPIO pin for motor A 1 pin.  Wiring Pi equivalent of 27 = 2
"""
APIN1 = 27
"""!
Raspberry Pi GPIO pin for motor A 2 pin.  Wiring Pi equivalent of 22 = 3
"""
APIN2 = 22

"""!
Raspberry Pi GPIO pin for motor B 1 pin.  Wiring Pi equivalent of 10 = 12
"""
BPIN1 = 10

"""!
Raspberry Pi GPIO pin for motor B 2 pin.  Wiring Pi equivalent of 9 = 13
"""
BPIN2 = 9

"""!
Raspberry Pi GPIO pin for motor A Soft PWM.  BCM equivalent of 14 = 11
"""
BPINP = 11


"""!
ISO setting for the picamera.  The initial value is set here, and is changed later by the user.
"""
ISO = 200

"""!
Initial exposure value for the picamera.  The practical limits of this value depend on the camera version,
but are generally between 1-33000.  This initial value may be changed later by the user.
"""
EXP = 10000

"""!
The amount to increment the shutter speed/exposure when the user increases or decreases it.
"""
DIFF_EXP = 1000

"""!
This is an arbitrary max value for the shutter speed/exposure.  We set this to prevent the user from
hitting the true max of the camera, as hitting this max value (which is not stable between camera versions)
does strange things when overrun.
"""
MAX_EXP = 30000


##\cond DOXYGEN_SHOULD_SKIP_THIS
# Initialize Pygame
if DEBUG:
	print("initializing pygame...")
pygame.display.init()
pygame.font.init()
if DEBUG:
	print("pygame initialized.")


# Get screen size
PYG_INF = pygame.display.Info()

##\endcond /* DOXYGEN_SHOULD_SKIP_THIS */
"""!
Width of the current display, as calculated by pygame
"""
CURR_W = PYG_INF.current_w
"""!
Height of the current display, as calculated by pygame
"""
CURR_H = PYG_INF.current_h

##\cond DOXYGEN_SHOULD_SKIP_THIS
if DEBUG:
	print("This monitor is", CURR_W, "x", CURR_H, "px")


# Define GPIO pins and set them up in the "off" position
if DEBUG:
	print("Defining GPIO pins...")
GPIO.setmode(GPIO.BCM)
if DEBUG:
	print("GPIO pins defined")
	print("starting motors with speed zero")
PINS = (APIN1, APIN2, APINP, BPIN1, BPIN2, BPINP)
for eachpin in PINS:
	GPIO.setup(eachpin, GPIO.OUT)
	if eachpin != APINP or BPINP:
		GPIO.output(eachpin, GPIO.LOW)
	else:
		GPIO.output(eachpin, GPIO.HIGH)
##\endcond /* DOXYGEN_SHOULD_SKIP_THIS */

"""!
Pulse width modulation factor for the vertical motor.  Changing the duty cycle of this object changes
motor speed.
"""
PWMA = GPIO.PWM(APINP, FREQ)

"""!
Pulse width modulation factor for the horizontal motor.  Changing the duty cycle of this object changes
motor speed.
"""
PWMB = GPIO.PWM(BPINP, FREQ)

##\cond DOXYGEN_SHOULD_SKIP_THIS
# Startup the PWM with 0%
PWMA.start(DCA)
PWMB.start(DCB)
if DEBUG:
	print("motors started.  Speed = 0.")


# Prepping Camera
if DEBUG:
	print("prepping camera...")
##\endcond /* DOXYGEN_SHOULD_SKIP_THIS */

"""!
Object pointing to the Raspberry Pi camera.  The object is initialized here and called later.  Various
settings (not necessarily visible within Doxygen documentation) include disabling the camera's LED,
enabling video stabilization, forcing the resolution to 1920x1080, and setting the color of the output
to grayscale.
"""
CAM = picamera.PiCamera()

##\cond DOXYGEN_SHOULD_SKIP_THIS
CAM.led = False
CAM.video_stabilization = True
CAM.resolution = (1920, 1080)
# turn camera to black and white
CAM.color_effects = (128, 128)
if DEBUG:
	print("camera prepped.")
##\endcond /* DOXYGEN_SHOULD_SKIP_THIS */


def calculate_picamera_window():
	"""!
	This function calculates how large to create the camera preview window for the user.  We want it to be
	as large as possible, but leaving a little room for text reminders.  The size is based on 95% of one
	dimensions, and the second dimension is calculated based on the actual camera output size ratio.
	
	@returns [local_w, local_h], a list containing the calculated values for width and height.
	"""
	local_h = int(CURR_H * 0.95)
	ratio = CAM.resolution[0]/CAM.resolution[1]
	local_w = int(local_h * ratio)
	if DEBUG:
		print("Calculated picamera window size:", local_w, "x", local_h, "px")
	return [local_w, local_h]


def mot_down():
	"""!
	Void function to set the motor bits to decrease the scope's altitude
	"""
	global DCA
	DCA = 100
	if DEBUG:
		print("moving down")
	PWMA.ChangeDutyCycle(DCA)
	GPIO.output(APIN1, GPIO.HIGH)
	GPIO.output(APIN2, GPIO.LOW)
	return

def mot_up():
	"""!
	Void function to set the motor bits to increase the scope's altitude
	"""
	global DCA
	DCA = 100
	if DEBUG:
		print("moving up")
	PWMA.ChangeDutyCycle(DCA)
	GPIO.output(APIN1, GPIO.LOW)
	GPIO.output(APIN2, GPIO.HIGH)
	return

def mot_right():
	"""!
	Void function to set the motor bits for clockwise azimuth adjustment
	"""
	global DCB
	DCB = 100
	if DEBUG:
		print("moving right")
	PWMB.ChangeDutyCycle(DCB)
	GPIO.output(BPIN1, GPIO.HIGH)
	GPIO.output(BPIN2, GPIO.LOW)
	return

def mot_left():
	"""!
	Void function to set the motor bits for counter-clockwise azimuth adjustment
	"""
	global DCB
	DCB = 100
	if DEBUG:
		print("moving left")
	PWMB.ChangeDutyCycle(DCB)
	GPIO.output(BPIN1, GPIO.LOW)
	GPIO.output(BPIN2, GPIO.HIGH)
	return

def mot_stop():
	"""!
	Void function to stop the motors.  At high duty cycles, jumps to 10%.  At low duty cycles, the output
	duty cycle is decreased by 1% each loop until reaching zero.  This gradual speed reduction prevents
	jerky motion which may mess with the hardware.
	"""
	global DCA
	global DCB
	if DEBUG:
		print("stopping",)
	while DCA > 0 or DCB > 0:
		if DCA == 100:
			DCA = 10     #quickly stop motor going full speed
		if DCB == 100:
			DCB = 10
		if DCA > 0:
			DCA = DCA - 1   #slowly stop motor going slow (tracking moon)
		if DCB > 0:
			DCB = DCB - 1
		PWMA.ChangeDutyCycle(DCA)
		PWMB.ChangeDutyCycle(DCB)
		time.sleep(.005)
	GPIO.output(APIN1, GPIO.LOW)
	GPIO.output(APIN2, GPIO.LOW)
	GPIO.output(BPIN1, GPIO.LOW)
	GPIO.output(BPIN2, GPIO.LOW)
	return

def cycle_iso():
	"""!
	Void function which cycles the ISO value for the picamera to the next valid cycle (100, 200, 400, 800)
	"""
	global ISO
	if ISO < 800:
		ISO = ISO * 2
	else:
		ISO = 100
	CAM.iso = ISO
	if DEBUG:
		print("iso set to ", ISO)
	return

def decrease_exposure():
	"""!
	Void function which decreases the exposure without letting it get all the way to 0
	"""
	global EXP
	if EXP < DIFF_EXP+1:
		EXP = 10
	else:
		EXP = EXP - DIFF_EXP
	CAM.shutter_speed = EXP
	if DEBUG:
		print("exposure time set to ", EXP)
	return

def increase_exposure():
	"""!
	Void function which increases the exposure without letting it get all the way to our arbitrary MAX_EXP
	"""
	global EXP
	if EXP > MAX_EXP-1:
		EXP = MAX_EXP
	else:
		EXP = EXP + DIFF_EXP
	CAM.shutter_speed = EXP
	if DEBUG:
		print("exposure time set to ", EXP)



def directions_screen(screen, font, margin, font_size):
	"""!
	This is the generator for the initial directions screen.  Prints red text on the black screen, and
	returns nothing.  The blit changes are not activated until the pygame.display.update() is called.
	
	@param screen Pygame object representing the display we are operating on
	@param font Pygame object holding the font information
	@param margin int value of how far to space text lines (our margins).  Additionally, this influences
	text position.
	@param font_size int value for how large our font should be.  Additionally, this influences text
	position.
	"""
	# First window with directions
	text_lines = ["-----------------FOCUS CONTROLS-----------------", \
				"", \
				"  Use arrow keys to move scope", \
				"", \
				"  [SPACEBAR] stops motors", \
				"", \
				"  [i] - Cycle camera ISO mode", \
				"", \
				"  [g] - Increase exposure", \
				"", \
				"  [b] - Decrease exposure", \
				"", \
				"  [q] - Exit this program", \
				"", \
				"Press [ENTER] to continue..." \
					]
	
	for i, line in enumerate(text_lines):
		if DEBUG:
			print(line)
		screen.blit(font.render(line, True, RED), (margin, i*(margin+font_size)))
	return

def ops_screen(screen, font, font_size):
	"""!
	This is the generator for the operating window.  Prints red text at the tippy top of the screen, and
	makes a red rectangle slightly larger than the expected preview window size.  Also calls the camera
	to start the preview window.  The blit changes are not activated until the pygame.display.update()
	is called.
	
	@param screen Pygame object representing the display we are operating on
	@param font Pygame object holding the font information
	@param font_size int value for how large our font should be.  In this screen, it is used to add margins
	to the red rectangle.
	"""
	# display text
	screen.blit(font.render("q=exit, g/b=exp, i=iso, dir move, space=stop", True, RED), (1, 1))
	
	# call camera on bottom left of screen
	prev_w, prev_h = calculate_picamera_window()
	CAM.start_preview(fullscreen=False, window=(0, CURR_H-prev_h, prev_w, prev_h))
	
	pygame.draw.rect(screen, RED, (0, CURR_H-prev_h-int(font_size/10), prev_w+int(font_size/5), prev_h))
	return












def main():
	"""!
	This is the main execution script.  The windows are called in order.  Pygame events are used to capture
	keypresses from the user.  These are hardcoded key bindings.  Once the code is complete, or in
	SystemExit conditions, cleanup methods are called for GPIO, picamera, and pygame if they were
	initialized	previously.
	"""
	# Calculate font size
	font_size = int(CURR_H/20)
	margin = int(font_size/3)

	# Declaire Font
	font = pygame.font.SysFont('monospace', font_size)


	# Prep the window
	pygame.display.set_caption("Picamera Focus")
	screen = pygame.display.set_mode([CURR_W, CURR_H], pygame.FULLSCREEN)

	# Blackout
	screen.fill(BLACK)
	pygame.display.update()
		
	# Display directions
	directions_screen(screen, font, margin, font_size)
	pygame.display.update()
	
	exit_cond = True
	while exit_cond:
		for event in pygame.event.get():
			if event.type == pygame.QUIT:
				if DEBUG:
					print("Caught a window exit press")
				GPIO.cleanup()
				raise SystemExit
			elif event.type == pygame.KEYDOWN:
				if event.key == pygame.K_RETURN:
					print("caught enter")
					exit_cond = False

	# Blackout
	screen.fill(BLACK)
	pygame.display.update()
	
	# Operations Screen
	ops_screen(screen, font, font_size)
	pygame.display.update()
	
	exit_cond = True
	while exit_cond:
		for event in pygame.event.get():
			if event.type == pygame.QUIT:
				if DEBUG:
					print("Caught a window exit press")
				exit_cond = False
			elif event.type == pygame.KEYDOWN:
				if event.key == pygame.K_i:
					cycle_iso()
				if event.key == pygame.K_g:
					increase_exposure()
				if event.key == pygame.K_b:
					decrease_exposure()
				if event.key == pygame.K_UP:
					mot_up()
				if event.key == pygame.K_RIGHT:
					mot_right()
				if event.key == pygame.K_DOWN:
					mot_down()
				if event.key == pygame.K_LEFT:
					mot_left()
				if event.key == pygame.K_SPACE:
					mot_stop()
				if event.key == pygame.K_q:
					if DEBUG:
						print("User pressed q to exit")
					exit_cond = False
	
	CAM.stop_preview()
	GPIO.cleanup()
	pygame.quit()
	return


if __name__ == "__main__":
	main()
