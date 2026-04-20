//
// Created by ender on 2026/4/20.
//

#include <iostream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif
#include "world/MapDrawer.h"
#include "shared/plan.h"
using namespace cns;

const std::string testmap1=
        "0123456789\n"
        "=        =\n"
        "=        =\n"
        "= @      =\n"
        "0123456789";


const char* testmap2=
        "============================================================================\n"
        "=                                                                          =\n"
        "=                                                                          =\n"
        "=                                          ==========                      =\n"
        "=                                                                          =\n"
        "=                                    ===                                   =\n"
        "=                                                                          =\n"
        "=                              ===                                         =\n"
        "=                                                                          =\n"
        "=                       ===                                                =\n"
        "=                                                                          =\n"
        "=                 ===                                                      =\n"
        "=                                                                          =\n"
        "=         =======                                                          =\n"
        "=  @                                                                       =\n"
        "============================================================================\n"
        "============================================================================\n"
        "============================================================================";


MapDrawer::MapDrawer() {
    currentmap=testmap2;
    consolePrepared = false;
}

MapDrawer::~MapDrawer() {
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != INVALID_HANDLE_VALUE && consolePrepared) {
        CONSOLE_CURSOR_INFO cursorInfo;
        if (GetConsoleCursorInfo(console, &cursorInfo)) {
            cursorInfo.bVisible = TRUE;
            SetConsoleCursorInfo(console, &cursorInfo);
        }
    }
#else
    if (consolePrepared) {
        std::cout << "\x1b[?25h";
    }
#endif
    std::cout << std::endl;
}

void MapDrawer::prepareConsole() {
    if (consolePrepared) {
        return;
    }

#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != INVALID_HANDLE_VALUE) {
        CONSOLE_CURSOR_INFO cursorInfo;
        if (GetConsoleCursorInfo(console, &cursorInfo)) {
            cursorInfo.bVisible = FALSE;
            SetConsoleCursorInfo(console, &cursorInfo);
        }

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD written = 0;
        COORD home = {0, 0};
        if (GetConsoleScreenBufferInfo(console, &csbi)) {
            DWORD cells = static_cast<DWORD>(csbi.dwSize.X) * static_cast<DWORD>(csbi.dwSize.Y);
            FillConsoleOutputCharacterA(console, ' ', cells, home, &written);
            FillConsoleOutputAttribute(console, csbi.wAttributes, cells, home, &written);
        }
        SetConsoleCursorPosition(console, home);
    }
#else
    std::cout << "\x1b[2J\x1b[?25l\x1b[H";
#endif

    consolePrepared = true;
}

void MapDrawer::draw() {
    prepareConsole();

#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != INVALID_HANDLE_VALUE) {
        COORD home = {0, 0};
        SetConsoleCursorPosition(console, home);
    }
#else
    std::cout << "\x1b[H";
#endif

    std::cout << currentmap << std::flush;

}
