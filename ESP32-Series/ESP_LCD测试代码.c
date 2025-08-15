#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"


#define TAG "提示"

// 数据结构和定义
// 引脚定义
#define LCD_SCL     4 
#define LCD_SDA     5
#define LCD_DC      7
#define LCD_RESET   6
#define LCD_CS      15
#define LCD_BCK     16

// SPI 速度定义
#define SPI_CLOCK_SPEED (12*1000*1000)
// LCD 屏幕参数定义
#define LCD_WIDTH   240
#define LCD_HEIGHT  280
#define LCD_DISP_BUF_LENGTH (LCD_WIDTH*LCD_HEIGHT)
// LCD 初始化配置数据的结构体
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t length;
}LCD_init_params_Typedef;



// 数据
static spi_device_handle_t spi_device_handle;



static spi_bus_config_t spi_bus_config_structure = {
    .sclk_io_num = LCD_SCL,     // 配置 SPI 时钟脚
    .mosi_io_num = LCD_SDA,     // 配置 SPI 数据发送脚
    .miso_io_num = -1,          // 不配置 SPI 数据接收脚
    .flags = SPICOMMON_BUSFLAG_DUAL | SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MASTER,
    .intr_flags = 0,            // 不触发中断
    .max_transfer_sz = 4096,       // 使用 DMA
};



static spi_device_interface_config_t spi_device_config_structure = {
    .clock_source = SPI_CLK_SRC_DEFAULT,    // 配置时钟源
    .clock_speed_hz = SPI_CLOCK_SPEED,      // 配置时钟频率
    .duty_cycle_pos = 128,                  // 配置占空比 （范围 0 ~ 255）
    .flags = SPI_DEVICE_3WIRE,
    .mode = 3,                              // 配置 SPI 的相位和极性【这里相当重要，极性配置错误，会无法正确通信从而正确点亮 LCD】
    .spics_io_num = -1,                     // 不配置 CS 引脚
    .queue_size = 100,
};



static spi_transaction_t spi_transaction_structure = {
    .rxlength = 0,
    .rx_buffer = NULL,
    .tx_buffer = NULL,
};



static gpio_config_t gpio_config_structure = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = ((0x01ULL << LCD_DC) | (0x01ULL << LCD_RESET) | (0x01ULL << LCD_BCK) | (0x01ULL << LCD_CS)),
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .pull_up_en = GPIO_PULLUP_DISABLE
};


DRAM_ATTR static const LCD_init_params_Typedef LCD_init_params[] = {
    {0x01, {}, 0x80},
    {0x11, {}, 0x80},
    /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
    {0x36, {0x00}, 1},
    /* 反转颜色 */
    {0x21, {0x01}, 1},
    /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x05}, 1},
    /* Porch Setting */
    {0xB2, {0x0c, 0x0c, 0x00, 0x33}, 4},
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    {0xB7, {0x35}, 1},
    /* VCOM Setting, VCOM=1.175V */
    {0xBB, {0x19}, 1},
    /* LCM Control, XOR: BGR, MX, MH */
    {0xC0, {0x2C}, 1},
    /* VDV and VRH Command Enable, enable=1 */
    {0xC2, {0x01, 0xff}, 2},
    /* VRH Set, Vap=4.4+... */
    {0xC3, {0x13}, 1},
    /* VDV Set, VDV=0 */
    {0xC4, {0x20}, 1},
    /* Frame Rate Control, 60Hz, inversion=0 */
    {0xC6, {0x0f}, 1},
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    {0xD0, {0xA4, 0xA1}, 2},
    /* Positive Voltage Gamma Control */    
    {0xE0, {0xD0,0x04,0x0D,0x11,0x13,0x0F,0x2B,0x33,0x39,0x00,0x0C,0x0F,0x2E,0x33,0x39}, 15},
    /* Negative Voltage Gamma Control */
    {0xE1, {0xD0,0x04,0x0C,0x11,0x12,0x13,0x0F,0x24,0x32,0x37,0x00,0x0C,0x0F,0x24,0x32}, 15},

    {0x29, {}, 0x80},
    {0, {0}, 0xFF}      // length = 0xff 表示结束位置，不执行
};



// 屏幕显示缓冲区
static uint16_t * LCD_DISP_BUF;



void lcd_init(void);        // LCD 初始化
void lcd_fill_by_color16(uint8_t r, uint8_t g, uint8_t b);  // 填充纯色到显示器
void lcd_render(void);      // 渲染 LCD_DISP_BUF 中的数据到屏幕



/**************************************
 *
 *      main 函数
 *
 **************************************/
