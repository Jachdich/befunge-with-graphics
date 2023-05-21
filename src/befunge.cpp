#include <stdio.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <string.h>
#include <iostream>
#include "../include/befunge.h"

void Befunge::error(std::string msg, bool exit) {
    printf("The program has created the following exception:\n%s\n", msg.c_str());
    running = !exit;
}

void Befunge::clean(std::vector<std::vector<table_t>>& table) {
    long unsigned int maxlen = 0;
    for (unsigned int i = 0; i < table.size(); i++) {
        std::vector<table_t> line = table[i];
        if (line.size() > maxlen) {
            maxlen = line.size();
        }
    }
    // printf("maxlen: %lu\n", maxlen);
    for (unsigned int i = 0; i < table.size(); i++) {
        // printf("size init: %d\n", table[i].size());
        table[i].resize(maxlen, 0);
        // printf("size post: %d\n", table[i].size());
    }
}

table_t Befunge::pop() {
    if (stack.size() > 0) {
        int64_t val = stack.back();
        stack.pop_back();
        return val;
    } else {
        return 0;
    }
}

void Befunge::push(table_t x) {
    stack.push_back(x);
}

int64_t Befunge::popvector() {
    if (return_vector_stack.size() > 0) {
        int64_t val = return_vector_stack.back();
        return_vector_stack.pop_back();
        return val;
    } else {
        // maybe we should warn the users...
        return 0;
    }
}

void Befunge::pushvector(int64_t x) {
    return_vector_stack.push_back(x);
}

void Befunge::advanceIP(int x) {
    switch (direction) {
        case '>': ip[0] += x; break;
        case '<': ip[0] -= x; break;
        case '^': ip[1] -= x; break;
        case 'v': ip[1] += x; break;
    }
}

const std::string reset("\033[0m");
const std::string green("\033[42m");

void DrawCircle(SDL_Renderer * renderer, int32_t centreX, int32_t centreY, int32_t radius) {
   const int32_t diameter = (radius * 2);

   int32_t x = (radius - 1);
   int32_t y = 0;
   int32_t tx = 1;
   int32_t ty = 1;
   int32_t error = (tx - diameter);

   while (x >= y) {
      // Each of the following renders an octant of the circle
      SDL_RenderDrawPoint(renderer, centreX + x, centreY - y);
      SDL_RenderDrawPoint(renderer, centreX + x, centreY + y);
      SDL_RenderDrawPoint(renderer, centreX - x, centreY - y);
      SDL_RenderDrawPoint(renderer, centreX - x, centreY + y);
      SDL_RenderDrawPoint(renderer, centreX + y, centreY - x);
      SDL_RenderDrawPoint(renderer, centreX + y, centreY + x);
      SDL_RenderDrawPoint(renderer, centreX - y, centreY - x);
      SDL_RenderDrawPoint(renderer, centreX - y, centreY + x);

      if (error <= 0) {
         ++y;
         error += ty;
         ty += 2;
      }

      if (error > 0) {
         --x;
         tx += 2;
         error += (tx - diameter);
      }
   }
}


void Befunge::step() {
    if (ip[1] < 0 || ip[0] < 0 || table.size() <= (unsigned)ip[1] || table[ip[1]].size() <= (unsigned)ip[0]) {
        // off the board
        running = false;
        return;
    }
    char curr_char = table[ip[1]][ip[0]];
    if (debug) {
    
        printf("%d, %d, ", ip[0], ip[1]);
        printf("%c, [", curr_char);
        for (int i = 0; i < stack.size(); i++) {
            printf("%li, ", stack[i]);
        }
        printf("]\n");
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
            case '$': { if (stack.size() > 0) stack.pop_back(); return;}
            case '#': { advanceIP(1); return;}
            case '!': { int a = pop(); push(a == 0 ? 1 : 0); return;}
            case '`': { int a = pop(); int b = pop(); push(b > a ? 1 : 0); return; }
            case '|': { int a = pop(); direction = (a == 0 ? 'v' : '^'); return; }
            case '_': { int a = pop(); direction = (a == 0 ? '>' : '<'); return; }
            case '\\': {int a = pop(); int b = pop(); push(a); push(b); return; }
            case '?' : { direction = "><v^"[rand() % 4]; return;}
            case '.': if (!quiet) { printf("%li ", pop()); } else { pop(); } return;
            case ',': if (!quiet) { printf("%c", (char)pop()); } else { pop(); } return;

            case '~': {
                push(fgetc(stdin));
                break;
            }
            
            case 'j': {
                int b = pop();
                int a = pop();
                if (a < 0 || b < 0) {
                    if (a == -1 && b == 0) {
                        if (renderer == NULL) return;
                        int64_t y = pop();
                        int64_t x = pop();
                        int64_t radius = pop();
                        DrawCircle(renderer, x, y, radius);
                    }
                } else {
                    // real jump
                    pushvector(ip[0]);
                    pushvector(ip[1]);
                    pushvector(direction);
                    ip[0] = a;
                    ip[1] = b;
                    direction = '>';
                advanceIP(-1);
                }
                return;
            }

            case 'r':
                direction = (char)popvector();
                ip[1] = popvector();
                ip[0] = popvector();
                //advanceIP(-1);
                return;

            case 'g': {
                int y = pop();
                int x = pop();
                // if (in_range(x, y)) {
                    push(table[y][x]);
                // } else {
                    // push(0);
                // }
                return;
            }

            case 'p': {
                int64_t y = pop();
                int64_t x = pop();
                table_t c = pop();
                // TODO resize to fit
                if (y >= 0 && y < table.size() && x >= 0 && x < table[y].size()) {
                    table[y][x] = c;
                }
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
                if (renderer == NULL) { return; }
                SDL_RenderClear(renderer);
                return;

            case 'x': {
                if (renderer == NULL) { return; }
                int b = pop();
                int a = pop();
                SDL_RenderDrawPoint(renderer, a, b);
                return;
            }

            case 'z': {
                if (renderer == NULL) { return; }
                SDL_Event event;
                SDL_PollEvent(&event);
                if (event.type == SDL_WINDOWEVENT) {
                     if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                         push(1);
                     } else {
                        push(0);
                    }
                } else if (event.type == SDL_KEYDOWN) {
                    push(event.key.keysym.sym);
                    push(2);
                    //printf("%d\n", event.key.keysym.sym);
                } else if (event.type == SDL_KEYUP) {
                    push(event.key.keysym.sym);
                    push(3);
                } else {
                    push(0);
                }
                return;
            }

            case 'l': {
                if (renderer == NULL) { return; }
                int y2 = pop();
                int x2 = pop();
                int y1 = pop();
                int x1 = pop();
                SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
                return;
            }

            case 'f': {
                if (renderer == NULL) { return; }
                int b = pop();
                int g = pop();
                int r = pop();
                SDL_SetRenderDrawColor(renderer, r, g, b, 0xFF);
                return;
            }

            case 'u':
                if (renderer == NULL) { return; }
                SDL_RenderPresent(renderer);
                return;


            case '@': running = false; return;
        }
    }
}

void Befunge::loadCode(std::string code) {
    std::stringstream ss(code);
    std::string to;

    while(std::getline(ss,to,'\n')) {
        table.push_back(std::vector<table_t>(to.begin(), to.end()));
    }
    clean(table);
}

// Robert Jenkins' 96 bit Mix Function
unsigned long mix(unsigned long a, unsigned long b, unsigned long c) {
    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);
    return c;
}

void Befunge::run() {
    unsigned long seed = mix(clock(), time(NULL), 0);
    srand(seed);
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
}
