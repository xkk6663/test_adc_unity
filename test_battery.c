/**
 * @file    test_battery.c
 * @brief   电池电压采集模块单元测试用例（Unity 框架）
 *
 * @details 本文件包含 9 个测试用例，覆盖以下测试设计方法：
 *
 *  【等价类划分】
 *    - test_adc_to_mv_normal            正常 ADC 值换算（有效等价类）
 *    - test_adc_to_mv_below_max         低于上限的正常值不被误钳位
 *
 *  【边界值分析】
 *    - test_adc_to_mv_zero              ADC 最小值 0（下边界）
 *    - test_adc_to_mv_full_scale_clamp  ADC 最大值 4095（上边界，触发钳位）
 *    - test_adc_to_mv_clamp_critical    钳位临界点：刚好不触发 / 刚好触发
 *
 *  【错误推测】
 *    - test_adc_to_mv_overvoltage       超压输入被正确钳位（异常等价类）
 *
 *  【容错测试】
 *    - test_read_battery_mv_hal_error   HAL 返回错误码时的容错处理
 *
 *  【接口交互测试（CMock 桩校验）】
 *    - test_read_battery_mv_success     正常读取：HAL 调用 1 次，返回正确电压
 *    - test_read_battery_mv_multi_calls 连续读取：HAL 调用 2 次，各自返回对应值
 *    - test_read_battery_mv_ignore_mode Ignore 模式：不校验次数，固定返回
 *
 * @note    每个用例的 setUp() 初始化 mock，tearDown() 校验并销毁 mock，
 *          确保用例之间相互独立（测试隔离原则）。
 *          setUp/tearDown 统一定义在 test_support.c 中，
 *          管理所有 CMock 桩（mock_hal_adc + mock_hal_gpio）。
 */
#include "unity.h"
#include "mock_hal_adc.h"
#include "battery.h"

/* ==========================================================================
 * 等价类划分 —— 有效等价类
 * ========================================================================== */

/**
 * @brief 正常 ADC 值换算验证
 *
 * @details 输入 adc_raw=2048（12 位 ADC 的中间值）：
 *          测得电压 = 2048 × 3300 / 4095 = 1650 mV
 *          电池电压 = 1650 × 3 = 4950 mV
 *          4950 < 8400（上限），不触发钳位
 *
 * @test    验证正常输入下换算公式正确，且不触发钳位
 */
void test_adc_to_mv_normal(void)
{
    uint16_t result = adc_to_mv(2048);
    TEST_ASSERT_EQUAL_UINT16(4950, result);
}

/**
 * @brief 低于上限的正常值不被误钳位
 *
 * @details 输入 adc_raw=1000：
 *          测得电压 = 1000 × 3300 / 4095 = 805 mV（整数除法）
 *          电池电压 = 805 × 3 = 2415 mV
 *          2415 远低于 8400 上限，不应被钳位
 *
 * @test    验证钳位逻辑不会"误杀"正常范围内的值
 */
void test_adc_to_mv_below_max(void)
{
    uint16_t result = adc_to_mv(1000);
    TEST_ASSERT_EQUAL_UINT16(2415, result);
}

/* ==========================================================================
 * 边界值分析
 * ========================================================================== */

/**
 * @brief ADC 最小值边界：adc_raw = 0
 *
 * @details 输入 0 时，测得电压 = 0，电池电压 = 0，
 *          下钳位到 BATTERY_MIN_MV(0)，结果为 0。
 *
 * @test    验证下边界输入处理正确（0 是 ADC 的最小有效值）
 */
void test_adc_to_mv_zero(void)
{
    uint16_t result = adc_to_mv(0);
    TEST_ASSERT_EQUAL_UINT16(0, result);
}

/**
 * @brief ADC 最大值边界 + 上钳位：adc_raw = 4095
 *
 * @details 输入 4095（12 位 ADC 满量程）：
 *          测得电压 = 4095 × 3300 / 4095 = 3300 mV
 *          电池电压 = 3300 × 3 = 9900 mV
 *          9900 > 8400（上限），触发上钳位，结果应为 8400
 *
 * @test    验证上边界输入触发钳位，且钳位值正确。
 *          这是钳位逻辑的关键验证点：若 battery.c 中钳位条件
 *          写反（> 写成 <），本用例会返回 9900 而非 8400，测试失败。
 */
