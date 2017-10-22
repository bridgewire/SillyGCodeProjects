

INCPATH		= -I. -I/usr/include/eigen3
CXXFLAGS	= -Wall -g -std=c++11
CXX=clang++

LINK	= g++
LFLAGS	= -m64 -Wl,-O1
LIBS	= 

.SUFFIXES: .o .c .cpp

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

TARGET	= printhexgrid

all: $(TARGET)

printhexgrid: boundingbox.o color.o command.o functions.o hexgrid-unittest.o numstring.o part.o renderer.o
	$(LINK) $(LFLAGS) -o $@ $? $(LIBS)

hexgrid-unittest.o: hexgrid.cpp
	$(CXX) -c -DUNITTEST $(CXXFLAGS) $(INCPATH) -o $@ $<

.PHONY: clean
clean:
	rm -f *.o $(TARGET)




