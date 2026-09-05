# ==============================================================================
# Unity+CMock Demo 工程 CMake 构建脚本（产品级分层架构）
# ==============================================================================
# 唯一构建系统：CMake（≥3.10），支持 Windows / Linux / macOS
#
# 代码架构分层（仿照 ELAB 框架）：
#   app/        —— 应用层（业务模块：battery、led）
#   hal/        —— 硬件抽象层接口
#   platform/   —— 平台层（固件主程序）
#   test/       —— 测试层（unit/ 单元测试、mocks/ CMock 桩）
#   framework/  —— 第三方框架（unity、cmock）
#
# 快速构建：
#   cmake -S . -B build
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# 覆盖率构建（gcc/clang）：
#   cmake -S . -B build -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build
#   ctest --test-dir build
# ==============================================================================

cmake_minimum_required(VERSION 3.10)
project(unity_cmock_demo C)

# C 语言标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译警告（GCC/Clang）
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

# ==============================================================================
# 覆盖率支持（gcc/clang，CI coverage Job 使用）
# ==============================================================================
option(ENABLE_COVERAGE "Enable gcov/lcov coverage instrumentation" OFF)
if(ENABLE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Coverage instrumentation enabled")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    else()
        message(WARNING "ENABLE_COVERAGE only supports GCC/Clang, ignored for ${CMAKE_C_COMPILER_ID}")
    endif()
endif()

# ==============================================================================
# 头文件搜索路径（按架构分层）
# ==============================================================================
include_directories(
    ${CMAKE_SOURCE_DIR}/app/battery
    ${CMAKE_SOURCE_DIR}/app/led
    ${CMAKE_SOURCE_DIR}/hal
    ${CMAKE_SOURCE_DIR}/test/mocks
    ${CMAKE_SOURCE_DIR}/framework/unity
    ${CMAKE_SOURCE_DIR}/framework/cmock/src
)

# ==============================================================================
# 目标1：PC 单元测试
# ==============================================================================
add_executable(unit_test
    # 测试用例 + 统一测试固件
    test/unit/test_battery.c
    test/unit/test_led.c
    test/unit/test_support.c
    # 被测业务模块（应用层）
    app/battery/battery.c
    app/led/led.c
    # CMock 桩（测试层）
    test/mocks/mock_hal_adc.c
    test/mocks/mock_hal_gpio.c
    # 第三方框架
    framework/unity/unity.c
    framework/cmock/src/cmock.c
)

# ==============================================================================
# 目标2：固件模拟
# ==============================================================================
add_executable(firmware_demo
    platform/firmware/firmware_main.c
    app/battery/battery.c
)

# ==============================================================================
# CTest 集成（CI 自动调用）
# ==============================================================================
enable_testing()
add_test(NAME unit_test COMMAND unit_test)
set_tests_properties(unit_test PROPERTIES TIMEOUT 30)
