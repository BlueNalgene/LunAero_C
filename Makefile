OBJS=LunAero_Moontracker
BIN=gcc C_LunAero.c

#CFLAGS+=-Wall -g -O3
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host -lm -lwiringPi -lpthread
LDFLAGS+=`pkg-config --cflags --libs gtk+-3.0`
#LDFLAGS+=`pkg-config --cflags --libs opencv`

INCLUDES+=-I/opt/vc/include/
INCLUDES+=-I/opt/vc/include/interface/vcos/pthreads
INCLUDES+=-I/opt/vc/include/interface/vmcs_host/linux

all:
	@rm -f $(OBJS)
	$(BIN) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $(OBJS)
