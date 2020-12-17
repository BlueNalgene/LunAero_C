#Motor control and recording for Lunaero

#Motor A is up and down
#Motor B is right and left

# Standard imports
import io
import os
import os.path
import time

# Non-standard imports
from PIL import Image
import pygame

# Third Party imports
import numpy as np
import picamera
import RPi.GPIO as GPIO

# Debugging output switch
DEBUG = True


# Initialize Pygame
if DEBUG:
	print("initializing pygame...")
pygame.display.init()
pygame.font.init()
if DEBUG:
	print("pygame initialized.")


# Get screen size
PYG_INF = pygame.display.Info()
CURR_W = PYG_INF.current_w
CURR_H = PYG_INF.current_h
if DEBUG:
	print("This monitor is", CURR_W, "x", CURR_H, "px")


# Defines the pins being used for the GPIO pins.
if DEBUG:
	print("Defining GPIO pins...")
GPIO.setmode(GPIO.BCM)
APINP = 17  #Pulse width pin for motor A (up and down)
APIN1 = 27  #Motor control - high for up
APIN2 = 22  #Motor control - high for down
BPIN1 = 10  #Motor control - high for left
BPIN2 = 9   #Motor control - high for right
BPINP = 11  #Pulse width pin for motor B (right and left)
if DEBUG:
	print("GPIO pins defined")


# Setup GPIO and start them with 'off' values
if DEBUG:
	print("starting motors with speed zero")
PINS = (APIN1, APIN2, APINP, BPIN1, BPIN2, BPINP)
for i in PINS:
	GPIO.setup(i, GPIO.OUT)
	if i != APINP or BPINP:
		GPIO.output(i, GPIO.LOW)
	else:
		GPIO.output(i, GPIO.HIGH)

FREQ = 10000
PWMA = GPIO.PWM(APINP, FREQ)   # Initialize PWM on pwmPins
PWMB = GPIO.PWM(BPINP, FREQ)
DCA = 0                          # Set duty cycle variable to zero at first
DCB = 0                         # Set duty cycle variable to zero at first
PWMA.start(DCA)                # Start pulse width at 0 (pin held low)
PWMB.start(DCB)                # Start pulse width at 0 (pin held low)
if DEBUG:
	print("motors started.  Speed = 0.")


# Colors
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
RED = (255, 0, 0)




# Prepping Camera
#stream = io.BytesIO()
if DEBUG:
	print("prepping camera...")
CAM = picamera.PiCamera()
CAM.led = False
CAM.video_stabilization = True
CAM.resolution = (1920, 1080)
CAM.color_effects = (128, 128) # turn camera to black and white
# initial ISO value
ISO = 200
# initial exposure/shutter speed
EXP = 10000
# increment to increase/decease exposure
DIFF_EXP = 1000
# maximum allowable exposure
MAX_EXP = 30000
if DEBUG:
	print("camera prepped.")



def calculate_picamera_window():
	"""
	"""
	local_h = int(CURR_H * 0.85)
	ratio = CURR_W/CURR_H
	local_w = int(local_h * ratio)
	if DEBUG:
		print("Calculated picamera window size:", local_w, "x", local_h, "px")
	return [local_w, local_h]


def mot_down():
	if DEBUG:
		print("moving down")
	GPIO.output(APIN1, GPIO.HIGH)
	GPIO.output(APIN2, GPIO.LOW)
	return

def mot_up():
	if DEBUG:
		print("moving up")
	GPIO.output(APIN1, GPIO.LOW)
	GPIO.output(APIN2, GPIO.HIGH)
	return

def mot_right():
	if DEBUG:
		print("moving right")
	GPIO.output(BPIN1, GPIO.HIGH)
	GPIO.output(BPIN2, GPIO.LOW)
	return

def mot_left():
	if DEBUG:
		print("moving left")
	GPIO.output(BPIN1, GPIO.LOW)
	GPIO.output(BPIN2, GPIO.HIGH)
	return

def mot_stop():
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
		pwmA.ChangeDutyCycle(DCA)
		pwmB.ChangeDutyCycle(DCB)
		time.sleep(.005)
	GPIO.output(APIN1, GPIO.LOW)
	GPIO.output(APIN2, GPIO.LOW)
	GPIO.output(BPIN1, GPIO.LOW)
	GPIO.output(BPIN2, GPIO.LOW)
	return

def cycle_iso():
	"""
	"""
	if ISO < 800:
		ISO = ISO * 2
	else:
		ISO = 100
	CAM.iso = ISO
	if DEBUG:
		print("iso set to ", ISO)
	return

def decrease_exposure():
	"""
	"""
	
	if EXP < DIFF_EXP:
		EXP = 10
	else:
		EXP = EXP - DIFF_EXP
	CAM.shutter_speed = EXP
	if DEBUG:
		print("exposure time set to ", EXP)
	return

def increase_exposure():
	"""
	"""
	
	if EXP > MAX_EXP:
		EXP = MAX_EXP
	else:
		EXP = EXP + DIFF_EXP
	CAM.shutter_speed = EXP
	if DEBUG:
		print("exposure time set to ", EXP)



def directions_screen(screen, font, margin, font_size):
	"""
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

def ops_screen(screen, font, margin, font_size):
	"""
	"""
	# display text
	screen.blit(font.render("q=exit, g/b=exp, i=iso, dir move, space=stop", True, RED, (1, 1)))
	
	# call camera on bottom left of screen
	prev_w, prev_h = calculate_picamera_window()
	CAM.start_preview(fullscreen=False, window=(0, CURR_H-prev_h, prev_w, prev_h))
	
	pygame.draw.rect(screen, RED, (0, CURR_H-prev_h-int(font_size/5), prev_w+int(font_size/5), prev_h))
	
	return












def main():
	"""
	"""
	# Calculate font size
	font_size = int(CURR_H/20)
	margin = int(font_size/3)

	# Declaire Font
	font = pygame.font.SysFont('monospace', font_size)


	# Prep the window
	pygame.display.set_caption("Picamera Focus")
	screen = pygame.display.set_mode([CURR_W, CURR_H])

	# Blackout
	screen.fill(BLACK)
	pygame.display.update()
		
	# Display directions
	directions_screen(screen, font, margin, font_size)
	pygame.display.update()
	
	exit_cond = True
	while (exit_cond):
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
	ops_screen(screen, font, margin, font_size)
	pygame.display.update()
	
	exit_cond = True
	while (exit_cond):
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
					motdown()
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

