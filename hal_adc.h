/**
 * @file    hal_adc.h
 * @brief   STM32 HAL 库 ADC 接口的简化声明（被测模块依赖的硬件抽象层）
 *
 * @details 在真实固件中，本文件由 STM32 HAL 库提供；在 PC 单元测试中，
 *          CMock 会根据本头文件自动生成桩函数 mock_hal_adc.c/.h，
 *          替换真实 HAL 实现，使业务逻辑能在 PC 上运行并被验证。
 *
 * @note    本头文件只声明接口，不包含实现；实现由以下二者之一提供：
 *          - 固件目标：firmware_main.c 中的真实 HAL 模拟实现
 *          - 测试目标：CMock 生成的 mock_hal_adc.c 桩函数
 */
#ifndef HAL_ADC_H
#define HAL_ADC_H

#include <stdint.h>

/**
 * @brief HAL ADC 读取失败时的错误码
 * @note  选用 0xFFFF 是因为 12 位 ADC 有效值范围为 0~4095，
 *        0xFFFF 不可能是合法采样值，可作为错误哨兵值（sentinel）。
 */
#define HAL_ADC_ERROR  0xFFFFu

/**
 * @brief 读取 ADC 原始采样值（STM32 HAL_ADC_GetValue 的简化模拟接口）
 *
 * @return  ADC 原始值，正常范围 0~4095；
 *          硬件读取失败时返回 HAL_ADC_ERROR (0xFFFF)。
 */
uint16_t HAL_ADC_GetValue(void);

#endif /* HAL_ADC_H */
