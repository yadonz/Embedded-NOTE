#include "soft_iic.h"

// -------------------- 位带操作宏定义 -------------------- 
#define IIC_SCL_LOW     siic_trans_scl_write_bit(false)
#define IIC_SCL_HIGH    siic_trans_scl_write_bit(true)
#define IIC_SDA_LOW     siic_trans_sda_write_bit(false)
#define IIC_SDA_HIGH    siic_trans_sda_write_bit(true)

// -------------------- 全局变量 -------------------- 
static uint32_t T_iic_speed = 10;  // 用来表示 IIC 通信速率的周期（是周期而非频率，等于 0 时速度最快，大约 800KHZ, 该值的大小和周期并不严格呈线性关系，须凭感觉调整）
static bool Is_ack = true;          // 用来确定是否确定应答位（true 为接受应答位）
static uint8_t Scl_pin_num;
static uint8_t Sda_pin_num;
static GPIO_TypeDef * Siic_GPIO;


// -------------------- 私有函数 -------------------- 

/// @brief 延时函数
/// @param x 
static void delay(int x)
{  
    while (x --)
    {
        __NOP();
    }
}



/// @brief IIC 总线开始时序
/// @param  
static void siic_trans_start(void)
{
    IIC_SCL_HIGH;
    IIC_SDA_HIGH;
    delay(T_iic_speed);
    IIC_SDA_LOW;    // 数据线先拉低
    delay(T_iic_speed);      // 延时
    IIC_SCL_LOW;    // 接着时钟线拉低
    delay(T_iic_speed);      // 延时
}

/// @brief IIC 总线结束时序
/// @param  
static void siic_trans_end(void)
{
    IIC_SCL_LOW;
    IIC_SDA_LOW;
    delay(T_iic_speed);
    IIC_SCL_HIGH;
    delay(T_iic_speed);
    IIC_SDA_HIGH;
    delay(T_iic_speed);
}

/// @brief IIC 总线发送数据位高电平
/// @param  
static void siic_trans_bit_high(void)
{
    IIC_SDA_HIGH;
    delay(T_iic_speed);      // 发送数据前
    IIC_SCL_HIGH;
    delay(T_iic_speed);      // SCL 跳变到高电平，发送数据
    IIC_SCL_LOW;
    delay(T_iic_speed);
}

/// @brief IIC 总线发送数据位低电平
/// @param  
static void siic_trans_bit_low(void)
{
    IIC_SDA_LOW;
    IIC_SCL_HIGH;
    delay(T_iic_speed);      // SCL 跳变到高电平，发送数据
    IIC_SCL_LOW;
    delay(T_iic_speed);
}

/// @brief IIC 总线接收应答信号
/// @param  
static uint8_t siic_trans_wait_ack(void)
{
    uint8_t bit;
    IIC_SDA_HIGH;               // 主机接收应答信号，释放 SDA
    IIC_SCL_LOW;                // 创造一个 SCL 信号的上升沿，从而在第九个时钟信号的高电平期间读取应答信号
    delay(T_iic_speed);         // 
    IIC_SCL_HIGH;               // 和延时的前一个操作一起创造一个 SCL 信号的上升沿，从而在第九个时钟信号的高电平期间读取应答信号
    // bit = GPIO_ReadInputDataBit(GPIO_PORT_SoftIIC, PIN_SDA); // 在上升沿稳定期间读取应答信号
    bit = siic_trans_sda_read_bit();
    IIC_SCL_LOW;                // 拉低 SCL 电平，为后续时序做准备
    delay(T_iic_speed);
    
    // 如果应答位为 false 那么始终返回 1（表示不响应应答位，但是需要以上时序）
    if (Is_ack == false)
        return 1;
    return !bit;                // 应答返回 1 无应答返回 0
}

/// @brief 该函数只是用于拼接时序，并不能直接使用
/// @param byte : 发送的字节
/// @return 返回是否应答，应答返回 1 否则返回 0
static uint8_t siic_trans_write_byte(char byte)
{
    for (signed short i = 7; i >= 0; i --)
    {
        if ((byte >> i) & 0x01)
        {
            siic_trans_bit_high();
        }
        else 
        {
            siic_trans_bit_low();
        }
    }
    return siic_trans_wait_ack();
}


/// @brief 从从机读取一个字节
/// @param ret 返回从从机读取到的一个字节
/// @return 返回 0 表示无应答，返回 1 表示应答
static uint8_t siic_trans_read_byte(uint8_t * ret)
{
    uint8_t byte = 0;
    for (signed short i = 7; i >= 0; i --)
    {
        IIC_SCL_HIGH;           // SCL 拉高，准备读取数据
        delay(T_iic_speed);
        if (siic_trans_sda_read_bit() == true)
        {
            byte |= (1 << i);   // 读取到高电平数据
        }
        IIC_SCL_LOW;            // SCL 拉低，准备下一位数据
        delay(T_iic_speed);
    }
    return siic_trans_wait_ack();
}

/// @brief 
/// @param bit 
static void siic_trans_scl_write_bit(bool bit)
{
    BIT_BAND_ODR(Siic_GPIO, Scl_pin_num) = bit;
}

/// @brief 
/// @param bit 
static void siic_trans_sda_write_bit(bool bit)
{
    BIT_BAND_ODR(Siic_GPIO, Sda_pin_num) = bit;
}

/// @brief 
/// @param  
/// @return 
static bool siic_trans_sda_read_bit(void)
{
    return BIT_BAND_IDR(Siic_GPIO, Scl_pin_num);
}

