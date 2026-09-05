/**
 * @file    led.c
 * @brief   LED 指示灯控制模块 —— 新增被测模块实现
 *
 * @details 本模块根据电池电压控制红/绿两个 LED，是演示"新增模块
 *          如何接入 Unity+CMock 单元测试体系"的完整示例。
 *
 *          测试要点：
 *          1. 纯逻辑验证：电压区间划分是否正确（边界值 6000、8400）
 *          2. 接口交互验证：HAL_GPIO_SetPin() 的调用次数、pin 参数、level 参数
 *          3. 状态验证：led_get_state() 返回值是否与输入电压匹配
 *
 * @note    本模块在双目标编译中的角色：
 *          - PC 测试目标：链接 mock_hal_gpio.c（CMock 桩替换 HAL）
 *          - 固件目标：  链接 firmware_main.c 中的真实 GPIO 模拟实现
 */
#include "led.h"
#include "hal_gpio.h"

/**
 * @brief 当前 LED 状态（模块内部静态变量）
 *
 * @note    静态变量在模块加载时初始化为 LED_STATE_OFF，
 *          每次调用 led_set_by_voltage() 后更新。
 *          单元测试中通过 led_get_state() 读取此值进行断言。
 */
static led_state_t current_state = LED_STATE_OFF;

/**
 * @brief 根据电池电压设置 LED 状态
 *
 * @param[in] battery_mv  电池电压（mV）
 *
 * @details 执行流程：
 *          1. 判断电压所在区间
 *          2. 调用 HAL_GPIO_SetPin() 设置红灯和绿灯的电平
 *          3. 更新内部状态 current_state
 *
 * @note    低压和过压都亮红灯，所以两个分支的 GPIO 操作相同，
 *          但分开写是为了清晰表达业务意图，也便于未来扩展
 *          （例如过压时闪烁红灯，低压时常亮红灯）。
 */
void led_set_by_voltage(uint16_t battery_mv)
{
    /* 分支1：低压 —— 电压低于 6000mV */
    if (battery_mv < LED_LOW_VOLTAGE_MV) {
        /* 红灯亮，绿灯灭 */
        HAL_GPIO_SetPin(GPIO_PIN_LED_RED,   GPIO_LEVEL_HIGH);
        HAL_GPIO_SetPin(GPIO_PIN_LED_GREEN, GPIO_LEVEL_LOW);
        current_state = LED_STATE_RED;
    }
    /* 分支2：过压 —— 电压高于 8400mV */
    else if (battery_mv > LED_MAX_VOLTAGE_MV) {
        /* 红灯亮，绿灯灭（与低压相同，但业务含义不同） */
        HAL_GPIO_SetPin(GPIO_PIN_LED_RED,   GPIO_LEVEL_HIGH);
        HAL_GPIO_SetPin(GPIO_PIN_LED_GREEN, GPIO_LEVEL_LOW);
        current_state = LED_STATE_RED;
    }
    /* 分支3：正常 —— 电压在 [6000, 8400] 范围内 */
    else {
        /* 绿灯亮，红灯灭 */
        HAL_GPIO_SetPin(GPIO_PIN_LED_RED,   GPIO_LEVEL_LOW);
        HAL_GPIO_SetPin(GPIO_PIN_LED_GREEN, GPIO_LEVEL_HIGH);
        current_state = LED_STATE_GREEN;
    }
}

/**
 * @brief 获取当前 LED 状态
 *
 * @return  当前 LED 状态（LED_STATE_OFF / RED / GREEN）
 *
 * @note    简单的 getter 函数，用于测试验证内部状态。
 *          不依赖任何硬件接口，可直接测试。
 */
led_state_t led_get_state(void)
{
    return current_state;
}
