

INCPATH		= -I. -I/usr/include/eigen3
CXXFLAGS	= -Wall -g -std=c++11
CXX=clang++

#DISTDIR = /home/ana/tmp/example/.tmp/echoclient1.0.0
LINK	= g++
LFLAGS	= -m64 -Wl,-O1
LIBS	= 
#LIBS	= $(SUBLIBS) -lQt5WebSockets -lQt5Network -lQt5Core -lpthread



.SUFFIXES: .o .c .cpp

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

TARGET	= printhexgrid

all: $(TARGET)

unittest: color.o functions.o numstring.o unittest.o
	$(LINK) $(LFLAGS) -o $@ $? $(LIBS)

printhexgrid: boundingbox.o color.o command.o functions.o hexgrid.o numstring.o part.o printhexgrid.o renderer.o
	$(LINK) $(LFLAGS) -o $@ $? $(LIBS)


.PHONY: clean
clean:
	rm -f *.o $(TARGET)




