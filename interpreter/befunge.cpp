#include <stdio.h>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <string.h>
#include <SDL2/SDL.h>
#include <iostream>
using namespace std;
typedef signed long table_t;
#define stacksize 1024 * 1024 * 102

vector<vector<table_t>> table;
signed long stack[stacksize];
int return_vector_stack[1024];
int sp = 0;
int vector_sp = 0;

string source;

int  ip[] = {0, 0};
char direction = '>';
bool running = true;
int  tablelen = 0;
bool stringMode = false;
bool quiet = false;
bool debug = false;

SDL_Surface * screen = NULL;
SDL_Window  * window = NULL;
SDL_Renderer * renderer = NULL;

void error(std::string msg, bool exit) {
    printf("The program has created the following exception:\n%s\n", msg.c_str());
    running = !exit;
}

void clean(vector<vector<table_t>>& table) {
    long unsigned int maxlen = 0;
    for (int i = 0; i < table.size(); i++) {
        vector<table_t> line = table[i];
        if (line.size() > maxlen) {
            maxlen = line.size();
        }
    }
    //printf("maxlen: %lu\n", maxlen);
    for (int i = 0; i < table.size(); i++) {
        int j = table[i].size();
        //printf("size init: %d\n", j);
        table[i].resize(maxlen, 0);
        //printf("size post: %d\n", table[i].size());
    }
}

table_t pop() {
    if (sp > 0) {
        return stack[sp--];
    } else {
        return 0;
    }
}

void push(table_t x) {
    stack[++sp] = x;
    if (sp >= stacksize) {
        printf("Out of memory!");
        running = false;
    }
}

int popvector() {
    if (vector_sp > 0) {
        return return_vector_stack[vector_sp--];
    } else {
        error("Try to return when no return vector was found\n", true);
        return 0;
    }
}

void pushvector(int x) {
    return_vector_stack[++vector_sp] = x;
    if (vector_sp >= 1024) {
        error("Max recursion depth exceeded\n", true);
    }
}

void advanceIP(int x) {
    switch (direction) {
        case '>': ip[0] += x; return;
        case '<': ip[0] -= x; return;
        case '^': ip[1] -= x; return;
        case 'v': ip[1] += x; return;
    }
}

const std::string reset("\033[0m");
const std::string green("\033[42m");

