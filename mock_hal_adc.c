/**
 * @file    mock_hal_adc.c
 * @brief   CMock 生成的 HAL ADC 桩函数实现（预生成等效版本）
 *
 * @details 本文件实现 mock_hal_adc.h 中声明的所有桩函数，核心机制：
 *          1. 用静态数组 g_return_array 存储预设的返回值序列
 *          2. 用 g_expected_count 记录"预期调用次数"（每次 ExpectAndReturn 自增）
 *          3. 用 g_call_count 记录"实际调用次数"（每次 HAL_ADC_GetValue 被调用自增）
 *          4. Verify() 时比较二者是否相等，不相等则报测试失败
 *          5. Ignore 模式下跳过次数校验，固定返回预设值
 *
 * @note    【预生成说明】
 *          本文件为 CMock 生成的等效预生成版本。真正的 CMock 生成代码会使用
 *          动态内存分配（malloc/realloc）和更复杂的参数校验结构体；本版本为了
 *          在没有 Ruby 的环境下直接编译，使用固定大小静态数组（128 个槽位，
 *          对单元测试绰绰有余），功能与 CMock 生成代码完全等价。
 *
 *          在安装了 Ruby 的环境中运行 `ruby gen_mocks.rb` 会重新生成
 *          动态内存版本的桩文件，覆盖本文件。
 */
#include "mock_hal_adc.h"

/* ==========================================================================
 * 桩内部状态（静态变量，仅本文件可见）
 * ========================================================================== */

/** @brief 预设返回值数组的最大容量（足够单元测试使用） */
#define MOCK_RETURN_ARRAY_SIZE  128

/** @brief 实际调用次数计数器 */
static uint16_t g_call_count = 0;

/** @brief 预期调用次数计数器（每次 ExpectAndReturn 自增） */
static uint16_t g_expected_count = 0;

/** @brief 存储预设返回值的数组，按调用顺序依次取出 */
static uint16_t g_return_array[MOCK_RETURN_ARRAY_SIZE];

/** @brief Ignore 模式标志：1=忽略次数校验，0=正常校验 */
static int g_ignore_mode = 0;

/** @brief Ignore 模式下的固定返回值 */
static uint16_t g_ignore_return = 0;

/* ==========================================================================
 * 桩生命周期管理
 * ========================================================================== */

/**
 * @brief 初始化桩状态（在每个测试用例的 setUp() 中调用）
 *
 * @details 清零所有计数器和标志，确保每个测试用例从干净状态开始，
 *          避免前一个用例的预设值泄漏到后一个用例。
 */
void mock_hal_adc_Init(void)
{
    g_call_count = 0;
    g_expected_count = 0;
    g_ignore_mode = 0;
    g_ignore_return = 0;
}

/**
 * @brief 销毁桩状态（在每个测试用例的 tearDown() 中调用）
 *
 * @note 本预生成版本使用静态数组，无需动态内存释放；
 *       保留此接口以兼容 CMock 生成代码的标准调用流程。
 */
void mock_hal_adc_Destroy(void)
{
    /* 静态数组无需释放 */
}

/**
 * @brief 校验调用次数（在每个测试用例的 tearDown() 中调用）
 *
 * @details 比较"预期调用次数"与"实际调用次数"：
 *          - 若相等：测试通过
 *          - 若实际 < 预期：说明被测代码没有按预设调用 HAL（漏调用）
 *          - 若实际 > 预期：说明被测代码调用 HAL 次数超出预期（多调用）
 *
 * @note Ignore 模式下跳过校验，因为该模式明确表示不关心调用次数。
 */
void mock_hal_adc_Verify(void)
{
    /* Ignore 模式下不校验调用次数 */
    if (g_ignore_mode) {
        return;
    }

    /* 预期次数与实际次数必须一致 */
    if (g_expected_count != g_call_count) {
        TEST_FAIL_MESSAGE("HAL_ADC_GetValue: expected call count does not match actual call count");
    }
}

/* ==========================================================================
 * expect 插件实现
 * ========================================================================== */

