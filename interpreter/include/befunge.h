#ifndef BEFUNGE_H
#define BEFUNGE_H
#include <string>
#include <vector>
typedef signed long table_t;

extern std::vector<std::vector<table_t>> table;
extern int pc[2];
extern bool quiet;
extern bool debug;
void run();
void loadCode(std::string code);
void step();

#endif
