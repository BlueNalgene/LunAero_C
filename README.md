![The first fully automated lunar bird tracker](https://i.imgur.com/jRGf8h6h.jpg "The first fully automated lunar bird tracker")

# The LunAero Project
LunAero is a hardware and software project using OpenCV to automatically
track birds as they migrate in front of the moon.  This repository contains
information related to the construction of LunAero hardware and the
software required for running the moon tracking program.  Bird detection
has "migrated" to a new repository at
https://github.com/BlueNalgene/CPP_Birdtracker.

Created by @BlueNalgene, working in the lab of @Eli-S-Bridge.

# LunAero_C vs. LunAero

The LunAero_C repository is an updated version of the LunAero robotics
software.  The previous implementation
(archived at https://github.com/BlueNalgene/LunAero) was a Python 3
script which operated hardware based on the ODROID N2 single board
computer.  This updated version is C++ code designed for the Raspberry
Pi v3, and includes many improvements, additional features, and is
generally more mature code.

## Hardware Recommendations

- Raspberry Pi v3
- USB 3 drive with transfer rate of at least 200 MB/s
- Raspberry Pi camera module
- Motors (2)
- TB6612FNG motor controller
- 1/4" Acrylic (about 18"x24")
- 1/8" Acetal (about 3"x3")
- Screws and Wires

This new version of LunAero requires a Raspberry Pi v3 single board
computer.  We recommend running this on the Raspbian operating system,
although most linux distros should work fine (untested, so use at your
own risk).  The reason we require v3 hardware is the addition of USB 3
slots over previous versions of the Raspberry Pi which only included USB
2.

A USB 3 drive with a minimum real transfer rate of 200 MB/s is required
for this hardware.  Due to the large size of long term high quality
video recorded by LunAero, this transfer rate is a must.  You should
check that your drive actually performs at this speed.  Some hard disk
manufacturers might claim transfer speeds of "up to" the USB 3 maximum
rate (625 MB/s) without actually being able to get anywhere near this
speed.

Unsure of your hard drive transfer rate?  On linux systems you can use
this command to test it:
```sh
cat /dev/zero | pv > /path/to/your/drive/junkfile

```
The real-time transfer rate is printed on the terminal.  Press
<kbd>ctrl</kbd>+<kbd>c</kbd> to stop the script and use:
```sh
rm /path/to/your/drive/junkfile
```
to remove the junkfile you created.

Whatever you do, **avoid storing video on the SD card!**  Even if you
have a giant SD card with a ton of space, the technology underlying the
SD card is fragile for frequent writes and rewrites.  Saving large
videos to your SD card is a good way to damage it, corrupting your OS.
So **avoid storing video on the SD card!**

This installation uses a Raspberry Pi camera module.  Currently, there
is no implementation for USB cameras and no support for this is planned.
The software makes use of Raspbian's `raspivid` command to handle
recording video.  We have tried v1 and v2 Raspberry Pi camera modules
and had no problems.  Fancy cameras such as the IR camera module have
not been tested.

A TB6612FNG breakout board is required to control two motors.  If you
don't have one of these laying around (we made our own board in house),
you can get them from Sparkfun https://www.sparkfun.com/products/14451.

This hardware implementation requires two low speed/high torque 5V
motors.  We used 0.5 rpm motors purchased from Amazon (uxcell DC 5V
0.5RPM Worm Gear Motor 6mm Shaft High Torque Turbine Reducer).  If you
change motors, be sure that the CAD designs match the drill holes
required for the motor to attach to the plastic.

This project include designs for plastic parts to be cut from 1/4"
acrylic plastic and 1/8" acetal plastic.  You can change these materials
if you like.  E.g. it would be reasonable to swap the acetal for some of
the 1/4" acyrlic if you are low on funds.  HOWEVER, the acetal makes
much better gears and will give you much smoother motion.

Hardware updates are planned, but the current version uses most of the
original design, which can be found at https://osf.io/k6nfs/ .

## Using the 2D CAD files

The CAD files in the 2D CAD folder are used to construct parts for
LunAero including gears and mounting brackets.  The parts are designed
to be cut on a laser CNC machine.  They can also be used as templates
for cutting on a band saw or using another CNC machine.

The units are in mm for all parts.

Part files can be found at: https://osf.io/k6nfs/ .

Assembly information can be found at: https://osf.io/y2n3j/ .

## Software Install Instructions

### Option 1: Compile this Repository

To follow my instructions, you need to use the Linux terminal, where you
can copy and paste the following scripts.  To open the Linux terminal,
press <kbd>ctrl</kbd>+<kbd>alt</kbd>+<kbd>t</kbd> or click the icon on
the Raspbian desktop.

When you install this for the first time on to a 'blank' Raspberry Pi,
you will need certain repositories installed to make it.  Use the
following command in your terminal (Note: this looks way more daunting
than it really is, your Raspberry Pi will have most of these installed
with a default Raspbian configuration):

```sh
sudo apt update
sudo apt -y install git libc6-dev libgcc-8-dev libraspberrypi0 wiringpi \
libstdc++-8-dev libgtk-3-dev libpango1.0-dev libatk1.0-dev libcairo2-dev \
libgdk-pixbuf2.0-dev libglib2.0-dev libopencv-shape-dev libopencv-dev \
libopencv-stitching-dev libopencv-superres-dev libopencv-videostab-dev \
libopencv-contrib-dev libopencv-video-dev libopencv-viz-dev \
libopencv-calib3d-dev libopencv-features2d-dev libopencv-flann-dev \
libopencv-objdetect-dev libopencv-ml-dev libopencv-highgui-dev \
libopencv-videoio-dev libopencv-imgcodecs-dev libopencv-photo-dev \
libopencv-imgproc-dev libopencv-core-dev libnotify-dev libc6 libglib2.0-0 \
libx11-6 libxi6 libxcomposite1 libxdamage1 libxfixes3 libatk-bridge2.0-0 \
libxkbcommon0 libwayland-cursor0 libwayland-egl1 libwayland-client0 \
libepoxy0 libharfbuzz0b libpangoft2-1.0-0 libfontconfig1 libfreetype6 \
libxinerama1 libxrandr2 libxcursor1 libxext6 libthai0 libfribidi0 \
libpixman-1-0 libpng16-16 libxcb-shm0 libxcb1 libxcb-render0 libxrender1 \
zlib1g libmount1 libselinux1 libffi6 libpcre3 libtbb2 libhdf5-103 libsz2 \
libjpeg62-turbo libtiff5 libvtk6.3 libgl2ps1.4 libglu1-mesa libsm6 \
libice6 libxt6 libgl1 libtesseract4 liblept5 libdc1394-22 libgphoto2-6 \
libgphoto2-port12 libavcodec58 libavformat58 libavutil56 libswscale5 \
libavresample4 libwebp6 libgdcm2.8 libilmbase23 libopenexr23 libgdal20 \
libdbus-1-3 libatspi2.0-0 libgraphite2-3 libexpat1 libuuid1 libdatrie1 \
libxau6 libxdmcp6 libblkid1 libatomic1 libaec0 libzstd1 liblzma5 libjbig0 \
libbsd0 libglvnd0 libglx0 libgomp1 libgif7 libopenjp2-7 libraw1394-11 \
libusb-1.0-0 libltdl7 libexif12 libswresample3 libvpx5 libwebpmux3 \
librsvg2-2 libzvbi0 libsnappy1v5 libaom0 libcodec2-0.8.1 libgsm1 \
libmp3lame0 libopus0 libshine3 libspeex1 libtheora0 libtwolame0 \
libvorbis0a libvorbisenc2 libwavpack1 libx264-155 libx265-165 \
libxvidcore4 libva2 libxml2 libbz2-1.0 libgme0 libopenmpt0 \
libchromaprint1 libbluray2 libgnutls30 libssh-gcrypt-4 libva-drm2 \
libva-x11-2 libvdpau1 libdrm2 libcharls2 libjson-c3 libarmadillo9 \
libproj13 libpoppler82 libfreexl1 libqhull7 libgeos-c1v5 libepsilon1 \
libodbc1 odbcinst1debian2 libkmlbase1 libkmldom1 libkmlengine1 \
libkmlxsd1 libkmlregionator1 libxerces-c3.2 libnetcdf13 libhdf4-0-alt \
libogdi3.2 libgeotiff2 libpq5 libdapclient6v5 libdapserver7v5 \
libdap25 libspatialite7 libcurl3-gnutls libfyba0 libmariadb3 libssl1.1 \
libsystemd0 libudev1 libsoxr0 libcroco3 libogg0 libicu63 libmpg123-0 \
libvorbisfile3 libp11-kit0 libidn2-0 libunistring2 libtasn1-6 libnettle6 \
libhogweed4 libgmp10 libgcrypt20 libgssapi-krb5-2 libarpack2 libsuperlu5 \
libnss3 libnspr4 liblcms2-2 libgeos-3.7.1 libpopt0 libminizip1 \
liburiparser1 libkmlconvenience1 libldap-2.4-2 libsqlite3-0 \
libnghttp2-14 librtmp1 libssh2-1 libpsl5 libkrb5-3 libk5crypto3 \
libcom-err2 liblz4-1 libgpg-error0 libkrb5support0 libkeyutils1 \
libgfortran5 libsasl2-2 libblas3 
```

Then, enable the Raspberry Pi camera in Raspbian.  This can be done using
the interactive config tool `raspi-config`, or by using the following
one-liner in the terminal to change the setting without using interactive
mode.  You may need to restart your Raspberry Pi after this step.

```
sudo raspi-config nonint do_camera 0
```

Next, use `git` to pull this repository.  I recommend saving it to a
location to makes sense, so in the following example, I am pulling it to
the `Documents` folder in the Raspberry Pi home directory.  Once the
LunAero `git` repository is pulled, you can compile it.  To do this,
execute the lines in the terminal:

```sh
cd /home/pi/Documents
git clone https://github.com/BlueNalgene/LunAero_C.git
cd LunAero_C
```

This script downloads the everything you need to compile the program from
this `git` repository.  Once you have the repository on your Raspberry
Pi and have installed all of the relevant packages from the `apt` command
above, enter the LunAero_C folder, and issue the following command:

```sh
make
```

The `make` command follows the instructions from the `Makefile` to
compile your program.  If you ever make changes to the source material,
remember to edit the `Makefile` to reflect changes to the packages used
or the required C++ files.  If `make` runs correctly, your terminal will
print some text that looks like `g++ blah blah -o LunAero_Moontracker`
spanning a few lines.  If the output is significantly longer than that
and includes words like ERROR or WARNING, something may have gone wrong,
and you should read the error messages to see if something needs to be
fixed.

### Option 2: Custom Raspbian Boot Image



### Option 3 (experimental): Download a Pre-Compiled Binary Release







## Running LunAero

Hooray! You compiled a file called `LunAero_Moontracker`.  When you are
ready to run the program, you can type the command:
```sh
/path/to/LunAero_C/LunAero_Moontracker
```
and the program will run.  We recommend the following steps to run the
program well:

### Edit the Settings

A file called `settings.cfg` has been provided to give the user the
ability to modify settings without needing to recompile.  Before
running, LunAero for the first time, it is prudent to check that you are
happy with the default settings.  This is especially true for the
General Settings at to top of the file.

You will likely need to edit the `DRIVE_NAME` setting.  This should be
the name you have assigned to the USB drive you are saving video to.
The default is `MOON1`.  This means that the program will try to save
video to the drive located at `/media/$USER/MOON1/`.  The program won't
work if you have this setting incorrect.

### Time Confirmation

Before you run LunAero, you should confirm that the date and time of your
system is correct.  This is a very important step since LunAero saves
videos based on the system time, and during analysis, an accurate time
stamp is necessary.  We recommend single second accuracy for of your
system time prior to starting.  You should also check that your
Raspberry Pi is configured for the correct timezone for the location
you are operating in.  The most reliable way to check these values on
Linux is to open a terminal and use:

```sh
timedatectl
```

The Raspberry Pi v3 automatically updates the system time when connected
to the internet.  If you are somewhere without access to the internet
(out birding in the middle of nowhere maybe), you will need to update the
time and date manually.  To set the date to July 20th, 1969 at 10:56:20
pm (The official time that Neil Armstrong stepped on the moon), you would
use:

```sh
sudo date +%Y%m%d -s "19690720"
sudo date +%T -s "22:56:20"
```

To set the timezone on your Raspberry Pi, you need to enter the config
screen, select "Localization Options", and "Change Timezone".  To edit
the config, use the Raspberry Pi config tool from the terminal with:

```sh
sudo raspi-config
```


### Focus Helper

On small screens, similar to the ones recommended by the LunAero
hardware specs, it may be difficult to see how "good" your video
looks using only `LunAero_Moontracker`.  This is because the preview
window in `LunAero_Moontracker` is only 1/4 the size of the display!
For your convenience, we have added a small Python script which shows
a much larger preview window.  In your terminal, execute:

```sh
python3 /path/to/LunAero_C/focus.py
```

The first screen displays instructions for how to use this script.
You are able to use directional keys to adjust the scope position,
<kbd>space</kbd> to stop the movement, <kbd>i</kbd> to cycle the
ISO setting of the camera, and the keys <kbd>g</kbd> and <kbd>b</kbd>
to increase and decrease the shutter speed, respectively.  Press
<kbd>q</kbd> at any time to exit the script.  Use the large preview
window which the script shows to adjust your scope for zoom and focus.
The image should be as sharp as possible, and the moon should not be
larger than the preview window.  A red border is present on the
preview to assist you in finding the edge of the screen, useful
when you are comparing a night sky to the black background of the
screen.


### Manual Adjustment Mode

Once you have confirmed the time is correct, run the program.  Upon
startup, you will enter "manual mode".  In this mode, you are able to
adjust the direction your scope is pointing and a few camera settings.
In "manual mode", the bottom center of the screen includes a preview
window overlayed on the GUI which shows what the camera sees like a
viewfinder.

Take the following steps:

+ Find the moon.  Use the arrow buttons on the screen with the mouse or
the arrow buttons on your keyboard to move the scope up, down, left, and
right to find the moon.  If you are having a hard time finding the moon,
we recommend tilting and rotating the hardware while the motors are NOT
moving to find approximately the correct left-right direction you should
be pointing the scope.  Then move up or down from that position.  It is
easier to find the moon if your scope is zoomed out.  You can always zoom
in once you have found the correct position.
+ Adjust the zoom of your scope such that the moon fills as much of the
preview window as reasonably possible.  Do not over zoom such that the
edge of the moon appears cut off.
+ Adjust the brightness of the image by using the ISO and Shutter Speed
buttons (or associated keyboard keys), and pass these commands to the
camera.  The ISO button cycles through the available ISO settings of the
Raspberry Pi camera.  Your options are 100, 200, 400, and 800.  For clear
nights, 100 will likely be the best setting.  The shutter speed/exposure
buttons (pluses and minuses) change the shutter speed.  The buttons with
a double plus or minus raise the and lower the settings to a greater
degree than the single buttons.  For a darker image, lower the setting,
for a brighter image, raise the setting.  None of these buttons update
the image in the viewfinder.  To use the settings, you must press the
"Reissue Camera Command".
+ Check that LunAero likes your settings.  
  - The value next to "FOCUS VAL" is the calculated focus quality of
  your image.  You must adjust the focus of your scope to influence it.
  For best results, attempt to maximize this value.  
  - If your image on the screen is determined by the software to be too
  bright, a message "TOO BRIGHT!" will appear.  Adjust your camera
  settings based on the instructions in the previous step to make this
  warning disappear.
+ Finally, perform any last minute adjustments on the position.  The
moon should be completely within the preview window, not touching the
edge.  If you are satisfied with the image...
+ Press the "▶" button or the <kbd>Enter</kbd> key.

### Automatic Mode

Once you are happy with the settings, and pressed <kbd>Enter</kbd> to run 
automatic mode, just step away.  Watch with joy and wonder as the scope
automatically follows the moon as it moves across the sky.  LunAero is
recording everything that happens in view of the scope now.  You can
walk away from it and it will continue recording.  Be sure to check that
the weather is good and you are in a secure location.  Members of the
LunAero team are not responsible for anything that happens to your scope
if left outside during inclement weather or sticky fingers.

## Viewing the Video Output

LunAero saves video data to folders on your output USB drive with the
following formula: `/media/$USER/DRIVE_NAME/YYYYMMDDHHmmSS/*.h264` These
output files are raw video footage not in a standard "container".  You
need special codecs to view the video.  We recommend the program VLC
(https://www.videolan.org/vlc/) for easiest use.  This is an open-source
program available for all OS's.  If you would like to view videos on your
Raspberry Pi, use the following commands to install it:

```sh
sudo apt update
sudo apt -y install vlc
```

## When Something Goes Wrong

Something always goes wrong.  It is the way of things.  When something
inevitably does go wrong with LunAero program, you should first check
the logs.  If the setting `DEBUG_COUT` in `settings.cfg` is set to `true`
the program will attempt to save a log file in the same directory where
the videos are saved for each run.  This is very detailed, so searching
for keywords like `WARNING` and `ERROR` are suggested.

### Something Went Wrong... and I can't find a log file

If there is no log file saved where you would expect it and you have
checked that your debug settings are correct, there may have been a
problem when writing the log file.  Did you see a little notification
pop up near the top right of the screen?  If you see one of those, it
means there was a problem before the log file could be written.  Check
that you have the correct drive name saved to `DRIVE_NAME` in the
settings.  Check to see if the path looks correct.  In terminal type

```sh
ls /media/pi/
```
In the output you should see `DRIVE_NAME`.  If you also see something
which looks like `DRIVE_NAME`_1, then something went wrong mounting the
drive.  Safely eject and disconnect all drives on the Raspberry Pi.  Run
the `ls` command again.  If you still see `DRIVE_NAME` variants WITH THE
DRIVE NOT CONNECTED, you need to remove them.

```sh
sudo rm -r /path/to/offending/drive
```

Be careful with that command.  If drives are still connected, you risk
data loss by using it.  Once the offending false mount points are
removed, try connecting your drive again.

This problem usually occurs when drives are incorrectly removed and
re-mounted.  Linux is very persnickety when it comes to mounting drives.
Be sure to eject prior to disconnecting a drive if the Raspberry Pi is
running.  It is best to have the power disconnected any time you want to
remove or add a drive.

### Something Went Wrong...and it isn't listed here

If you need help, raise an Issue on this `git` repository with
descriptive information so the package maintainer can help you.

## Bird Detection Software

As of June 21st, 2019, the original Python software for tracking birds 
in video of the moon has been migrated to a new repository at 
https://github.com/BlueNalgene/Birdtracker_LunAero, although work on this
version has been discontinued in favor of C++.  The new version may be
found at https://github.com/BlueNalgene/CPP_Birdtracker .

## What if I Want to Play with the Source Code?

The source code is documented with the `Doxygen` standard.  Every
function and most variables are heavily commented to make it easy for
you.  You can view the documentation online by going to:
https://bluenalgene.github.io/LunAero_C/html/index.html .

