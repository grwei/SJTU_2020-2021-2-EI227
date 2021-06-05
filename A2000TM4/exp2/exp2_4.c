//*****************************************************************************
//
// Copyright: 2020-2021, 上海交通大学电子工程系实验教学中心
// File name: exp2_3.c
// Description:
//    任务2_3
// Author:	上海交通大学电子工程系实验教学中心；危国锐(313017602@qq.com)
// Version: 1.0.0.20201228
// Date：2020-12-28
// History：
//    2021-04-27  修改.
//*****************************************************************************

//*****************************************************************************
//
// 头文件
//
//*****************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"		 // 基址宏定义
#include "inc/hw_types.h"		 // 数据类型宏定义，寄存器访问函数
#include "driverlib/debug.h"	 // 调试用
#include "driverlib/gpio.h"		 // 通用IO口宏定义
#include "driverlib/pin_map.h"	 // TM4C系列MCU外围设备管脚宏定义
#include "driverlib/sysctl.h"	 // 系统控制定义
#include "driverlib/systick.h"	 // SysTick Driver 原型
#include "driverlib/interrupt.h" // NVIC Interrupt Controller Driver 原型

#include "tm1638.h" // 与控制TM1638芯片有关的函数

//*****************************************************************************
//
// 宏定义
//
//*****************************************************************************
#define SYSTICK_FREQUENCY 50 // SysTick频率为50Hz，即循环定时周期20ms

#define V_T100ms 5	// 0.1s软件定时器溢出值，5个20ms
#define V_T500ms 25 // 0.5s软件定时器溢出值，25个20ms

//*****************************************************************************
//
// 函数原型声明
//
//*****************************************************************************
void GPIOInit(void);	// GPIO初始化
void SysTickInit(void); // 设置SysTick中断
void DevicesInit(void); // MCU器件初始化，注：会调用上述函数
//*****************************************************************************
//
// 变量定义
//
//*****************************************************************************

// 软件定时器计数
uint8_t clock100ms = 0;
uint8_t clock500ms = 0;

// 软件定时器溢出标志
uint8_t clock100ms_flag = 0;
uint8_t clock500ms_flag = 0;

// 测试用计数器
uint32_t test_counter = 0;

// 8位数码管显示的数字或字母符号
// 注：板上数码位从左到右序号排列为4、5、6、7、0、1、2、3
uint8_t digit[8] = {' ', ' ', ' ', ' ', '_', ' ', '_', ' '};

// 8位小数点 1亮  0灭
// 注：板上数码位小数点从左到右序号排列为4、5、6、7、0、1、2、3
uint8_t pnt = 0x04;

// 8个LED指示灯状态，0灭，1亮
// 注：板上指示灯从左到右序号排列为7、6、5、4、3、2、1、0
//     对应元件LED8、LED7、LED6、LED5、LED4、LED3、LED2、LED1
uint8_t led[] = {1, 1, 1, 1, 1, 1, 1, 0};

// 当前按键值
uint8_t key_code = 0;

// 系统时钟频率
uint32_t ui32SysClock;

// 当前状态号. 0-计时进行；1-计时暂停；2-计时清零.
// 状态转移规则：(C01, C10) 当状态0/1时，按一次1号键，转移至状态1/0；(C02, C12) 当状态0/1时，按一次2号键，转移至状态2；(C2*) 当状态2时，完成一次状态动作后，返回至上一状态.
// 状态转移动作：(C02, C12) 保存当前状态号（通过更新 pre_state），将计数值清零；(C01, C10) 保存当前状态号（通过更新 pre_state）.
// 状态动作：0-计时进行，最左端LED灯亮，其余全灭；1-计时暂停，LED灯全灭；2-立即刷新数码管的值，然后返回上一状态号(pre_state).
uint8_t cur_state = 0;
uint8_t pre_state = 0; // 上一状态号.
bool flag_DecCnt = 1;  // 计数方向标志. 1-减计数，0-加计数

