OBJS=LunAero_Moontracker
#BIN=gcc C_LunAero.c
BIN=g++ LunAero.cpp
BIN+=gtk_LunAero.cpp
BIN+=motors_LunAero.cpp
BIN+=camera_LunAero.cpp

#~ CFLAGS+=-Wall -g -O3
CFLAGS+=-std=c++17
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host -lm -lwiringPi -lpthread
LDFLAGS+=-DGTK_MULTIHEAD_SAFE=1 `pkg-config --cflags --libs gtk+-3.0`
#~ LDFLAGS+=`Magick++-config --cppflags --cxxflags --ldflags --libs`
#LDFLAGS+=`pkg-config --cflags --libs opencv`

INCLUDES+=-I/opt/vc/include/
INCLUDES+=-I/opt/vc/include/interface/vcos/pthreads
INCLUDES+=-I/opt/vc/include/interface/vmcs_host/linux

all:
	@rm -f $(OBJS)
	$(BIN) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $(OBJS)
