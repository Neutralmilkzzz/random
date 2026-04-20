# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
LDFLAGS =

# 目标可执行文件（根据平台自动设置后缀）
ifeq ($(OS),Windows_NT)
    TARGET = testcpp1.exe
else
    TARGET = testcpp1
endif

# 源文件
SRCS = main.cpp \
       Player.cpp \
       MapDrawer.cpp \
       KeyStateManager.cpp

# 头文件
HEADERS = Player.h \
          MapDrawer.h \
          KeyStateManager.h \
          plan.h

# 对象文件
OBJS = $(SRCS:.cpp=.o)

# 默认目标
all: $(TARGET)

# 链接可执行文件
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# 编译源文件
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 平台无关的清理命令
clean:
ifeq ($(OS),Windows_NT)
	@if exist *.o del /Q *.o 2>nul
	@if exist $(TARGET) del /Q $(TARGET) 2>nul
else
	@rm -f $(OBJS) $(TARGET) 2>/dev/null || true
endif
	@echo "清理完成"

# 重新构建
rebuild: clean all

# 运行程序
run: $(TARGET)
ifeq ($(OS),Windows_NT)
	@echo "运行程序..."
	@$(TARGET)
else
	@echo "运行程序..."
	@./$(TARGET)
endif

# 调试模式
debug: CXXFLAGS += -g -DDEBUG
debug: rebuild

# 显示项目信息
info:
	@echo "=== 项目信息 ==="
	@echo "操作系统: $(OS)"
	@echo "编译器: $(CXX)"
	@echo "目标文件: $(TARGET)"
	@echo "源文件: $(SRCS)"
	@echo "头文件: $(HEADERS)"

# 显示帮助
help:
	@echo "可用命令:"
	@echo "  make all     - 编译程序 (默认)"
	@echo "  make clean   - 清理生成的文件"
	@echo "  make rebuild - 重新编译"
	@echo "  make run     - 编译并运行"
	@echo "  make debug   - 编译调试版本"
	@echo "  make info    - 显示项目信息"

# 伪目标
.PHONY: all clean rebuild run debug info help
