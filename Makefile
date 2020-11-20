#
# C_LunAero/Makefile - Bash Makefile for robotic moon tracking scope
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
#

OBJS=LunAero_Moontracker
BIN=g++ LunAero.cpp
BIN+=gtk_LunAero.cpp
BIN+=motors_LunAero.cpp
BIN+=camera_LunAero.cpp

# For this program, the following packages need to be installed on your Raspi:
# libc6-dev
# libgcc-8-dev
# libraspberrypi0
# wiringpi
# libstdc++-8-dev
# libgtk-3-dev
# libpango1.0-dev
# libatk1.0-dev
# libcairo2-dev
# libgdk-pixbuf2.0-dev
# libglib2.0-dev
# libopencv-shape-dev
# libopencv-stitching-dev
# libopencv-superres-dev
# libopencv-videostab-dev
# libopencv-contrib-dev
# libopencv-video-dev
# libopencv-viz-dev
# libopencv-calib3d-dev
# libopencv-features2d-dev
# libopencv-flann-dev
# libopencv-objdetect-dev
# libopencv-ml-dev
# libopencv-highgui-dev
# libopencv-videoio-dev
# libopencv-imgcodecs-dev
# libopencv-photo-dev
# libopencv-imgproc-dev
# libopencv-core-dev
# libnotify-dev
# libc6
# libglib2.0-0
# libx11-6
# libxi6
# libxcomposite1
# libxdamage1
# libxfixes3
# libatk-bridge2.0-0
# libxkbcommon0
# libwayland-cursor0
# libwayland-egl1
# libwayland-client0
# libepoxy0
# libharfbuzz0b
# libpangoft2-1.0-0
# libfontconfig1
# libfreetype6
# libxinerama1
# libxrandr2
# libxcursor1
# libxext6
# libthai0
# libfribidi0
# libpixman-1-0
# libpng16-16
# libxcb-shm0
# libxcb1
# libxcb-render0
# libxrender1
# zlib1g
# libmount1
# libselinux1
# libffi6
# libpcre3
# libtbb2
# libhdf5-103
# libsz2
# libjpeg62-turbo
# libtiff5
# libvtk6.3
# libgl2ps1.4
# libglu1-mesa
# libsm6
# libice6
# libxt6
# libgl1
# libtesseract4
# liblept5
# libdc1394-22
# libgphoto2-6
# libgphoto2-port12
# libavcodec58
# libavformat58
# libavutil56
# libswscale5
# libavresample4
# libwebp6
# libgdcm2.8
# libilmbase23
# libopenexr23
# libgdal20
# libdbus-1-3
# libatspi2.0-0
# libgraphite2-3
# libexpat1
# libuuid1
# libdatrie1
# libxau6
# libxdmcp6
# libblkid1
# libatomic1
# libaec0
# libzstd1
# liblzma5
# libjbig0
# libbsd0
# libglvnd0
# libglx0
# libgomp1
# libgif7
# libopenjp2-7
# libraw1394-11
# libusb-1.0-0
# libltdl7
# libexif12
# libswresample3
# libvpx5
# libwebpmux3
# librsvg2-2
# libzvbi0
# libsnappy1v5
# libaom0
# libcodec2-0.8.1
# libgsm1
# libmp3lame0
# libopus0
# libshine3
# libspeex1
# libtheora0
# libtwolame0
# libvorbis0a
# libvorbisenc2
# libwavpack1
# libx264-155
# libx265-165
# libxvidcore4
# libva2
# libxml2
# libbz2-1.0
# libgme0
# libopenmpt0
# libchromaprint1
# libbluray2
# libgnutls30
# libssh-gcrypt-4
# libva-drm2
# libva-x11-2
# libvdpau1
# libdrm2
# libcharls2
# libjson-c3
# libarmadillo9
# libproj13
# libpoppler82
# libfreexl1
# libqhull7
# libgeos-c1v5
# libepsilon1
# libodbc1
# odbcinst1debian2
# libkmlbase1
# libkmldom1
# libkmlengine1
# libkmlxsd1
# libkmlregionator1
# libxerces-c3.2
# libnetcdf13
# libhdf4-0-alt
# libogdi3.2
# libgeotiff2
# libpq5
# libdapclient6v5
# libdapserver7v5
# libdap25
# libspatialite7
# libcurl3-gnutls
# libfyba0
# libmariadb3
# libssl1.1
# libsystemd0
# libudev1
# libsoxr0
# libcroco3
# libogg0
# libicu63
# libmpg123-0
# libvorbisfile3
# libp11-kit0
# libidn2-0
# libunistring2
# libtasn1-6
# libnettle6
# libhogweed4
# libgmp10
# libgcrypt20
# libgssapi-krb5-2
# libarpack2
# libsuperlu5
# libnss3
# libnspr4
# liblcms2-2
# libgeos-3.7.1
# libpopt0
# libminiz


# CFLAGS+=-Wall -g -O3
CFLAGS+=-std=c++17
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host -lm -lwiringPi -lpthread -lstdc++fs 
LDFLAGS+=-DGTK_MULTIHEAD_SAFE=1 `pkg-config --cflags --libs gtk+-3.0`
LDFLAGS+=`pkg-config --cflags --libs opencv`
LDFLAGS+=`pkg-config --cflags --libs libnotify`

INCLUDES+=-I/opt/vc/include/
INCLUDES+=-I/opt/vc/include/interface/vcos/pthreads
INCLUDES+=-I/opt/vc/include/interface/vmcs_host/linux

all:
	@rm -f $(OBJS)
	$(BIN) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $(OBJS)

