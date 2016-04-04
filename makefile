
SOURCES  := $(wildcard src/*.cpp)
INCLUDES := -Iinc -I.
LIBNAME  := libdwarf

GPP_FLAGS := -std=c++17 -Wall -pedantic -O2 -Wno-unknown-pragmas

default : build
	
build:
	g++ $(GPP_FLAGS) $(INCLUDES) $(SOURCES) -o $(LIBNAME).elf

clean:
	rm -r *.o
	rm -r *.elf
	rm -r *.a
