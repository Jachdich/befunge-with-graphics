#include <ncurses.h>
#include <ctype.h>
#include <string>

bool running = true;

int selx0 = 3;
int sely0 = 4;
int selx1 = 5;
int sely1 = 5;
void inpchar(char ch) {
    addch(ch);
    refresh();
}

void backspace(){}

void doStuff() {
    int ch = getch();
    if (isprint(ch) || ch == 10) {
        inpchar(ch);
        //printw((std::to_string(ch) + " ").c_str()); refresh();
    }
    switch (ch) {
       case KEY_BACKSPACE: backspace(); break;
       case KEY_UP: { int x,y; getyx(stdscr, y, x); wmove(stdscr, y - 1, x); break; }
       case KEY_LEFT: { int x,y; getyx(stdscr, y, x); wmove(stdscr, y, x - 1); break; }
       case KEY_DOWN: { int x,y; getyx(stdscr, y, x); wmove(stdscr, y + 1, x); break; }
       case KEY_RIGHT: { int x,y; getyx(stdscr, y, x); wmove(stdscr, y, x + 1); break; }
    }
    int y,x;
    getyx(stdscr, y, x);
    for (int i = selx0; i < selx1; i++) {
        for (int j = sely0; j < sely1; j++) {
            mvwaddch(stdscr, j, i, mvgetch(j, i) | A_UNDERLINE);
        }
    }
    wmove(stdscr, y, x);
}

int main() {
    initscr();
    raw();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    while (running) {
        doStuff();
    }
    endwin();
    return 0;
}
