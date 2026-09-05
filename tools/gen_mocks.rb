#!/usr/bin/env ruby
# ==============================================================================
# CMock 桩生成脚本（产品级分层架构）
# ==============================================================================
# 用途：根据 hal/ 目录下的 HAL 头文件自动生成 CMock 桩文件，输出到 test/mocks/
#
# 当前支持的头文件：
#   - hal/hal_adc.h   → test/mocks/mock_hal_adc.c/h   （电池电压采集模块依赖）
#   - hal/hal_gpio.h  → test/mocks/mock_hal_gpio.c/h  （LED 指示灯模块依赖）
#
# 使用方法（在工程根目录执行）：
#   ruby tools/gen_mocks.rb
#
# 依赖：
#   - Ruby 运行环境
#   - framework/cmock/ 目录下已包含 CMock 源码
#
# 注意：
#   1. 本工程已检入 CMock 生成的桩文件（test/mocks/），无 Ruby 环境可直接编译。
#   2. 修改任何 hal/*.h 接口后，必须重新运行本脚本更新桩。
#   3. 新增 HAL 头文件时，在下方 header_files 数组中添加即可。
#   4. CMock 配置（插件、前缀、输出路径等）见 tools/CMockConfig.yml。
# ==============================================================================

# 将 CMock 库目录加入 Ruby 加载路径
# __dir__ 是本脚本所在目录（tools/），CMock 源码在 ../framework/cmock/lib/
$LOAD_PATH.unshift(File.join(__dir__, '..', 'framework', 'cmock', 'lib'))

# 加载 CMock 主库
require 'cmock'

# 配置文件路径（与脚本同目录）
config_file = File.join(__dir__, 'CMockConfig.yml')

# 检查配置文件是否存在
unless File.exist?(config_file)
  puts "ERROR: CMock config file not found: #{config_file}"
  exit 1
end

# 创建 CMock 实例（传入配置文件路径）
cmock = CMock.new(config_file)

# 需要生成桩的头文件列表（相对于工程根目录）
# 新增 HAL 头文件时在此数组中添加
header_files = ['hal/hal_adc.h', 'hal/hal_gpio.h']

puts "Generating CMock mocks for: #{header_files.join(', ')}"
puts "Config: #{config_file}"

# 生成桩文件
# setup_mocks 会读取每个头文件，解析函数原型，生成对应的 mock_*.c/h
# 输出路径由 CMockConfig.yml 中的 :mock_path 决定（test/mocks/）
cmock.setup_mocks(header_files)

puts "CMock mocks generated successfully."
puts "Output dir: test/mocks/"
puts "Output files: mock_hal_adc.h, mock_hal_adc.c, mock_hal_gpio.h, mock_hal_gpio.c"
