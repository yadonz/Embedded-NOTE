/*
 * @Author: 东东是个鬼 yadonz@foxmail.com
 * @Date: 2025-08-14 15:45:05
 * @LastEditors: yadonz yadonz@foxmail.com
 * @LastEditTime: 2025-08-15 18:38:30
 * @FilePath: \stm32f1xx_soft-iic-12864_demo\source\oled.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 *      这个库使用的是软件 iic 通信，
 *       1. 注意这里通信引脚并不是完全的可以随意配置，而是要求必须是属于同一组 GPIO 的引脚
 *          （比如：SCL 和 SDA 同时属于 GPIOA，或者同时属于 GPIOB 但是不能同时一个是 GPIOA 的引脚，一个是 
 *          GPIOB 的引脚。其实你也配置不了，接口中就一个 GPIO 端口配置选项）
 *       2.iic 的好处自然是对于通信引脚的选择自由度更大，虽然没有完全实现对通信引脚的任意配置，但是这样也够用。
 *       3.这里只是一个学习项目，用于后续学习的调试用，所有并没有进一步的优化，原谅我💩一样的代码风格
 *       4.除了 ascii 字体的显示位码部分全部手打，不限制使用，注明来源即可，虽然你不这么做我也没办法，举手之劳。
 */
#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>
#include <string.h>
#include "stdio.h"
#include "stdbool.h"
#include "stm32f10x.h"

// OLED 基础命令定义
#define OLED_IIC_ADDR        0x3C  // 默认 I²C 地址
#define OLED_CMD_MODE        0x00  // 发送命令
#define OLED_DATA_MODE       0x40  // 发送数据
 
// 行列
#define OLED_ROW_WIDTH  128
#define OLED_COL_WIDTH  8
#define OLED_PAGE_WIDTH 8

static void oled_write_cmd(uint8_t cmd);
static void oled_set_cursor(uint8_t Y, uint8_t X);
static void oled_show_char(uint8_t ascii);

/// @brief 软件 IIC 实现的 OLED 初始化函数
/// @param is_color_reverse 是否反转颜色 
/// @param T_iic_speed iic 的时钟速度的周期，最小值为 0 此时对应的 iic 速度大约 800KHz（示波器测量）
/// @param is_ack 配置是否响应应答位
/// @param iic_gpio_port 配置 iic 的端口（这里目前只支持同一个端口的 IO 配置）
/// @param iic_pin_scl 对应的 iic 时钟线引脚
/// @param iic_pin_sda 对应的 iic 数据线引脚
void OLED_Init(bool is_color_reverse, uint32_t T_iic_speed, bool is_ack, GPIO_TypeDef * iic_gpio_port, uint16_t iic_pin_scl, uint16_t iic_pin_sda);

/// @brief 对缓冲区所有内容进行清空的操作（这个效率比一个一个的设置像素的颜色的效率高得多）
/// @param  
void OLED_Clear(void);

/// @brief 将缓冲区的内容更新到 OLED 上去
/// @param  
void OLED_Update(void);

/// @brief 绘制坐标点
/// @param x 绘制的点的 x 坐标
/// @param y 绘制的点的 y 坐标
/// @param pixel 绘制的点的颜色（true/false，注意这里使用了 stdbool.h 库）
void OLED_SetPixel(uint8_t x, uint8_t y, bool pixel);

/// @brief 用于设置显示的字符的光标位置（想象成将要绘制的字符的左上角的点的位置）
/// @param x 光标位置的 x 坐标
/// @param y 光标位置的 y 坐标
void OLED_SetCharacterCursor(uint8_t x, uint8_t y);


#endif //__OLED_H
