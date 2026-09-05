# ==============================================================================
# Unity+CMock Demo 工程 Makefile（产品级分层架构）
# ==============================================================================
# 适用环境：
#   - Windows + MinGW：使用 mingw32-make
#   - Linux / macOS：  使用 make（需安装 gcc）
#
# 构建目标：
#   make            编译两个目标（unit_test + firmware_demo）
#   make test       编译并运行单元测试
#   make firmware   编译并运行固件模拟
#   make mocks      用 CMock 重新生成桩文件
#   make clean      清理构建产物
#
# 代码架构分层（仿照 ELAB 框架）：
#   app/        —— 应用层（业务模块：battery、led）
#   hal/        —— 硬件抽象层接口（hal_adc.h、hal_gpio.h）
#   platform/   —— 平台层（固件主程序、真实 HAL 实现）
#   test/       —— 测试层（unit/ 单元测试、mocks/ CMock 桩）
#   framework/  —— 第三方框架（unity、cmock）
#   tools/      —— 构建工具脚本（gen_mocks.rb、CMockConfig.yml）
# ==============================================================================

# ==============================================================================
# 目录定义
# ==============================================================================
APP_DIR       = app
HAL_DIR       = hal
PLATFORM_DIR  = platform
TEST_DIR      = test
FRAMEWORK_DIR = framework
TOOLS_DIR     = tools

# ==============================================================================
# 编译器与编译选项
# ==============================================================================
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99
LDFLAGS =

# 头文件搜索路径（按架构分层添加）
# 产品级做法：每层代码只通过 -I 暴露自己的公共头文件目录，
# 避免跨层直接引用源文件目录。
INCLUDES = -I$(APP_DIR)/battery \
           -I$(APP_DIR)/led \
           -I$(HAL_DIR) \
           -I$(TEST_DIR)/mocks \
           -I$(FRAMEWORK_DIR)/unity \
           -I$(FRAMEWORK_DIR)/cmock/src

# 跨平台可执行文件后缀检测
ifeq ($(OS),Windows_NT)
    EXE_EXT = .exe
else
    EXE_EXT =
endif

# ==============================================================================
# 目标1：PC 单元测试（链接 CMock 桩）
# ==============================================================================
# 源文件按架构分层组织：
#   test/unit/    —— 各模块测试用例 + 统一测试固件
#   app/          —— 被测业务模块
#   test/mocks/   —— CMock 生成的 HAL 桩
#   framework/    —— Unity + CMock 运行时
TEST_SRCS = $(TEST_DIR)/unit/test_battery.c \
            $(TEST_DIR)/unit/test_led.c \
            $(TEST_DIR)/unit/test_support.c \
            $(APP_DIR)/battery/battery.c \
            $(APP_DIR)/led/led.c \
            $(TEST_DIR)/mocks/mock_hal_adc.c \
            $(TEST_DIR)/mocks/mock_hal_gpio.c \
            $(FRAMEWORK_DIR)/unity/unity.c \
            $(FRAMEWORK_DIR)/cmock/src/cmock.c

TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_BIN  = unit_test$(EXE_EXT)

# ==============================================================================
# 目标2：固件模拟（链接真实 HAL 实现）
# ==============================================================================
FW_SRCS = $(PLATFORM_DIR)/firmware/firmware_main.c \
          $(APP_DIR)/battery/battery.c

FW_OBJS = $(FW_SRCS:.c=.o)
FW_BIN  = firmware_demo$(EXE_EXT)

# ==============================================================================
# 默认目标：编译两个目标
# ==============================================================================
all: $(TEST_BIN) $(FW_BIN)

# 单元测试可执行文件
$(TEST_BIN): $(TEST_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

# 固件模拟可执行文件
$(FW_BIN): $(FW_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

# 通用编译规则：.c → .o
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# ==============================================================================
# 便捷目标
# ==============================================================================

# 编译并运行单元测试
test: $(TEST_BIN)
	./$(TEST_BIN)

# 编译并运行固件模拟
firmware: $(FW_BIN)
	./$(FW_BIN)

# 用 CMock 重新生成桩文件
# 需 Ruby 环境；桩输出到 test/mocks/ 目录
mocks:
	ruby $(TOOLS_DIR)/gen_mocks.rb

# 清理所有构建产物（自动适配 Windows / Linux / macOS）
clean:
ifeq ($(OS),Windows_NT)
	@if exist $(APP_DIR)\battery\*.o del /Q $(APP_DIR)\battery\*.o
	@if exist $(APP_DIR)\led\*.o del /Q $(APP_DIR)\led\*.o
	@if exist $(TEST_DIR)\unit\*.o del /Q $(TEST_DIR)\unit\*.o
	@if exist $(TEST_DIR)\mocks\*.o del /Q $(TEST_DIR)\mocks\*.o
	@if exist $(FRAMEWORK_DIR)\unity\*.o del /Q $(FRAMEWORK_DIR)\unity\*.o
	@if exist $(FRAMEWORK_DIR)\cmock\src\*.o del /Q $(FRAMEWORK_DIR)\cmock\src\*.o
	@if exist $(PLATFORM_DIR)\firmware\*.o del /Q $(PLATFORM_DIR)\firmware\*.o
	@if exist $(TEST_BIN) del /Q $(TEST_BIN)
	@if exist $(FW_BIN) del /Q $(FW_BIN)
else
	rm -f $(APP_DIR)/battery/*.o $(APP_DIR)/led/*.o
	rm -f $(TEST_DIR)/unit/*.o $(TEST_DIR)/mocks/*.o
	rm -f $(FRAMEWORK_DIR)/unity/*.o $(FRAMEWORK_DIR)/cmock/src/*.o
	rm -f $(PLATFORM_DIR)/firmware/*.o
	rm -f $(TEST_BIN) $(FW_BIN)
endif

# 声明伪目标（不与文件名冲突）
.PHONY: all test firmware mocks clean
