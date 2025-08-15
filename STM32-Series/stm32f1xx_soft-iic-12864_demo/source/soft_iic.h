/*
 * @Author: yadonz yadonz@foxmail.com
 * @Date: 2025-08-14 15:45:05
 * @LastEditors: yadonz yadonz@foxmail.com
 * @LastEditTime: 2025-08-15 15:56:12
 * @FilePath: \stm32f1xx_soft-iic-12864_demo\source\soft_iic.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __SOFT_IIC_H
#define __SOFT_IIC_H

#include "stm32f10x.h"
#include "stdlib.h"
#include "stdbool.h"

typedef struct
{
    uint8_t data[100];   // 缓冲区分配的空间大小
    int32_t len;       // 缓冲区数据实际占用的大小
    uint8_t lock;       // 数据的锁，0 表示未锁定，1 表示 数据锁定，
}DataType;



// ----------------------- 位带操作来加速 iic -----------------------
#define BIT_BAND_ODR(GPIO_PORT, GPIO_PIN) (*((uint32_t *) (PERIPH_BB_BASE + (((uint32_t)&(GPIO_PORT)->ODR) - PERIPH_BASE) * 32 + (GPIO_PIN) * 4)))
#define BIT_BAND_IDR(GPIO_PORT, GPIO_PIN) (*((uint32_t *) (PERIPH_BB_BASE + (((uint32_t)&(GPIO_PORT)->ODR) - PERIPH_BASE) * 32 + (GPIO_PIN) * 4)))


// 私有函数
static void siic_trans_start(void);                         // 传输开始电平信号
static void siic_trans_end(void);                           // 传输结束电平信号
static void siic_trans_bit_high(void);                      // 传送一比特高电平数据位
static void siic_trans_bit_low(void);                       // 传送一比特低电平数据位
static uint8_t siic_trans_wait_ack(void);                   // 接收一位应答位
static uint8_t siic_trans_write_byte(char byte);            // 发送用于拼接的 byte 的时序
uint8_t SoftIIC_Receive_current(uint8_t addr, DataType * receive_buffer, int32_t receive_len);
static uint8_t siic_trans_read_byte(uint8_t * ret);         // 从从机读取一个比特
static void siic_trans_scl_write_bit(bool bit);             // 往 SCL ODR 写电平
static void siic_trans_sda_write_bit(bool bit);             // 往 SDA ODR 写电平
static bool siic_trans_sda_read_bit(void);                  // 读取 SDA IDR 电平

void SoftIIC_Test(int x);                   // 用于测试的一段时序

// 对外接口
void SoftIIC_Init(uint32_t T_iic_speed, bool is_ack, GPIO_TypeDef * iic_gpio_port, uint16_t iic_pin_scl, uint16_t iic_pin_sda);
uint8_t SoftIIC_Write(uint8_t addr, uint8_t reg, uint8_t * bytes, int32_t bytes_nums);
uint8_t SoftIIC_Receive_current(uint8_t addr, DataType* receive_buffer, int32_t receive_len);
uint8_t SoftIIC_Receive_reg(uint8_t addr, uint8_t reg, DataType* receive_buffer, int32_t receive_len);






#endif //__SOFT_IIC_H

