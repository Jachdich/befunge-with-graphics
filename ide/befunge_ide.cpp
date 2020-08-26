#include <ncurses.h>
#include <ctype.h>
#include <string>
#include <signal.h>
#include <vector>
#include <fstream>

struct ScreenPos {
    int x;
    int y;
};

struct Area {
    int x0 = -1;
    int y0 = -1;
    int x1 = -1;
    int y1 = -1;
};

void paste();
void cut();
void copy(int dummy);

bool running = true;

std::vector<std::vector<char>> clipboard;
std::vector<std::vector<char>> file;
std::vector<ScreenPos> posStack;

void pushPos() {
    int x,y;
    getyx(stdscr, y, x);
    posStack.push_back(ScreenPos(x, y));
}

void popPos() {
    ScreenPos pos = posStack.pop_back();
    wmove(stdscr, pos.y, pos.x);
}

void screenToBuffer(Area a, std::vector<std::vector<char>>& buffer) {
    for (int i = 0; i < buffer.size(); i++) {
        buffer[i].clear();
    }
    buffer.clear();

    if (buffer.size() < a.y1 - a.y0) {
        buffer.resize(a.y1 - a.y0);
    }

    pushPos();
    for (int j = sely0; j < sely1; j++) {
        if (buffer[j - a.y0].size() <= a.x1 - a.x0) {
            buffer[j - a.y0].resize(a.x1 - a.x0);
        }
        for (int i = selx0; i < a.x1; i++) {
            int cha =  mvinch(j, i);
            buffer[j - a.y0][i - a.x0] = (char)cha;
        }
    }
    popPos();
}

void bufferToScreen(ScreenPos p, std::vector<std::vector<char>>& buffer) {
    pushPos();
    for (int j = 0; j < buffer.size(); j++) {
        for (int i = 0; i < buffer[j].size(); i++) {
            int cha = buffer[j][i];
            mvaddch(j + p.y, i + p.x, cha);
        }
    }
    popPos();
}

void log(std::string x) {
    std::ofstream f("log.txt", std::ios::app);
    f << x << "\n";
    f.close();
}

void inpchar(char ch) {
    addch(ch);
    int x,y;
    getyx(stdscr, y, x);
    if (y >= file.size()) {
        file.resize(y + 1);
    }
    if (x >= file[y].size()) {
        file[y].resize(x + 1, ' ');
    }
    file[y][x] = ch;
}

void backspace(){}

void mainLoop() {
    int ch = getch();
    if (isprint(ch) || ch == 10) {
        inpchar(ch);
        //printw((std::to_string(ch) + " ").c_str()); refresh();
    }

    int y,x;
    if (ch == KEY_UP || ch == 337) {
        getyx(stdscr, y, x);
        wmove(stdscr, y - 1, x);
    } else if (ch == KEY_LEFT || ch == 393) {
        getyx(stdscr, y, x);
        wmove(stdscr, y, x - 1);
    } else if (ch == KEY_DOWN || ch == 336) {
       getyx(stdscr, y, x);
       wmove(stdscr, y + 1, x);
   } else if (ch == KEY_RIGHT || ch == 402) {
       getyx(stdscr, y, x);
       wmove(stdscr, y, x + 1);
   }

   switch (ch) {
       case 27: running = false; break;
        case KEY_BACKSPACE: backspace(); break;
        case 402: //shr
        case 337: //shu
        case 393: //shl
        case 336: if (selx0 == -1) { selx0 = x; sely0 = y; } selx1 = x + 1; sely1 = y + 1; break; //shd
        case KEY_LEFT:
        case KEY_RIGHT:
        case KEY_UP:
        case KEY_DOWN: {
            if (selx0 < 0) break;
            pushPos();
            for (int i = selx0; i < selx1; i++) {
                for (int j = sely0; j < sely1; j++) {
                    int cha =  mvinch(j, i);
                    mvaddch(j, i, cha & (A_REVERSE ^ 0xFFFFFFFF));
                }
            }
            selx0 = -1;
            sely0 = -1;
            popPos();
            break;
        }
        case 22: paste(); break;
        case 24: cut(); break;
    }
    if (selx0 >= 0) {
        pushPos();
        for (int i = selx0; i < selx1; i++) {
            for (int j = sely0; j < sely1; j++) {
                int cha =  mvinch(j, i);
                mvaddch(j, i, cha | A_REVERSE);
            }
        }
        popPos();
    }
    refresh();
}

void copy(int dummy) {
    int x3, y3;
    getyx(stdscr, y3, x3);

    for (int i = 0; i < clipboard.size(); i++) {
        clipboard[i].clear();
    }
    clipboard.clear();

    if (clipboard.size() < sely1 - sely0) {
        clipboard.resize(sely1 - sely0);
    }
    for (int j = sely0; j < sely1; j++) {
        if (clipboard[j - sely0].size() <= selx1 - selx0) {
            clipboard[j - sely0].resize(selx1 - selx0);
        }
        for (int i = selx0; i < selx1; i++) {
            int cha =  mvinch(j, i);
            clipboard[j - sely0][i - selx0] = (char)cha;
        }
    }
    wmove(stdscr, y3, x3);
}

void cut() {
    copy(0);
    int x3, y3;
    getyx(stdscr, y3, x3);
    for (int i = selx0; i < selx1; i++) {
        for (int j = sely0; j < sely1; j++) {
            mvaddch(j, i, ' ');
        }
    }
    wmove(stdscr, y3, x3);
}

void paste() {
    int x3, y3;
    getyx(stdscr, y3, x3);

    wmove(stdscr, y3, x3);
    refresh();
}

int main() {
    signal(SIGINT, copy);
    initscr();
    raw();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    while (running) {
        mainLoop();
    }
    endwin();
    return 0;
}
