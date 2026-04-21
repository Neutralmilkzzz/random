#include "input/KeyStateManager.h"

#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdlib>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#endif

#ifndef _WIN32
static struct termios orig_termios;
static bool terminal_configured = false;

namespace {

const int kMovementHoldFrames = 12;
const int kJumpHoldFrames = 12;
const int kActionHoldFrames = 2;

int holdFramesForKey(int keyCode) {
    switch (keyCode) {
    case 'a':
    case 'A':
    case 'd':
    case 'D':
    case 'w':
    case 'W':
    case 's':
    case 'S':
        return kMovementHoldFrames;
    case ' ':
        return kJumpHoldFrames;
    default:
        return kActionHoldFrames;
    }
}

void restoreTerminalMode() {
    if (!terminal_configured) {
        return;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);

    const int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    }

    terminal_configured = false;
}

} // namespace
#endif

int runtimeDashKeyCode() {
#ifdef _WIN32
    return 0x10;
#else
    return 'q';
#endif
}

const char* runtimeDashKeyLabel() {
#ifdef _WIN32
    return "SHIFT";
#else
    return "Q";
#endif
}

KeyStateManager::KeyStateManager() {
}

KeyStateManager::~KeyStateManager() {
#ifndef _WIN32
    restoreTerminalMode();
#endif
}

void KeyStateManager::readKeys() {
#ifdef _WIN32
    const char watchedLetters[] = {
            'A', 'C', 'D', 'E', 'W', 'S',
            'J', 'U', 'N', 'I', 'K', 'L', 'Q',
            'O', 'M', 'R', 'H',
            'P', 'X'
    };
    const char watchedDigits[] = {'1', '2', '3', '4'};

    for (size_t index = 0; index < sizeof(watchedLetters) / sizeof(watchedLetters[0]); ++index) {
        const char uppercase = watchedLetters[index];
        const char lowercase = static_cast<char>(uppercase + ('a' - 'A'));
        const bool pressed = (GetAsyncKeyState(uppercase) & 0x8000) != 0;

        keyStates[uppercase] = pressed;
        keyStates[lowercase] = pressed;
    }

    for (size_t index = 0; index < sizeof(watchedDigits) / sizeof(watchedDigits[0]); ++index) {
        const char digit = watchedDigits[index];
        keyStates[digit] = (GetAsyncKeyState(digit) & 0x8000) != 0;
    }

    keyStates[' '] = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    keyStates[runtimeDashKeyCode()] = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    keyStates[0x0D] = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
    keyStates[0x1B] = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
#else
    if (!terminal_configured) {
        struct termios new_termios;
        tcgetattr(STDIN_FILENO, &orig_termios);
        new_termios = orig_termios;
        new_termios.c_lflag &= ~(ICANON | ECHO);
        new_termios.c_cc[VMIN] = 0;
        new_termios.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        terminal_configured = true;
        std::atexit(restoreTerminalMode);
    }

    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        char ch;
        while (read(STDIN_FILENO, &ch, 1) == 1) {
            if (ch == '\r' || ch == '\n') {
                registerKeyState(0x0D, kActionHoldFrames);
            } else if (static_cast<unsigned char>(ch) == 0x1B) {
                registerKeyState(0x1B, kActionHoldFrames);
            } else if (ch == ' ') {
                registerKeyState(' ', kJumpHoldFrames);
            } else if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
                const char upper = ch >= 'a' && ch <= 'z' ? static_cast<char>(ch - ('a' - 'A')) : ch;
                const char lower = ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch + ('a' - 'A')) : ch;
                const int holdFrames = holdFramesForKey(lower);

                registerKeyState(upper, holdFrames);
                registerKeyState(lower, holdFrames);
            } else if (ch >= '1' && ch <= '4') {
                registerKeyState(ch, kActionHoldFrames);
            }
        }
    }
#endif
}

void KeyStateManager::clearKeys() {
#ifdef _WIN32
    keyStates.clear();
#else
    keyStates.clear();

    std::unordered_map<int, int> nextHoldFrames;
    for (std::unordered_map<int, int>::const_iterator it = keyHoldFrames.begin();
         it != keyHoldFrames.end();
         ++it) {
        const int remaining = it->second - 1;
        if (remaining > 0) {
            nextHoldFrames[it->first] = remaining;
            keyStates[it->first] = true;
        }
    }

    keyHoldFrames.swap(nextHoldFrames);
#endif
}

#ifndef _WIN32
void KeyStateManager::registerKeyState(int keyCode, int holdFrames) {
    keyStates[keyCode] = true;
    const std::unordered_map<int, int>::iterator it = keyHoldFrames.find(keyCode);
    if (it == keyHoldFrames.end() || it->second < holdFrames) {
        keyHoldFrames[keyCode] = holdFrames;
    }
}
#endif