/**
 * @brief 预设一次 HAL_ADC_GetValue() 调用，并指定返回值
 *
 * @param[in] to_return  预设的返回值
 *
 * @details 将返回值存入数组的下一个槽位，并增加预期调用次数。
 *          当被测代码实际调用 HAL_ADC_GetValue() 时，按存入顺序
 *          依次取出返回值。
 *
 * @note 若预设次数超过数组容量（128），后续返回值会被丢弃，
 *       但预期计数器仍会自增，Verify 时会因次数不匹配而失败。
 *       正常单元测试不会超过 128 次预设。
 */
void HAL_ADC_GetValue_ExpectAndReturn(uint16_t to_return)
{
    if (g_expected_count < MOCK_RETURN_ARRAY_SIZE) {
        g_return_array[g_expected_count] = to_return;
    }
    g_expected_count++;
}

/**
 * @brief 预设一次调用（不校验参数），并指定返回值
 *
 * @param[in] to_return  预设的返回值
 *
 * @note 对于无参数函数 HAL_ADC_GetValue(void)，本函数与
 *       ExpectAndReturn 行为完全相同；保留此接口是为了兼容
 *       CMock 生成代码的完整 API（expect_any_args 插件）。
 */
void HAL_ADC_GetValue_ExpectAnyArgsAndReturn(uint16_t to_return)
{
    HAL_ADC_GetValue_ExpectAndReturn(to_return);
}

/* ==========================================================================
 * ignore 插件实现
 * ========================================================================== */

/**
 * @brief 进入 Ignore 模式：不校验调用次数，每次调用固定返回指定值
 *
 * @param[in] to_return  固定返回值
 *
 * @details 调用本函数后：
 *          - g_ignore_mode 置 1，HAL_ADC_GetValue() 被调用任意次都返回 to_return
 *          - Verify() 跳过次数校验
 *          - 之前通过 ExpectAndReturn 设置的预设值不再生效
 *
 * @note 适用于"不关心 HAL 被调用多少次，只关心上层业务逻辑"的测试场景。
 */
void HAL_ADC_GetValue_IgnoreAndReturn(uint16_t to_return)
{
    g_ignore_mode = 1;
    g_ignore_return = to_return;
}

/**
 * @brief 退出 Ignore 模式，恢复 Expect 调用次数校验
 *
 * @note 退出后，后续调用 HAL_ADC_GetValue() 会重新按 Expect 预设
 *       的顺序返回值，Verify 也会恢复次数校验。
 */
void HAL_ADC_GetValue_StopIgnore(void)
{
    g_ignore_mode = 0;
}

/* ==========================================================================
 * 被 mock 的实际函数（替换真实 HAL 实现）
 * ========================================================================== */

/**
 * @brief 桩函数：替换真实 HAL_ADC_GetValue()
 *
 * @return  预设的返回值（Expect 模式按顺序取，Ignore 模式固定返回）
 *
 * @details 执行逻辑：
 *          1. 若处于 Ignore 模式，直接返回固定值 g_ignore_return
 *          2. 若实际调用次数已达到预期次数，说明被测代码多调用了 HAL，
 *             立即报测试失败（"called more times than expected"）
 *          3. 否则按顺序从 g_return_array 取出预设返回值，调用次数自增，返回
 *
 * @note 此函数由被测代码 read_battery_mv() 间接调用；测试代码不应直接调用。
 *       失败信息会指向当前测试执行位置，帮助定位是哪次调用超出预期。
 */
uint16_t HAL_ADC_GetValue(void)
{
    /* Ignore 模式：固定返回，不计数 */
    if (g_ignore_mode) {
        return g_ignore_return;
    }

    /* 实际调用次数超过预期次数：被测代码多调用了 HAL */
    if (g_call_count >= g_expected_count) {
        TEST_FAIL_MESSAGE("HAL_ADC_GetValue: called more times than expected");
        return 0;  /* 不会执行到这里，TEST_FAIL_MESSAGE 会 longjmp 跳出 */
    }

    /* 按预设顺序取出返回值，调用次数自增 */
    uint16_t ret = g_return_array[g_call_count];
    g_call_count++;
    return ret;
}
