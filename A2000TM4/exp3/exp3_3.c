//*****************************************************************************
//
// Copyright: 2020-2021, 上海交通大学电子工程系实验教学中心
// File name: exp3_3.c
// Description:
//     任务3_3
// Author:	上海交通大学电子工程系实验教学中心；危国锐(313017602@qq.com)
// Version: 1.0.0.20201228
// Date：2020-12-28
// History：
//    2021-04-30 修改：改为用非阻塞（中断）方式进行发送与接收
//
//*****************************************************************************

#ifndef PART_TM4C1294NCPDT
#define PART_TM4C1294NCPDT // pin_map.h 需要
#endif

//*****************************************************************************
//
// 头文件
//
//*****************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"       // 基址宏定义
#include "inc/hw_types.h"        // 数据类型宏定义，寄存器访问函数
#include "driverlib/debug.h"     // 调试用
#include "driverlib/gpio.h"      // 通用IO口宏定义和函数原型
#include "driverlib/pin_map.h"   // TM4C系列MCU外围设备管脚宏定义
#include "driverlib/sysctl.h"    // 系统控制定义
#include "driverlib/systick.h"   // SysTick Driver 原型
#include "driverlib/interrupt.h" // NVIC Interrupt Controller Driver 原型
#include "driverlib/uart.h"      // 与UART有关的宏定义和函数原型
#include "tm1638.h"              // 与控制TM1638芯片有关的宏定义和函数原型
#include "inc/tm4c1294ncpdt.h"   // TM4C1294NCPDT Register Definitions
#include <ctype.h>               // Character handling functions
#include <string.h>              // C Strings
#include <stdio.h>               // C library to perform Input/Output operations
#include <stdlib.h>              // C Standard General Utilities Library

//*****************************************************************************
//
// 宏定义
//
//*****************************************************************************
#define SYSTICK_FREQUENCY 50            // SysTick频率为50Hz，即循环定时周期20ms
#define V_T1000ms 50                    // 1.0s软件定时器溢出值，50个20ms
const uint8_t CMD_RX_BUF_MAX_SIZE = 60; // 命令接收缓存区最大容量
#define CMD_TX_BUF_MAX_SIZE 60          // 命令发送缓存区最大容量

//*****************************************************************************
//
// 函数原型声明
//
//*****************************************************************************
void GPIOInit(void);                                         // GPIO初始化
void SysTickInit(void);                                      // 设置SysTick中断
void UARTInit(void);                                         // UART初始化
void DevicesInit(void);                                      // MCU器件初始化，注：会调用上述函数
void UARTStringPut(uint32_t ui32Base, const char *cMessage); // 向UART发送字符串
void UART_clock(void);                                       // 基于 UART 的时钟处理函数

//*****************************************************************************
//
// 变量定义
//
//*****************************************************************************

// 软件定时器计数
volatile uint8_t clock1000ms = 0;

// 软件定时器溢出标志
volatile uint8_t clock1000ms_flag = 0;

// 时分秒计数器
volatile uint8_t counter_hh = 0, counter_mm = 0, counter_ss = 0;

// 8位数码管显示的数字或字母符号
// 注：板上数码位从左到右序号排列为4、5、6、7、0、1、2、3
uint8_t digit[8] = {' ', ' ', ' ', ' ', '_', 0, '_', ' '};

// 8位小数点 1亮  0灭
// 注：板上数码位小数点从左到右序号排列为4、5、6、7、0、1、2、3
uint8_t pnt = 0xA2;

// 8个LED指示灯状态，0灭，1亮
// 注：板上指示灯从左到右序号排列为7、6、5、4、3、2、1、0
//     对应元件LED8、LED7、LED6、LED5、LED4、LED3、LED2、LED1
uint8_t led[] = {0, 0, 0, 0, 0, 0, 0, 0};

// 当前按键值
volatile uint8_t key_code = 0;

// 系统时钟频率
uint32_t ui32SysClock;

/**
 * @brief 命令接收缓存区结构体
 * 
 */
struct cmd_Rx_buf_t
{
    volatile uint8_t size;                                // 当前缓存区内容长度
    const uint8_t max_size;                               // 缓存区最大容量
    volatile unsigned char data[CMD_RX_BUF_MAX_SIZE + 1]; // 缓存区数据
    volatile bool WriteEnable;                            // True-写允许；False-提示缓存区中有一个待处理的命令
} cmd_Rx_buf = {0, CMD_RX_BUF_MAX_SIZE, {'\0'}, true};

