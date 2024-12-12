SYSTEMC_HOME    = /home/alis/systemc-2.3.3/systemc-2.3.3
TARGET_ARCH     = linux64

SYSTEMC_INC_DIR = $(SYSTEMC_HOME)/include
SYSTEMC_LIB_DIR = $(SYSTEMC_HOME)/lib-$(TARGET_ARCH)
#/home/alis/neurocore/libsystemc-2.3.3.so

FLAGS           = -g -Wall -pedantic -Wno-long-long \
                 -DSC_INCLUDE_DYNAMIC_PROCESSES -fpermissive \
                 -I $(SYSTEMC_INC_DIR) 
LDFLAGS         = -L $(SYSTEMC_LIB_DIR) -lsystemc -lm

SRCS = src/cpu.cpp src/mem.cpp src/main.cpp
OBJS = $(SRCS:.cpp=.o)
	
build:
	g++ -o model -std=c++11 $(LDFLAGS) $(FLAGS) $(SRCS)

run:
	export LD_LIBRARY_PATH=/home/alis/systemc-2.3.3/systemc-2.3.3/lib-linux64/; ./model
