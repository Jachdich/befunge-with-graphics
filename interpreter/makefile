SOURCES := $(shell find src -type f -name *.cpp)
HEADERS := $(shell find include -type f -name *.h)
OBJECTS := $(patsubst src/%,obj/%,$(SOURCES:.cpp=.o))

befunge: $(OBJECTS)
	g++ $(OBJECTS) -o $@ -lSDL2

obj/%.o: src/%.cpp $(HEADERS)
	g++ -c -o $@ $< -Wall -Werror -g -Iinclude

debug: befunge
	gdb befunge

run: befunge
	./befunge $@

clean:
	rm obj/*.o
	rm befunge

.PHONY: clean
