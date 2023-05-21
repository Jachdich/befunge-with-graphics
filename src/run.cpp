#include "befunge.h"
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>

Befunge vm;

int main(int argc, char ** argv) {
    if (argc > 2) {
        if (std::string(argv[2]) == "-q") {
            vm.quiet = true;
        } else if (std::string(argv[2]) == "-d") {
            vm.debug = true;
        }
    }
    std::string source;
    std::ifstream myReadFile;
    myReadFile.open(argv[1]);
    if (myReadFile.is_open()) {
        while (!myReadFile.eof()) {
            std::string temp;
            getline(myReadFile, temp);
            source += temp + "\n";
        }
    } else {
        std::cout << "Error opening file\n";
        return 1;
    }
    myReadFile.close();

    vm.loadCode(source);
    vm.run();
    return 0;
}
