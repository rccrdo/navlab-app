# Author: 2013 Riccardo Lucchese riccardo.lucchese@gmail.com


CPP = g++
CPPFLAGS =  -Wall -g3 -O0 -I./

UNAME := $(shell uname -o)
TARGETS = tracker calib
ifeq ($(UNAME), Msys)
# mingw

else 
# unix
CFLAGS += -fPIC
CPPFLAGS += -fPIC

# opencv
OPENCV_CPPFLAGS += $(shell pkg-config --cflags opencv)
OPENCV_LDFLAGS = $(shell pkg-config --libs opencv)
endif


.PHONY: default
    default: $(TARGETS)


TRACKER_OBJS = camera.o		\
	patrolling.o		\
	v4l2_input.o		\
	image-helpers.o		\
	main.o			\
	grabber.o		\
	tracking.o

CALIB_OBJS = calib.o		\
	image-helpers.o		\
	v4l2_input.o		\
	grabber.o



.PHONY: tracker
    tracker: $(TRACKER_OBJS)
	$(CPP) -o tracker $(TRACKER_OBJS) -lpthread $(OPENCV_LDFLAGS)

.PHONY: calib
    calib: $(CALIB_OBJS)
	$(CPP) -o calib $(CALIB_OBJS) -lpthread $(OPENCV_LDFLAGS)

camera.o: camera.cpp
	$(CPP) $(CPPFLAGS) $(OPENCV_CPPFLAGS) -c $^ -o $@

calib.o: calib.cc
	$(CPP) $(CPPFLAGS) $(OPENCV_CPPFLAGS) -c $^ -o $@

patrolling.o: patrolling.cpp
	$(CPP) $(CPPFLAGS) $(OPENCV_CPPFLAGS) -c $^ -o $@

v4l2_input.o: v4l2_input.cpp
	$(CPP) $(CPPFLAGS) -c $^ -o $@

image-helpers.o: image-helpers.cpp
	$(CPP) $(CPPFLAGS) -c $^ -o $@

main.o: main.cpp
	$(CPP) $(CPPFLAGS) $(OPENCV_CPPFLAGS) -c $^ -o $@

grabber.o: grabber.cpp
	$(CPP) $(CPPFLAGS) $(OPENCV_CPPFLAGS) -c $^ -o $@

tracking.o: tracking.cpp
	$(CPP) $(CPPFLAGS) $(OPENCV_CPPFLAGS) -c $^ -o $@


.PHONY: new
    new: clean default

.PHONY: clean
    clean: 
	-rm -f tracker
	-rm -f calib
	-rm -f *.o
	-rm -f *~
	-rm -f *.a