/**
 * @brief 命令发送缓存区结构体
 * 
 */
struct cmd_Tx_buf_t
{
    volatile uint8_t size;                                // 当前缓存区内容长度
    const uint8_t max_size;                               // 缓存区最大容量
    volatile unsigned char data[CMD_TX_BUF_MAX_SIZE + 1]; // 数据
    volatile uint8_t next_trans_index;                    // 下一次将发送的数据的下标. 当达到size时，表示已发送完成.
    volatile bool WriteEnable;                            // True-写允许；False-提示缓存区中有未发送完毕的数据
} cmd_Tx_buf = {0, CMD_TX_BUF_MAX_SIZE, {'\0'}, 0, true};

//*****************************************************************************
//
// 主程序
//
//*****************************************************************************
int main(void)
{
    DevicesInit(); //  MCU器件初始化

    SysCtlDelay(60 * (ui32SysClock / 3000)); // 延时>60ms,等待TM1638上电完成
    TM1638_Init();                           // 初始化TM1638

    while (1)
    {
        if (clock1000ms_flag == 1) // 检查1.0秒定时是否到
        {
            char src_str[CMD_RX_BUF_MAX_SIZE] = {'\0'};

            clock1000ms_flag = 0;
            if (++counter_ss == 60)
            {
                counter_ss = 0;
                if (++counter_mm == 60)
                {
                    counter_mm = 0;
                    if (++counter_hh == 24)
                    {
                        counter_hh = 0;
                    }
                }
            }

            // 3.2.2 准备写入命令发送缓存区的数据
            sprintf(src_str, "现在是中华人民共和国北京时间%.2hhu:%.2hhu:%.2hhu\n",
                    counter_hh, counter_mm, counter_ss);

            // 3.2.3 向命令发送缓存区写入数据，并置相应的标志
            strcpy((char *)cmd_Tx_buf.data, (const char *)src_str);
            cmd_Tx_buf.size = strlen((const char *)cmd_Tx_buf.data);
            cmd_Tx_buf.next_trans_index = 0;
            cmd_Tx_buf.WriteEnable = false; // 向命令发送缓存区写入完毕，关写允许（将触发一个发送事件）

            // 尝试用命令发送缓存区的数据（如果有）填满 Tx FIFO
            while (!cmd_Tx_buf.WriteEnable && cmd_Tx_buf.size && UARTSpaceAvail(UART0_BASE))
            {
                if (UARTCharPutNonBlocking(UART0_BASE, cmd_Tx_buf.data[cmd_Tx_buf.next_trans_index]))
                {
                    // 若全部发送完毕，则清空命令发送缓存区，开写允许
                    if (++cmd_Tx_buf.next_trans_index >= cmd_Tx_buf.size)
                    {
                        cmd_Tx_buf.size = cmd_Tx_buf.next_trans_index = 0;
                        cmd_Tx_buf.WriteEnable = true;
                    }
                }
            }
        }

        // 更新数码管显示
        digit[4] = counter_hh / 10;
        digit[5] = counter_hh % 10;
        digit[6] = counter_mm / 10;
        digit[7] = counter_mm % 10;
        digit[0] = counter_ss / 10;
        digit[1] = counter_ss % 10;

        UART_clock(); // 基于 UART 的时钟处理
    }
}

//*****************************************************************************
//
// 函数原型：void GPIOInit(void)
// 函数功能：GPIO初始化。使能PortK，设置PK4,PK5为输出；使能PortM，设置PM0为输出。
//          （PK4连接TM1638的STB，PK5连接TM1638的DIO，PM0连接TM1638的CLK）
// 函数参数：无
// 函数返回值：无
//
//*****************************************************************************
void GPIOInit(void)
{
    //配置TM1638芯片管脚
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK); // 使能端口 K
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK))
    {
    }; // 等待端口 K准备完毕

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM); // 使能端口 M
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOM))
    {
    }; // 等待端口 M准备完毕

    // 设置端口 K的第4,5位（PK4,PK5）为输出引脚		PK4-STB  PK5-DIO
    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    // 设置端口 M的第0位（PM0）为输出引脚   PM0-CLK
    GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, GPIO_PIN_0);
}