void test_adc_to_mv_full_scale_clamp(void)
{
    uint16_t result = adc_to_mv(4095);
    TEST_ASSERT_EQUAL_UINT16(8400, result);
}

/**
 * @brief 钳位临界点测试：刚好不触发 vs 刚好触发
 *
 * @details 计算临界点：电池电压 = adc_raw × 3300 / 4095 × 3
 *          令电池电压 ≤ 8400，解得 adc_raw ≤ 3475.5
 *
 *          - adc_raw = 3475：3475×3300/4095 = 2800, ×3 = 8400
 *            刚好等于上限，不触发钳位（自然结果就是 8400）
 *          - adc_raw = 3476：3476×3300/4095 = 2801, ×3 = 8403
 *            超过上限 3mV，触发钳位，结果应为 8400
 *
 * @test    验证钳位逻辑的临界精度：差 1 个 ADC 码值，
 *          一个自然到达边界，一个被钳位到边界，结果都正确。
 */
void test_adc_to_mv_clamp_critical(void)
{
    /* 刚好不触发钳位：自然结果 = 8400 */
    TEST_ASSERT_EQUAL_UINT16(8400, adc_to_mv(3475));

    /* 刚好触发钳位：自然结果 8403 > 8400，被钳位到 8400 */
    TEST_ASSERT_EQUAL_UINT16(8400, adc_to_mv(3476));
}

/* ==========================================================================
 * 错误推测 —— 异常等价类
 * ========================================================================== */

/**
 * @brief 超压输入被正确钳位
 *
 * @details 输入 adc_raw=4000（非正常高值，模拟采样异常或过压）：
 *          测得电压 = 4000 × 3300 / 4095 = 3226 mV
 *          电池电压 = 3226 × 3 = 9678 mV
 *          9678 > 8400，触发钳位，结果应为 8400
 *
 * @test    验证异常高输入下钳位防护生效，防止后续逻辑使用异常电压。
 *          对应真实场景：ADC 采样毛刺、分压电阻失效、电池过压。
 */
void test_adc_to_mv_overvoltage(void)
{
    uint16_t result = adc_to_mv(4000);
    TEST_ASSERT_EQUAL_UINT16(8400, result);
}

/**
 * @brief 非法 ADC 输入防护：adc_raw 超过 12 位满量程
 *
 * @details 输入 adc_raw=5000（超过 4095 的非法值，模拟寄存器读乱码、
 *          DMA 传输错误或未初始化的内存值）：
 *          adc_to_mv() 应检测到非法输入，返回 BATTERY_MIN_MV(0)，
 *          而不是将 5000 当作合法 ADC 值进行换算。
 *
 * @test    验证非法输入防护：超过硬件范围的输入被拦截，返回安全下限值。
 *          若 battery.c 中遗漏此检查，5000 会被换算：
 *          5000 × 3300 / 4095 = 4031 mV, ×3 = 12093 mV（uint16_t 溢出），
 *          结果完全错误，本用例会失败。
 */
void test_adc_to_mv_invalid_input(void)
{
    /* 超过 12 位满量程的非法值 */
    uint16_t result = adc_to_mv(5000);
    TEST_ASSERT_EQUAL_UINT16(BATTERY_MIN_MV, result);

    /* 另一个非法值：0xFFFF（HAL_ADC_ERROR，但直接传入换算函数时也应防护） */
    TEST_ASSERT_EQUAL_UINT16(BATTERY_MIN_MV, adc_to_mv(0xFFFF));
}

/* ==========================================================================
 * 接口交互测试（CMock 桩校验）
 * ========================================================================== */

/**
 * @brief 正常读取电池电压：HAL 被调用 1 次，返回正确电压
 *
 * @details 预设 HAL_ADC_GetValue() 返回 2048：
 *          read_battery_mv() 应调用 HAL 一次，换算得到 4950 mV。
 *          tearDown() 中的 Verify() 会校验 HAL 调用次数 = 1。
 *
 * @test    验证 read_battery_mv() 正确调用 HAL 接口（次数=1），
 *          且对 HAL 返回值的换算处理正确。
 */
