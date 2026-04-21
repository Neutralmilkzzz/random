//
// Created by ender on 2026/4/20.
//

#ifndef TESTCPP1_MAPDRAWER_H
#define TESTCPP1_MAPDRAWER_H

#include <vector>
#include <string>

class MapDrawer {
public:
    std::string currentmap;
    std::vector<int> testmap1iniplace;
    std::vector<int> testmap2iniplace;
    bool consolePrepared;
    bool useAnsiCursorControl;
    size_t lastFrameWidth;
    size_t lastFrameHeight;

public:
    MapDrawer();
    ~MapDrawer();
    void draw();

private:
    void prepareConsole();

};


#endif //TESTCPP1_MAPDRAWER_H
