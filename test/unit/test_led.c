/**
 * @file    test_led.c
 * @brief   LED 指示灯模块单元测试用例（Unity 框架）
 *
 * @details 本文件包含 8 个测试用例，覆盖以下测试设计方法：
 *
 *  【等价类划分】
 *    - test_led_low_voltage       低压区间（<6000mV）→ 红灯亮
 *    - test_led_normal_voltage    正常区间（6000~8400mV）→ 绿灯亮
 *    - test_led_overvoltage       过压区间（>8400mV）→ 红灯亮
 *
 *  【边界值分析】
 *    - test_led_boundary_5999     低压上限：5999mV → 红灯
 *    - test_led_boundary_6000     正常下限：6000mV → 绿灯
 *    - test_led_boundary_8400     正常上限：8400mV → 绿灯
 *    - test_led_boundary_8401     过压下限：8401mV → 红灯
 *
 *  【状态切换测试】
 *    - test_led_state_transition  从正常切到低压，验证状态和 GPIO 操作正确更新
 *
 * @note    setUp/tearDown 统一定义在 test_support.c 中，
 *          本文件只需专注测试用例本身。
 *
 * @note    CMock 桩用法说明：
 *          - HAL_GPIO_SetPin_Expect(pin, level) 预设一次调用的参数
 *          - 因为 HAL_GPIO_SetPin 返回 void，所以用 _Expect 而非 _ExpectAndReturn
 *          - 每次 led_set_by_voltage() 内部调用 2 次 HAL_GPIO_SetPin（红灯+绿灯），
 *            所以每个用例需要预设 2 次 Expect
 *          - tearDown 中的 mock_hal_gpio_Verify() 会校验调用次数和参数
 */
#include "unity.h"
#include "mock_hal_gpio.h"
#include "led.h"

/* ==========================================================================
 * 等价类划分 —— 三个电压区间
 * ========================================================================== */

/**
 * @brief 低压区间：电压 < 6000mV → 红灯亮，绿灯灭
 *
 * @details 输入 5000mV：
 *          - 5000 < 6000，进入低压分支
 *          - 红灯引脚设为 HIGH，绿灯引脚设为 LOW
 *          - 内部状态更新为 LED_STATE_RED
 *
 * @test    验证低压区间的 GPIO 操作和状态更新
 */
void test_led_low_voltage(void)
{
    /* 预设 2 次 GPIO 调用：红灯 HIGH，绿灯 LOW */
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_RED,   GPIO_LEVEL_HIGH);
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_GREEN, GPIO_LEVEL_LOW);

    /* 调用被测函数 */
    led_set_by_voltage(5000);

    /* 验证内部状态 */
    TEST_ASSERT_EQUAL_INT(LED_STATE_RED, led_get_state());
}

/**
 * @brief 正常区间：6000mV ≤ 电压 ≤ 8400mV → 绿灯亮，红灯灭
 *
 * @details 输入 7000mV：
 *          - 6000 ≤ 7000 ≤ 8400，进入正常分支
 *          - 红灯引脚设为 LOW，绿灯引脚设为 HIGH
 *          - 内部状态更新为 LED_STATE_GREEN
 *
 * @test    验证正常区间的 GPIO 操作和状态更新
 */
void test_led_normal_voltage(void)
{
    /* 预设 2 次 GPIO 调用：红灯 LOW，绿灯 HIGH */
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_RED,   GPIO_LEVEL_LOW);
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_GREEN, GPIO_LEVEL_HIGH);

    led_set_by_voltage(7000);

    TEST_ASSERT_EQUAL_INT(LED_STATE_GREEN, led_get_state());
}

/**
 * @brief 过压区间：电压 > 8400mV → 红灯亮，绿灯灭
 *
 * @details 输入 9000mV：
 *          - 9000 > 8400，进入过压分支
 *          - 红灯引脚设为 HIGH，绿灯引脚设为 LOW
 *          - 内部状态更新为 LED_STATE_RED
 *
 * @test    验证过压区间的 GPIO 操作和状态更新
 * @note    过压和低压的 GPIO 操作相同，但业务含义不同，
 *          分开测试确保两个分支都被覆盖。
 */
