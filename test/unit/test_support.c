/**
 * @file    test_support.c
 * @brief   统一的测试固件（setUp/tearDown）—— 管理所有 CMock 桩
 *
 * @details Unity 框架要求 setUp() 和 tearDown() 是全局函数，
 *          每个测试用例执行前自动调用 setUp()，执行后自动调用 tearDown()。
 *
 *          当工程中有多个测试文件（test_battery.c、test_led.c 等）时，
 *          setUp/tearDown 只能定义一次。本文件集中管理所有 CMock 桩的
 *          初始化和销毁，确保：
 *          - 每个用例从干净的桩状态开始（测试隔离）
 *          - 每个用例结束后校验所有桩的调用次数
 *
 *          新增模块时：
 *          1. 在 setUp() 中添加 mock_xxx_Init()
 *          2. 在 tearDown() 中添加 mock_xxx_Verify() 和 mock_xxx_Destroy()
 *          3. 在 CMakeLists.txt 的源文件列表中添加新测试文件
 *          4. CI 会自动编译运行所有测试，无需修改 CI 配置
 */
#include "unity.h"
#include "mock_hal_adc.h"
#include "mock_hal_gpio.h"

/* ==========================================================================
 * 外部测试函数声明（定义在各 test_*.c 中）
 * ==========================================================================
 * C 语言中，跨文件调用函数需要先声明。此处声明所有测试函数，
 * 供下方 main() 中的 RUN_TEST() 宏使用。
 *
 * 新增模块时，在此处添加对应测试函数的 extern 声明，
 * 并在下方 main() 中添加 RUN_TEST()。
 */

/* battery 模块（test_battery.c） */
extern void test_adc_to_mv_normal(void);
extern void test_adc_to_mv_below_max(void);
extern void test_adc_to_mv_zero(void);
extern void test_adc_to_mv_full_scale_clamp(void);
extern void test_adc_to_mv_clamp_critical(void);
extern void test_adc_to_mv_overvoltage(void);
extern void test_adc_to_mv_invalid_input(void);
extern void test_read_battery_mv_success(void);
extern void test_read_battery_mv_multi_calls(void);
extern void test_read_battery_mv_ignore_mode(void);
extern void test_read_battery_mv_hal_error(void);

/* led 模块（test_led.c） */
extern void test_led_low_voltage(void);
extern void test_led_normal_voltage(void);
extern void test_led_overvoltage(void);
extern void test_led_boundary_5999(void);
extern void test_led_boundary_6000(void);
extern void test_led_boundary_8400(void);
extern void test_led_boundary_8401(void);
extern void test_led_state_transition(void);

/**
 * @brief 每个测试用例执行前的初始化
 *
 * @note  初始化所有 CMock 桩状态。即使某个用例不用到某个桩，
 *        初始化它也没有副作用（Verify 时会检查 0 次调用）。
 *        这样做的好处是 setUp/tearDown 统一管理，新增模块时
 *        只需在此处添加一行，不需要修改各个测试文件。
 */
void setUp(void)
{
    mock_hal_adc_Init();   /* ADC 桩（battery 模块用） */
    mock_hal_gpio_Init();  /* GPIO 桩（led 模块用） */
}

/**
 * @brief 每个测试用例执行后的校验与清理
 *
 * @note  校验所有桩的调用次数是否符合预期，然后销毁桩状态。
 *        顺序：先 Verify（校验），后 Destroy（清理）。
 *        如果某个用例没有 Expect 某个函数，但被测代码调用了它，
 *        Verify 会报"调用次数不匹配"失败——这正是我们想要的，
 *        确保测试用例明确声明了所有外部依赖调用。
 */
void tearDown(void)
{
    mock_hal_adc_Verify();
    mock_hal_adc_Destroy();

    mock_hal_gpio_Verify();
    mock_hal_gpio_Destroy();
}

/* ==========================================================================
 * 测试入口（main 函数）
 * ========================================================================== */

/**
 * @brief 单元测试主函数 —— 注册所有测试用例
 *
 * @details Unity 框架的标准入口模式：
 *          UNITY_BEGIN() 初始化测试统计 → RUN_TEST() 逐个执行用例 →
 *          UNITY_END() 输出汇总结果并返回失败数。
 *
 *          所有模块的测试用例都在此处注册。新增模块时：
 *          1. 编写 test_xxx.c（含 test_xxx 函数）
 *          2. 在此处添加 RUN_TEST(test_xxx)
 *          3. 在 Makefile/CMakeLists.txt 的源文件列表中添加 test_xxx.c
 *          4. CI 会自动编译运行，无需修改 CI 配置
 *
 * @return  失败的测试用例数（0 = 全部通过）
 */
int main(void)
{
    UNITY_BEGIN();

    /* ===== battery 模块测试（11 个） ===== */
    /* 等价类划分 */
    RUN_TEST(test_adc_to_mv_normal);
    RUN_TEST(test_adc_to_mv_below_max);
    /* 边界值分析 */
    RUN_TEST(test_adc_to_mv_zero);
    RUN_TEST(test_adc_to_mv_full_scale_clamp);
    RUN_TEST(test_adc_to_mv_clamp_critical);
    /* 错误推测 */
    RUN_TEST(test_adc_to_mv_overvoltage);
    RUN_TEST(test_adc_to_mv_invalid_input);
    /* 接口交互测试 */
    RUN_TEST(test_read_battery_mv_success);
    RUN_TEST(test_read_battery_mv_multi_calls);
    RUN_TEST(test_read_battery_mv_ignore_mode);
    /* 容错测试 */
    RUN_TEST(test_read_battery_mv_hal_error);

    /* ===== led 模块测试（8 个） ===== */
    /* 等价类划分 */
    RUN_TEST(test_led_low_voltage);
    RUN_TEST(test_led_normal_voltage);
    RUN_TEST(test_led_overvoltage);
    /* 边界值分析 */
    RUN_TEST(test_led_boundary_5999);
    RUN_TEST(test_led_boundary_6000);
    RUN_TEST(test_led_boundary_8400);
    RUN_TEST(test_led_boundary_8401);
    /* 状态切换测试 */
    RUN_TEST(test_led_state_transition);

    return UNITY_END();
}
