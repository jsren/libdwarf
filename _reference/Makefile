dwarfexample: dwarfexample.o
	gcc -m32 -g -o dwarfexample dwarfexample.o libdwarf/libdwarf.a -lelf -lstdc++
dwarfexample.o: dwarfexample.cpp
	gcc -c -m32 -g -I libdwarf dwarfexample.cpp
clean:
	rm dwarfexample.o dwarfexample