//*****************************************************************************
//
// 函数原型：SysTickInit(void)
// 函数功能：设置SysTick中断
// 函数参数：无
// 函数返回值：无
//
//*****************************************************************************
void SysTickInit(void)
{
    SysTickPeriodSet(ui32SysClock / SYSTICK_FREQUENCY); // 设置心跳节拍,定时周期20ms
    SysTickEnable();                                    // SysTick使能
    SysTickIntEnable();                                 // SysTick中断允许
}

//*****************************************************************************
//
// 函数原型：void UARTStringPut(uint32_t ui32Base, const char *cMessage)
// 函数功能：向UART模块发送字符串
// 函数参数：ui32Base：UART模块
//          cMessage：待发送字符串
// 函数返回值：无
//
//*****************************************************************************
void UARTStringPut(uint32_t ui32Base, const char *cMessage)
{
    while (*cMessage != '\0')
        UARTCharPut(ui32Base, *(cMessage++));
}

//*****************************************************************************
//
// 函数原型：void UARTInit(void)
// 函数功能：UART初始化。使能UART0，设置PA0,PA1为UART0 RX,TX引脚；
//          设置波特率及帧格式。
// 函数参数：无
// 函数返回值：无
//
//*****************************************************************************
void UARTInit(void)
{
    // 引脚配置
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); // 使能UART0模块
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); // 使能端口 A
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))
        ; // 等待端口 A准备完毕

    GPIOPinConfigure(GPIO_PA0_U0RX); // 设置PA0为UART0 RX引脚
    GPIOPinConfigure(GPIO_PA1_U0TX); // 设置PA1为UART0 TX引脚

    // 设置端口 A的第0,1位（PA0,PA1）为UART引脚
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // 波特率及帧格式设置
    UARTConfigSetExpClk(UART0_BASE,
                        ui32SysClock,
                        115200,                  // 波特率：115200
                        (UART_CONFIG_WLEN_8 |    // 数据位：8
                         UART_CONFIG_STOP_ONE |  // 停止位：1
                         UART_CONFIG_PAR_NONE)); // 校验位：无

    IntEnable(INT_UART0);                                               // UART0中断允许
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT | UART_INT_TX); // 使能UART0 RX,RT,TX中断
    UARTFIFOLevelSet(UART0_BASE, UART_FIFO_TX1_8, UART_FIFO_RX7_8);     // Sets the FIFO level at which interrupts are generated.

    // 初始化完成后向PC端发送"Hello, 2A!"字符串
    UARTStringPut(UART0_BASE, (const char *)"\r\nHello, 2A!\r\n");
}

//*****************************************************************************
//
// 函数原型：DevicesInit(void)
// 函数功能：MCU器件初始化，包括系统时钟设置、GPIO初始化和SysTick中断设置
// 函数参数：无
// 函数返回值：无
//
//*****************************************************************************
void DevicesInit(void)
{
    // 使用外部25MHz主时钟源，经过PLL，然后分频为20MHz
    ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
                                       SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
                                      20000000);

    GPIOInit();        // GPIO初始化
    SysTickInit();     // 设置SysTick中断
    UARTInit();        // UART初始化
    IntMasterEnable(); // 总中断允许
}

//*****************************************************************************
//
// 函数原型：void SysTick_Handler(void)
// 函数功能：SysTick中断服务程序
// 函数参数：无
// 函数返回值：无
//
//*****************************************************************************
void SysTick_Handler(void) // 定时周期为20ms
{
    // 1.0秒钟软定时器计数
    if (++clock1000ms >= V_T1000ms)
    {
        clock1000ms_flag = 1; // 当1.0秒到时，溢出标志置1
        clock1000ms = 0;
    }

    // 刷新全部数码管和LED指示灯
    TM1638_RefreshDIGIandLED(digit, pnt, led);

    // 检查当前键盘输入，0代表无键操作，1-9表示有对应按键
    // 键号显示在一位数码管上
    key_code = TM1638_Readkeyboard();
}

/**
 * @brief UART0 中断服务程序
 * 
 */
