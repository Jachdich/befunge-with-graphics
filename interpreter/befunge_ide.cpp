#include <ncurses.h>
#include <ctype.h>
#include <string>
#include <signal.h>
#include <vector>
#include <fstream>
#include <iostream>
#include "befunge.h"

typedef std::vector<std::vector<char>> buffer_t;

struct ScreenPos {
    int x;
    int y;
    ScreenPos() {}
    ScreenPos(int x, int y) {
        this->x = x;
        this->y = y;
    }
    ScreenPos operator-(const ScreenPos &other) {
        int x = this->x - other.x;
        int y = this->y - other.y;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        return ScreenPos(x, y);
    }

    ScreenPos operator+=(const ScreenPos& other) {
        this->x += other.x;
        this->y += other.y;
        return ScreenPos(this->x, this->y);
    }

    ScreenPos operator+(const ScreenPos& other) {
        return ScreenPos(this->x + other.x, this->y + other.y);
    }

    void setBounds(ScreenPos max) {
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > max.x) x = max.x;
        if (y > max.y) y = max.y;
    }
};

struct Area {
    int x0 = -1;
    int y0 = -1;
    int x1 = -1;
    int y1 = -1;

    int width() {
        return x1 - x0;
    }
    int height() {
        return y1 - y0;
    }
};

/*
struct Buffer {
    buffer_t data;

    Buffer() {}

    char& operator()(const int a, const int b) {
        return data[b][a];
    }
};*/

void paste();
void cut();
void copy(int dummy);

std::string statusLine = "";
std::string filename;
ScreenPos max;
ScreenPos pos;
ScreenPos dir = ScreenPos(1, 0);
ScreenPos offset = ScreenPos(0, 0);
Area sel = {-1, -1, -1, -1};
bool running = true;
bool insert = false;

buffer_t clipboard;
buffer_t file;
std::vector<ScreenPos> posStack;

ScreenPos getPos() {
    int x,y;
    getyx(stdscr, y, x);
    return ScreenPos(x, y);
}

void setPos(ScreenPos pos) {
    wmove(stdscr, pos.y, pos.x);
}

void pushPos() {
    posStack.push_back(getPos());
}

void popPos() {
    ScreenPos pos = posStack.back();
    posStack.pop_back();
    setPos(pos);
}

void screenToBuffer(Area a, buffer_t& buffer) {
    for (int i = 0; i < buffer.size(); i++) {
        buffer[i].clear();
    }
    buffer.clear();

    if (buffer.size() < a.y1 - a.y0) {
        buffer.resize(a.y1 - a.y0);
    }

    for (int j = a.y0; j < a.y1; j++) {
        if (buffer[j - a.y0].size() <= a.x1 - a.x0) {
            buffer[j - a.y0].resize(a.x1 - a.x0);
        }
        for (int i = a.x0; i < a.x1; i++) {
            int cha =  mvinch(j, i);
            buffer[j - a.y0][i - a.x0] = (char)cha;
        }
    }
}

void bufferToScreen(ScreenPos p, buffer_t& buffer) {
    for (int j = 0; j < buffer.size(); j++) {
        for (int i = 0; i < buffer[j].size(); i++) {
            mvaddch(j + p.y, i + p.x, buffer[j][i]);
        }
    }
}

void bufferToBuffer(Area from, ScreenPos to, buffer_t &a, buffer_t& b) {
    if (b.size() <= from.height() + to.y) {
        b.resize(from.height() + to.y);
    }
    for (int j = 0; j < from.height(); j++) {
        if (b[j + to.y].size() <= from.width() + to.x) {
            b[j + to.y].resize(from.width() + to.x, ' ');
        }
        for (int i = 0; i < from.width(); i++) {
            b[j + to.y][i + to.x] = a[j + from.y0][i + from.x0];
        }
    }
}

void bufferToBuffer(ScreenPos p, buffer_t &a, buffer_t& b) {
    Area from = {0, 0, a[a.size() - 1].size(), a.size()};
    bufferToBuffer(from, p, a, b);
}

