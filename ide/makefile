run: befunge.o
	g++ -lncurses -g befunge_ide.cpp befunge.o -o befunge_ide
	./befunge_ide testfile.txt

befunge.o:
	cp ../interpreter/obj/befunge.o .
	