//*****************************************************************************
//
// 主程序
//
//*****************************************************************************
int main(void)
{
	uint8_t i;

	DevicesInit(); //  MCU器件初始化

	while (clock100ms < 3)
		;		   // 延时>60ms,等待TM1638上电完成
	TM1638_Init(); // 初始化TM1638

	while (1)
	{
		static uint8_t cur_key_code;
		uint8_t pre_key_code = cur_key_code;
		cur_key_code = key_code;

		// 更新计数方向标志 flag_DecCnt
		if (pre_key_code != 3 && cur_key_code == 3)
		{
			flag_DecCnt = !flag_DecCnt;
		}

		// 更新左侧4位数码管(digit[4~7])的显示
		digit[4] = digit[7] = '_';
		digit[5] = digit[6] = (flag_DecCnt ? 'A' : 'U');

		/**
		 * FSM.
		 * 
		 * 当前状态号. 0-计时进行；1-计时暂停；2-计时清零.
		 * 状态转移规则：(C01, C10) 当状态0/1时，按一次1号键，转移至状态1/0；(C02, C12) 当状态0/1时，按一次2号键，转移至状态2；(C2*) 当状态2时，完成一次状态动作后，返回至上一状态.
		 * 状态转移动作：(C02, C12) 保存当前状态号（通过更新 pre_state），将计数值清零；(C01, C10) 保存当前状态号（通过更新 pre_state）.
		 * 状态动作：0-计时进行，最左端LED灯亮，其余全灭；1-计时暂停，LED灯全灭；2-立即刷新数码管的值，然后返回上一状态号(pre_state).
		 */
		switch (cur_state)
		{
		case 0:
			// C01: 当状态0/1时，按一次1号键，转移至状态1/0；保存当前状态号（通过更新 pre_state）
			if (pre_key_code != 1 && cur_key_code == 1)
			{
				pre_state = 0;
				cur_state = 1;
			}

			// C02: 保存当前状态号（通过更新 pre_state），将计数值清零
			if (pre_key_code != 2 && cur_key_code == 2)
			{
				test_counter = (flag_DecCnt ? 0 : 9999);
				pre_state = 0;
				cur_state = 2;
			}

			// 状态动作0-计时进行，最左端LED灯亮，其余全灭
			led[7] = 1;
			for (i = 0; i < 7; i++)
			{
				*(led + i) = 0;
			}
			break;
		case 1:
			// C10: 当状态0/1时，按一次1号键，转移至状态1/0；保存当前状态号（通过更新 pre_state）
			if (pre_key_code != 1 && cur_key_code == 1)
			{
				pre_state = 1;
				cur_state = 0;
			}

			// C12: 保存当前状态号（通过更新 pre_state），将计数值清零
			if (pre_key_code != 2 && cur_key_code == 2)
			{
				test_counter = (flag_DecCnt ? 0 : 9999);
				pre_state = 1;
				cur_state = 2;
			}

			// 状态动作1-计时暂停，LED灯全灭
			for (i = 0; i < 8; i++)
			{
				*(led + i) = 0;
			}
			break;
		case 2:
			// 状态动作2-立即刷新数码管的值，然后返回上一状态号(pre_state).
			digit[0] = digit[1] = digit[2] = digit[3] = (flag_DecCnt ? 9 : 0);
			cur_state = pre_state;
			break;
		default:
			break;
		}

		if (clock100ms_flag == 1) // 检查0.1秒定时是否到
		{
			clock100ms_flag = 0;
			// [仅状态0有效] 每0.1秒累加计时值在数码管上以十进制显示
			if (cur_state == 0)
			{
				// 减计数
				// 要避免uint类型的变量发生下溢，因可能引发异常.
				if (flag_DecCnt)
				{
					if (!test_counter)
					{
						test_counter = 10000;
					}
					--test_counter;
				}

				// 加计数
				if (!flag_DecCnt && ++test_counter >= 10000)
				{
					test_counter = 0;
				}

				// 刷新数码管显示
				digit[0] = test_counter / 1000;		// 计算百位数
				digit[1] = test_counter / 100 % 10; // 计算十位数
				digit[2] = test_counter / 10 % 10;	// 计算个位数
				digit[3] = test_counter % 10;		// 计算百分位数
			}
		}

		if (clock500ms_flag == 1) // 检查0.5秒定时是否到
		{
			clock500ms_flag = 0;
		}
	}
}

//*****************************************************************************
//
// 函数原型：void GPIOInit(void)
// 函数功能：GPIO初始化. 使能PortK，设置PK4,PK5为输出；使能PortM，设置PM0为输出.
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
	SysTickEnable();									// SysTick使能
	SysTickIntEnable();									// SysTick中断允许
}

//*****************************************************************************
//
// 函数原型：void DevicesInit(void)
// 函数功能：CU器件初始化，包括系统时钟设置、GPIO初始化和SysTick中断设置
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

	GPIOInit();		   // GPIO初始化
	SysTickInit();	   // 设置SysTick中断
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
	// 0.1秒钟软定时器计数
	if (++clock100ms >= V_T100ms)
	{
		clock100ms_flag = 1; // 当0.1秒到时，溢出标志置1
		clock100ms = 0;
	}

	// 0.5秒钟软定时器计数
	if (++clock500ms >= V_T500ms)
	{
		clock500ms_flag = 1; // 当0.5秒到时，溢出标志置1
		clock500ms = 0;
	}

	// 刷新全部数码管和LED指示灯
	TM1638_RefreshDIGIandLED(digit, pnt, led);

	// 检查当前键盘输入，0代表无键操作，1-9表示有对应按键
	// 键号显示在一位数码管上
	key_code = TM1638_Readkeyboard();
}
