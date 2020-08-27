#include <ncurses.h>
#include <ctype.h>
#include <string>
#include <signal.h>
#include <vector>
#include <fstream>

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

void paste();
void cut();
void copy(int dummy);

ScreenPos max;
ScreenPos pos;
Area sel = {-1, -1, -1, -1};
bool running = true;
bool insert = false;

std::vector<std::vector<char>> clipboard;
std::vector<std::vector<char>> file;
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

void screenToBuffer(Area a, std::vector<std::vector<char>>& buffer) {
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

void bufferToScreen(ScreenPos p, std::vector<std::vector<char>>& buffer) {
    for (int j = 0; j < buffer.size(); j++) {
        for (int i = 0; i < buffer[j].size(); i++) {
            int cha = buffer[j][i];
            mvaddch(j + p.y, i + p.x, cha);
        }
    }
}

void bufferToBuffer(ScreenPos p, std::vector<std::vector<char>>& a, std::vector<std::vector<char>>& b) {
    if (b.size() <= (a.size() + p.y)) {
        b.resize(a.size() + p.y);
    }
    for (int j = 0; j < a.size(); j++) {
        if (b[j + pos.y].size() <= a[j].size() + p.x) {
            b[j + pos.y].resize(a[j].size() + p.x, ' ');
        }
        for (int i = 0; i < a[j].size(); i++) {
            b[j + pos.y][i + pos.x] = a[j][i];
        }
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
        pos.x += 1;
    }
    setPos(pos);
    if (beforePos.y >= file.size()) {
        file.resize(beforePos.y + 1);
    }
    if (beforePos.x >= file[beforePos.y].size()) {
        file[beforePos.y].resize(beforePos.x + 1, ' ');
    }
    if (insert) {
        if (ch == 10) {
            file.insert(file.begin() + pos.y, std::vector<char>());
        } else {
            file[beforePos.y].insert(beforePos.x + file[beforePos.y].begin(), ch);
        }
    } else {
        file[beforePos.y][beforePos.x] = ch;
    }
}

void backspace() {
    ScreenPos beforePos = pos;
    mvaddch(pos.y, pos.x - 1, ' ');
    pos = pos - ScreenPos(1, 0);
    setPos(pos);
    if (beforePos.y >= file.size()) { return; }
    if (beforePos.x > file[beforePos.y].size()) { return; }
    if (insert) {
        file[pos.y].erase(file[pos.y].begin() + pos.x);
    } else {
        file[pos.y][pos.x] = ' ';
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
    pushPos();

    setPos(ScreenPos(0, max.y));
    if (insert) {
        printw("Insert Mode: on ");
    } else {
        printw("Insert Mode: off");
    }
    popPos();
}

void mainLoop() {
    int ch = getch();
    erase();
    if (isprint(ch) || ch == 10) {
        inpchar(ch);
    }
    //printw((std::to_string(ch) + " ").c_str()); refresh();
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

   switch (ch) {
       case 27: running = false; break;
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
        case KEY_END: setPos(ScreenPos(max.x, getPos().y)); break;
        case KEY_HOME: setPos(ScreenPos(0, getPos().y)); break;
        case 9: toggleInsert(); break;
    }
    if (sel.x0 >= 0) {
        addEffect(sel, A_REVERSE);
    }
    bufferToScreen(ScreenPos(0, 0), file);
    if (sel.x0 > -1) {
        addEffect(sel, A_REVERSE);
    }
    drawBottomBar();
    setPos(pos);
    refresh();
}

void copy(int dummy) {
    screenToBuffer(sel, clipboard);
}

void cut() {
    copy(0);
    std::vector<std::vector<char>> spaces = std::vector<std::vector<char>>(sel.height(), std::vector<char>(sel.width(), ' '));
    bufferToBuffer(ScreenPos(sel.x0, sel.y0), spaces, file);
}

void paste() {
    bufferToScreen(pos, clipboard);
    bufferToBuffer(pos, clipboard, file);
    refresh();
}

int main() {
    signal(SIGINT, copy);
    initscr();
    raw();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    int y,x;
    getmaxyx(stdscr, y, x);
    max = ScreenPos(x - 1, y - 1);
    drawBottomBar();
    while (running) {
        mainLoop();
    }
    endwin();
    return 0;
}