void UART0_Handler(void)
{
    int32_t uart0_int_status;

    uart0_int_status = UARTIntStatus(UART0_BASE, true); // 取中断状态
    UARTIntClear(UART0_BASE, uart0_int_status);         // 清中断标志

    switch (uart0_int_status)
    {
    case UART_INT_RT:                      // Receive Timeout Interrupt
    case UART_INT_RX:                      // Receive Interrupt
        while (UARTCharsAvail(UART0_BASE)) // 重复从接收 FIFO 读取字符
        {
            uint8_t uart_receive_char = UARTCharGetNonBlocking(UART0_BASE); // 读入一个字符

            /**
             * 若命令接收缓存区允许写入，
             * 则将 UART 端口的 Rx FIFO 中的内容写入 cmd_Rx_buf，
             * 直至读到一个命令终止符.
             */
            if (cmd_Rx_buf.WriteEnable)
            {
                // 若命令接收缓存区已满，则关写允许
                if (cmd_Rx_buf.size >= cmd_Rx_buf.max_size)
                {
                    cmd_Rx_buf.data[cmd_Rx_buf.max_size - 1] = '\0';
                    cmd_Rx_buf.WriteEnable = false;
                }
                // tab ('\t'), white-space control codes ('\f','\v','\n','\r')
                // 和 space (' ')被视为命令终止符，写入停止.
                else if (isspace(uart_receive_char))
                {
                    cmd_Rx_buf.data[cmd_Rx_buf.size] = '\0'; // 保证缓存区构成合法字符串
                    // 若首字符即为命令终止符，则忽略此次接收，即不要关写允许
                    if (cmd_Rx_buf.size)
                    {
                        cmd_Rx_buf.WriteEnable = false; // 写入完毕，关写允许
                    }
                }
                else
                {
                    cmd_Rx_buf.data[cmd_Rx_buf.size++] = uart_receive_char;
                }
            }
        }
        break;
    case UART_INT_TX: // Transmit Interrupt
        // 尝试用命令发送缓存区的数据（如果有）填满 Tx FIFO
        while (!cmd_Tx_buf.WriteEnable && cmd_Tx_buf.size && UARTSpaceAvail(UART0_BASE))
        {
            if (UARTCharPutNonBlocking(UART0_BASE, cmd_Tx_buf.data[cmd_Tx_buf.next_trans_index]))
            {
                // 若全部发送完毕，则清空命令发送缓存区，开写允许
                if (++cmd_Tx_buf.next_trans_index >= cmd_Tx_buf.size)
                {
                    cmd_Tx_buf.size = cmd_Tx_buf.next_trans_index = 0;
                    cmd_Tx_buf.WriteEnable = true;
                }
            }
        }
        break;
    default:
        break;
    }
}

/**
 * @brief 基于 UART 的时钟处理函数
 * 
 * 首先判断命令接收缓存区 cmd_Rx_buf 是否存在已接收完毕的待处理命令，
 * 若有，则作出相应处理，以非阻塞方式向命令发送缓存区 cmd_Tx_buf 填数据；
 * 然后，以非阻塞方式将命令发送缓存区的数据送往 UART 端口的 Tx FIFO.
 */
