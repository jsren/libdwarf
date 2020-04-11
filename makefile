GPP_FLAGS := -I. -std=c++17 -Wall -pedantic -O2 -Wno-unknown-pragmas -Wno-format

.PHONY: default build clean elf

default : elf dwarf

elf: $(wildcard elf/*.cpp) $(wildcard elf/*.hpp)
	g++ $(GPP_FLAGS) -fPIC $(wildcard elf/*.cpp) -shared -o libelf.so

dwarf: $(wildcard dwarf/*.cpp) $(wildcard dwarf/*.hpp)
	g++ $(GPP_FLAGS) -fPIC $(wildcard dwarf/*.cpp) -shared -o libdwarf.so

example: elf example.cpp
	g++ $(GPP_FLAGS) example.cpp libelf.o -o example

clean:
	rm libelf.so libdwarf.so
