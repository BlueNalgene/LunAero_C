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
#BIN=gcc C_LunAero.c
BIN=g++ LunAero.cpp
BIN+=gtk_LunAero.cpp
BIN+=motors_LunAero.cpp
BIN+=camera_LunAero.cpp

#~ CFLAGS+=-Wall -g -O3
CFLAGS+=-std=c++17
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host -lm -lwiringPi -lpthread -lstdc++fs 
LDFLAGS+=-DGTK_MULTIHEAD_SAFE=1 `pkg-config --cflags --libs gtk+-3.0`
#~ LDFLAGS+=`Magick++-config --cppflags --cxxflags --ldflags --libs`
#LDFLAGS+=`pkg-config --cflags --libs opencv`

INCLUDES+=-I/opt/vc/include/
INCLUDES+=-I/opt/vc/include/interface/vcos/pthreads
INCLUDES+=-I/opt/vc/include/interface/vmcs_host/linux

all:
	@rm -f $(OBJS)
	$(BIN) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $(OBJS)
