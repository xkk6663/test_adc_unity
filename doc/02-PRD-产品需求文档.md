# 02. 产品需求文档（PRD）

> 文档版本：v1.0
> 创建日期：2026-09-05
> 上游文档：[01-MRD-市场需求文档.md](01-MRD-市场需求文档.md)
> 产品名称：嵌入式电池电压监测与指示模块

---

## 1. 文档目的

本文档基于 MRD 定义产品级功能需求，包括用户故事、功能详述、验收标准和非功能需求，作为技术设计和测试的直接依据。

---

## 2. 产品概述

### 2.1 产品定位

一个**可直接用于真实嵌入式产品**的电池电压监测参考工程，展示如何在 PC 上对嵌入式 C 代码进行产品级单元测试和 CI/CD 自动化。

### 2.2 核心功能

1. **电压采集**：通过 ADC 读取原始值，换算为毫伏(mV)电压
2. **状态指示**：根据电压值控制 LED（低电压红灯、正常绿灯、过压红灯快闪）
3. **固件模拟**：在 PC 上模拟固件主循环运行
4. **自动化测试**：19 个单元测试用例，覆盖正常/边界/异常场景
5. **CI/CD**：push 自动触发静态分析、多平台构建、覆盖率报告

---

## 3. 用户故事

### 3.1 嵌入式工程师

| 编号 | 用户故事 | 验收标准 |
|------|----------|----------|
| US-01 | 作为嵌入式工程师，我希望业务代码与硬件分离，这样我可以在 PC 上测试逻辑 | app/ 层代码不包含任何硬件寄存器操作，只调用 hal/ 接口 |
| US-02 | 作为嵌入式工程师，我希望新增模块时有清晰的步骤，这样我可以快速上手 | README 中有"新增模块指南"，包含 7 步完整流程 |
| US-03 | 作为嵌入式工程师，我希望代码有详细注释，这样我可以理解每个函数的设计意图 | 每个 .c 文件函数级注释覆盖率 100% |

### 3.2 测试工程师

| 编号 | 用户故事 | 验收标准 |
|------|----------|----------|
| US-04 | 作为测试工程师，我希望测试用例自动运行，这样我不需要手动烧录硬件 | `make test` 一键运行全部 19 个用例 |
| US-05 | 作为测试工程师，我希望看到测试覆盖率，这样我可以评估测试充分性 | CI 生成 lcov HTML 覆盖率报告并上传 |
| US-06 | 作为测试工程师，我希望测试用例有文档记录，这样我可以追溯每个用例的设计依据 | doc/06-测试用例.md 记录每个用例的编号、方法、预期 |

### 3.3 项目经理

| 编号 | 用户故事 | 验收标准 |
|------|----------|----------|
| US-07 | 作为项目经理，我希望每次 push 都自动验证，这样我可以及时发现问题 | CI 三 Job 全部通过才能合并 |
| US-08 | 作为项目经理，我希望有完整的文档链，这样新人可以快速上手 | doc/ 下 9 份文档齐全 |

---

## 4. 功能需求详述

### 4.1 电池电压采集模块（battery）

#### FR-001：ADC 原始值换算为毫伏

- **输入**：12 位 ADC 原始值（0 ~ 4095）
- **输出**：毫伏电压（0 ~ 3300 mV）
- **换算公式**：`voltage_mv = adc_raw * 3300 / 4095`
- **边界处理**：
  - 输入 0 → 输出 0 mV
  - 输入 4095 → 输出 3300 mV
  - 输入超过 4095 → 钳位到 4095（返回 3300 mV）
- **接口**：`uint16_t adc_to_mv(uint16_t adc_raw)`

#### FR-002：读取电池电压

- **功能**：调用 HAL ADC 接口读取原始值，换算为毫伏返回
- **依赖**：`hal_adc.h` → `uint16_t HAL_ADC_GetValue(void)`
- **接口**：`uint16_t read_battery_mv(void)`
- **异常处理**：HAL 返回 0 时视为电池已移除或 ADC 故障，返回 0

### 4.2 LED 指示灯模块（led）

#### FR-003：根据电压设置 LED 状态

- **电压区间**：
  - `< 6000 mV` → 低电量：红灯亮，绿灯灭
  - `6000 ~ 8400 mV` → 正常：绿灯亮，红灯灭
  - `> 8400 mV` → 过压：红灯亮，绿灯灭（告警）