void app_main(void)
{
    lcd_init();
    // lcd_fill_by_color16(0xff, 0xff, 0xff);
    // while(1);
    LCD_DISP_BUF = heap_caps_malloc(LCD_DISP_BUF_LENGTH * 2, MALLOC_CAP_DMA);
    for (int i = 0; i < LCD_DISP_BUF_LENGTH; i ++) {
        // int r = i/240;
        // int c = i%240;
        // LCD_DISP_BUF[i] = 0x07e0;
        if (i / 10 % 3 == 0)
            LCD_DISP_BUF[i] = 0xF800;
        else if (i / 10 % 3 == 1)
            LCD_DISP_BUF[i] = 0x07E0;
        else if (i / 10 % 3 == 2)
            LCD_DISP_BUF[i] = 0x001F;
    }
    lcd_render();

    heap_caps_free(LCD_DISP_BUF);
}



/// @brief SPI 命令发送函数
/// @param cmd 
void lcd_send_cmd(const uint8_t cmd)
{
    gpio_set_level(LCD_DC, 0);
    spi_transaction_structure.tx_buffer = &cmd;
    spi_transaction_structure.length = 1 * 8;               // length 指的是 tx_buffer 中数据的位数，而不是字节数
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_device_handle, &spi_transaction_structure));
}



/// @brief SPI 数据流发送函数
/// @param buffer 
/// @param buffer_length 
void lcd_send_data(const uint8_t * buffer, int buffer_length)
{
    gpio_set_level(LCD_DC, 1);
    // 分块传输数据
    int index_mod = buffer_length%4096;
    int index_loop_end = buffer_length - index_mod;
    int i = 0;
    int cnt = 0;
    // uint8_t *p_buffer = buffer;
    for (i = 0; i < index_loop_end; i += 4096) {    // 循环发送部分
        cnt += 4096;
        spi_transaction_structure.tx_buffer = buffer + i;
        spi_transaction_structure.length = 8 * 4096;   // length 指的是 tx_buffer 中数据的位数，而不是字节数
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_device_handle, &spi_transaction_structure)); 
    }
    if (index_mod != 0) {   // 发送最后不足 4096 长度部分
        cnt += index_mod;
        spi_transaction_structure.tx_buffer = buffer + index_loop_end;
        spi_transaction_structure.length = 8 * index_mod;   // length 指的是 tx_buffer 中数据的位数，而不是字节数
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_device_handle, &spi_transaction_structure));
    }
    ESP_LOGI(TAG, "<--- %d --->", cnt);
}



/// @brief SPI 和 LCD 屏幕初始化函数
/// @param  
void lcd_init(void)
{
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &spi_bus_config_structure, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &spi_device_config_structure, &spi_device_handle));
    ESP_ERROR_CHECK(gpio_config(&gpio_config_structure));
    // 打开 LCD 背光
    gpio_set_level(LCD_BCK, 1);
    // LCD 复位
    gpio_set_level(LCD_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LCD_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LCD_RESET, 1);
    // 片选信号始终有效
    gpio_set_level(LCD_CS, 0);
    // 初始化屏幕参数

    for (int i = 0; LCD_init_params[i].length != 0xff; i ++) {
        lcd_send_cmd(LCD_init_params[i].cmd);
        if (LCD_init_params[i].length == 0x80) {
            vTaskDelay(pdMS_TO_TICKS(10));
        } else if (LCD_init_params[i].length != 0x00) {
            lcd_send_data(LCD_init_params[i].data, LCD_init_params[i].length);
        }
    }

}



/// @brief 屏幕缓冲区填充函数
/// @param r 
/// @param g 
/// @param b 
void lcd_fill_by_color16(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t color = ((((uint16_t)r) << 8) & 0xf800) | ((((uint16_t)g) << 3) & 0x07e0) | (((uint16_t)b) >> 3);  // 将 rgb 转换为 565 格式的 16 位二进制数
    for (int i = 0; i < LCD_DISP_BUF_LENGTH; i ++) {
        LCD_DISP_BUF[i] = 0xffff;
    }
}



/// @brief 
/// @param  
void lcd_render(void)
{
    uint16_t x_bias = 0;
    uint16_t y_bias = 20;

    uint8_t x_area[4] = {x_bias >> 8, x_bias & 0xff, (LCD_WIDTH + x_bias - 1) >> 8, (LCD_WIDTH + x_bias - 1) & 0xff};
    uint8_t y_area[4] = {y_bias >> 8, y_bias & 0xff, (LCD_HEIGHT + y_bias - 1) >> 8, (LCD_HEIGHT + y_bias - 1) & 0xff};
    lcd_send_cmd(0x2A);
    lcd_send_data(x_area, 4);
    lcd_send_cmd(0x2B);
    lcd_send_data(y_area, 4);
    lcd_send_cmd(0x2C);
    lcd_send_data((uint8_t *)LCD_DISP_BUF, LCD_DISP_BUF_LENGTH*2);
}