/// @brief 初始化函数
/// @param  
void SoftIIC_Init(uint32_t t_iic_speed, 
                bool is_ack, 
                GPIO_TypeDef * iic_gpio_port, 
                uint16_t iic_pin_scl, 
                uint16_t iic_pin_sda)
{
    // ------------- 指定 spi 速度 -------------
    T_iic_speed = t_iic_speed;
    // ------------- 配置是否响应应答位 -------------
    Is_ack = is_ack;
    // ------------- 配置软件 iic 的 gpio 端口 -------------
    Siic_GPIO = iic_gpio_port;
    Scl_pin_num = iic_pin_scl;
    Sda_pin_num = iic_pin_sda;
    // ------------- 初始化 GPIO -------------
    // 获取需要初始化的 GPIO 端口时钟 RCC_APB2Periph
    uint32_t RCC_APB2Periph;
    switch ((uint32_t)iic_gpio_port)  
    {// 根据 GPIO 确定要初始化的 GPIO 时钟
    case (uint32_t)GPIOA:
        RCC_APB2Periph = RCC_APB2Periph_GPIOA;
        break;
    case (uint32_t)GPIOB:
        RCC_APB2Periph = RCC_APB2Periph_GPIOB;
        break;
    case (uint32_t)GPIOC:
        RCC_APB2Periph = RCC_APB2Periph_GPIOC;
        break;
    case (uint32_t)GPIOD:
        RCC_APB2Periph = RCC_APB2Periph_GPIOD;
        break;
    case (uint32_t)GPIOE:
        RCC_APB2Periph = RCC_APB2Periph_GPIOE;
        break;
    case (uint32_t)GPIOF:
        RCC_APB2Periph = RCC_APB2Periph_GPIOF;
        break;
    case (uint32_t)GPIOG:
        RCC_APB2Periph = RCC_APB2Periph_GPIOG;
        break;
    }
    RCC_APB2PeriphClockCmd(RCC_APB2Periph, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;           // 开漏输出
    GPIO_InitStruct.GPIO_Pin = (0x01 << Scl_pin_num) | (0x01 << Sda_pin_num);   // 配置 SCL SDA
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(iic_gpio_port, &GPIO_InitStruct);


    // SCL 和 SDA 默认配置为高电平
    GPIO_WriteBit(iic_gpio_port, (0x01 << Scl_pin_num) | (0x01 << Sda_pin_num), Bit_SET); // SCL 和 SDA 默认开漏拉高
}


/// @brief 指定地址写操作
/// @param addr iic 从机地址
/// @param reg 从机寄存器地址
/// @param bytes 待写入的字节流
/// @param bytes_nums 字节流的宽度
/// @return 如果发送成功返回 0，否则返回 1
uint8_t SoftIIC_Write(uint8_t addr, 
                    uint8_t reg,
                    uint8_t * bytes, 
                    int32_t bytes_nums)
{
    uint8_t bit;
    siic_trans_start();                         // IIC 开始时序
    // 发送从机的iic地址和写操作指令，访问iic总线上的设备
    uint8_t first_byte = (addr << 1) & 0xFE;    // 对指定地址的设备写操作
    bit = siic_trans_write_byte(first_byte);    // 写入设备地址，写操作
    if (bit == 0) return 0;     // 如果没有应答，返回 0 ，表示发送失败 
    // 发送从机的寄存器地址
    bit = siic_trans_write_byte(reg);           // 写入从机寄存器地址
    if (bit == 0) return 0;     // 如果没有应答，返回 0 ，表示发送失败 
    // 写入数据流
    for (int i = 0; i < bytes_nums; i ++)       // 向设备写入数据
    {
        bit = siic_trans_write_byte(bytes[i]);
        if (bit == 0)
        {
            break;
        }
    }
    siic_trans_end();   // IIC 终止时序
    return bit;         // 返回是否应答，应答返回 1 无应答返回 0
}

/// @brief 当前地址读操作
/// @param addr 从机地址
/// @param recive_buffer 接收数据缓冲区
/// @param receive_len 读取的数据宽度
/// @return 
uint8_t SoftIIC_Receive_current(uint8_t addr,
                                DataType* receive_buffer,
                                int32_t receive_len)
{
    uint8_t bit = 1;
    uint8_t first_byte = addr << 1 | 0x01;      // 对指定地址的设备读操作
    receive_buffer->len = receive_len;
    siic_trans_start();                 // iic 启动时序
    siic_trans_write_byte(first_byte);  // 地址和读操作指令
    for (int i = 0; i < receive_len; i ++)
    {
        bit = siic_trans_read_byte(&(receive_buffer->data[i]));
        if (bit == 0)
        {
            break;
        }
    }
    siic_trans_end();   // iic 结束时序
    return bit;         // 从从设备读取一串数据，读取成功返回 1 失败返回 0
}

/// @brief 指定地址读
/// @param addr 
/// @param reg 
/// @return 
uint8_t SoftIIC_Receive_reg(uint8_t addr, 
                                uint8_t reg,
                                DataType* receive_buffer,
                                int32_t receive_len)
{

    // 先使用写操作来设定从机的寄存器指针（但是不写入数据，所以发送字节流为空，宽度为 0）
    uint8_t bit = SoftIIC_Write(addr, reg, NULL, 0);    // 使用写入函数（不写入数据）发送从机寄存器的地址
    if (bit == 0)
    {   // 如果 bit == 0 说明写入寄存器地址操作失败
        return bit;
    }
    // 写操作指定好寄存器地址后，使用读操作来读取从机指定地址寄存器的数据
    bit = SoftIIC_Receive_current(addr, receive_buffer, 1);

    return bit; // 应答返回 1 无应答返回 0
}


/// @brief 用于测试的 一段时序
/// @param x 
void SoftIIC_Test(int x)
{
    IIC_SCL_HIGH;
    IIC_SDA_HIGH;
    delay(x);
}
