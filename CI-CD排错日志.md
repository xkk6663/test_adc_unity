# CI/CD 排错日志

> 记录本工程 CI/CD 流水线从搭建到跑通过程中遇到的所有报错、根因分析和修复方案。
> 每次 CI 失败后更新此文件，作为团队知识库和避坑指南。

| 项目 | 内容 |
|------|------|
| 仓库 | https://github.com/xkk6663/test_adc_unity |
| CI 配置 | `.github/workflows/ci.yml` |
| 首次搭建 | 2026-09-03 |
| 最近更新 | 2026-09-05 |

---

## 日志索引

| # | 日期 | 失败阶段 | 根因 | 状态 |
|---|------|----------|------|------|
| 01 | 2026-09-03 | CMock 桩生成 | `cmock/config/production_environment.rb` 缺失 | ✅ 已修复 |
| 02 | 2026-09-03 | CMock 桩生成 | `unity/auto/type_sanitizer.rb` 缺失 | ✅ 已修复 |
| 03 | 2026-09-03 | CMock 桩生成 | `CMockConfig.yml` 无效配置项导致 `#include "unity"` | ✅ 已修复 |
| 04 | 2026-09-03 | Makefile 编译 | `cmock/src/cmock.h` 缺失（CMock C 运行时未 vendor） | ✅ 已修复 |
| 05 | 2026-09-03 | 运行单元测试 | Linux 上 Makefile 生成 `unit_test.exe` 但 CI 运行 `./unit_test` | ✅ 已修复 |
| 06 | 2026-09-05 | Windows 工具版本打印 | PowerShell 不支持 Unix 语法（`&&`、`2>/dev/null`） | ✅ 已修复 |

---

## 01. CMock config 目录缺失

**日期**：2026-09-03
**CI Run**：#33768624330（commit d44f1ee）
**失败步骤**：Step 5 "Regenerate mocks with CMock"（`ruby gen_mocks.rb`）

### 报错信息

```
/usr/lib/ruby/.../require.rb: cannot load such file --
cmock/config/production_environment.rb (LoadError)
```

### 根因分析

CMock 在加载时（`cmock/lib/cmock.rb` 第 9 行）会执行：
```ruby
require "#{__dir__}/../config/production_environment"
```

当初 vendor CMock 时只复制了 `cmock/lib/*.rb`（Ruby 生成器），遗漏了 `cmock/config/` 目录下的 `.rb` 文件（当时只尝试复制 `config/*.yml`，但该目录下没有 .yml 文件）。

`production_environment.rb` 的作用是将 `lib/` 目录加入 Ruby 的 `$LOAD_PATH`，是 CMock 加载的必要依赖。

### 修复方案

重新 clone CMock 仓库，将完整的 `config/` 目录复制到工程：
```bash
cp -r _tmp_cmock/config/* unity_cmock_demo/cmock/config/
```

补全的文件：
- `cmock/config/production_environment.rb`
- `cmock/config/test_environment.rb`

### 教训

Vendor 第三方库时，**不能只复制看起来"有用"的目录**。CMock 的 `config/` 目录虽然没有 .rb 之外的文件，但 `production_environment.rb` 是启动必需的。应复制完整的目录结构，或阅读源码确认所有 `require` 依赖。

---

## 02. Unity auto/ 目录缺失

**日期**：2026-09-03
**CI Run**：#33768911803（commit 935f66d）
**失败步骤**：Step 5 "Regenerate mocks with CMock"

### 报错信息

```
cannot load such file -- unity/auto/type_sanitizer (LoadError)
```

### 根因分析

`cmock/lib/cmock_generator.rb` 在初始化时会尝试加载 Unity 的 `auto/type_sanitizer.rb`：
```ruby
unity_path_in_ceedling = "#{here}/../../unity"  # 工程根/unity
if File.exist? unity_path_in_ceedling
  require "#{unity_path_in_ceedling}/auto/type_sanitizer"
end
```

当初 vendor Unity 时只复制了 `src/` 下的 3 个 C 文件（`unity.c/h/internals.h`）到 `unity/` 目录，遗漏了 `auto/` 目录（Ruby 辅助脚本）。

CMock 检测到 `unity/` 目录存在（误认为是 Ceedling 集成的完整 Unity 仓库），就尝试 require `unity/auto/type_sanitizer.rb`，但该文件不存在，导致 LoadError。

### 修复方案

重新 clone Unity 仓库，将 `auto/` 目录复制到 `unity/` 下：
```bash
cp -r _tmp_unity/auto unity_cmock_demo/unity/
```

补全的关键文件：`unity/auto/type_sanitizer.rb`（CMock 依赖的类型处理工具）

### 教训

Unity 仓库不仅有 C 源码（`src/`），还有 Ruby 辅助脚本（`auto/`）。CMock 在生成桩时依赖 `auto/type_sanitizer.rb`。Vendor Unity 时必须包含 `auto/` 目录，否则 CMock 无法加载。