void test_read_battery_mv_success(void)
{
    /* 预设：HAL 被调用一次，返回 2048 */
    HAL_ADC_GetValue_ExpectAndReturn(2048);

    /* 执行被测函数 */
    int32_t voltage = read_battery_mv();

    /* 校验返回值 */
    TEST_ASSERT_EQUAL_INT32(4950, voltage);
    /* tearDown 中的 Verify 会自动校验 HAL 调用次数 = 1 */
}

/**
 * @brief 连续读取两次：HAL 被调用 2 次，各自返回对应电压
 *
 * @details 预设两次 HAL 调用，分别返回 1000 和 2048：
 *          - 第一次 read_battery_mv() → 1000 → 2415 mV
 *          - 第二次 read_battery_mv() → 2048 → 4950 mV
 *          tearDown() 中的 Verify() 校验 HAL 调用次数 = 2。
 *
 * @test    验证多次读取时 HAL 调用顺序和次数正确，
 *          且每次返回值按预设顺序对应。
 */
void test_read_battery_mv_multi_calls(void)
{
    /* 预设两次调用，按顺序返回不同值 */
    HAL_ADC_GetValue_ExpectAndReturn(1000);
    HAL_ADC_GetValue_ExpectAndReturn(2048);

    /* 第一次读取 */
    int32_t v1 = read_battery_mv();
    TEST_ASSERT_EQUAL_INT32(2415, v1);

    /* 第二次读取 */
    int32_t v2 = read_battery_mv();
    TEST_ASSERT_EQUAL_INT32(4950, v2);

    /* tearDown 中的 Verify 会自动校验 HAL 调用次数 = 2 */
}

/**
 * @brief Ignore 模式：不校验 HAL 调用次数，固定返回值
 *
 * @details 使用 IgnoreAndReturn(2048) 后，HAL 被调用任意次都返回 2048，
 *          且 Verify() 跳过次数校验。连续调用 3 次 read_battery_mv()，
 *          每次都应返回 4950 mV（2048 换算结果）。
 *
 * @test    验证 Ignore 插件的工作方式：适用于"不关心底层调用次数，
 *          只关注上层业务逻辑"的测试场景。
 */
void test_read_battery_mv_ignore_mode(void)
{
    /* 进入 Ignore 模式：HAL 调用任意次都返回 2048，不校验次数 */
    HAL_ADC_GetValue_IgnoreAndReturn(2048);

    /* 连续调用 3 次，每次都应返回 4950 */
    TEST_ASSERT_EQUAL_INT32(4950, read_battery_mv());
    TEST_ASSERT_EQUAL_INT32(4950, read_battery_mv());
    TEST_ASSERT_EQUAL_INT32(4950, read_battery_mv());

    /* tearDown 中的 Verify 在 Ignore 模式下会跳过次数校验 */
}

/* ==========================================================================
 * 容错测试
 * ========================================================================== */

/**
 * @brief HAL 返回错误码时的容错处理
 *
 * @details 预设 HAL_ADC_GetValue() 返回 HAL_ADC_ERROR (0xFFFF)：
 *          read_battery_mv() 应检测到错误码，返回 BATTERY_READ_ERROR (-1)，
 *          而不是将 0xFFFF 当作合法 ADC 值进行换算。
 *
 * @test    验证容错逻辑：硬件读取失败时，函数返回明确的错误码，
 *          上层可据此采取降级策略（如使用上一次有效值、报警等）。
 *
 * @note    若 battery.c 中遗漏了 HAL_ADC_ERROR 检查，0xFFFF 会被当作
 *          ADC 值换算：65535 × 3300 / 4095 = 52800 mV（uint16_t 溢出），
 *          结果完全错误，本用例会失败。
 */
void test_read_battery_mv_hal_error(void)
{
    /* 预设 HAL 返回错误码 */
    HAL_ADC_GetValue_ExpectAndReturn(HAL_ADC_ERROR);

    /* 执行被测函数，应返回错误码而非换算结果 */
    int32_t result = read_battery_mv();
    TEST_ASSERT_EQUAL_INT32(BATTERY_READ_ERROR, result);

    /* tearDown 中的 Verify 会校验 HAL 调用次数 = 1 */
}

/* 注：main 函数和测试用例注册统一定义在 test_support.c 中，
 * 新增模块的测试用例只需在 test_support.c 的 main() 中添加 RUN_TEST()。 */
