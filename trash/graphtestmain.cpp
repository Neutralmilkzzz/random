/*
// pure_ascii_animation.cpp
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    // 纯ASCII字符表情
    const char* happy =
            "  -----  \n"
            " | o o | \n"
            " |  ^  | \n"
            " | \\_/ | \n"
            "  -----  \n";

    const char* sad =
            "  -----  \n"
            " | o o | \n"
            " |  -  | \n"
            " | / \\ | \n"
            "  -----  \n";

    std::cout << "Press Ctrl+C to stop\n\n";

    for (int i = 0; i < 20; i++) {
        // 清屏方法1：输出大量换行
        for (int j = 0; j < 50; j++) {
            std::cout << '\n';
        }

        // 显示当前帧
        if (i % 2 == 0) {
            std::cout << "Status: HAPPY\n" << happy << std::endl;
        } else {
            std::cout << "Status: SAD\n" << sad << std::endl;
        }

        // 等待0.5秒
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
*/
