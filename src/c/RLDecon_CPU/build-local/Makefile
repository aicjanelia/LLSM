# @(#)makefile

FFTWTHREAD   = -lfftw3f_threads -lpthread
FFTWLIBS     = -lfftw3f


LDLIBS      = $(FFTWTHREAD) $(FFTWLIBS)
CXXFLAGS += -Wall -DNDEBUG
CXXFLAGS += -std=c++11
CXXFLAGS += -O2 -fopenmp
#CXXFLAGS      += -g

LDLIBS  = -lfftw3f -lfftw3f_threads -ltiff -lX11
LDLIBS += -lboost_program_options -lboost_filesystem -lboost_system

LDFLAGS += -fopenmp

# ifneq (, $(filter $(shell hostname -s), e05u07 e05u03))
#   CXX=/usr/local/gcc-4.9.1/bin/g++
#   CC=/usr/local/gcc-4.9.1/bin/gcc
#   CXXFLAGS += -I/usr/local/boost-1.57/include
#   LDFLAGS += -L/usr/local/boost-1.57/lib -Wl,-rpath=/usr/local/gcc-4.9.1/lib64,-rpath=/usr/local/boost-1.57/lib
# endif

# ifeq ($(shell uname), Darwin)
#   CXXFLAGS += -I/opt/local/include
#   CXXFLAGS += -I/usr/X11/include
#   LDFLAGS += -L/opt/local/lib -L/usr/X11/lib
# endif

%.o: ../%.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@ 

all: cpuDeconv

cpuDeconv: linearDecon.o RL-Biggs-Andrews.o boostfs.o RLgpuImpl.o geometryTransform.o otfgen.o
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	$(RM) *.o *.exe *~

depend:
	makedepend -- $(CFLAGS) -- $(SRC)


