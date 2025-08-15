/*
 * @Author: yadonz yadonz@foxmail.com
 * @Date: 2025-08-14 15:45:05
 * @LastEditors: yadonz yadonz@foxmail.com
 * @LastEditTime: 2025-08-15 17:10:25
 * @FilePath: \stm32f1xx_soft-iic-12864_demo\source\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * ************************************************
 * 
 *              STM32 blink gcc demo
 * 
 *  CPU: STM32F103C8
 *  PIN: PA1
 * 
 * ************************************************
*/

#include "stm32f10x.h"
#include "stdlib.h"
#include "oled.h"
#include "stdio.h"

int main()
{
    // 初始化 GPIO 端口，速度和是否应答（为了简化代码，SCL 和 SDA 只能使用同一个端口）
    OLED_Init(true, 0, false, GPIOB, 6, 7);

    // OLED_SetCharacterCursor(0, 0);  // 设置显示光标的位置，该位置为待显示的字符（8x8）的右上角（注意，可以指定任意位置显示，可以无视分页，反正分页也无法提高性能）
    // printf("Hello world!\n");       // 换行符默认换行到下一行的行首（没有实现换行符之外的其它控制字符）
    // OLED_Update();                  // 更新缓冲区


    // for (int i = 33; i < 128; i ++)
    // {
    //     printf((char*)&i);
    //     OLED_Update();
    //     // for (int i = 0; i < 100; i ++)
    //     // for (int j = 0; j < 1000; j ++);
    // }

    int i = 0, j = 0;
    while (true)
    {
        OLED_Clear();
        OLED_SetCharacterCursor(i ++, j ++);
        printf("test");
        OLED_Update();
        i ++;
        if (i >= 128)
        {
            i = 0;
            j = 0;
        }
    }


    // while (1)
    // {
    //     for (int i = 0; i < 128; i ++)
    //     { // 绘制马赛克
    //         for (int j = 0; j < 64; j ++)
    //         {
    //             if ((i/x + j/x) & 0x01)
    //                 OLED_SetPixel(i, j, 0);
    //             else
    //                 OLED_SetPixel(i, j, 1);
    //         }
    //     }
    //     OLED_Update();
    //     x ++;
    //     if (x >= 129)
    //     {
    //         x = 1;
    //     }
    // }
}
