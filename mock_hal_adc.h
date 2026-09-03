/**
 * @file    mock_hal_adc.h
 * @brief   CMock 生成的 HAL ADC 桩函数头文件（预生成等效版本）
 *
 * @details 本文件由 CMock 根据 hal_adc.h 自动生成，提供以下能力：
 *          1. 替换真实 HAL_ADC_GetValue() 实现，使业务代码在 PC 上运行
 *          2. 预设返回值：ExpectAndReturn / IgnoreAndReturn
 *          3. 校验调用次数：Verify() 检查实际调用次数是否等于预期次数
 *
 * @note    【预生成说明】
 *          本文件为 CMock 生成的等效预生成版本，已检入仓库以便在没有
 *          Ruby 环境的机器上直接编译运行。在安装了 Ruby 的环境中，可通过
 *          以下命令重新生成（会覆盖本文件）：
 *              ruby gen_mocks.rb
 *
 *          CMock 配置见 CMockConfig.yml（启用 expect / ignore 插件，
 *          mock_prefix = "mock_"）。
 *
 * @warning 请勿手动修改本文件中的桩函数逻辑；如需修改接口，请修改
 *          hal_adc.h 后重新运行 gen_mocks.rb。
 */
#ifndef MOCK_HAL_ADC_H
#define MOCK_HAL_ADC_H

#include "unity.h"
#include "hal_adc.h"

/* ==========================================================================
 * CMock 桩生命周期管理
 * ========================================================================== */

/**
 * @brief 初始化 mock 状态（清零所有调用计数和期望）
 * @note  在每个测试用例的 setUp() 中调用
 */
void mock_hal_adc_Init(void);

/**
 * @brief 销毁 mock 状态（释放动态分配的资源）
 * @note  在每个测试用例的 tearDown() 中调用；本预生成版本使用静态
 *        数组，无需释放，但保留接口以兼容 CMock 生成代码的调用方式。
 */
void mock_hal_adc_Destroy(void);

/**
 * @brief 校验所有函数的实际调用次数是否等于预期次数
 * @note  在每个测试用例的 tearDown() 中调用；若次数不匹配，
 *        会通过 Unity 报出测试失败。
 */
void mock_hal_adc_Verify(void);

/* ==========================================================================
 * expect 插件 —— 精确预设调用顺序与返回值
 * ========================================================================== */

/**
 * @brief 预设一次 HAL_ADC_GetValue() 调用，并指定返回值
 * @param[in] to_return  预设的返回值
 *
 * @details 每次调用本函数会增加一次"预期调用次数"。
 *          当被测代码实际调用 HAL_ADC_GetValue() 时，按预设顺序
 *          依次返回对应的值。若实际调用次数超过预设次数，Verify()
 *          会报失败。
 *
 * @example 预设两次调用，分别返回 1000 和 2000：
 *          HAL_ADC_GetValue_ExpectAndReturn(1000);
 *          HAL_ADC_GetValue_ExpectAndReturn(2000);
 *          // 被测代码第一次调用返回 1000，第二次返回 2000
 */
void HAL_ADC_GetValue_ExpectAndReturn(uint16_t to_return);

/**
 * @brief 预设一次调用（不校验参数），并指定返回值
 * @param[in] to_return  预设的返回值
 * @note  对于无参数函数，本函数与 ExpectAndReturn 行为相同；
 *        保留此接口以兼容 CMock 生成代码的完整 API。
 */
void HAL_ADC_GetValue_ExpectAnyArgsAndReturn(uint16_t to_return);

/* ==========================================================================
 * ignore 插件 —— 忽略调用次数，固定返回值
 * ========================================================================== */

/**
 * @brief 忽略 HAL_ADC_GetValue() 的调用次数校验，每次调用固定返回指定值
 * @param[in] to_return  固定返回值
 *
 * @details 调用本函数后，HAL_ADC_GetValue() 被调用任意次都返回 to_return，
 *          且不参与调用次数校验。适用于"不关心 HAL 被调用多少次，只关心
 *          上层逻辑"的场景。
 *
 * @note    与 ExpectAndReturn 互斥：设置 Ignore 后，Expect 的预设不再生效。
 */
void HAL_ADC_GetValue_IgnoreAndReturn(uint16_t to_return);

/**
 * @brief 停止 Ignore 模式，恢复 Expect 调用次数校验
 */
void HAL_ADC_GetValue_StopIgnore(void);

/* ==========================================================================
 * 被 mock 的实际函数（替换真实 HAL 实现）
 * ========================================================================== */

/**
 * @brief 桩函数：替换真实 HAL_ADC_GetValue()
 * @return  按预设顺序返回的值，或 Ignore 模式下的固定值
 *
 * @note  此函数由 CMock 生成，被测代码调用 HAL_ADC_GetValue() 时
 *        实际执行的是本桩函数。不要在测试代码中直接调用本函数。
 */
uint16_t HAL_ADC_GetValue(void);

#endif /* MOCK_HAL_ADC_H */
