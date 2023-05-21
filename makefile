#g++ -c -g -o obj/ide.o befunge_ide.cpp -Iinclude
#g++ -o ide obj/ide.o obj/befunge.o -Iinclude -lSDL2 -lncurses

SOURCES := $(shell find src -type f -name *.cpp)
HEADERS := $(shell find include -type f -name *.h)
#OBJECTS := $(patsubst src/%,obj/%,$(SOURCES:.cpp=.o))

OBJECTS := obj/run.o obj/befunge.o
IDE_OBJECTS := obj/befunge.o obj/ide.o

interpreter: $(OBJECTS)
	g++ $(OBJECTS) -o befunge -lSDL2

ide: $(IDE_OBJECTS)
	g++ $(IDE_OBJECTS) -o ide -lSDL2 -lncurses

obj/%.o: src/%.cpp $(HEADERS)
	g++ -c -o $@ $< -Wall -ggdb -Iinclude -fPIE

clean:
	rm obj/*.o
	rm befunge
	rm ide

.PHONY: clean
