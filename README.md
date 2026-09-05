# 嵌入式单元测试 Demo（Unity + CMock）

> 产品级嵌入式 C 语言单元测试工程，仿照 **ELAB（Embedded Laboratory）框架**的分层架构设计，使用 Unity（断言框架）+ CMock（自动桩生成）实现 PC 端单元测试，并配套多平台 CI/CD 流水线。

[![CI](https://github.com/xkk6663/test_adc_unity/actions/workflows/ci.yml/badge.svg)](https://github.com/xkk6663/test_adc_unity/actions)

---

## 目录

- [1. 项目简介](#1-项目简介)
- [2. 工程架构（ELAB 风格分层）](#2-工程架构elab-风格分层)
- [3. 快速开始](#3-快速开始)
- [4. 架构分层详解](#4-架构分层详解)
- [5. 单元测试指南](#5-单元测试指南)
- [6. CMock 桩生成](#6-cmock-桩生成)
- [7. CI/CD 流水线（产品级）](#7-cicd-流水线产品级)
- [8. 新增模块指南](#8-新增模块指南)
- [9. 常见问题](#9-常见问题)

---

## 1. 项目简介

本工程演示如何在嵌入式 C 项目中落地**产品级单元测试**：

- **被测对象**：电池电压采集模块（`battery`）+ LED 指示灯模块（`led`）
- **测试框架**：Unity（轻量级 C 断言库）+ CMock（自动桩生成器）
- **构建系统**：纯 CMake 跨平台构建
- **CI/CD**：GitHub Actions，多平台矩阵 + 静态分析 + 覆盖率
- **架构设计**：仿照 ELAB 框架的分层思想，按职责分离代码

### 核心特性

| 特性 | 说明 |
|------|------|
| 纯 CMake 构建 | 跨平台统一构建系统，Windows/Linux/macOS 一致 |
| 19 个测试用例 | 覆盖等价类、边界值、错误推测、接口交互、容错、状态切换 |
| 跨平台 | Windows / Linux / macOS 均可编译运行 |
| 零硬件依赖 | 所有硬件操作通过 HAL 接口抽象，PC 上用 CMock 桩替换 |
| 产品级 CI | 静态分析 + 三平台构建矩阵 + 覆盖率报告 + 产物上传 |

---

## 2. 工程架构（ELAB 风格分层）

仿照 ELAB 框架的分层思想，代码按职责分为六层，依赖方向自上而下（上层依赖下层，下层不感知上层）：

```
┌─────────────────────────────────────────────────────────┐
│                    app/  应用层                          │
│  业务模块：battery（电压采集）、led（指示灯控制）          │
│  只依赖 hal/ 接口，不直接操作硬件寄存器                    │
├─────────────────────────────────────────────────────────┤
│                    hal/  硬件抽象层                       │
│  接口声明：hal_adc.h、hal_gpio.h                          │
│  定义硬件操作的统一接口，上层只面向接口编程                 │
├─────────────────────────────────────────────────────────┤
│                 platform/  平台层                         │
│  固件主程序 + 真实 HAL 实现（platform/firmware/）          │
│  编译为固件目标，在真实硬件或 PC 模拟器上运行               │
├─────────────────────────────────────────────────────────┤
│                   test/  测试层                           │
│  unit/   —— 单元测试用例（test_battery.c、test_led.c）    │
│  mocks/  —— CMock 生成的 HAL 桩（替换真实硬件实现）        │
├─────────────────────────────────────────────────────────┤
│                framework/  第三方框架                      │
│  unity/  —— Unity 测试框架（断言、测试运行器）             │
│  cmock/  —— CMock 桩生成器（Ruby 生成器 + C 运行时）       │
├─────────────────────────────────────────────────────────┤
│                  tools/  工具脚本                         │
│  gen_mocks.rb    —— CMock 桩生成入口                      │
│  CMockConfig.yml —— CMock 配置（插件、输出路径等）         │
└─────────────────────────────────────────────────────────┘
```

### 完整目录结构

```
unity_cmock_demo/
├── app/                          # 【应用层】业务模块
│   ├── battery/                  # 电池电压采集模块
│   │   ├── battery.c             # 被测模块实现（adc_to_mv + read_battery_mv）
│   │   └── battery.h             # 模块接口声明（硬件参数宏 + 函数原型）
│   └── led/                      # LED 指示灯模块
│       ├── led.c                 # 被测模块实现（根据电压控制红/绿灯）
│       └── led.h                 # 模块接口声明（状态枚举 + 函数原型）
│
├── hal/                          # 【硬件抽象层】接口声明
│   ├── hal_adc.h                 # ADC 接口（HAL_ADC_GetValue）
│   └── hal_gpio.h                # GPIO 接口（HAL_GPIO_SetPin）
│
├── platform/                     # 【平台层】固件相关
│   └── firmware/
│       └── firmware_main.c       # 固件主程序 + 真实 HAL 模拟实现
│
├── test/                         # 【测试层】
│   ├── unit/                     # 单元测试用例
│   │   ├── test_battery.c        # battery 模块测试（11 个用例）
│   │   ├── test_led.c            # led 模块测试（8 个用例）
│   │   └── test_support.c        # 统一 setUp/tearDown/main（管理所有桩）
│   └── mocks/                    # CMock 生成的桩
│       ├── mock_hal_adc.c/h      # ADC 接口桩
│       └── mock_hal_gpio.c/h     # GPIO 接口桩
│
├── framework/                    # 【第三方框架】（已 vendor，无需额外安装）
│   ├── unity/                    # Unity 测试框架
│   │   ├── unity.c
│   │   ├── unity.h
│   │   ├── unity_internals.h
│   │   └── auto/                 # Unity Ruby 辅助脚本（CMock 依赖）
│   └── cmock/                    # CMock 桩生成器
│       ├── lib/                  # Ruby 生成器库
│       ├── config/               # Ruby 环境配置
│       └── src/                  # C 运行时（cmock.c/h，编译桩时需链接）
│
├── tools/                        # 【工具脚本】
│   ├── gen_mocks.rb              # CMock 桩生成入口
│   └── CMockConfig.yml           # CMock 配置
│
├── .github/workflows/
│   └── ci.yml                    # CI/CD 流水线配置
│
├── CMakeLists.txt                # CMake 构建脚本（唯一构建系统）
├── .gitignore
├── README.md                     # 本文档
├── 测试方案.md                    # 单元测试方案
└── 测试用例.md                    # 19 个测试用例详情
```

---

## 3. 快速开始

### 3.1 环境要求

| 工具 | 最低版本 | 用途 | 是否必需 |
|------|----------|------|----------|
| CMake | 3.10 | 构建系统 | ✅ 必需 |
| gcc / clang / MSVC | C99 兼容 | C 编译器 | ✅ 必需 |
| Ruby | 2.5 | 重新生成 CMock 桩 | ❌ 桩已检入，可直接编译 |

### 3.2 构建与测试

```bash
# 配置（Windows 上自动使用 Visual Studio 或 Ninja）
cmake -S . -B build

# 编译
cmake --build build

# 运行单元测试（19 个用例）
ctest --test-dir build --output-on-failure

# 运行固件模拟
./build/firmware_demo        # Linux/macOS
build\Release\firmware_demo.exe  # Windows
```

**Windows（Visual Studio）**：CMake 自动检测 VS 生成器，使用 MSVC 编译。
**Windows（MinGW/Ninja）**：`cmake -S . -B build -G "Ninja"` 后构建。

```
test/unit/test_support.c:114:test_adc_to_mv_normal:PASS
test/unit/test_support.c:115:test_adc_to_mv_below_max:PASS
...（共 19 个用例）...
-----------------------
19 Tests 0 Failures 0 Ignored
OK
```

---

## 4. 架构分层详解

### 4.1 设计原则（ELAB 思想）

ELAB 框架的核心思想是**"一切外设统一为设备对象，业务代码只面向接口"**。本工程借鉴其分层原则：

1. **依赖倒置**：应用层（app/）依赖抽象接口（hal/），不依赖具体实现
2. **跨平台**：通过替换驱动层实现（真实 HAL vs CMock 桩），同一份业务代码跑在 PC 和硬件上
3. **测试隔离**：每个模块的测试独立，通过统一的 setUp/tearDown 管理桩状态
4. **按职责分层**：每层只做一件事，目录结构即架构

### 4.2 各层职责

| 层 | 目录 | 职责 | 依赖 |
|----|------|------|------|
| 应用层 | `app/` | 业务逻辑（电压换算、LED 状态机） | `hal/` |
| 抽象层 | `hal/` | 硬件接口声明（函数原型、枚举） | 无 |
| 平台层 | `platform/` | 固件主程序、真实 HAL 实现 | `app/`, `hal/` |
| 测试层 | `test/` | 单元测试用例、CMock 桩 | `app/`, `hal/`, `framework/` |
| 框架层 | `framework/` | Unity + CMock 第三方代码 | 无（被依赖） |
| 工具层 | `tools/` | 构建辅助脚本 | `framework/cmock/` |

### 4.3 双目标编译

同一份 `app/battery/battery.c` 被两个 CMake 目标同时链接：

```
┌──────────────┐     ┌──────────────┐
│  unit_test   │     │ firmware_demo│
│  (PC 测试)    │     │  (固件模拟)   │
├──────────────┤     ├──────────────┤
│ test/unit/*.c│     │platform/     │
│ battery.c    │     │ firmware/    │
│ led.c        │     │ main.c       │
│ mock_hal_*.c │     │ (真实 HAL)   │
│ unity.c      │     │ battery.c    │
│ cmock.c      │     │              │
└──────────────┘     └──────────────┘
```

- **unit_test**：链接 `test/mocks/mock_hal_*.c`（CMock 桩），在 PC 上跑单元测试
- **firmware_demo**：链接 `platform/firmware/firmware_main.c`（真实 HAL 模拟），模拟固件运行

---

## 5. 单元测试指南

### 5.1 测试用例写什么

针对被测模块的**每个公开函数**，验证三类场景：

| 类别 | 说明 | 例子（led 模块） |
|------|------|-----------------|
| **正常输入** | 有效等价类的典型值 | 输入 7000mV → 绿灯亮 |
| **边界值** | 区间临界点（最容易出错） | 5999/6000/8400/8401mV |
| **异常/交互** | 非法输入、错误处理、外部调用验证 | HAL 调用次数、参数校验 |

### 5.2 测试用例怎么写

固定四步套路：

```c
void test_led_normal_voltage(void)
{
    // 1. 预设外部依赖的返回值/参数（CMock 桩）
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_RED,   GPIO_LEVEL_LOW);
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_GREEN, GPIO_LEVEL_HIGH);

    // 2. 调用被测函数
    led_set_by_voltage(7000);

    // 3. 断言结果
    TEST_ASSERT_EQUAL_INT(LED_STATE_GREEN, led_get_state());

    // 4. tearDown 自动校验桩调用次数（在 test_support.c 中统一管理）
}
```

### 5.3 测试固件（setUp/tearDown）

所有测试用例的 `setUp()` 和 `tearDown()` 统一定义在 `test/unit/test_support.c` 中：

```c
void setUp(void) {
    mock_hal_adc_Init();   // 初始化 ADC 桩
    mock_hal_gpio_Init();  // 初始化 GPIO 桩
}

void tearDown(void) {
    mock_hal_adc_Verify();   // 校验 ADC 调用次数
    mock_hal_adc_Destroy();  // 销毁 ADC 桩
    mock_hal_gpio_Verify();  // 校验 GPIO 调用次数
    mock_hal_gpio_Destroy(); // 销毁 GPIO 桩
}
```

`main()` 函数也在 `test_support.c` 中，通过 `RUN_TEST()` 宏注册所有用例。新增模块时在此添加。

---

## 6. CMock 桩生成

### 6.1 什么时候需要重新生成桩

- 修改了 `hal/` 目录下任何头文件的函数原型
- 新增了 HAL 头文件
- 修改了 `tools/CMockConfig.yml` 配置

### 6.2 如何生成

```bash
# 在工程根目录执行（需 Ruby 环境）
ruby tools/gen_mocks.rb
```

输出到 `test/mocks/` 目录，覆盖已检入的桩文件。

### 6.3 桩文件说明

- CMock 生成的桩使用**动态内存分配**（malloc/realloc），预设返回值数量可动态增长
- 桩文件头部标注 `AUTOGENERATED FILE. DO NOT EDIT.`，请勿手动修改
- 编译时需链接 `framework/cmock/src/cmock.c`（CMock 运行时）
- 桩已检入仓库，**没有 Ruby 环境也能直接编译运行**

---

## 7. CI/CD 流水线（产品级）

配置文件：`.github/workflows/ci.yml`

### 7.1 三 Job 架构

```
                    ┌─────────────────────┐
                    │  static-analysis    │
                    │  (cppcheck, Ubuntu) │
                    └─────────┬───────────┘
                              │
                    ┌─────────▼───────────┐
                    │   build-and-test    │
                    │  (多平台矩阵)        │
                    │  Ubuntu · macOS     │
                    │  · Windows          │
                    └─────────┬───────────┘
                              │
                    ┌─────────▼───────────┐
                    │     coverage        │
                    │  (gcov/lcov, Ubuntu) │
                    └─────────────────────┘
```

### 7.2 Job 1：静态分析（cppcheck）

- 检查 `app/`、`hal/`、`platform/` 下的代码
- 启用 warning / style / performance / portability 检查
- 发现问题即失败（`--error-exitcode=1`）

### 7.3 Job 2：多平台构建矩阵

| 平台 | 编译器 | 构建方式 |
|------|--------|----------|
| Ubuntu | gcc | CMake |
| macOS | clang | CMake |
| Windows | MSVC (Ninja) | CMake |

每个平台执行：
1. CMock 重新生成桩（验证桩生成流程）
2. CMake 配置 + 构建
3. CTest 运行 19 个单元测试
4. 运行固件模拟
5. 上传构建产物（artifact）

### 7.4 Job 3：测试覆盖率

- 使用 `--coverage` 标志编译
- 运行单元测试后用 `lcov` 生成覆盖率报告
- 排除框架代码（`framework/`）和测试代码（`test/`）
- 上传 HTML 覆盖率报告为 artifact

### 7.5 本地复现 CI

```bash
# 1. 静态分析
cppcheck --enable=warning,style,performance,portability \
         -I app/battery -I app/led -I hal app/ hal/ platform/

# 2. 桩生成 + 构建 + 测试
ruby tools/gen_mocks.rb
cmake -S . -B build && cmake --build build
ctest --test-dir build --output-on-failure

# 3. 覆盖率（需 lcov，gcc/clang）
cmake -S . -B build -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
lcov --capture --directory build --output-file coverage.info
```

---

## 8. 新增模块指南

以新增一个 `sensor` 模块为例，完整步骤：

### 步骤 1：创建 HAL 接口（如需要）

```bash
# 新建 hal/hal_sensor.h，声明硬件接口
# 例如：uint16_t HAL_SENSOR_Read(void);
```

### 步骤 2：创建业务模块

```bash
mkdir app/sensor
# 新建 app/sensor/sensor.h 和 app/sensor/sensor.c
```

### 步骤 3：生成 CMock 桩

```bash
# 在 tools/gen_mocks.rb 的 header_files 数组中添加 'hal/hal_sensor.h'
ruby tools/gen_mocks.rb
# 生成 test/mocks/mock_hal_sensor.c/h
```

### 步骤 4：编写测试用例

```bash
# 新建 test/unit/test_sensor.c
# 按"预设桩 → 调用被测函数 → 断言"套路写用例
```

### 步骤 5：注册到测试框架

在 `test/unit/test_support.c` 中：
1. 添加测试函数的 `extern` 声明
2. 在 `main()` 中添加 `RUN_TEST(test_sensor_xxx)`
3. 在 `setUp()`/`tearDown()` 中添加 `mock_hal_sensor_Init/Verify/Destroy`

### 步骤 6：更新构建系统

在 `CMakeLists.txt` 中：
- `unit_test` 目标添加：`app/sensor/sensor.c`、`test/unit/test_sensor.c`、`test/mocks/mock_hal_sensor.c`
- 如固件需要该模块，在 `firmware_demo` 目标中也添加

### 步骤 7：验证并推送

```bash
cmake -S . -B build && cmake --build build && ctest --test-dir build  # 本地验证
git add -A && git commit && git push  # CI 自动跑全部测试
```

> **CI 无需修改**——只要源文件加进了 CMakeLists.txt，CI 会自动编译运行所有测试。

---

## 9. 常见问题

### Q1：为什么代码要分这么多层？放在一个目录不行吗？

**A**：分层的核心价值是**可测试性**和**可维护性**：
- `app/` 和 `hal/` 分离 → 业务代码不依赖硬件，可在 PC 上测试
- `test/` 和源码分离 → 测试代码不污染固件构建
- `framework/` 独立 → 第三方代码不与业务代码混在一起
- 每层职责单一 → 新增模块时知道文件放哪、依赖谁

### Q2：CMock 生成的桩和手写桩有什么区别？

**A**：CMock 自动生成的桩功能完整（Expect/Ignore/参数校验/调用次数），但需要 Ruby 环境生成。手写桩灵活但易遗漏。本工程使用 CMock 生成桩并检入仓库，兼顾"生成流程可验证"和"无 Ruby 可编译"。

### Q3：CI 覆盖率怎么看？

**A**：CI 的 `coverage` Job 会生成 HTML 覆盖率报告并上传为 artifact。在 GitHub Actions 页面点击对应 run → Artifacts → `coverage-report` 下载，打开 `index.html` 查看逐行覆盖率。

### Q4：Windows 上怎么构建？

**A**：CMake 会自动检测可用的生成器。如果安装了 Visual Studio，使用 VS 生成器；如果安装了 Ninja，使用 `cmake -S . -B build -G "Ninja"`。CI 中 Windows 平台使用 Ninja + MSVC 环境（`ilammy/msvc-dev-cmd`），这是最稳定的方案。

### Q5：测试中发现 bug 怎么办？

**A**：单元测试的价值就在这里——在 PC 上发现 bug 比在硬件上调试快得多。修复 `app/` 下的代码后重新运行 `ctest --test-dir build`，确认所有用例通过后再推送，CI 会自动验证。

---

## 相关文档

完整产品开发文档链见 [doc/](doc/) 目录，包含 MRD、PRD、架构设计、详细设计、测试方案、测试用例、用户手册、发布说明、运维文档共 9 份。

- [doc/05-测试方案.md](doc/05-测试方案.md) — 单元测试方案（测试策略、方法、环境）
- [doc/06-测试用例.md](doc/06-测试用例.md) — 19 个测试用例详情 + 查错能力验证记录
- [CI-CD排错日志.md](CI-CD排错日志.md) — CI/CD 历次故障排查记录（7 次失败根因分析）
