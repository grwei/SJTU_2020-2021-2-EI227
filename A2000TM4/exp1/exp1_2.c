//**************************************************************************************
//
// Copyright: 2020-2021, 上海交通大学电子工程系实验教学中心
// File name: exp1_2.c
// Description: 当按下USR_SW1-PJ0时，点亮D1-PN1；放开时，熄灭D1-PN1;
//              当按下USR_SW2-PJ1时，点亮D2-PN0；放开时，熄灭D2-PN0.
// Author:	上海交通大学电子工程系实验教学中心；危国锐(313017602@qq.com)
// Version: 1.0.0.20201228
// Date：2020-12-28
// History：
//    2021-04-26  修改.
//**************************************************************************************

//**************************************************************************************
//
// 头文件
//
//**************************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"     // 基址宏定义
#include "inc/hw_types.h"      // 数据类型宏定义，寄存器访问函数
#include "driverlib/debug.h"   // 调试用
#include "driverlib/gpio.h"    // 通用IO口宏定义
#include "driverlib/pin_map.h" // TM4C系列MCU外围设备管脚宏定义
#include "driverlib/sysctl.h"  // 系统控制宏定义
#include "driverlib/uart.h"    // UART相关宏定义
#include "utils/uartstdio.h"   // UART0作为控制台相关函数原型声明

//**************************************************************************************
//
// 宏定义
//
//**************************************************************************************
#define MilliSecond 4000   // 形成1ms时延所需循环次数
#define FASTFLASHTIME 50   // 短延时（50ms）
#define SLOWFLASHTIME 3000 // 长延时（3000ms）

//**************************************************************************************
//
// 函数原型声明
//
//**************************************************************************************
void DelayMilliSec(uint32_t ui32DelaySecond); // 延迟一定时长，单位为毫秒
void GPIOInit(void);                          // GPIO初始化
void PF0Flash(uint8_t ui8KeyValue);           // 根据传入的按键值，决定PF0快闪或慢闪
uint32_t exp1_1_SysClockSel(uint8_t number);  // 实验1，表格1-1

//uint32_t ui32SysClock;
//**************************************************************************************
//
// 主程序
//
//**************************************************************************************
int main(void)
{
    volatile uint8_t ui8KeyValue;
    uint32_t ui32SysClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_320, 16000000);
    GPIOInit(); // GPIO初始化

    while (1) // 无限循环
    {
        // 读取 PJ0、PJ1 键值. 0-按下 1-松开. Any bit that is not specified by ui8Pins is returned as a 0.
        ui8KeyValue = GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);

        // 若 PJ0/1 为 1（松开），则熄灭 PN1/0 （写入0）；否则.
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, ~((ui8KeyValue << 1) | (ui8KeyValue >> 1)));
    }
}

//**************************************************************************************
//
// 函数原型：void DelayMilliSec(uint32_t ui32DelaySecond)
// 函数功能：延迟一定时长，单位为毫秒
// 函数参数：ui32DelaySecond：延迟毫秒数
//
//**************************************************************************************
void DelayMilliSec(uint32_t ui32DelaySecond)
{
    uint32_t ui32Loop;

    ui32DelaySecond = ui32DelaySecond * MilliSecond;
    for (ui32Loop = 0; ui32Loop < ui32DelaySecond; ui32Loop++)
    {
    };
}

//**************************************************************************************
//
// 函数原型：void GPIOInit(void)
// 函数功能：GPIO初始化. 使能PortF，设置PF0、PF1为输出；使能PortJ，设置PJ0、PJ1为输入
// 函数参数：无
//
//**************************************************************************************
void GPIOInit(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION); // 使能端口 N
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
        ; // 等待端口 N 准备完毕

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // 使能端口 J
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ))
    {
    }; // 等待端口 J 准备完毕

    // 设置端口 N 的第0、1位（PN0、PN1）为输出引脚
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // 设置端口 J 的第0、1位（PJ0、PJ1）为输入引脚
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // 端口 J 的第0、1位作为按键输入，类型设置成“推挽上拉”
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);
}

//**************************************************************************************
//
// 函数原型：void PF0Flash(uint8_t ui8KeyValue)
// 函数功能：根据传入的按键值，决定PF0快闪或慢闪. 0-快闪，1-慢闪
// 函数参数：ui8KeyValue：按键值
//
//**************************************************************************************
void PF0Flash(uint8_t ui8KeyValue)
{
    uint32_t ui32DelayTime;

    if (ui8KeyValue == 0) // PUSH1(USR_SW1-PJ0) 按下
        ui32DelayTime = FASTFLASHTIME;
    else // PUSH1(USR_SW1-PJ0) 松开
        ui32DelayTime = SLOWFLASHTIME;

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0); // 点亮 LED4(D4-PF0)
    DelayMilliSec(ui32DelayTime);                          // 延时ui32DelayTime毫秒

    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0x0); // 关闭 LED4(D4-PF0)
    DelayMilliSec(ui32DelayTime);                   // 延时ui32DelayTime毫秒
}

/**
 * @brief 表格 1-1
 * 
 * @param number 序号
 * @return uint32_t The actual configured system clock frequency in Hz or zero if the value could not be changed
due to a parameter error or PLL lock failure.
 */
uint32_t exp1_1_SysClockSel(uint8_t number)
{
    uint32_t ui32SysClock;
    switch (number)
    {
    case 1:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_OSC_INT | SYSCTL_USE_OSC, 16000000);
        break;
    case 2:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_OSC_INT | SYSCTL_USE_OSC, 12000000);
        break;
    case 3:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_OSC_INT | SYSCTL_USE_OSC, 8000000);
        break;
    case 4:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_OSC, 25000000);
        break;
    case 5:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_OSC, 12000000);
        break;
    case 6:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_OSC, 1000000);
        break;
    case 7:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 25000000);
        break;
    case 8:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 20000000);
        break;
    case 9:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 8000000);
        break;
    case 10:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_OSC_INT | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 20000000);
        break;
    case 11:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_OSC_INT | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 8000000);
        break;
    case 12:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_OSC_INT | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 1000000);
        break;
    default:
        ui32SysClock = SysCtlClockFreqSet(SYSCTL_OSC_INT | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 8000000);
        break;
    }
    return ui32SysClock;
}