---

## 03. CMockConfig.yml 无效配置项

**日期**：2026-09-03
**CI Run**：#33769295189（commit d3a0dd5）
**失败步骤**：Step 6 "Build with Makefile"

### 报错信息

```
mock_hal_adc.h:6:17: fatal error: unity: No such file or directory
 #include "unity"
                 ^
```

### 根因分析

`CMockConfig.yml` 中配置了：
```yaml
:includes:
  - :unity
```

CMock 生成桩时，会将 `:includes` 数组中的每个元素直接转为 `#include "..."`。符号 `:unity` 被 to_s 为 `"unity"`，生成了错误的 `#include "unity"`（缺少 `.h` 后缀）。

实际上，CMock **自动**在生成的桩头文件中包含 `#include "unity.h"`（第 5 行正确），不需要显式配置 `:includes`。`:includes` 是用于**额外**包含的头文件。

此外，原配置中还有两个无效项：
- `:output_path: "."` —— CMock 标准配置项是 `:mock_path`，不是 `:output_path`
- `:unity_path: unity/` —— CMock 默认配置中没有此项

### 修复方案

1. 将 `:output_path: "."` 改为 `:mock_path: "."`（标准配置项）
2. 移除无效的 `:unity_path`
3. 移除 `:includes: [:unity]`（CMock 自动包含 unity.h，显式配置反而生成错误的 include）

### 教训

使用第三方工具的配置文件时，**必须对照官方文档/源码确认配置项名称**。CMock 的默认配置定义在 `cmock/lib/cmock_config.rb` 中，应以此为准。不要凭猜测添加配置项。

---

## 04. CMock C 运行时（src/）缺失

**日期**：2026-09-03
**CI Run**：commit 1a253d1（本地复现）
**失败步骤**：Step 6 "Build with Makefile"

### 报错信息

```
mock_hal_adc.c:5:19: fatal error: cmock.h: No such file or directory
 #include "cmock.h"
                   ^
```

### 根因分析

CMock 生成的桩文件（`mock_hal_adc.c`）开头包含 `#include "cmock.h"`。`cmock.h` 是 CMock 的 **C 运行时头文件**，定义了桩函数依赖的内存管理宏和函数（`CMock_Guts_MemNew` 等）。

当初 vendor CMock 时只复制了：
- `cmock/lib/` —— Ruby 生成器（运行 `gen_mocks.rb` 需要）
- `cmock/config/` —— Ruby 环境配置（修复 #01 后补全）

但遗漏了 `cmock/src/` —— **C 运行时**（编译桩文件时需要链接）。

CMock 仓库有三个关键目录：
| 目录 | 用途 | 何时需要 |
|------|------|----------|
| `lib/` | Ruby 代码生成器 | 运行 `gen_mocks.rb` 时 |
| `config/` | Ruby 环境配置 | 运行 `gen_mocks.rb` 时 |
| `src/` | C 运行时（cmock.c/h） | 编译链接桩文件时 |

### 修复方案

1. 重新 clone CMock，复制 `src/` 目录：
   ```bash
   cp -r _tmp_cmock/src unity_cmock_demo/cmock/
   ```
2. 在 Makefile 和 CMakeLists.txt 中：
   - 添加 `-Icmock/src` 到头文件搜索路径
   - 添加 `cmock/src/cmock.c` 到单元测试目标的源文件列表

### 教训

CMock 是"**生成时 + 编译时**"双重依赖的工具：
- 生成桩需要 Ruby + `lib/` + `config/`
- 编译桩需要 C 编译器 + `src/`（cmock.c/h）

Vendor CMock 时必须同时包含 `lib/`、`config/`、`src/` 三个目录。

---

## 05. Makefile 跨平台可执行文件后缀

**日期**：2026-09-03
**CI Run**：commit 1a253d1
**失败步骤**：Step 7 "Run unit tests (Makefile)"

### 报错信息

```
./unit_test: No such file or directory
Error: Process completed with exit code 127.
```

### 根因分析

Makefile 中硬编码了目标名：
```makefile
TEST_BIN = unit_test.exe
```

CI 运行在 Linux（ubuntu-latest）上，gcc 生成的可执行文件名就是 `unit_test.exe`（Makefile 指定的输出名，即使在 Linux 上也带 .exe 后缀）。但 CI 步骤运行的命令是 `./unit_test`（不带 .exe），导致找不到文件。

在 Windows 上，gcc 生成的可执行文件**必须**带 `.exe` 后缀才能运行；在 Linux/macOS 上，可执行文件**不需要**后缀。

### 修复方案

在 Makefile 中添加操作系统检测，动态决定后缀：
```makefile
ifeq ($(OS),Windows_NT)
    EXE_EXT = .exe
else
    EXE_EXT =
endif

TEST_BIN = unit_test$(EXE_EXT)
FW_BIN   = firmware_demo$(EXE_EXT)
```

