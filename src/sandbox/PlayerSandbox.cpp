#include <chrono>
#include <iostream>
#include <thread>

#include "input/KeyStateManager.h"
#include "player/Player.h"
#include "world/MapDrawer.h"

int main() {
    std::cout << "[PlayerSandbox] ESC exit, reuse current movement prototype.\n";

    MapDrawer mapDrawer;
    KeyStateManager keyStateManager;
    Player player(keyStateManager);

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();
        if (keyStateManager.keyStates[0x1B]) {
            break;
        }

        player.move(mapDrawer.currentmap);
        mapDrawer.draw();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