- **依赖**：`hal_gpio.h` → `void HAL_GPIO_SetPin(uint8_t pin, uint8_t level)`
- **接口**：`void led_set_by_voltage(uint16_t voltage_mv)`

#### FR-004：获取当前 LED 状态

- **接口**：`led_state_t led_get_state(void)`
- **返回值**：`LED_STATE_OFF` / `LED_STATE_RED` / `LED_STATE_GREEN`

### 4.3 固件主程序

#### FR-005：固件主循环

- **功能**：周期性读取电池电压，更新 LED 状态，打印日志
- **模拟周期**：1 秒（PC 模拟用 `sleep(1)`）
- **输出**：每秒打印当前电压和 LED 状态
- **接口**：`int main(void)` in `platform/firmware/firmware_main.c`

### 4.4 构建系统

#### FR-006：Makefile 构建

- 支持 `make all`（编译单元测试 + 固件）
- 支持 `make test`（编译并运行单元测试）
- 支持 `make clean`（清理产物）
- 跨平台：Windows 生成 `.exe`，Linux/macOS 无后缀

#### FR-007：CMake 构建

- 支持 `cmake -S . -B build && cmake --build build`
- 支持 `ctest` 运行单元测试
- 跨平台：Unix Makefiles / Ninja / Visual Studio

### 4.5 CI/CD

#### FR-008：静态分析

- 工具：cppcheck
- 检查范围：app/、hal/、platform/
- 启用：warning、style、performance、portability
- 失败条件：任何 error 级别告警

#### FR-009：多平台构建矩阵

| 平台 | 编译器 | 构建方式 |
|------|--------|----------|
| Ubuntu | gcc | Makefile + CMake |
| macOS | clang | Makefile + CMake |
| Windows | MSVC (Ninja) | CMake |

#### FR-010：测试覆盖率

- 工具：gcov + lcov
- 排除：framework/、test/、tools/
- 输出：HTML 报告，上传为 artifact

---

## 5. 非功能需求

### 5.1 性能

| 指标 | 要求 |
|------|------|
| 单元测试执行时间 | < 1 秒（19 个用例） |
| CI 完整流水线时间 | < 5 分钟 |
| 固件主循环单次执行 | < 10ms（不含 sleep） |

### 5.2 可移植性

- 业务代码（app/）符合 C99 标准，不依赖特定编译器扩展
- HAL 接口仅使用 stdint.h 类型
- 构建系统支持 gcc / clang / MSVC

### 5.3 可维护性

- 代码注释覆盖率：函数级 100%
- 圈复杂度：每个函数 ≤ 10
- 单元测试覆盖率：业务代码 ≥ 90%

### 5.4 安全性

- 无动态内存分配（除 CMock 桩内部使用）
- 所有数组访问有边界检查
- 无未初始化变量使用

---

## 6. 验收标准汇总

| 编号 | 验收项 | 验证方式 |
|------|--------|----------|
| AC-01 | 19 个单元测试全部通过 | `make test` 输出 `19 Tests 0 Failures` |
| AC-02 | 编译零警告 | `make all` 无 warning 输出 |
| AC-03 | CI 三 Job 全部通过 | GitHub Actions 徽章 passing |
| AC-04 | 覆盖率 ≥ 90% | lcov 报告 Lines ≥ 90% |
| AC-05 | cppcheck 零 error | CI static-analysis Job 通过 |
| AC-06 | 三平台编译通过 | Ubuntu/macOS/Windows 矩阵全绿 |
| AC-07 | 文档 9/9 齐全 | doc/ 目录检查 |

---

## 7. 需求追踪矩阵

| PRD 需求 | 对应测试用例 | 对应代码模块 |
|----------|-------------|-------------|
| FR-001 ADC 换算 | TC-BAT-001 ~ TC-BAT-005 | app/battery/battery.c: adc_to_mv |
| FR-002 读取电压 | TC-BAT-006 ~ TC-BAT-011 | app/battery/battery.c: read_battery_mv |
| FR-003 LED 状态 | TC-LED-001 ~ TC-LED-006 | app/led/led.c: led_set_by_voltage |
| FR-004 获取状态 | TC-LED-007 ~ TC-LED-008 | app/led/led.c: led_get_state |
| FR-005 固件主循环 | 固件运行验证 | platform/firmware/firmware_main.c |
| FR-006/007 构建 | CI 构建验证 | Makefile / CMakeLists.txt |
| FR-008~010 CI/CD | CI 流水线验证 | .github/workflows/ci.yml |