这样 Windows 上生成 `unit_test.exe`，Linux/macOS 上生成 `unit_test`，CI 运行 `./unit_test` 就能找到。

### 教训

跨平台 Makefile 中，**可执行文件后缀必须根据操作系统动态决定**，不能硬编码 `.exe`。`$(OS)` 变量在 Windows 上为 `Windows_NT`，在 Linux/macOS 上未定义，可用于平台判断。

---

## 06. Windows 平台 shell 兼容性（PowerShell vs Bash）

**日期**：2026-09-05
**CI Run**：commit e268c1a（产品级架构重构后首次运行）
**失败步骤**：Step "Show tool versions"（windows-latest）

### 报错信息

```
Out-File: D:\a\_temp\ae46f30f-...ps1:2
Line |
   2 |  echo "=== OS ===" && uname -a 2>/dev/null || ver
     |                ~
Could not find a part of the path 'D:\dev\null'.
Error: Process completed with exit code 1.
```

### 根因分析

GitHub Actions 的 `run` 步骤在不同平台使用不同的默认 shell：
- Ubuntu / macOS：默认 `bash`
- Windows：默认 **PowerShell**（不是 cmd，也不是 bash）

"Show tool versions" 步骤的命令使用了 Unix 语法：
```bash
echo "=== OS ===" && uname -a 2>/dev/null || ver
```

在 Windows PowerShell 中：
- `&&` —— PowerShell 5.1 不支持（PowerShell 7+ 才支持）
- `2>/dev/null` —— `/dev/null` 是 Unix 设备文件，Windows 上不存在（PowerShell 用 `$null`）
- `uname` —— Windows 上没有此命令

PowerShell 尝试将 `2>/dev/null` 解析为重定向到文件路径 `D:\dev\null`，导致 "Could not find a part of the path" 错误。

### 修复方案

给该步骤显式指定 `shell: bash`：
```yaml
- name: Show tool versions
  shell: bash
  run: |
    echo "=== OS ===" && uname -a 2>/dev/null || ver
    echo "=== cmake ===" && cmake --version
    echo "=== ruby ===" && ruby --version
```

GitHub Actions 的 Windows runner 预装了 Git Bash，指定 `shell: bash` 后会使用 Git Bash 执行命令，Unix 语法全部可用。

### 教训

在多平台 CI 矩阵中，**不要假设所有平台都用 bash**。Windows runner 默认用 PowerShell，语法差异很大：

| 特性 | bash | PowerShell 5.1 |
|------|------|----------------|
| 命令串联 | `&&` / `\|\|` | 不支持（用 `;`） |
| 空设备 | `/dev/null` | `$null` |
| 环境变量 | `$VAR` | `$env:VAR` |
| 路径分隔 | `/` | `\`（但 `/` 也常被接受） |

**最佳实践**：跨平台的 `run` 步骤统一指定 `shell: bash`，确保三个平台行为一致。Windows runner 的 Git Bash 完全支持标准 Unix 语法。

---

## 附录：CI 调试技巧

### 1. 无法获取 CI 日志时怎么办？

GitHub API 的日志端点（`/actions/jobs/{job_id}/logs`）需要认证，未认证访问返回 403。替代方案：
- **CI 徽章**：`https://github.com/{owner}/{repo}/actions/workflows/{workflow}.badge.svg`，返回 passing/failing
- **Jobs API**：`/actions/runs/{run_id}/jobs` 返回每个 step 的 conclusion（success/failure/skipped），不需要日志权限
- **用户截图**：让用户在 GitHub Actions 页面截图报错步骤

### 2. 本地复现 CI 环境

CI 失败后，优先在本地复现：
- 安装相同版本的 Ruby（`winget install RubyInstallerTeam.Ruby.3.3`）
- 运行 `ruby tools/gen_mocks.rb` 验证桩生成
- 运行 `make all && ./unit_test` 验证编译测试
- 本地通过但 CI 失败时，重点检查**平台差异**（路径、shell、换行符）

### 3. 常见 CI 失败模式速查

| 失败现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| `ruby gen_mocks.rb` 报 LoadError | CMock/Unity 依赖文件缺失 | 检查 `require` 链，补全 vendor 目录 |
| 编译报 `No such file or directory` | 头文件路径不对 | 检查 `-I` 路径和 `#include` 文件名 |
| `./unit_test: No such file` | 可执行文件名不匹配 | 检查跨平台后缀（.exe） |
| Windows 上报语法错误 | PowerShell 不支持 Unix 语法 | 添加 `shell: bash` |
| cppcheck 失败 | 代码有潜在问题 | 阅读 cppcheck 输出，修复或加 `--inline-suppr` |
| CTest 找不到测试 | 可执行文件名或路径不对 | 检查 CMakeLists.txt 中 `add_test` 的 COMMAND |

---

*本文档随 CI 排错持续更新。每次 CI 失败修复后，请在此追加一条记录。*
