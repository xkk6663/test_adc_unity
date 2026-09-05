# 09. 运维与 CI/CD 文档

> 文档版本：v1.0
> 创建日期：2026-09-05
> 本文档描述 CI/CD 流水线的运维管理、环境配置、监控告警和故障排查。

---

## 1. CI/CD 流水线总览

### 1.1 配置文件

- 路径：`.github/workflows/ci.yml`
- 名称：CI
- 触发：push / PR 到 main 或 master 分支

### 1.2 流水线结构

```
push/PR to main
    │
    ▼
┌─────────────────────┐
│ Job 1: static-analysis │  约 30 秒
│ cppcheck 静态检查      │
└──────────┬──────────┘
           │ needs
           ▼
┌─────────────────────┐
│ Job 2: build-and-test │  约 2-4 分钟
│ 三平台矩阵构建测试     │
│  ├─ ubuntu-latest    │
│  ├─ macos-latest     │
│  └─ windows-latest   │
└──────────┬──────────┘
           │ needs
           ▼
┌─────────────────────┐
│ Job 3: coverage      │  约 1 分钟
│ gcov/lcov 覆盖率     │
└─────────────────────┘
```

### 1.3 各 Job 详解

#### Job 1：static-analysis

| 项目 | 内容 |
|------|------|
| 运行环境 | ubuntu-latest |
| 工具 | cppcheck |
| 检查范围 | app/、hal/、platform/ |
| 启用检查 | warning、style、performance、portability |
| 失败条件 | 任何 error 级别告警（`--error-exitcode=1`） |
| 预计耗时 | ~30 秒 |

#### Job 2：build-and-test（矩阵）

| 平台 | 编译器 | 构建方式 | 预计耗时 |
|------|--------|----------|----------|
| ubuntu-latest | gcc | Makefile + CMake | ~2 分钟 |
| macos-latest | clang | Makefile + CMake | ~3 分钟 |
| windows-latest | MSVC (cl.exe) | CMake + Ninja | ~3 分钟 |

每个平台执行步骤：
1. Checkout 代码
2. 安装依赖（Ubuntu: apt / macOS: brew / Windows: setup-ruby + msvc-dev-cmd）
3. 打印工具版本
4. CMock 重新生成桩（验证桩生成流程）
5. Makefile 构建（仅 Ubuntu/macOS）
6. Makefile 单元测试运行（仅 Ubuntu/macOS）
7. Makefile 固件运行（仅 Ubuntu/macOS）
8. CMake 构建（全平台）
9. CTest 单元测试运行（全平台）
10. 上传构建产物

#### Job 3：coverage

