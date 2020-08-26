#ifndef BEFUNGE_H
#define BEFUNGE_H

#define table_t signed long
#define stacksize 1024 * 1024 * 20

std::vector<table_t> * table;
signed long stack[stacksize];
int sp = 0;

std::string source;

int ip[] = {0, 0};
char direction = '>';
bool running = true;
int tablelen = 0;
bool stringMode = false;

std::string directions = "><^v";

void push(table_t x);
table_t pop();
void advanceIP(int x);


/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <vector>
#include "step.h"

#define table_t signed long

#define stacksize 1024 * 1024 * 20*/

#endif