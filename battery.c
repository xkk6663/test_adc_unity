/**
 * @file    battery.c
 * @brief   电池电压采集模块 —— 被测模块实现
 *
 * @details 本文件包含两个核心函数：
 *          1. adc_to_mv()      —— 纯软件换算 + 边界钳位（不依赖硬件）
 *          2. read_battery_mv() —— 调用 HAL 读取 ADC + 换算 + 容错
 *
 *          这两个函数正是单元测试的核心被测对象：
 *          - adc_to_mv() 验证"函数健全性"：正常/边界/非法输入下的换算与钳位
 *          - read_battery_mv() 验证"接口交互正确性"：HAL 调用次数/参数/容错
 *
 * @note    本文件在 CMake 双目标编译中被两个目标同时链接：
 *          - PC 测试目标：链接 CMock 生成的 mock_hal_adc.c（桩替换 HAL）
 *          - 固件目标：  链接 firmware_main.c 中的真实 HAL 模拟实现
 *          同一份业务代码，两种运行环境，这正是 Unity+CMock 方案的核心。
 */
#include "battery.h"
#include "hal_adc.h"

/**
 * @brief 将 ADC 原始值换算为电池电压（mV），并做边界钳位
 *
 * @param[in] adc_raw  ADC 原始值（0~4095）
 * @return    电池电压 mV，钳位在 [BATTERY_MIN_MV, BATTERY_MAX_MV]
 *
 * @details 换算流程：
 *          步骤1: ADC 引脚测得电压 = adc_raw × ADC_REF_MV / ADC_RAW_MAX
 *                 （整数除法，结果范围 0~3300 mV）
 *          步骤2: 电池电压 = ADC 引脚测得电压 × BATTERY_DIVIDER
 *                 （结果范围 0~9900 mV，超过 2S 满电 8.4V）
 *          步骤3: 下钳位到 BATTERY_MIN_MV（防御性，正常不会触发）
 *          步骤4: 上钳位到 BATTERY_MAX_MV（关键防护，超压时触发）
 *
 * @note    整数运算顺序很重要：先乘后除可以避免精度损失。
 *          例如 adc_raw=2048: 2048×3300/4095 = 1650 (mV), ×3 = 4950 (mV)
 */
uint16_t adc_to_mv(uint16_t adc_raw)
{
    /* 步骤0：非法输入防护
     * ADC 原始值超过 12 位满量程 (4095) 时，视为非法输入（如寄存器读乱码、
     * DMA 传输错误），返回下限值 BATTERY_MIN_MV(0)，避免异常值扩散到后续逻辑。
     * 这是对"硬件不可靠"的防御性编程。 */
    if (adc_raw > ADC_RAW_MAX) {
        return BATTERY_MIN_MV;
    }

    /* 步骤1：ADC 原始值 → ADC 引脚测得电压（mV）
     * 使用 uint32_t 中间变量防止乘法溢出：
     * 4095 × 3300 = 13,513,500，超过 uint16_t 最大值 65,535 */
    uint32_t measured_mv = (uint32_t)adc_raw * ADC_REF_MV / ADC_RAW_MAX;

    /* 步骤2：通过分压比还原电池电压
     * measured_mv 最大 3300，×3 = 9900，仍在 uint32_t 范围内 */
    uint32_t battery_mv = measured_mv * BATTERY_DIVIDER;

    /* 步骤3：上钳位 —— 本模块的关键防护逻辑
     * 当电池电压超过 2S 满电 8.4V 时，视为过压或采样异常，
     * 钳位到最大值，防止后续控制逻辑使用异常电压导致危险操作。
     *
     * 注意：这是单元测试的重点验证点之一。
     * 若此处条件写反（如 > 写成 <），边界用例会立刻失败。 */
    if (battery_mv > BATTERY_MAX_MV) {
        battery_mv = BATTERY_MAX_MV;
    }

    /* battery_mv 最大为 BATTERY_MAX_MV(8400)，安全转为 uint16_t */
    return (uint16_t)battery_mv;
}

/**
 * @brief 读取电池电压：调用 HAL 获取 ADC 原始值，换算并做容错处理
 *
 * @return  电池电压 mV（≥0）；HAL 读取失败时返回 BATTERY_READ_ERROR (-1)
 *
 * @details 执行流程：
 *          1. 调用 HAL_ADC_GetValue() 读取 ADC 原始值
 *          2. 检查返回值：若为 HAL_ADC_ERROR(0xFFFF)，说明硬件读取失败，
 *             返回 BATTERY_READ_ERROR，由上层决定降级策略
 *          3. 正常情况下调用 adc_to_mv() 换算为电池电压并返回
 *
 * @note    本函数依赖 HAL 接口，是 CMock 桩的主要拦截对象。
 *          单元测试可验证：
 *          - HAL 是否被调用（调用次数校验）
 *          - HAL 返回正常值时的换算结果
 *          - HAL 返回错误码时的容错行为
 */
int32_t read_battery_mv(void)
{
    /* 步骤1：调用 HAL 接口读取 ADC 原始值
     * 在 PC 测试环境中，此调用被 CMock 桩拦截，返回预设的模拟值 */
    uint16_t adc_raw = HAL_ADC_GetValue();

    /* 步骤2：容错处理 —— HAL 返回错误码时返回错误
     * HAL_ADC_ERROR = 0xFFFF，不在 12 位 ADC 有效值 0~4095 范围内 */
    if (adc_raw == HAL_ADC_ERROR) {
        return BATTERY_READ_ERROR;
    }

    /* 步骤3：正常换算并返回
     * adc_to_mv() 返回 uint16_t，这里提升为 int32_t 以统一返回类型 */
    return (int32_t)adc_to_mv(adc_raw);
}
