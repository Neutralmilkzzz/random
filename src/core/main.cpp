//
// Created by ender on 2026/4/20.
//


#include <thread>
#include "world/MapDrawer.h"
#include "player/Player.h"
#include "input/KeyStateManager.h"

int main(){
    MapDrawer mapDrawer1;
    std::vector<int> testmapiniplace=mapDrawer1.testmap2iniplace;
    KeyStateManager ksm;
    Player Player1(ksm);


    while(true){
        ksm.clearKeys();
        ksm.readKeys();
        if (ksm.keyStates[0x1B]) {
            break;
        }
        Player1.move(mapDrawer1.currentmap);
        mapDrawer1.draw();
        std::this_thread::sleep_for(std::chrono::milliseconds(16/*16*/));

    }
}
