# Unity + CMock 嵌入式单元测试 Demo 工程

[![CI](https://github.com/xkk6663/test_adc_unity/actions/workflows/ci.yml/badge.svg)](https://github.com/xkk6663/test_adc_unity/actions/workflows/ci.yml)

> 一个面向嵌入式（STM32 飞控）项目的最小可运行单元测试 Demo：用 **Unity** 做测试框架、**CMock** 自动打桩，在 PC 上验证 C 业务代码逻辑，实现"测试左移"。已接入 GitHub Actions CI/CD，每次提交自动编译并运行全部测试。

---

## 目录

- [1. 项目简介](#1-项目简介)
- [2. 工程结构](#2-工程结构)
- [3. 快速开始](#3-快速开始)
- [4. 核心机制讲解](#4-核心机制讲解)
- [5. 测试用例说明](#5-测试用例说明)
- [6. 如何扩展本工程](#6-如何扩展本工程)
- [7. 常见问题 FAQ](#7-常见问题-faq)
- [8. 局限性与边界](#8-局限性与边界)
- [9. 相关文档](#9-相关文档)
- [10. CI/CD 流水线](#10-cicd-流水线)

---

## 1. 项目简介

### 1.1 解决什么问题

嵌入式开发中，业务逻辑（如 PID 算法、协议解析、电压换算、状态机）通常和硬件初始化代码混在一起，必须烧录到芯片上才能调试。这导致：

- **调试周期长**：改一行代码 → 编译 → 烧录 → 上电 → 看结果，动辄几分钟
- **难以自动化**：硬件测试无法批量回归，每次改代码都要手动重测
- **bug 发现晚**：逻辑缺陷要到硬件联调阶段才暴露，修复成本高

本 Demo 展示了一种**在 PC 上对嵌入式 C 代码做单元测试**的方案：

- 同一份业务代码，编译出两个程序：一个跑在芯片上（固件），一个跑在 PC 上（单元测试）
- PC 测试中，硬件相关的 HAL 函数被 CMock 生成的"桩函数"替换，可以预设返回值、校验调用次数
- 测试用例用 Unity 框架编写，一键编译运行，秒级出结果

### 1.2 被测模块

本工程以飞控项目中的**电池电压采集模块**为例：

| 函数 | 功能 | 测试重点 |
|------|------|----------|
| `adc_to_mv(adc_raw)` | ADC 原始值 → 电池电压（mV），含边界钳位和非法输入防护 | 换算正确性、钳位逻辑、非法输入防护 |
| `read_battery_mv()` | 调用 HAL 读 ADC → 换算 → 容错 | HAL 调用次数/顺序、错误容错 |

硬件参数：12 位 ADC（0~4095）、3.3V 参考电压、3:1 分压电阻、2S 锂电池（满电 8.4V）。

### 1.3 验证结果

本工程已在本地编译运行验证通过：

| 验证项 | 结果 |
|--------|------|
| `mingw32-make all` 双目标编译 | ✅ 成功，零警告 |
| `./unit_test.exe` 单元测试 | ✅ 11 Tests, 0 Failures |
| `./firmware_demo.exe` 固件模拟 | ✅ 输出 4950mV |
| **故意注入 bug（钳位条件写反）** | ✅ **11 个用例中 9 个失败**，缺陷被精准拦截 |

---

## 2. 工程结构

```
unity_cmock_demo/
├── README.md                 # 本文件（项目教程）
├── 测试方案.md                # 单元测试方案文档
├── 测试用例.md                # 详细测试用例文档（11 个用例）
│
├── battery.c                 # 【被测模块】电池电压采集实现
├── battery.h                 # 【被测模块】头文件（接口声明 + 硬件参数宏）
├── hal_adc.h                 # 【HAL 接口】STM32 ADC 接口声明（CMock 根据此文件生成桩）
│
├── test_battery.c            # 【单元测试】11 个测试用例（Unity 框架）
├── mock_hal_adc.c            # 【CMock 桩】HAL ADC 桩实现（预生成版本）
├── mock_hal_adc.h            # 【CMock 桩】HAL ADC 桩头文件（预生成版本）
│
├── firmware_main.c           # 【固件模拟】主程序 + 真实 HAL 模拟实现（双目标之一）
│
├── Makefile                  # 构建脚本（mingw32-make / GNU make，开箱即用）
├── CMakeLists.txt            # CMake 构建脚本（跨平台构建 + CTest 集成）
├── .gitignore                # Git 忽略规则
├── .github/
│   └── workflows/
│       └── ci.yml            # GitHub Actions CI/CD 流水线
├── CMockConfig.yml           # CMock 配置（插件、前缀、输出路径）
├── gen_mocks.rb              # CMock 桩生成脚本（有 Ruby 时运行）
│
├── unity/                    # 【Unity 测试框架】源码（已 vendor，无需额外安装）
│   ├── unity.c
│   ├── unity.h
│   └── unity_internals.h
│
└── cmock/                    # 【CMock 生成器】源码（已 vendor，供 gen_mocks.rb 调用）
    └── lib/                  # CMock Ruby 库（cmock.rb + 各插件）
```

### 文件职责速查

| 你想修改什么 | 改哪个文件 |
|-------------|-----------|
| 业务逻辑（换算公式、钳位阈值） | `battery.c` / `battery.h` |
| HAL 接口签名 | `hal_adc.h`（改完需重新生成桩） |
| 添加/修改测试用例 | `test_battery.c` |
| 桩的生成配置（插件、前缀） | `CMockConfig.yml` |
| 构建选项（编译 flag、源文件列表） | `Makefile` / `CMakeLists.txt` |
| 固件模拟的 HAL 行为 | `firmware_main.c` |

---

## 3. 快速开始

### 3.1 环境要求

**必须安装：**

| 工具 | Windows | Linux / macOS | 验证命令 |
|------|---------|---------------|----------|
| GCC | MinGW-w64（`C:\MinGW\bin\gcc.exe`） | 系统 gcc | `gcc --version` |
| Make | mingw32-make | GNU make | `mingw32-make --version` / `make --version` |

**可选安装：**

| 工具 | 用途 | 不安装的影响 |
|------|------|-------------|
| CMake | 替代 Makefile 的构建系统 | 使用 Makefile 即可，不影响功能 |
| Ruby | 运行 `gen_mocks.rb` 重新生成 CMock 桩 | 桩文件已预生成，可直接编译运行 |

### 3.2 编译与运行

#### Windows（MinGW + mingw32-make）

```bash
cd unity_cmock_demo

# 编译两个目标（单元测试 + 固件模拟）
mingw32-make all

# 运行单元测试
.\unit_test.exe

# 运行固件模拟
.\firmware_demo.exe

# 一键编译并运行测试
mingw32-make test

# 清理构建产物
mingw32-make clean
```

#### Linux / macOS

```bash
cd unity_cmock_demo
make all
./unit_test
./firmware_demo
```

#### CMake 方式（推荐用于跨平台 / CI 环境）

CMake 是比 Makefile 更通用的构建系统，支持 Windows / Linux / macOS，并且本工程已集成 **CTest**（CMake 的测试运行器），可直接接入 CI 流水线。

**步骤 1：创建构建目录（out-of-source build，不污染源码目录）**

```bash
cd unity_cmock_demo
mkdir build
cd build
```

**步骤 2：配置项目（生成构建文件）**

```bash
# Linux / macOS（默认 Unix Makefiles）
cmake ..

# Windows + MinGW（指定 MinGW Makefiles 生成器）
cmake -G "MinGW Makefiles" ..

# Windows + Visual Studio（自动检测最新版 VS）
cmake ..
```

配置成功后会输出类似信息：

```
-- The C compiler identification is GNU 11.4.0
-- Configuring done
-- Generating done
-- Build files have been written to: /path/to/unity_cmock_demo/build
```

**步骤 3：编译（双目标：unit_test + firmware_demo）**

```bash
# Linux / macOS / MinGW
make

# 或通用方式（不依赖具体生成器）
cmake --build .

# Windows + Visual Studio
cmake --build . --config Release
```

编译成功后，`build/` 目录下会生成两个可执行文件：
- `unit_test`（或 `unit_test.exe`）—— 单元测试程序
- `firmware_demo`（或 `firmware_demo.exe`）—— 固件模拟程序

**步骤 4：运行单元测试**

方式一：直接运行可执行文件

```bash
./unit_test          # Linux / macOS / MinGW
.\unit_test.exe      # Windows (PowerShell / cmd)
```

方式二：用 CTest 运行（推荐，CI 环境标准用法）

```bash
ctest --output-on-failure
```

CTest 输出示例：

```
Test project /path/to/unity_cmock_demo/build
    Start 1: unit_test
1/1 Test #1: unit_test ........................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.01 sec
```

**步骤 5：运行固件模拟**

```bash
./firmware_demo      # Linux / macOS / MinGW
.\firmware_demo.exe  # Windows
```

输出：

```
=== Battery Voltage Monitor (Firmware Demo) ===
Battery voltage: 4950 mV (4.95 V)
Status: LOW VOLTAGE (< 6.0V)
```

**清理构建产物**

```bash
# 方式一：删除整个 build 目录（最干净）
cd ..
rm -rf build        # Linux / macOS / Git Bash
rmdir /s /q build   # Windows cmd

# 方式二：在 build 目录内执行清理
cd build
make clean          # Linux / macOS / MinGW
cmake --build . --target clean  # 通用方式
```

> **Makefile vs CMake 怎么选？**
> - 本地快速验证、Windows MinGW 环境 → 用 `Makefile`（`mingw32-make all`，零配置）
> - 跨平台开发、CI/CD 流水线、需要 CTest 集成 → 用 `CMake`（本工程 CI 即使用 CMake + CTest）
> - 两者可以共存，本工程同时提供了 `Makefile` 和 `CMakeLists.txt`，互不干扰。

### 3.3 预期输出

**单元测试输出：**

```
test_battery.c:322:test_adc_to_mv_normal:PASS
test_battery.c:323:test_adc_to_mv_below_max:PASS
...（共 11 个用例）...
-----------------------
11 Tests 0 Failures 0 Ignored
OK
```

**固件模拟输出：**

```
=== Battery Voltage Monitor (Firmware Demo) ===
Battery voltage: 4950 mV (4.95 V)
Status: LOW VOLTAGE (< 6.0V)
```

---

## 4. 核心机制讲解

### 4.1 双目标编译：一份代码，两个程序

这是本方案的核心设计思想。

```
                    ┌─────────────────────┐
                    │   battery.c（业务代码）  │
                    │  adc_to_mv()         │
                    │  read_battery_mv()   │
                    └──────────┬──────────┘
                               │
              ┌────────────────┴────────────────┐
              │                                 │
    ┌─────────▼──────────┐           ┌──────────▼─────────┐
    │  目标1：unit_test   │           │ 目标2：firmware_demo │
    │                     │           │                      │
    │  链接 mock_hal_adc.c│           │  链接 firmware_main.c │
    │  （CMock 桩函数）    │           │  （真实 HAL 模拟）    │
    │                     │           │                      │
    │  运行在 PC CPU 上    │           │  运行在 PC CPU 上     │
    │  （模拟芯片环境）     │           │  （模拟固件运行）      │
    └─────────────────────┘           └──────────────────────┘
```

**关键点：**

- `battery.c` 业务代码**不做任何修改**，同时被两个目标链接
- 两个目标的唯一区别是链接了不同的 `HAL_ADC_GetValue()` 实现
- 测试目标用桩函数（可预设返回值），固件目标用真实实现（固定返回 2048）
- 这保证了"测试的代码"和"烧录到芯片的代码"是同一份，消除了测试与实际的差异

在真实 STM32 项目中，固件目标会替换为：
- 交叉编译器：`arm-none-eabi-gcc`
- HAL 实现：STM32 HAL 库
- 输出：`.hex` / `.bin` 固件文件

### 4.2 CMock 打桩原理

CMock 是一个**基于 Ruby 的 C 语言 mock 代码生成器**。它读取头文件（如 `hal_adc.h`），自动生成对应的桩函数代码。

#### 桩函数能做什么

以 `HAL_ADC_GetValue()` 为例，CMock 生成的桩提供：

| 函数 | 作用 | 示例 |
|------|------|------|
| `HAL_ADC_GetValue_ExpectAndReturn(val)` | 预设一次调用的返回值，按顺序依次返回 | 预设返回 2048 |
| `HAL_ADC_GetValue_IgnoreAndReturn(val)` | 忽略调用次数，固定返回值 | 不关心调用多少次，每次都返回 2048 |
| `mock_hal_adc_Verify()` | 校验实际调用次数是否等于预期次数 | 预期调用 1 次，实际调用了 2 次 → 测试失败 |

#### 桩的内部工作机制

```
测试代码预设：
  HAL_ADC_GetValue_ExpectAndReturn(2048);   → 预期次数=1, 返回值数组[0]=2048
  HAL_ADC_GetValue_ExpectAndReturn(1000);   → 预期次数=2, 返回值数组[1]=1000

被测代码调用：
  read_battery_mv() → HAL_ADC_GetValue()    → 实际次数=1, 返回数组[0]=2048
  read_battery_mv() → HAL_ADC_GetValue()    → 实际次数=2, 返回数组[1]=1000

tearDown 校验：
  mock_hal_adc_Verify()                      → 预期次数(2) == 实际次数(2) → 通过
```

如果被测代码**多调用了一次** HAL：
- 第三次调用时，实际次数(3) > 预期次数(2) → 桩函数立即报测试失败

如果被测代码**少调用了一次** HAL：
- tearDown 中 Verify() 发现预期次数(2) > 实际次数(1) → 测试失败

#### 本工程的桩文件说明

为了让工程在**没有 Ruby 的环境**下也能直接编译运行，`mock_hal_adc.c/.h` 已预生成并检入仓库。

- 预生成版本使用**静态数组**存储返回值（大小 128，足够单元测试）
- CMock 实时生成版本使用**动态内存分配**（malloc/realloc）
- 两者功能完全等价，API 完全一致

**有 Ruby 环境时**，可重新生成桩（会覆盖预生成版本）：

```bash
ruby gen_mocks.rb
```

### 4.3 Unity 测试框架

Unity 是一个轻量级的 C 语言单元测试框架（ThrowTheSwitch 出品），核心只有 3 个文件：`unity.c`、`unity.h`、`unity_internals.h`。

#### 标准测试结构

```c
#include "unity.h"

// 每个用例执行前调用（初始化桩状态）
void setUp(void) {
    mock_hal_adc_Init();
}

// 每个用例执行后调用（校验 + 清理）
void tearDown(void) {
    mock_hal_adc_Verify();
    mock_hal_adc_Destroy();
}

// 测试用例
void test_xxx(void) {
    // 1. 预设桩返回值
    HAL_ADC_GetValue_ExpectAndReturn(2048);

    // 2. 调用被测函数
    int32_t result = read_battery_mv();

    // 3. 断言校验
    TEST_ASSERT_EQUAL_INT32(4950, result);
}

// 测试入口
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_xxx);
    return UNITY_END();
}
```

#### 常用断言宏

| 宏 | 用途 | 示例 |
|----|------|------|
| `TEST_ASSERT_EQUAL_INT(expected, actual)` | 整数相等 | `TEST_ASSERT_EQUAL_INT(4950, result)` |
| `TEST_ASSERT_EQUAL_UINT16(expected, actual)` | 16 位无符号整数相等 | `TEST_ASSERT_EQUAL_UINT16(8400, result)` |
| `TEST_ASSERT_EQUAL_INT32(expected, actual)` | 32 位整数相等 | `TEST_ASSERT_EQUAL_INT32(-1, result)` |
| `TEST_ASSERT_NOT_EQUAL(expected, actual)` | 不相等 | `TEST_ASSERT_NOT_EQUAL(0, result)` |
| `TEST_ASSERT_NULL(pointer)` | 指针为空 | `TEST_ASSERT_NULL(ptr)` |
| `TEST_FAIL_MESSAGE(msg)` | 直接失败并输出消息 | `TEST_FAIL_MESSAGE("should not reach here")` |

---

## 5. 测试用例说明

本工程共 **11 个测试用例**，覆盖 5 种测试设计方法：

| 编号 | 用例名称 | 测试方法 | 验证点 |
|------|----------|----------|--------|
| TC-001 | test_adc_to_mv_normal | 等价类 | 正常 ADC 值换算正确 |
| TC-002 | test_adc_to_mv_below_max | 等价类 | 正常值不被误钳位 |
| TC-003 | test_adc_to_mv_zero | 边界值 | ADC=0 下边界 |
| TC-004 | test_adc_to_mv_full_scale_clamp | 边界值 | ADC=4095 上边界 + 钳位 |
| TC-005 | test_adc_to_mv_clamp_critical | 边界值 | 钳位临界点（差 1 个码值） |
| TC-006 | test_adc_to_mv_overvoltage | 错误推测 | 超压输入被钳位 |
| TC-007 | test_adc_to_mv_invalid_input | 错误推测 | 非法 ADC 值（>4095）被拦截 |
| TC-008 | test_read_battery_mv_success | 接口校验 | 正常读取，HAL 调用 1 次 |
| TC-009 | test_read_battery_mv_multi_calls | 接口校验 | 连续读取，HAL 调用 2 次，顺序正确 |
| TC-010 | test_read_battery_mv_ignore_mode | 接口校验 | Ignore 模式，不校验次数 |
| TC-011 | test_read_battery_mv_hal_error | 容错 | HAL 返回错误码时返回 BATTERY_READ_ERROR |

详细的用例描述（输入、操作步骤、预期结果、实际结果）见 [测试用例.md](./测试用例.md)。

---

## 6. 如何扩展本工程

### 6.1 添加一个新的被测模块

假设要添加一个"电机控制模块" `motor.c`：

**步骤 1：创建模块文件**

```c
// motor.h
#ifndef MOTOR_H
#define MOTOR_H
#include <stdint.h>
void motor_set_pwm(uint16_t duty);
uint16_t motor_get_rpm(void);
#endif
```

```c
// motor.c
#include "motor.h"
#include "hal_pwm.h"   // 假设依赖 PWM HAL

void motor_set_pwm(uint16_t duty) {
    // ... 业务逻辑 ...
    HAL_PWM_SetDuty(duty);
}
```

**步骤 2：如果新模块依赖新的 HAL 接口，创建 HAL 头文件并生成桩**

```c
// hal_pwm.h
#ifndef HAL_PWM_H
#define HAL_PWM_H
#include <stdint.h>
void HAL_PWM_SetDuty(uint16_t duty);
#endif
```

修改 `gen_mocks.rb`，将新头文件加入生成列表：

```ruby
header_files = ['hal_adc.h', 'hal_pwm.h']  # 添加 hal_pwm.h
```

运行 `ruby gen_mocks.rb` 生成 `mock_hal_pwm.c/.h`。

**步骤 3：编写测试用例** `test_motor.c`，参考 `test_battery.c` 的结构。

**步骤 4：更新构建脚本**，在 `Makefile` 中添加新的源文件：

```makefile
TEST_SRCS = test_battery.c test_motor.c battery.c motor.c mock_hal_adc.c mock_hal_pwm.c unity/unity.c
```

### 6.2 添加一个新的测试用例

在 `test_battery.c` 中：

1. 编写测试函数（`void test_xxx(void) { ... }`）
2. 在 `main()` 中添加 `RUN_TEST(test_xxx);`
3. 重新编译运行

### 6.3 修改 HAL 接口后重新生成桩

如果修改了 `hal_adc.h` 中的函数签名（如添加参数、修改返回值类型）：

1. 修改 `hal_adc.h`
2. 运行 `ruby gen_mocks.rb` 重新生成桩
3. 更新测试用例中对桩 API 的调用（如 `_ExpectAndReturn` 的参数）
4. 重新编译运行

> 没有 Ruby 环境时，需手动更新 `mock_hal_adc.c/.h` 中的桩函数签名，使其与 `hal_adc.h` 一致。

### 6.4 接入 CI 流水线

本工程已内置完整的 GitHub Actions CI/CD 配置，详见 [第 10 章 CI/CD 流水线](#10-cicd-流水线) 和 [`.github/workflows/ci.yml`](./.github/workflows/ci.yml)。

每次 `push` 或发起 `pull_request` 时，CI 会自动执行：CMock 桩重新生成 → Makefile 构建 + 测试 → 固件模拟 → CMake 构建 + CTest，全流程通过才会合入。

---

## 7. 常见问题 FAQ

### Q1：为什么不用 Keil 仿真，而要在 PC 上做单元测试？

**A**：两者是互补关系，不是替代关系。

| 维度 | Keil 仿真 / 实物板子 | Unity + CMock 单元测试 |
|------|---------------------|----------------------|
| 运行环境 | 芯片（ARM） | PC（x86） |
| 测试范围 | 硬件 + 软件一起测 | 仅 C 代码逻辑 |
| 擅长发现 | 硬件交互、时序、崩溃死机类 bug | 逻辑分支、边界值、参数校验、容错 |
| 执行速度 | 慢（编译+烧录+上电） | 快（秒级编译运行） |
| 自动化 | 困难 | 容易（可接入 CI） |
| 硬件电气现象 | ✅ 可测（示波器、逻辑分析仪） | ❌ 不可测 |

**推荐流程**：先在 PC 单元测试把逻辑 bug 干掉 → 再把固件放到芯片环境做软硬件联合验证。

### Q2：CMock 能模拟 ADC 采样噪声、PWM 时序吗？

**A**：不能。CMock 不是硬件仿真器，它只是**拦截上层代码对 HAL 函数的调用**，预设返回值、校验参数和调用次数。

- ✅ 能测：PID 算法、电压换算、协议解析、状态机、阈值判断、参数校验等纯软件逻辑
- ❌ 不能测：I2C 总线噪声、PWM 时序、中断冲突、DMA 异常、寄存器硬件故障

硬件层面问题必须靠 Keil 调试、ST-Link、示波器、真实硬件台架测试。

### Q3：桩文件是预生成的，和 CMock 实时生成的有什么区别？

**A**：功能完全等价，API 完全一致，唯一区别是内部内存管理方式：

| 版本 | 内存管理 | 适用场景 |
|------|---------|---------|
| 预生成版本（本工程默认） | 静态数组（大小 128） | 无 Ruby 环境，开箱即用 |
| CMock 实时生成 | 动态分配（malloc/realloc） | 有 Ruby 环境，桩数量动态增长 |

对于单元测试，128 个预设槽位绰绰有余。有 Ruby 环境时运行 `ruby gen_mocks.rb` 即可切换到动态分配版本。

### Q4：测试中发现的"预期 2418 实际 2415"是怎么回事？

**A**：这是整数除法的精度问题。`1000 × 3300 / 4095`：

- 先算乘法：`1000 × 3300 = 3,300,000`
- 再算整数除法：`3,300,000 / 4095 = 805`（不是 806，因为整数除法截断小数）
- 最后乘分压比：`805 × 3 = 2415`

手算时容易把 `3,300,000 / 4095 ≈ 805.86` 四舍五入成 806，但 C 语言整数除法是**截断**（向零取整），所以是 805。编写测试用例时务必用实际编译运行的结果校准预期值。

### Q5：如何验证我的测试用例真的能抓出 bug？

**A**：采用"变异测试"思路——故意在被测代码中注入一个缺陷，看测试是否能拦截。

本工程已验证：将 `battery.c` 中钳位条件 `>` 改为 `<`，重新编译运行，11 个用例中 9 个失败。如果你的测试用例在注入 bug 后全部通过，说明用例覆盖不足，需要补充。

### Q6：Windows 下 `mingw32-make` 报错怎么办？

**A**：常见问题及解决：

| 错误 | 原因 | 解决 |
|------|------|------|
| `gcc: command not found` | MinGW 未加入 PATH | 将 `C:\MinGW\bin` 加入系统 PATH，或用完整路径 `C:\MinGW\bin\gcc.exe` |
| `mingw32-make: command not found` | 未安装 mingw32-make | MinGW 安装时勾选 `mingw32-make`，或下载独立包 |
| `del: command not found` | make 调用了 bash 而非 cmd | `make clean` 时忽略此错误（前面有 `-` 前缀），或手动删除 `.o` 和 `.exe` 文件 |
| 中文路径编译报错 | MinGW 对中文路径支持不佳 | 将工程放到纯英文路径下（如 `C:\dev\unity_cmock_demo`） |

---

## 8. 局限性与边界

### 8.1 技术局限性

1. **仅验证 C 代码逻辑**：无法仿真硬件电气特性（采样噪声、时序、中断等）。
2. **PC 与芯片编译器差异**：PC 用 gcc 编译，芯片用 `arm-none-eabi-gcc` 编译，两者在整数提升、结构体对齐、位域行为上可能有细微差异。关键逻辑建议在芯片编译器下也做一次编译验证。
3. **桩的真实性**：CMock 桩只是"按预设返回值"，不模拟 HAL 函数的真实行为（如寄存器读写副作用、DMA 异步完成）。
4. **不覆盖并发与中断**：单元测试是单线程同步执行，无法验证中断优先级、竞态条件、死锁等并发问题。

### 8.2 适用场景

| 适合用 Unity+CMock 测试 | 不适合（需硬件测试） |
|-------------------------|---------------------|
| 算法类：PID、滤波、换算、校验 | ADC 采样精度、参考电压漂移 |
| 协议类：通信帧解析、打包、状态机 | 总线时序、信号完整性 |
| 逻辑类：阈值判断、模式切换、参数校验 | 中断响应、DMA 传输 |
| 接口类：模块间调用顺序、参数传递 | 寄存器读写、外设初始化 |
| 容错类：错误码处理、异常输入防护 | 硬件故障、电气过压 |

### 8.3 与其他测试方式的关系

```
开发阶段 → PC 单元测试（Unity+CMock）→ 硬件在环测试（HIL）→ 实机试飞
              ↑ 发现逻辑 bug               ↑ 发现软硬件交互 bug    ↑ 发现系统级 bug
              秒级反馈                      小时级反馈              天级反馈
```

三种测试方式层层递进，各有侧重，共同保证产品质量。PC 单元测试是成本最低、反馈最快的第一道防线。

---

## 9. 相关文档

| 文档 | 说明 |
|------|------|
| [测试方案.md](./测试方案.md) | 完整的单元测试方案（测试目标、范围、策略、方法、环境、准入准出标准、风险分析） |
| [测试用例.md](./测试用例.md) | 11 个测试用例的详细描述（输入、步骤、预期、实际结果）+ 查错能力验证记录 |
| `battery.h` | 被测模块头文件，含硬件参数宏和接口注释 |
| `battery.c` | 被测模块实现，每行关键逻辑都有注释 |
| `test_battery.c` | 测试用例源码，每个用例有详细的文档注释 |
| `CMockConfig.yml` | CMock 配置文件，含各配置项说明 |
| `gen_mocks.rb` | CMock 桩生成脚本，含使用说明 |

### 外部参考

- Unity 官方仓库：https://github.com/ThrowTheSwitch/Unity
- CMock 官方仓库：https://github.com/ThrowTheSwitch/CMock
- Ceedling（Unity+CMock 的完整构建系统）：https://github.com/ThrowTheSwitch/ceedling
- 《单元测试的艺术》（Roy Osherove）—— 单元测试方法论经典书籍

---

## 10. CI/CD 流水线

本工程已接入 **GitHub Actions** CI/CD，每次提交代码自动执行完整的构建-测试流程，实现自动化质量门禁。

### 10.1 配置文件

CI 流水线定义在 [`.github/workflows/ci.yml`](./.github/workflows/ci.yml)。

### 10.2 触发条件

| 事件 | 分支 | 说明 |
|------|------|------|
| `push` | `main` / `master` | 直接推送代码时触发 |
| `pull_request` | `main` / `master` | 发起 PR 时触发，作为合入门禁 |

### 10.3 流水线步骤详解

CI 在 `ubuntu-latest` 环境中依次执行以下 9 个步骤：

| 步骤 | 命令/操作 | 目的 |
|------|----------|------|
| 1. Checkout | `actions/checkout@v4` | 检出仓库代码 |
| 2. Install deps | `apt-get install gcc make cmake ruby` | 安装编译工具链和 Ruby（CMock 依赖） |
| 3. Show versions | 打印各工具版本 | 便于排查环境问题 |
| 4. Regenerate mocks | `ruby gen_mocks.rb` | 用 CMock 从 `hal_adc.h` 重新生成桩文件，验证桩生成流程可用 |
| 5. Build (Makefile) | `make all` | 用 Makefile 构建双目标（unit_test + firmware_demo） |
| 6. Run tests (Makefile) | `./unit_test` | 运行 Makefile 构建的单元测试 |
| 7. Run firmware demo | `./firmware_demo` | 验证固件模拟目标可正常运行 |
| 8. Build (CMake) | `mkdir build && cd build && cmake .. && make` | 用 CMake 构建，验证 CMakeLists.txt 可用 |
| 9. Run tests (CTest) | `cd build && ctest --output-on-failure` | 用 CTest 运行 CMake 构建的单元测试 |

**关键设计**：
- 步骤 4 用 CMock **重新生成桩**，而不是使用仓库中预生成的桩——这确保了 `hal_adc.h` → `mock_hal_adc.c/.h` 的自动生成流程始终可用，避免"预生成桩能编译但 CMock 生成流程已坏"的问题。
- 同时验证 **Makefile** 和 **CMake** 两套构建系统，确保跨平台构建能力。
- 任何一步失败，整个 CI 变红，PR 无法合入。

### 10.4 如何查看 CI 结果

1. 打开仓库页面：https://github.com/xkk6663/test_adc_unity
2. 点击顶部 **Actions** 标签页
3. 选择对应的 workflow run（按 commit 信息或分支筛选）
4. 点击 **build-and-test** job 查看每个步骤的详细日志
5. 失败的步骤会标红，展开可查看具体错误信息

### 10.5 状态徽章

README 顶部的徽章实时反映 CI 状态：

- ✅ 绿色通过：最近一次提交的 CI 全流程通过
- ❌ 红色失败：最近一次提交的 CI 有步骤失败
- ⏳ 黄色运行中：CI 正在执行

徽章链接：`https://github.com/xkk6663/test_adc_unity/actions/workflows/ci.yml/badge.svg`

### 10.6 本地模拟 CI 流程

在提交代码前，可以在本地手动模拟 CI 全流程，提前发现问题：

```bash
cd unity_cmock_demo

# 步骤 4：重新生成桩（需要 Ruby）
ruby gen_mocks.rb

# 步骤 5-7：Makefile 构建 + 测试 + 固件模拟
make all
./unit_test
./firmware_demo

# 步骤 8-9：CMake 构建 + CTest
mkdir -p build && cd build
cmake ..
make
ctest --output-on-failure
```

> 没有 Ruby 环境时，可跳过步骤 4（使用预生成桩），其余步骤均可正常执行。

### 10.7 自定义 CI

如需修改 CI 行为（如增加覆盖率统计、添加静态分析、部署产物等），编辑 [`.github/workflows/ci.yml`](./.github/workflows/ci.yml) 即可。常见扩展：

- **代码覆盖率**：添加 `gcov` / `lcov` 步骤，生成覆盖率报告并上传到 Codecov
- **静态分析**：添加 `cppcheck` / `clang-tidy` 步骤，检查代码规范
- **多平台测试**：添加 `matrix` 策略，在 ubuntu / macos / windows 多平台运行
- **构建产物上传**：用 `actions/upload-artifact` 保存编译出的固件文件

---

## 附录：从零重建验证记录

为确保工程可被用户直接跑通，已执行从零全量重建验证：

```bash
# 1. 清理所有构建产物
mingw32-make clean

# 2. 全量编译
mingw32-make all
# → 编译成功，零警告，生成 unit_test.exe 和 firmware_demo.exe

# 3. 运行单元测试
.\unit_test.exe
# → 11 Tests 0 Failures 0 Ignored / OK

# 4. 运行固件模拟
.\firmware_demo.exe
# → Battery voltage: 4950 mV (4.95 V)

# 5. 注入 bug 验证查错能力
#    将 battery.c 中 > 改为 <
#    重新编译运行 → 11 Tests 9 Failures（缺陷被精准拦截）
#    还原 > → 重新编译运行 → 11 Tests 0 Failures（恢复全绿）
```

**结论：工程可在 Windows + MinGW 环境下开箱即用，编译运行验证通过。**
