/**
 * @file    firmware_main.c
 * @brief   固件模拟主程序（双目标编译中的"固件目标"）
 *
 * @details 本文件是 CMake/Make 双目标编译中的第二个目标：
 *          - 目标1（unit_test）：链接 CMock 桩 mock_hal_adc.c，在 PC 上跑单元测试
 *          - 目标2（firmware_demo）：链接本文件中的真实 HAL 模拟实现，
 *            编译成 PC 可执行文件，模拟固件在芯片上的运行效果
 *
 *          两个目标链接的是同一份 battery.c 业务代码，区别仅在于 HAL 实现：
 *          - 测试目标用 CMock 桩（可预设返回值、校验调用次数）
 *          - 固件目标用真实 HAL 模拟（固定返回一个 ADC 值，模拟硬件采样）
 *
 * @note    在真实 STM32 项目中，本文件对应 main.c，HAL 实现由 STM32 HAL 库提供；
 *          这里为了在 PC 上演示双目标编译效果，用一个固定返回值模拟 HAL 行为。
 */
#include <stdio.h>
#include "battery.h"
#include "hal_adc.h"

/* ==========================================================================
 * 真实 HAL 模拟实现（固件目标专用）
 * ========================================================================== */

/**
 * @brief 模拟 STM32 HAL_ADC_GetValue() 的真实实现
 *
 * @return  固定返回 ADC 原始值 2048
 *
 * @details 在真实固件中，此函数由 STM32 HAL 库实现，通过 DMA/中断
 *          从 ADC 外设寄存器读取采样值。这里为了在 PC 上演示，
 *          固定返回 2048（对应电池电压约 4.95V）。
 *
 * @note    此实现仅用于固件模拟目标；单元测试目标使用的是
 *          CMock 生成的桩函数 mock_hal_adc.c，二者不会同时链接。
 */
uint16_t HAL_ADC_GetValue(void)
{
    /* 模拟 ADC 采样到一个固定值 2048
     * 换算：2048 × 3300 / 4095 × 3 = 4950 mV ≈ 4.95V */
    return 2048;
}

/* ==========================================================================
 * 主函数
 * ========================================================================== */

/**
 * @brief 固件模拟主函数
 *
 * @details 模拟飞控上电后的电池电压采集流程：
 *          1. 调用 read_battery_mv() 读取电池电压
 *          2. 打印结果到串口（PC 上用 printf 模拟串口输出）
 *
 * @return  程序退出码（0 = 正常）
 */
int main(void)
{
    printf("=== Battery Voltage Monitor (Firmware Demo) ===\n");

    /* 读取电池电压 */
    int32_t voltage = read_battery_mv();

    /* 输出结果 */
    if (voltage == BATTERY_READ_ERROR) {
        printf("ERROR: Failed to read battery voltage (HAL error)\n");
        return 1;
    }

    printf("Battery voltage: %d mV (%.2f V)\n", (int)voltage, voltage / 1000.0);

    /* 电压状态判断（简单演示） */
    if (voltage >= 8400) {
        printf("Status: OVERVOLTAGE (>= 8.4V)\n");
    } else if (voltage >= 6000) {
        printf("Status: NORMAL (2S battery)\n");
    } else {
        printf("Status: LOW VOLTAGE (< 6.0V)\n");
    }

    return 0;
}
