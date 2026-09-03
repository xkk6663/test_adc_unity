# ==============================================================================
# Unity+CMock Demo 工程 CMake 构建脚本
# ==============================================================================
# 适用环境：安装了 CMake（≥3.10）的 Windows / Linux / macOS
#
# 本脚本实现"双目标编译"：
#   1. unit_test      —— PC 单元测试目标，链接 CMock 桩 mock_hal_adc.c
#   2. firmware_demo  —— 固件模拟目标，链接 firmware_main.c 中的真实 HAL 实现
#   两个目标共享同一份 battery.c 业务代码
#
# 同时集成 CTest，可接入 CI 流水线（如 GitHub Actions）作为质量门禁。
#
# 使用方法：
#   mkdir build && cd build
#   cmake ..
#   make            (Windows 用 mingw32-make 或 cmake --build .)
#   ctest           (运行单元测试)
# ==============================================================================

cmake_minimum_required(VERSION 3.10)
project(unity_cmock_demo C)

# C 语言标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译警告
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

# 头文件搜索路径：工程根目录 + Unity 框架目录
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/unity)

# ==============================================================================
# 目标1：PC 单元测试
# ==============================================================================
# 链接 CMock 桩 mock_hal_adc.c，替换真实 HAL 实现
add_executable(unit_test
    test_battery.c       # 单元测试用例
    battery.c            # 被测模块（业务逻辑）
    mock_hal_adc.c       # CMock 生成的 HAL 桩（预生成等效版本）
    unity/unity.c        # Unity 测试框架
)

# ==============================================================================
# 目标2：固件模拟
# ==============================================================================
# 链接 firmware_main.c，其中包含真实 HAL 的模拟实现
add_executable(firmware_demo
    firmware_main.c      # 固件主程序 + 真实 HAL 模拟实现
    battery.c            # 被测模块（同一份业务代码）
)

# ==============================================================================
# CTest 集成（可接入 CI 流水线）
# ==============================================================================
enable_testing()
add_test(
    NAME unit_test
    COMMAND unit_test
)
# 设置测试超时（秒）
set_tests_properties(unit_test PROPERTIES TIMEOUT 30)
