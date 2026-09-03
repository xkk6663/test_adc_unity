#!/usr/bin/env ruby
# ==============================================================================
# CMock 桩生成脚本
# ==============================================================================
# 用途：根据 hal_adc.h 自动生成 CMock 桩文件 mock_hal_adc.c / mock_hal_adc.h
#
# 依赖：
#   - Ruby 运行环境（CMock 是基于 Ruby 的代码生成器）
#   - 本工程 cmock/ 目录下已包含 CMock 源码（无需额外安装 gem）
#
# 使用方法：
#   ruby gen_mocks.rb
#
# 执行后会在当前目录生成（或覆盖）：
#   - mock_hal_adc.h   桩函数头文件
#   - mock_hal_adc.c   桩函数实现
#
# 注意：
#   1. 本工程已检入预生成的 mock_hal_adc.c/h，在没有 Ruby 的环境下可直接编译。
#      运行本脚本会用 CMock 重新生成的版本覆盖预生成版本。
#   2. 修改 hal_adc.h 中的接口后，必须重新运行本脚本更新桩文件。
#   3. CMock 配置（插件、前缀等）见 CMockConfig.yml。
# ==============================================================================

# 将 CMock 库目录加入 Ruby 加载路径
# __dir__ 是本脚本所在目录，CMock 源码在 cmock/lib/ 下
$LOAD_PATH.unshift(File.join(__dir__, 'cmock', 'lib'))

# 加载 CMock 主库
require 'cmock'

# 配置文件路径
config_file = File.join(__dir__, 'CMockConfig.yml')

# 检查配置文件是否存在
unless File.exist?(config_file)
  puts "ERROR: CMock config file not found: #{config_file}"
  exit 1
end

# 创建 CMock 实例（传入配置文件路径）
cmock = CMock.new(config_file)

# 需要生成桩的头文件列表
# 如需为更多 HAL 头文件生成桩，在此数组中添加即可
header_files = ['hal_adc.h']

puts "Generating CMock mocks for: #{header_files.join(', ')}"
puts "Config: #{config_file}"

# 生成桩文件
# setup_mocks 会读取每个头文件，解析函数原型，生成对应的 mock_*.c/h
cmock.setup_mocks(header_files)

puts "CMock mocks generated successfully."
puts "Output files: mock_hal_adc.h, mock_hal_adc.c"
