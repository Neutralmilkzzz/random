/*
// keyboard_face.cpp
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <thread>

// 获取单个按键（无需回车）
int getKey() {
    struct termios oldt, newt;
    int ch;

    // 保存当前终端设置
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // 设置新属性：禁用规范模式和回显
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // 读取按键
    ch = getchar();

    // 恢复原始终端设置
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return ch;
}

// 清屏函数
void clearScreen() {
    std::cout << "\033[2J\033[H";
}

int main() {
    // ASCII表情定义
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

    const char* neutral =
            "  -----  \n"
            " | o o | \n"
            " |  -  | \n"
            " | ___ | \n"
            "  -----  \n";

    // 初始状态
    const char* currentFace = neutral;
    std::string status = "Neutral";

    clearScreen();
    std::cout << "=== 键盘表情控制 ===\n";
    std::cout << "按 'a' 键: 笑脸\n";
    std::cout << "按 'b' 键: 哭脸\n";
    std::cout << "按 'q' 键: 退出\n\n";

    bool running = true;

    while (running) {
        // 显示当前状态
        std::cout << "状态: " << status << "\n\n";
        std::cout << currentFace << "\n";
        std::cout << "等待按键... (a/b/q) ";

        // 获取按键
        int key = getKey();

        // 处理按键
        clearScreen();
        std::cout << "=== 键盘表情控制 ===\n";
        std::cout << "按 'a' 键: 笑脸\n";
        std::cout << "按 'b' 键: 哭脸\n";
        std::cout << "按 'q' 键: 退出\n\n";

        switch (key) {
            case 'a':
            case 'A':
                currentFace = happy;
                status = "Happy (a键按下)";
                break;

            case 'b':
            case 'B':
                currentFace = sad;
                status = "Sad (b键按下)";
                break;

            case 'q':
            case 'Q':
                running = false;
                std::cout << "退出程序...\n";
                break;

            default:
                currentFace = neutral;
                status = "Neutral (按了其他键)";
                break;
        }
    }

    // 显示退出信息
    clearScreen();
    std::cout << "程序结束！\n";

    return 0;
}
*/