void fileToBuffer(std::string filename, buffer_t& buffer) {
    std::string line;
    std::ifstream f(filename);
    if (f.is_open()) {
        while (getline(f, line)) {
            buffer.push_back(std::vector<char>(line.begin(), line.end()));
        }
        statusLine = "[Read " + std::to_string(file.size()) + " lines]";
    } else {
        statusLine = "[New File]";
    }
    f.close();
}

void bufferToFile(std::string filename, buffer_t& buffer) {
    std::ofstream f(filename);
    for (int i = 0; i < buffer.size(); i++) {
        for (int j = 0; j < buffer[i].size(); j++) {
            f << buffer[i][j];
        }
        f << "\n";
    }
    statusLine = "[Wrote " + std::to_string(file.size()) + " lines]";
}

void stripEmptyLines(buffer_t& buffer) {
    int remove_from = -1;
    for (int i = buffer.size() - 1; i >= 0; i--) {
        for (char c : buffer[i]) {
            if (c != ' ') {
                remove_from = i + 1;
                break;
            }
        }
        if (remove_from != -1) break;
    }
    if (remove_from != -1) {
        buffer.erase(buffer.begin() + remove_from, buffer.end());
    }
}

void log(std::string x) {
    std::ofstream f("log.txt", std::ios::app);
    f << x << "\n";
    f.close();
}

void toggleInsert() {
    insert = !insert;
}

void inpchar(char ch) {
    ScreenPos beforePos = pos;
    if (ch == 10) { //newline
        pos.y += 1;
    } else {
        pos += dir;
    }
    ScreenPos adj = beforePos - offset;
    if (adj.y >= file.size()) {
        file.resize(adj.y + 1);
    }
    if (adj.x >= file[adj.y].size()) {
        file[adj.y].resize(adj.x + 1, ' ');
    }
    if (insert) {
        if (ch == 10) {
            file.insert(file.begin() + pos.y + offset.y, std::vector<char>());
        } else {
            file[adj.y].insert(adj.x + file[adj.y].begin(), ch);
        }
    } else {
        file[adj.y][adj.x] = ch;
    }
}

void backspace() {
    ScreenPos beforePos = pos;
    mvaddch(pos.y - offset.y, pos.x - offset.x - 1, ' ');
    pos = pos - dir;
    ScreenPos adj = beforePos - offset,
              adja= pos - offset;
    if (adj.y >= file.size()) { return; }
    if (adj.x > file[adj.y].size()) { return; }
    if (insert) {
        file[adja.y].erase(file[adja.y].begin() + adja.x);
    } else {
        file[adja.y][adja.x] = ' ';
    }
}

void clearEffect(Area a, int effect) {
    for (int i = a.x0; i < a.x1; i++) {
        for (int j = a.y0; j < a.y1; j++) {
            int cha =  mvinch(j, i);
            mvaddch(j, i, cha & (effect ^ 0xFFFFFFFF));
        }
    }
}

void addEffect(Area a, int effect) {
    for (int i = a.x0; i < a.x1; i++) {
        for (int j = a.y0; j < a.y1; j++) {
            int cha =  mvinch(j, i);
            mvaddch(j, i, cha | effect);
        }
    }
}

void drawBottomBar() {
    setPos(ScreenPos(0, max.y));
    if (insert) {
        printw("Insert Mode: on ");
    } else {
        printw("Insert Mode: off");
    } //16 chars
    setPos(ScreenPos(max.x / 2 - statusLine.length() / 2, max.y));
    printw(statusLine.c_str());
}

void save() {
    stripEmptyLines(file);
    bufferToFile(filename, file);
}

void scrollDown() {
    offset.y -= 1;
}

void scrollUp() {
    offset.y += 1;
    if (offset.y > 0) offset.y = 0;
}

void scrollLeft() {
    offset.x += 1;
    if (offset.x > 0) offset.x = 0;
}

void scrollRight() {
    offset.x -= 1;
}