void test_led_overvoltage(void)
{
    /* 预设 2 次 GPIO 调用：红灯 HIGH，绿灯 LOW */
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_RED,   GPIO_LEVEL_HIGH);
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_GREEN, GPIO_LEVEL_LOW);

    led_set_by_voltage(9000);

    TEST_ASSERT_EQUAL_INT(LED_STATE_RED, led_get_state());
}

/* ==========================================================================
 * 边界值分析 —— 区间临界点
 * ========================================================================== */

/**
 * @brief 边界：5999mV（低压上限，刚好不进入正常区间）→ 红灯
 *
 * @test    验证 < 6000 的最后一个值仍属于低压区间
 */
void test_led_boundary_5999(void)
{
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_RED,   GPIO_LEVEL_HIGH);
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_GREEN, GPIO_LEVEL_LOW);

    led_set_by_voltage(5999);

    TEST_ASSERT_EQUAL_INT(LED_STATE_RED, led_get_state());
}

/**
 * @brief 边界：6000mV（正常下限，刚好进入正常区间）→ 绿灯
 *
 * @test    验证 ≥ 6000 的第一个值属于正常区间
 * @note    这是最容易出错的边界：如果条件写成 <= 而非 <，
 *          6000mV 会被误判为低压，此用例会立刻失败。
 */
void test_led_boundary_6000(void)
{
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_RED,   GPIO_LEVEL_LOW);
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_GREEN, GPIO_LEVEL_HIGH);

    led_set_by_voltage(6000);

    TEST_ASSERT_EQUAL_INT(LED_STATE_GREEN, led_get_state());
}

/**
 * @brief 边界：8400mV（正常上限，刚好不进入过压区间）→ 绿灯
 *
 * @test    验证 ≤ 8400 的最后一个值仍属于正常区间
 */
void test_led_boundary_8400(void)
{
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_RED,   GPIO_LEVEL_LOW);
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_GREEN, GPIO_LEVEL_HIGH);

    led_set_by_voltage(8400);

    TEST_ASSERT_EQUAL_INT(LED_STATE_GREEN, led_get_state());
}

/**
 * @brief 边界：8401mV（过压下限，刚好进入过压区间）→ 红灯
 *
 * @test    验证 > 8400 的第一个值属于过压区间
 * @note    这是另一个容易出错的边界：如果条件写成 >= 而非 >，
 *          8400mV 会被误判为过压，test_led_boundary_8400 会失败。
 */
void test_led_boundary_8401(void)
{
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_RED,   GPIO_LEVEL_HIGH);
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_GREEN, GPIO_LEVEL_LOW);

    led_set_by_voltage(8401);

    TEST_ASSERT_EQUAL_INT(LED_STATE_RED, led_get_state());
}

/* ==========================================================================
 * 状态切换测试
 * ========================================================================== */

/**
 * @brief 状态切换：从正常切到低压，验证状态和 GPIO 操作正确更新
 *
 * @details 连续调用两次 led_set_by_voltage：
 *          1. 先设为 7000mV（正常）→ 绿灯亮，状态 GREEN
 *          2. 再设为 5000mV（低压）→ 红灯亮，状态 RED
 *
 *          总共预设 4 次 GPIO 调用（每次 2 次），
 *          CMock 按顺序校验每次调用的参数。
 *
 * @test    验证连续调用时状态正确切换，GPIO 操作按顺序执行
 * @note    这个用例验证了模块的"有状态"行为——current_state 静态变量
 *          在多次调用间正确更新。
 */
void test_led_state_transition(void)
{
    /* 第一次调用：正常电压 → 红灯 LOW，绿灯 HIGH */
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_RED,   GPIO_LEVEL_LOW);
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_GREEN, GPIO_LEVEL_HIGH);
    led_set_by_voltage(7000);
    TEST_ASSERT_EQUAL_INT(LED_STATE_GREEN, led_get_state());

    /* 第二次调用：低压 → 红灯 HIGH，绿灯 LOW */
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_RED,   GPIO_LEVEL_HIGH);
    HAL_GPIO_SetPin_Expect(GPIO_PIN_LED_GREEN, GPIO_LEVEL_LOW);
    led_set_by_voltage(5000);
    TEST_ASSERT_EQUAL_INT(LED_STATE_RED, led_get_state());
}