void step() {
    char curr_char = table[ip[1]][ip[0]];
    if (debug) {
        for (int i = 0; i < table.size(); i++) {
            for (int j = 0; j < table[i].size(); j++) {
                if (i == ip[1] && j == ip[0]) cout << green;
                cout << (char)table[i][j];
                if (i == ip[1] && j == ip[0]) cout << reset;
            }
            cout << "\n";
        }
    /*
        printf("%d, %d, ", ip[0], ip[1]);
        printf("%c, [", curr_char);
        for (int i = 0; i < sp; i++) {
            printf("%li, ", stack[i]);
        }
        printf("]\n");*/
    }
    if (curr_char == '"') {
        stringMode = !stringMode; return;
    } else if (stringMode) {
        push(curr_char); return;
    } else if (curr_char == ' ') {
        return;
    } else {
        if (curr_char - '0' >= 0 && curr_char - '0' <= 9) { push(curr_char - '0'); return; }
        switch (curr_char) {
            case '>': direction = curr_char; return;
            case '<': direction = curr_char; return;
            case 'v': direction = curr_char; return;
            case '^': direction = curr_char; return;
        }

        switch (curr_char) {
            case '+': { int a = pop(); int b = pop(); push(b + a); return; }
            case '-': { int a = pop(); int b = pop(); push(b - a); return; }
            case '*': { int a = pop(); int b = pop(); push(b * a); return; }
            case '/': { int a = pop(); int b = pop(); push(b / a); return; }
            case '%': { int a = pop(); int b = pop(); push(b % a); return; }
            case ':': { int a = pop(); push(a); push(a); return; }
            case '$': { --sp; return;}
            case '#': { advanceIP(1); return;}
            case '!': { int a = pop(); push(a == 0 ? 1 : 0); return;}
            case '`': { int a = pop(); int b = pop(); push(b >  a ? 1 : 0); return; }
            case '|': { int a = pop(); direction = (a == 0 ? 'v' : '^'); return; }
            case '_': { int a = pop(); direction = (a == 0 ? '>' : '<'); return; }
            case '\\': {int a = pop(); int b = pop(); push(a); push(b); return; }
          //case '?' : {direction = "><v^".charAt((int)(Math.random() * 4)); return;}
            case '.': if (!quiet) { printf("%li ", pop()); } else { pop(); } return;
            case ',': if (!quiet) { printf("%c", (char)pop()); } else { pop(); } return;

            case 'j': {
                int b = pop();
                int a = pop();
                pushvector(ip[0]);
                pushvector(ip[1]);
                pushvector(direction);
                ip[0] = a;
                ip[1] = b;
                direction = '>';
                advanceIP(-1);
                return;
            }

            case 'r':
                direction = (char)popvector();
                ip[1] = popvector();
                ip[0] = popvector();
                //advanceIP(-1);
                return;

            case 'g': {
                int b = pop();
                push(table[pop()][b]);
                return;
            }

            case 'p': {
                int b = pop();
                int a = pop();
                table_t c = pop();
                table[a][b] = c;
                return;
            }

            case 's': {
                int b = pop();
                int a = pop();
                if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                    error("Failed to initialise screen", true);
                    return;
                }

                if ((window = SDL_CreateWindow("foo", 0, 0, b, a, SDL_WINDOW_SHOWN/*SDL_WINDOW_OPENGL*/)) == NULL) {
                    error("Failed to initiate screen", true);
                    return;
                }

                if ((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)) == NULL) {
                    printf("%s\n", SDL_GetError());
                    error("Unable to init renderer", true);
                }
            }

            case 'c':
                if (renderer == NULL) { error("Screen not initialised", true); return; }
                SDL_RenderClear(renderer);
                return;

            case 'x': {
                if (renderer == NULL) { error("Screen not initialised", true); return; }
                int b = pop();
                int a = pop();
                SDL_RenderDrawPoint(renderer, a, b);
                return;
            }

            case 'z': {
                if (renderer == NULL) { error("Screen not initialised", true); return; }
                SDL_Event event;
                SDL_PollEvent(&event);
                if (event.type == SDL_WINDOWEVENT) {
                     if (event.window.event ==SDL_WINDOWEVENT_CLOSE) {
                         push(1);
                    }
                }
                if (event.type == SDL_KEYDOWN) {
                    push(event.key.keysym.sym);
                    push(2);
                    //printf("%d\n", event.key.keysym.sym);
                }
                if (event.type == SDL_KEYUP) {
                    push(event.key.keysym.sym);
                    push(3);
                }
                return;
            }

            case 'l': {
                if (renderer == NULL) { error("Screen not initialised", true); return; }
                int y2 = pop();
                int x2 = pop();
                int y1 = pop();
                int x1 = pop();
                SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
                return;
            }

            case 'f': {
                if (renderer == NULL) { error("Screen not initialised", true); return; }
                int b = pop();
                int g = pop();
                int r = pop();
                SDL_SetRenderDrawColor(renderer, r, g, b, 0xFF);
                return;
            }

            case 'u':
                if (renderer == NULL) { error("Screen not initialised", true); return; }
                SDL_RenderPresent(renderer);
                return;


            case '@': running = false; return;
        }
    }
}

vector<table_t> toVector(string x) {
    vector<table_t> y;
    y.reserve(x.length());
    for (int i = 0; i < x.length(); i++) {
        y.push_back((table_t)x[i]);
    }
    return y;
}

int main(int argc, char ** argv) {
    if (argc > 2) {
        if (strcmp(argv[2], "-q") == 0) {
            quiet = true;
        } else if (strcmp(argv[2], "-d") == 0) {
            debug = true;
        }
    }

    ifstream myReadFile;
    myReadFile.open(argv[1]);
    if (myReadFile.is_open()) {
        while (!myReadFile.eof()) {
            string temp;
            getline(myReadFile, temp);
            source += temp + "\n";
        }
    } else {
        printf("Error opening file\n");
        return 1;
    }
    myReadFile.close();

    std::stringstream ss(source);
    std::string to;

    while(std::getline(ss,to,'\n')) {
        table.push_back(vector<table_t>(to.begin(), to.end()));
    }
    clean(table);

    while (running) {
        step();
        advanceIP(1);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
        SDL_Quit();
    }
    return 0;
}
/*
graphics operations, not yet supported
            case 's':
                b = pop();
                a = pop();
                screen = new GraphicsWindow(a, b);
                return;

            case 'c':
                screen.clear();
                return;

            case 'x':
                b = pop();
                a = pop();
                screen.pixel(a, b);
                return;

            case 'z':
                Event e = screen.getEvent();
                for (int x: e.args) {
                    push(x);
                }
                push(e.type);
                return;

            case 'l':
                int y2 = pop();
                int x2 = pop();
                int y1 = pop();
                int x1 = pop();
                screen.line(x1, y1, x2, y2);
                return;

            case 'f':
                b = pop();
                int g = pop();
                int r = pop();
                screen.setForeground(new Color(r, g, b));
                return;

            case 'b':
                b = pop();
                int g1 = pop();
                int r1 = pop();
                screen.setBackground(new Color(r1, g1, b));
                return;

            case 'u':
                screen.flip();
                return;*/
