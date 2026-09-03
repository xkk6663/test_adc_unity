# ==============================================================================
# Unity+CMock Demo 工程 Makefile
# ==============================================================================
# 适用环境：
#   - Windows + MinGW：使用 mingw32-make
#   - Linux / macOS：  使用 make（需安装 gcc）
#
# 构建目标：
#   make            编译两个目标（unit_test + firmware_demo）
#   make test       编译并运行单元测试
#   make firmware   编译并运行固件模拟
#   make clean      清理构建产物
#
# 双目标说明：
#   unit_test      —— 链接 CMock 桩 mock_hal_adc.c，在 PC 上跑单元测试
#   firmware_demo  —— 链接 firmware_main.c 中的真实 HAL 模拟，模拟固件运行
#   两个目标共享同一份 battery.c 业务代码
# ==============================================================================

# 编译器与编译选项
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -I. -Iunity -Icmock/src
LDFLAGS =

# ==============================================================================
# 目标1：PC 单元测试（链接 CMock 桩）
# ==============================================================================
TEST_SRCS = test_battery.c battery.c mock_hal_adc.c unity/unity.c cmock/src/cmock.c
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_BIN  = unit_test.exe

# ==============================================================================
# 目标2：固件模拟（链接真实 HAL 实现）
# ==============================================================================
FW_SRCS = firmware_main.c battery.c
FW_OBJS = $(FW_SRCS:.c=.o)
FW_BIN  = firmware_demo.exe

# ==============================================================================
# 默认目标：编译两个目标
# ==============================================================================
all: $(TEST_BIN) $(FW_BIN)

# 单元测试可执行文件
$(TEST_BIN): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 固件模拟可执行文件
$(FW_BIN): $(FW_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 通用编译规则：.c → .o
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ==============================================================================
# 便捷目标
# ==============================================================================

# 编译并运行单元测试
test: $(TEST_BIN)
	./$(TEST_BIN)

# 编译并运行固件模拟
firmware: $(FW_BIN)
	./$(FW_BIN)

# 清理所有构建产物（自动适配 Windows / Linux / macOS）
# Windows 下用 if exist + del，避免文件不存在时 del 报错；
# 类 Unix 下用 rm -f。不再混用 rm（Windows cmd 无此命令）。
clean:
ifeq ($(OS),Windows_NT)
	@if exist *.o del /Q *.o
	@if exist unity\*.o del /Q unity\*.o
	@if exist $(TEST_BIN) del /Q $(TEST_BIN)
	@if exist $(FW_BIN) del /Q $(FW_BIN)
else
	rm -f *.o unity/*.o $(TEST_BIN) $(FW_BIN)
endif

# 声明伪目标（不与文件名冲突）
.PHONY: all test firmware clean
