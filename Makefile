SYSTEMC_HOME    = /home/alis/systemc-2.3.3/systemc-2.3.3
TARGET_ARCH     = linux64

SYSTEMC_INC_DIR = $(SYSTEMC_HOME)/include
SYSTEMC_LIB_DIR = $(SYSTEMC_HOME)/lib-$(TARGET_ARCH)
HEADERS_PATH = /home/alis/neurocore/src/
#/home/alis/neurocore/libsystemc-2.3.3.so

FLAGS           = -g -Wall -pedantic -Wno-long-long \
                 -DSC_INCLUDE_DYNAMIC_PROCESSES -fpermissive -std=c++2a \
                 -I $(SYSTEMC_INC_DIR) -I $(HEADERS_PATH)
LDFLAGS         = -L $(SYSTEMC_LIB_DIR) -lsystemc -lm

SRCS = src/*.cpp src/*.h Pure-CPP20-Neural-Network/src/*
OBJS = $(SRCS:.cpp=.o)
	
build:
	g++-13 -o model2  $(LDFLAGS) $(FLAGS) $(SRCS) 

run:
	export LD_LIBRARY_PATH=/home/alis/systemc-2.3.3/systemc-2.3.3/lib-linux64/; ./model2
