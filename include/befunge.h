#ifndef BEFUNGE_H
#define BEFUNGE_H
#include <string>
#include <vector>
#include <SDL2/SDL.h>

typedef signed long table_t;
#define stacksize 1024 * 1024 * 102

class Befunge {
private:
    std::vector<std::vector<table_t>> table;
    std::vector<int64_t> stack;
    std::vector<int64_t> return_vector_stack;
    
    int32_t  ip[2] = {0, 0};
    char direction = '>';
    bool running = true;
    int  tablelen = 0;
    bool stringMode = false;
    
    SDL_Surface * screen = NULL;
    SDL_Window  * window = NULL;
    SDL_Renderer * renderer = NULL;

    void error(std::string msg, bool exit);
    void clean(std::vector<std::vector<table_t>>& table);

    table_t pop();
    void push(table_t x);

    int64_t popvector();
    void pushvector(int64_t x);
    void advanceIP(int x);

public:
    bool quiet = false;
    bool debug = false;
    
    void step();
    void loadCode(std::string code);
    void run();
    inline Befunge() {}
};

#endif