| 项目 | 内容 |
|------|------|
| 运行环境 | ubuntu-latest |
| 工具 | gcov + lcov + genhtml |
| 编译标志 | `--coverage -O0 -g` |
| 排除范围 | /usr/*、framework/*、test/*、tools/* |
| 输出 | HTML 覆盖率报告，上传为 artifact |
| 预计耗时 | ~1 分钟 |

---

## 2. 环境配置

### 2.1 GitHub Actions Runner 环境

| 平台 | 预装工具 | 需安装 |
|------|----------|--------|
| ubuntu-latest | git, make, gcc, cmake | cppcheck, ruby, lcov |
| macos-latest | git, clang, make | cmake, ruby (brew) |
| windows-latest | git, cmake, Visual Studio Build Tools, Ninja | ruby (setup-ruby), MSVC 环境 (msvc-dev-cmd) |

### 2.2 关键 Action 版本

| Action | 版本 | 用途 |
|--------|------|------|
| actions/checkout | v4 | 检出代码 |
| actions/upload-artifact | v4 | 上传产物 |
| ruby/setup-ruby | v1 | Windows 安装 Ruby |
| ilammy/msvc-dev-cmd | v1 | Windows 设置 MSVC 环境 |

### 2.3 依赖版本锁定

- Ruby：3.3（Windows 通过 setup-ruby 指定）
- CMake：≥ 3.10（CMakeLists.txt 要求）
- Unity：vendor 固定版本（framework/unity/）
- CMock：vendor 固定版本（framework/cmock/）

---

## 3. 产物管理

### 3.1 构建产物

每个平台的构建产物上传为 artifact，保留 90 天（GitHub 默认）：

| Artifact 名称 | 内容 | 来源 Job |
|---------------|------|----------|
| binaries-ubuntu-latest | unit_test, firmware_demo | build-and-test |
| binaries-macos-latest | unit_test, firmware_demo | build-and-test |
| binaries-windows-latest | unit_test.exe, firmware_demo.exe | build-and-test |
| coverage-report | HTML 覆盖率报告 | coverage |

### 3.2 下载方式

1. 打开 GitHub Actions 对应 workflow run
2. 滚动到页面底部 Artifacts 区域
3. 点击对应 artifact 名称下载 ZIP

---

## 4. 监控与告警

### 4.1 CI 状态徽章

在 README.md 中显示 CI 状态：
```markdown
[![CI](https://github.com/xkk6663/test_adc_unity/actions/workflows/ci.yml/badge.svg)](https://github.com/xkk6663/test_adc_unity/actions)
```

- 绿色 passing：所有 Job 通过
- 红色 failing：至少一个 Job 失败
- 黄色：正在运行

### 4.2 邮件通知

GitHub Actions 默认在 CI 失败时向提交者发送邮件通知。可在仓库 Settings → Notifications 中配置。

### 4.3 建议的告警规则

| 指标 | 阈值 | 动作 |
|------|------|------|
| main 分支 CI 失败 | 立即 | 邮件通知提交者，禁止合并 |
| 覆盖率下降 | > 5% | PR 评论提醒 |
| cppcheck 新增告警 | 任何 | CI 失败 |
| CI 运行时间 | > 10 分钟 | 调查性能瓶颈 |

---

## 5. 故障排查

### 5.1 快速排查流程

```
CI 失败
  │
  ├─ 哪个 Job 失败？
  │   ├─ static-analysis → 代码风格问题，看 cppcheck 输出
  │   ├─ build-and-test  → 编译或测试失败，看具体平台
  │   └─ coverage        → 覆盖率工具问题
  │
  ├─ 哪个平台失败？
  │   ├─ 仅 Ubuntu → 依赖安装问题 / Linux 特定问题
  │   ├─ 仅 macOS  → brew 依赖问题 / clang 兼容性
  │   └─ 仅 Windows → shell 语法 / 路径 / MSVC 环境
  │
  ├─ 本地能否复现？
  │   ├─ 能 → 代码问题，本地修复
  │   └─ 不能 → 平台差异，重点查 CI 环境
  │
  └─ 查看排错日志
      └─ CI-CD排错日志.md（记录了 7 次历史失败）
```

### 5.2 常见故障速查

| 故障 | 原因 | 解决方案 | 日志编号 |
|------|------|----------|----------|
| CMock LoadError | vendor 不完整 | 补全 cmock/config 或 unity/auto | #01, #02 |
| `#include "unity"` 错误 | CMockConfig.yml 无效配置 | 移除 :includes 配置项 | #03 |
| `cmock.h: No such file` | 缺少 CMock C 运行时 | 补全 cmock/src/，添加编译路径 | #04 |
| `./unit_test: No such file` | 跨平台后缀问题 | Makefile 动态检测 OS | #05 |
| PowerShell 语法错误 | Windows 默认 shell | 指定 shell: bash | #06 |
| Visual Studio 生成器失败 | runner 上 VS 不可用 | 改用 Ninja + msvc-dev-cmd | #07 |

### 5.3 本地复现 CI 环境

```bash
# 1. 静态分析
cppcheck --enable=warning,style,performance,portability \
         -I app/battery -I app/led -I hal app/ hal/ platform/

# 2. 桩生成 + 构建 + 测试
ruby tools/gen_mocks.rb
make all
./unit_test

# 3. 覆盖率
make clean
make all CFLAGS="-Wall -Wextra -std=c99 --coverage -O0 -g"
./unit_test
lcov --capture --directory . --output-file coverage.info
```

---

## 6. 日常运维操作

### 6.1 手动触发 CI

```bash
# 空提交触发（不推荐，仅用于调试）
git commit --allow-empty -m "trigger CI"
git push

# 推荐：在 GitHub Actions 页面点击 "Re-run jobs"
```

### 6.2 跳过 CI

在 commit message 中添加 `[skip ci]` 或 `[ci skip]`：
```bash
git commit -m "update docs [skip ci]"
```

### 6.3 更新依赖版本

```yaml
# .github/workflows/ci.yml
# 更新 Action 版本
uses: actions/checkout@v4  # → v5 发布后修改
```

更新后提交 PR，观察 CI 是否通过。

### 6.4 清理旧产物

GitHub Actions 产物默认保留 90 天，自动清理。如需手动清理：
- 仓库 Settings → Actions → Artifact and log retention
- 可调整保留天数（1-400 天）

---

## 7. 安全与权限

### 7.1 仓库权限

- 分支保护：main 分支要求 CI 通过才能合并
- 推送权限：仅协作者可推送
- PR 审查：建议至少 1 人审查

### 7.2 Secrets 管理

本工程 CI 不需要 Secrets（公开仓库，无部署步骤）。
未来如需添加部署步骤，Secrets 配置路径：
仓库 Settings → Secrets and variables → Actions

### 7.3 第三方 Action 安全

使用的 Action 均为官方或社区广泛使用的：
- actions/checkout — GitHub 官方
- actions/upload-artifact — GitHub 官方
- ruby/setup-ruby — Ruby 官方
- ilammy/msvc-dev-cmd — 社区高星（200+ stars）

建议定期检查 Action 版本更新，避免使用已废弃的版本。

---

## 8. 性能优化记录

| 优化项 | 优化前 | 优化后 | 效果 |
|--------|--------|--------|------|
| fail-fast: false | 一个平台失败全部取消 | 各平台独立运行 | 便于同时排查多平台问题 |
| 桩文件检入 | 每次 CI 必须有 Ruby | 无 Ruby 也能编译 | 降低环境依赖 |
| Ninja 替代 VS 生成器 | VS 生成器不稳定 | Ninja 稳定快速 | Windows 构建更可靠 |
| 三 Job 分离 | 单 Job 串行 | 静态分析/构建/覆盖率并行 | 总耗时缩短 |

---

## 9. 备份与恢复

### 9.1 代码备份

代码托管在 GitHub，自动有分布式备份。本地也应保持最新 clone：
```bash
git fetch origin
git pull origin main
```

### 9.2 CI 配置备份

CI 配置（`.github/workflows/ci.yml`）随代码版本管理，可通过 git 历史恢复：
```bash
git log --oneline .github/workflows/ci.yml
git checkout <commit> -- .github/workflows/ci.yml
```

### 9.3 文档备份

doc/ 目录随代码版本管理，CI-CD排错日志.md 记录了所有历史故障和修复方案。
