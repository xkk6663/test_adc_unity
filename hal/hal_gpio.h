/**
 * @file    hal_gpio.h
 * @brief   HAL GPIO 接口声明 —— 被 led 模块依赖
 *
 * @details 本文件定义了 GPIO 操作的硬件抽象层接口。
 *          在真实固件中，这些函数由芯片厂商的 HAL 库实现；
 *          在 PC 单元测试中，由 CMock 生成的 mock_hal_gpio.c 替换，
 *          用于验证 led 模块是否正确调用了 GPIO 接口。
 *
 * @note    这是新增模块 led 依赖的 HAL 接口。与 hal_adc.h 类似，
 *          修改本文件后需重新运行 `ruby gen_mocks.rb` 更新桩。
 */
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>

/* ==========================================================================
 * GPIO 引脚枚举
 * ========================================================================== */

/**
 * @brief GPIO 引脚编号
 * @note  本工程只用到两个 LED 引脚，实际项目中会有更多
 */
typedef enum {
    GPIO_PIN_LED_RED   = 0,  ///< 红色 LED 引脚（低压/过压警告）
    GPIO_PIN_LED_GREEN = 1   ///< 绿色 LED 引脚（正常状态）
} gpio_pin_t;

/* ==========================================================================
 * GPIO 电平枚举
 * ========================================================================== */

/**
 * @brief GPIO 输出电平
 */
typedef enum {
    GPIO_LEVEL_LOW  = 0,  ///< 低电平（LED 灭，假设共阳接法）
    GPIO_LEVEL_HIGH = 1   ///< 高电平（LED 亮）
} gpio_level_t;

/* ==========================================================================
 * HAL 接口函数
 * ========================================================================== */

/**
 * @brief 设置 GPIO 引脚输出电平
 *
 * @param[in] pin    引脚编号（gpio_pin_t）
 * @param[in] level  输出电平（gpio_level_t）
 *
 * @note    这是 led 模块唯一依赖的硬件接口。
 *          单元测试中用 CMock 桩拦截此调用，验证：
 *          - 是否被调用（调用次数）
 *          - 传入的 pin 和 level 是否正确
 */
void HAL_GPIO_SetPin(gpio_pin_t pin, gpio_level_t level);

#endif /* HAL_GPIO_H */