void UART_clock(void)
{
    // 当有接收事件（此时接收缓存区的写允许标志被关闭），且命令发送缓存区 cmd_Tx_buf 允许写入，则开始处理接收缓存区的数据
    if (!cmd_Rx_buf.WriteEnable && cmd_Tx_buf.WriteEnable)
    {
        // 1. 接收到查询时间命令
        if (!strcmp("AT+GET", (const char *)cmd_Rx_buf.data))
        {
            // char *src_str = (char *)calloc(cmd_Tx_buf.max_size, sizeof(unsigned char));
            char src_str[CMD_RX_BUF_MAX_SIZE] = {'\0'};

            // 1.1 准备写入命令发送缓存区的数据
            sprintf(src_str, "%.2hhu:%.2hhu:%.2hhu",
                    counter_hh, counter_mm, counter_ss);

            // 1.2 向命令发送缓存区写入数据，并置相应的标志
            strcpy((char *)cmd_Tx_buf.data, (const char *)src_str);
            cmd_Tx_buf.size = strlen((const char *)cmd_Tx_buf.data);
            cmd_Tx_buf.next_trans_index = 0;
            cmd_Tx_buf.WriteEnable = false; // 向命令发送缓存区写入完毕，关写允许

            // free(src_str);
        }
        // 2. 接收到（疑似）绝对对时命令
        else if (!strncmp("AT+SET", (const char *)cmd_Rx_buf.data, 6))
        {
            uint8_t hh, mm, ss;

            // 2.1 若解析不成功，或解析成功但取值不合规，则准备发送报错信息
            if (sscanf((const char *)cmd_Rx_buf.data,
                       "AT+SET%2hhu:%2hhu:%3hhu", &hh, &mm, &ss) < 3 ||
                hh > 23 || mm > 59 || ss > 59) // inc_ss采取%3hhu形式，是为检查秒数位数超过2
            {
                strcpy((char *)cmd_Tx_buf.data, "Error Command!");
                cmd_Tx_buf.size = strlen((const char *)cmd_Tx_buf.data);
                cmd_Tx_buf.next_trans_index = 0;
                cmd_Tx_buf.WriteEnable = false; // 向命令发送缓存区写入完毕，关写允许
            }
            // 2.2 匹配成功，执行绝对对时动作
            else
            {
                counter_hh = hh;
                counter_mm = mm;
                counter_ss = ss;
            }
        }
        // 3. 接收到（疑似）相对对时命令
        else if (!strncmp("AT+INC", (const char *)cmd_Rx_buf.data, 6))
        {
            uint8_t inc_hh, inc_mm, inc_ss;

            // 3.1 若解析不成功，或解析成功但取值不合规，则准备发送报错信息
            if (sscanf((const char *)cmd_Rx_buf.data,
                       "AT+INC%2hhu:%2hhu:%3hhu", &inc_hh, &inc_mm, &inc_ss) < 3 ||
                inc_hh > 23 || inc_mm > 59 || inc_ss > 59) // inc_ss采取%3hhu形式，是为检查秒数位数超过2
            {
                strcpy((char *)cmd_Tx_buf.data, "Error Command!");
                cmd_Tx_buf.size = strlen((const char *)cmd_Tx_buf.data);
                cmd_Tx_buf.next_trans_index = 0;
                cmd_Tx_buf.WriteEnable = false; // 向命令发送缓存区写入完毕，关写允许
            }
            // 3.2 匹配成功，执行相对对时动作，然后向PC端发回当前时间
            else
            {
                // char *src_str = (char *)calloc(cmd_Tx_buf.max_size, sizeof(char));
                char src_str[CMD_RX_BUF_MAX_SIZE] = {'\0'};

                // 3.2.1 执行相对对时动作
                if ((counter_ss += inc_ss) > 60)
                {
                    counter_mm += counter_ss / 60;
                    counter_ss %= 60;
                }
                if ((counter_mm += inc_mm) > 60)
                {
                    counter_hh += counter_mm / 60;
                    counter_mm %= 60;
                }
                if ((counter_hh += inc_hh) > 23)
                {
                    counter_hh %= 24;
                }

                // 3.2.2 准备写入命令发送缓存区的数据
                sprintf(src_str, "%.2hhu:%.2hhu:%.2hhu",
                        counter_hh, counter_mm, counter_ss);

                // 3.2.3 向命令发送缓存区写入数据，并置相应的标志
                strcpy((char *)cmd_Tx_buf.data, (const char *)src_str);
                cmd_Tx_buf.size = strlen((const char *)cmd_Tx_buf.data);
                cmd_Tx_buf.next_trans_index = 0;
                cmd_Tx_buf.WriteEnable = false; // 向命令发送缓存区写入完毕，关写允许

                // free(src_str);
            }
        }
        // 4. 接收到非法命令
        else
        {
            strcpy((char *)cmd_Tx_buf.data, "Error Command!");
            cmd_Tx_buf.size = strlen((const char *)cmd_Tx_buf.data);
            cmd_Tx_buf.next_trans_index = 0;
            cmd_Tx_buf.WriteEnable = false; // 向命令发送缓存区写入完毕，关写允许
        }

        // 命令接收缓存区数据处理完毕，复位命令接收缓存区
        cmd_Rx_buf.size = 0;           // 清空命令接收缓存区
        cmd_Rx_buf.WriteEnable = true; // 处理完毕，命令接收缓存区开写允许
    }

    // 若命令发送缓存区已写入完毕（此时写允许关闭，且缓存区非空），
    // 则尝试用命令发送缓存区的数据（如果有）填满 Tx FIFO
    while (!cmd_Tx_buf.WriteEnable && cmd_Tx_buf.size && UARTSpaceAvail(UART0_BASE))
    {
        if (UARTCharPutNonBlocking(UART0_BASE, cmd_Tx_buf.data[cmd_Tx_buf.next_trans_index]))
        {
            // 若全部发送完毕，则复位命令发送缓存区
            if (++cmd_Tx_buf.next_trans_index >= cmd_Tx_buf.size)
            {
                cmd_Tx_buf.size = cmd_Tx_buf.next_trans_index = 0; // 清空命令发送缓存区
                cmd_Tx_buf.WriteEnable = true;                     // 开命令发送缓存区写允许
            }
        }
    }
}
