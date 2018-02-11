

INCPATH		= -I. -I/usr/include/eigen3
CXXFLAGS	= -Wall -g -std=c++11
CXX=clang++

LINK	= g++
LFLAGS	= -m64 -Wl,-O1
LIBS	= 

.SUFFIXES: .o .c .cpp

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

TARGET	= printhexgrid color-unittest

all: $(TARGET)

printhexgrid: boundingbox.o color.o command.o functions.o hexgrid-unittest.o numstring.o part.o renderer.o
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)

hexgrid-unittest.o: hexgrid.cpp
	$(CXX) -DHEXGRID_UNITTEST -c $(CXXFLAGS) $(INCPATH) -o $@ $<

color-unittest: color.cpp
	$(CXX) -DBWCNCCOLOR_UNITTEST $(CXXFLAGS) $(INCPATH) -o $@ $<


.PHONY: clean
clean:
	rm -f *.o $(TARGET)

