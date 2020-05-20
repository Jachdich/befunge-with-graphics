befunge: befunge.cpp
	g++ -m64 -O3 -L/usr/lib/x86_64-linux-gnu -lSDL2 befunge.cpp -o befunge

run: befunge
	./befunge $@