int x = 0;
bool progRunning = false;
void run() {
    progRunning = true;
}

void mainLoop() {
    if (progRunning) {
        timeout(0);
        int ch = getch();
        step();
        if (ch == 10) progRunning = false;
        refresh();
        return;
    }
    timeout(-1);
    int ch = getch();
    erase();
    if (isprint(ch) || ch == 10) {
        inpchar(ch);
    }
    //printw((std::to_string(ch) + " ").c_str()); if (ch == 9) running = false; refresh(); return;
    ScreenPos before = pos;
    if (ch == KEY_UP || ch == 337) {
        pos.y -= 1;
    } else if (ch == KEY_LEFT || ch == 393) {
        pos.x -= 1;
    } else if (ch == KEY_DOWN || ch == 336) {
        pos.y += 1;
   } else if (ch == KEY_RIGHT || ch == 402) {
       pos.x += 1;
   }
   if (pos.y > max.y) {
       scrollDown();
   }
   if (pos.y < 0) {
       scrollUp();
   }
   if (pos.x > max.x) {
       scrollRight();
   }
   if (pos.x < 0) {
       scrollLeft();
   }

   pos.setBounds(max);

   switch (ch) {
       case 27: running = false; save(); break;
        case KEY_BACKSPACE: backspace(); break;
        case 402: //shr
        case 337: //shu
        case 393: //shl
        case 336:
        if (sel.x0 == -1) {
            sel.x0 = before.x;
            sel.y0 = before.y;
        }
        sel.x1 = pos.x + 1;
        sel.y1 = pos.y + 1;
        break; //shd
        case KEY_LEFT:
        case KEY_RIGHT:
        case KEY_UP:
        case KEY_DOWN: {
            if (sel.x0 < 0) break;
            clearEffect(sel, A_REVERSE);
            sel.x0 = -1;
            sel.y0 = -1;
            break;
        }
        case 22: paste(); break;
        case 24: cut(); break;
        case 19: save(); break;
        case KEY_END: pos.x = max.x; break;
        case KEY_HOME: pos.x = 0; break;
        case 9: toggleInsert(); break;
        case 17: running = false; break; //don't save, then exit
        case 566: dir = ScreenPos(0, -1); break; //ctrl-up
        case 560: dir = ScreenPos(1,  0); break; //ctrl-right
        case 525: dir = ScreenPos(0,  1); break; //ctrl-down
        case 545: dir = ScreenPos(-1, 0); break; //ctrl-left
        case 269: run(); break; //f5: run
    }
    bufferToScreen(offset, file);
    if (sel.x0 > -1) {
        addEffect(sel, A_REVERSE);
    }
    drawBottomBar();
    setPos(pos);
    refresh();
}

void copy(int dummy) {
    //screenToBuffer(sel, clipboard);
    bufferToBuffer(sel, ScreenPos(0, 0), file, clipboard);
}

void cut() {
    copy(0);
    buffer_t spaces = buffer_t(sel.height(), std::vector<char>(sel.width(), ' '));
    bufferToBuffer(ScreenPos(sel.x0, sel.y0), spaces, file);
}

void paste() {
    if (sel.x0 >= 0) { //there is a selection
        pos = ScreenPos(sel.x0, sel.y0); //set pos to top-left
    }
    bufferToScreen(pos, clipboard);
    bufferToBuffer(pos, clipboard, file);
}

int main(int argc, char ** argv) {
    if (argc != 2) {
        std::cout << "Usage: " + std::string(argv[0]) + " <filename>\n";
        return 1;
    }
    filename = std::string(argv[1]);
    signal(SIGINT, copy);
    initscr();
    raw();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    int y,x;
    getmaxyx(stdscr, y, x);
    max = ScreenPos(x - 1, y - 1);
    fileToBuffer(filename, file);
    bufferToScreen(ScreenPos(0, 0), file);
    drawBottomBar();
    pos = ScreenPos(0, 0);
    setPos(pos);
    while (running) {
        mainLoop();
    }
    endwin();
    return 0;
}
