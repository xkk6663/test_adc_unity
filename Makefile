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

# 跨平台可执行文件后缀检测：
#   Windows 下 gcc 生成的可执行文件需带 .exe 后缀；
#   Linux / macOS 下不需要后缀。CI 运行在 Linux 上，
#   若硬编码 .exe 会导致 ./unit_test 找不到文件。
ifeq ($(OS),Windows_NT)
    EXE_EXT = .exe
else
    EXE_EXT =
endif

# ==============================================================================
# 目标1：PC 单元测试（链接 CMock 桩）
# ==============================================================================
# 源文件说明：
#   test_battery.c   —— battery 模块的测试用例（11 个）
#   test_led.c       —— led 模块的测试用例（8 个）
#   test_support.c   —— 统一的 setUp/tearDown（管理所有 CMock 桩）
#   battery.c        —— 被测模块1：电池电压采集
#   led.c            —— 被测模块2：LED 指示灯控制
#   mock_hal_adc.c   —— ADC 接口桩（battery 模块依赖）
#   mock_hal_gpio.c  —— GPIO 接口桩（led 模块依赖）
#   unity/unity.c    —— Unity 测试框架
#   cmock/src/cmock.c —— CMock 运行时
#
# 新增模块时，只需在此处添加被测模块 .c、测试文件 .c、对应桩 .c，
# CI 会自动编译运行所有测试，无需修改 CI 配置。
TEST_SRCS = test_battery.c test_led.c test_support.c \
            battery.c led.c \
            mock_hal_adc.c mock_hal_gpio.c \
            unity/unity.c cmock/src/cmock.c
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_BIN  = unit_test$(EXE_EXT)

# ==============================================================================
# 目标2：固件模拟（链接真实 HAL 实现）
# ==============================================================================
FW_SRCS = firmware_main.c battery.c
FW_OBJS = $(FW_SRCS:.c=.o)
FW_BIN  = firmware_demo$(EXE_EXT)

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
