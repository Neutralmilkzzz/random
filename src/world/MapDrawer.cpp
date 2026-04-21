//
// Created by ender on 2026/4/20.
//

#include <iostream>
#include <sstream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif
#include "world/MapDrawer.h"
#include "shared/plan.h"
using namespace cns;

namespace {

std::string buildStableFrame(const std::string& frame, size_t& lastFrameWidth, size_t& lastFrameHeight) {
    std::vector<std::string> lines;
    std::string currentLine;

    for (size_t index = 0; index < frame.size(); ++index) {
        if (frame[index] == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
        } else {
            currentLine.push_back(frame[index]);
        }
    }
    lines.push_back(currentLine);

    size_t currentWidth = 0;
    for (size_t index = 0; index < lines.size(); ++index) {
        if (lines[index].size() > currentWidth) {
            currentWidth = lines[index].size();
        }
    }

    const size_t targetWidth = currentWidth > lastFrameWidth ? currentWidth : lastFrameWidth;
    const size_t targetHeight = lines.size() > lastFrameHeight ? lines.size() : lastFrameHeight;

    std::ostringstream stableFrame;
    for (size_t row = 0; row < targetHeight; ++row) {
        std::string line = row < lines.size() ? lines[row] : std::string();
        if (line.size() < targetWidth) {
            line.append(targetWidth - line.size(), ' ');
        }
        stableFrame << line;
        if (row + 1 < targetHeight) {
            stableFrame << '\n';
        }
    }

    lastFrameWidth = currentWidth;
    lastFrameHeight = lines.size();
    return stableFrame.str();
}

} // namespace

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
    useAnsiCursorControl = false;
    lastFrameWidth = 0;
    lastFrameHeight = 0;
}

MapDrawer::~MapDrawer() {
#ifdef _WIN32
    if (useAnsiCursorControl) {
        std::cout << "\x1b[?25h";
    } else {
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        if (console != INVALID_HANDLE_VALUE && consolePrepared) {
            CONSOLE_CURSOR_INFO cursorInfo;
            if (GetConsoleCursorInfo(console, &cursorInfo)) {
                cursorInfo.bVisible = TRUE;
                SetConsoleCursorInfo(console, &cursorInfo);
            }
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
        DWORD consoleMode = 0;
        if (GetConsoleMode(console, &consoleMode)) {
            const DWORD ansiMode = consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (SetConsoleMode(console, ansiMode)) {
                useAnsiCursorControl = true;
            }
        }

        if (useAnsiCursorControl) {
            std::cout << "\x1b[2J\x1b[?25l\x1b[H";
        }

        CONSOLE_CURSOR_INFO cursorInfo;
        if (GetConsoleCursorInfo(console, &cursorInfo)) {
            cursorInfo.bVisible = FALSE;
            SetConsoleCursorInfo(console, &cursorInfo);
        }

        if (!useAnsiCursorControl) {
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
    }
#else
    std::cout << "\x1b[2J\x1b[?25l\x1b[H";
#endif

    consolePrepared = true;
}

void MapDrawer::draw() {
    prepareConsole();

    const std::string stableFrame = buildStableFrame(currentmap, lastFrameWidth, lastFrameHeight);

#ifdef _WIN32
    if (useAnsiCursorControl) {
        std::cout << "\x1b[H";
    } else {
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        if (console != INVALID_HANDLE_VALUE) {
            COORD home = {0, 0};
            SetConsoleCursorPosition(console, home);
        }
    }
#else
    std::cout << "\x1b[H";
#endif

    std::cout << stableFrame << std::flush;

}
