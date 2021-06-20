/**
 * @file exp4_3.c
 * @author 上海交通大学电子工程系实验教学中心; Guorui Wei (313017602@qq.com)
 * @brief 实验4_3
 * @version 0.1
 * @date 2021-06-05
 * 
 * @copyright 2020-2021, 上海交通大学电子工程系实验教学中心
 * 
 */

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
#include "driverlib/gpio.h"      // 通用IO口宏定义
#include "driverlib/pin_map.h"   // TM4C系列MCU外围设备管脚宏定义
#include "driverlib/sysctl.h"    // 系统控制定义
#include "driverlib/systick.h"   // SysTick Driver 原型
#include "driverlib/interrupt.h" // NVIC Interrupt Controller Driver 原型
#include "JLX12864.h"            // 与控制JLX12864G有关的函数
#include "tm1638.h"              // 与控制TM1638芯片有关的函数
#include "string.h"              //

//*****************************************************************************
//
// 宏定义
//
//*****************************************************************************
#define SYSTICK_FREQUENCY 50  // SysTick频率为50Hz，即循环定时周期20ms
#define V_T100ms 5            // 0.1s软件定时器溢出值，5个20ms
#define V_T500ms 25           // 0.5s软件定时器溢出值，25个20ms
#define V_T2s 100             // 2.0s软定时器溢出值，100个20ms
#define V_T5s 250             // 5.0s软定时器溢出值，250个20ms
#define V_T10s 500            // 10.0s软定时器溢出值，500个20ms
#define LCD_MAX_BLOCK 15      // 显示屏上的最大8x8分区数
#define LCD_MAX_BLOCK_CHAR 15 // 显示屏上每个分区的最大字符数

//*****************************************************************************
//
// 函数原型声明
//
//*****************************************************************************
void GPIOInit(void);    // GPIO初始化
void SysTickInit(void); // 设置SysTick中断
void DevicesInit(void); // MCU器件初始化，注：会调用上述函数

/** 
 * UI状态机相关函数
 */

void ui_state_proc(uint16_t ui_state);
void ui_proc0(void);
void ui_proc005(void);
void ui_proc100(void);
void ui_proc101(void);
void ui_proc102(void);
void ui_proc201(void);
void ui_proc202(void);
void ui_proc203(void);
void ui_proc301(void);
void ui_proc303(void);
void ui_proc305(void);
void ui_proc306(void);
void ui_proc4(void);
void ENTER_detect(void);
void LEFT_detect(void);
void RIGHT_detect(void);
void INCREASE_detect(void);
void DECREASE_detect(void);

//*****************************************************************************
//
// 变量定义
//
//*****************************************************************************

// 软件定时器计数
uint8_t clock100ms = 0;
uint8_t clock500ms = 0;
uint8_t clock2s = 0;
uint8_t NOKEY_clock5s = 0;
uint8_t ACT4_clock5s = 0;

// 软件定时器溢出标志
uint8_t clock100ms_flag = 0;
uint8_t clock500ms_flag = 0;
uint8_t clock2s_flag = 0;
uint8_t NOKEY_clock5s_flag = 0;
uint8_t ACT4_clock5s_flag = 0;

/**
 * 按键事件标志
 */
uint8_t key_LEFT_flag = 0;
uint8_t key_RIGHT_flag = 0;
uint8_t key_INCREASE_flag = 0;
uint8_t key_DECREASE_flag = 0;
uint8_t key_ENTER_flag = 0;

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
uint8_t pre_key_code; // 上一按键值

// 系统时钟频率
uint32_t ui32SysClock;

/**
 * 用户界面（UI）状态机相关变量定义
 */

uint16_t ui_state = 0x0; // 用户界面（UI）状态机当前状态

/**
 * @brief 状态参数结构体
 * 
 */
struct ACT_T
{
    uint8_t row_page[LCD_MAX_BLOCK];                      // 显示屏上每个分区的起始行页号
    uint8_t col_page[LCD_MAX_BLOCK];                      // 显示屏上每个分区的起始列页号
    unsigned char str[LCD_MAX_BLOCK][LCD_MAX_BLOCK_CHAR]; // 显示屏上每个分区的显示内容
    const uint8_t SIZE;                                   // 显示屏上有效分区的数量
};

struct ACT_T act0 = {
    {3, 3, 3, 3, 3, 7},
    {3, 11, 12, 13, 14, 1},
    {"模式A", "1", ".", "1", "Hz", "设置"},
    6};

struct ACT_T act1 = {
    {3, 5, 7},
    {3, 3, 13},
    {"工作模式", "工作参数", "返回"},
    3};

struct ACT_T act2 = {
    {3, 3, 7, 7},
    {1, 11, 1, 13},
    {"工作模式：", "模式A", "确定", "取消"},
    4};

struct ACT_T act3 = {
    {3, 3, 3, 3, 3, 7, 7},
    {1, 11, 12, 13, 14, 1, 13},
    {"工作参数：", "1", ".", "1", "Hz", "确定", "取消"},
    7};

struct ACT_T act4 = {
    {3},
    {1},
    {"工作参数不合法"},
    1};

struct ACT_T *act[] = {&act0, &act1, &act2, &act3, &act4};

//*****************************************************************************
//
// 主程序
//
//*****************************************************************************
int main(void)
{
    uint8_t temp, i;
    DevicesInit(); //  MCU器件初始化

    while (clock100ms < 3)
        ;           // 延时>60ms,等待TM1638上电完成
    TM1638_Init();  // 初始化TM1638
    initial_lcd();  // 初始化JLX12864
    clear_screen(); //clear all dots

    while (1)
    {
        if (clock100ms_flag == 1) // 检查0.1秒定时是否到
        {
            clock100ms_flag = 0;
            // 每0.1秒累加计时值在数码管上以十进制显示，有键按下时暂停计时
            if (key_code == 0)
            {
                if (++test_counter >= 10000)
                    test_counter = 0;
                digit[0] = test_counter / 1000;     // 计算百位数
                digit[1] = test_counter / 100 % 10; // 计算十位数
                digit[2] = test_counter / 10 % 10;  // 计算个位数
                digit[3] = test_counter % 10;       // 计算百分位数
            }
        }

        if (clock500ms_flag == 1) // 检查0.5秒定时是否到
        {
            clock500ms_flag = 0;
            // 8个指示灯以走马灯方式，每0.5秒向右（循环）移动一格
            temp = led[0];
            for (i = 0; i < 7; i++)
                led[i] = led[i + 1];
            led[7] = temp;
        }

        ui_state_proc(ui_state);
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

    GPIOInit();        // GPIO初始化
    SysTickInit();     // 设置SysTick中断
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
    pre_key_code = key_code;          // 保存上一按键值
    key_code = TM1638_Readkeyboard(); // 更新当前按键值

    digit[5] = key_code;

    ENTER_detect();
    LEFT_detect();
    RIGHT_detect();
    INCREASE_detect();
    DECREASE_detect();

    // 5.0秒钟软定时器计数
    if (!key_code && ++NOKEY_clock5s >= V_T5s) // 当无键按下时
    {
        NOKEY_clock5s_flag = 1; // 当5.0秒到时，溢出标志置1
        NOKEY_clock5s = 0;
    }
    // 若有键按下，则5.0秒计数清零
    if (key_code)
    {
        NOKEY_clock5s = 0;
    }

    // ACT4（警示画面）的5.0秒钟软定时器计数
    if (ui_state == 0x4 && ++ACT4_clock5s >= V_T5s) // 当无键按下时
    {
        ACT4_clock5s_flag = 1; // 当5.0秒到时，溢出标志置1
        ACT4_clock5s = 0;
    }
}

/**
 * @brief UI状态机处理函数
 * 
 * @param ui_state UI状态机当前状态
 */
void ui_state_proc(uint16_t ui_state)
{
    switch (ui_state)
    {
    case 0x0: // ACT0
        ui_proc0();
        break;
    case 0x005: //ACT005
        ui_proc005();
        break;
    case 0x100: // ACT100
        ui_proc100();
        break;
    case 0x101: // ACT101
        ui_proc101();
        break;
    case 0x102: // ACT102
        ui_proc102();
        break;
    case 0x201:
        ui_proc201();
        break;
    case 0x202:
        ui_proc202();
        break;
    case 0x203:
        ui_proc203();
        break;
    case 0x301:
        ui_proc301();
        break;
    case 0x303:
        ui_proc303();
        break;
    case 0x305:
        ui_proc305();
        break;
    case 0x306:
        ui_proc306();
        break;
    case 0x4: // ACT4
        ui_proc4();
        break;
    default:
        ui_state = 0x0;
        break;
    }
}

/**
 * @brief UI状态机ACT0状态处理
 * 开机初始画面，不显示光标
 * 
 */
void ui_proc0(void)
{
    uint8_t i = 0;
    // 显示开机初始画面，无光标
    for (i = 0; i < act[0]->SIZE; ++i)
    {
        display_GB2312_string(act[0]->row_page[i], act[0]->col_page[i] * 8 - 7, act[0]->str[i], 0);
    }

    // 当有任意按键被按下："设置"做反白效果（光标），转移至状态ACT005
    if (!pre_key_code && key_code)
    {
        key_LEFT_flag = key_RIGHT_flag = key_INCREASE_flag = key_DECREASE_flag = key_ENTER_flag = 0;
        display_GB2312_string(act[0]->row_page[5], act[0]->col_page[5] * 8 - 7, act[0]->str[5], 1);
        ui_state = 0x005;
    }
}

/**
 * @brief UI状态机ACT005状态处理
 * 光标在"设置"的位置
 * 
 */
void ui_proc005(void)
{
    uint8_t i = 0;
    // 当"确定"键按下：光标移到ACT1的"工作参数"，下一状态ACT100
    if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // 清屏，显示ACT1的画面
        clear_screen();
        for (i = 0; i < act[1]->SIZE; ++i)
        {
            display_GB2312_string(act[1]->row_page[i], act[1]->col_page[i] * 8 - 7, act[1]->str[i], 0);
        }
        // 显示光标
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 1);
        ui_state = 0x100;
    }
    else if (key_LEFT_flag || key_RIGHT_flag || key_INCREASE_flag || key_DECREASE_flag)
    {
        key_LEFT_flag = key_RIGHT_flag = key_DECREASE_flag = key_INCREASE_flag = 0;
    }

    // 当5秒无键操作："设置"反白效果解除，下一状态ACT0
    if (NOKEY_clock5s_flag)
    {
        NOKEY_clock5s_flag = 0;
        display_GB2312_string(act[0]->row_page[5], act[0]->col_page[5] * 8 - 7, act[0]->str[5], 0);
        ui_state = 0x0;
    }
}

/**
 * @brief UI状态机ACT100状态处理
 * 光标在"工作模式"的位置
 * 
 */
void ui_proc100(void)
{
    uint8_t i = 0;
    // 当"左"键按下：光标移到"返回"，下一状态ACT102
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 0);
        display_GB2312_string(act[1]->row_page[2], act[1]->col_page[2] * 8 - 7, act[1]->str[2], 1);
        ui_state = 0x102;
    }
    // 当"右"键按下：光标移到"工作参数"，下一状态ACT101
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 0);
        display_GB2312_string(act[1]->row_page[1], act[1]->col_page[1] * 8 - 7, act[1]->str[1], 1);
        ui_state = 0x101;
    }
    // 当"确定"键按下：光标移到ACT2的"模式#"，下一状态ACT201
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // 清屏，显示ACT2的画面
        clear_screen();
        for (i = 0; i < act[2]->SIZE; ++i)
        {
            display_GB2312_string(act[2]->row_page[i], act[2]->col_page[i] * 8 - 7, act[2]->str[i], 0);
        }
        // 显示光标
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 1);
        ui_state = 0x201;
    }
    else if (key_INCREASE_flag || key_DECREASE_flag)
    {
        key_DECREASE_flag = key_INCREASE_flag = 0;
    }
}

/**
 * @brief UI状态机ACT101状态处理
 * 光标在"工作参数"的位置
 * 
 */
void ui_proc101(void)
{
    uint8_t i = 0;
    // 当"左"键按下：光标移到"工作模式"，下一状态ACT100
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[1]->row_page[1], act[1]->col_page[1] * 8 - 7, act[1]->str[1], 0);
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 1);
        ui_state = 0x100;
    }
    // 当"右"键按下：光标移到"返回"，下一状态ACT102
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[1]->row_page[1], act[1]->col_page[1] * 8 - 7, act[1]->str[1], 0);
        display_GB2312_string(act[1]->row_page[2], act[1]->col_page[2] * 8 - 7, act[1]->str[2], 1);
        ui_state = 0x102;
    }
    // 当"确定"键按下：光标移到ACT3的工作参数个位的位置，下一状态ACT301
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // 清屏，显示ACT3的画面
        clear_screen();
        for (i = 0; i < act[3]->SIZE; ++i)
        {
            display_GB2312_string(act[3]->row_page[i], act[3]->col_page[i] * 8 - 7, act[3]->str[i], 0);
        }
        // 显示光标
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 1);
        ui_state = 0x301;
    }
    else if (key_INCREASE_flag || key_DECREASE_flag)
    {
        key_DECREASE_flag = key_INCREASE_flag = 0;
    }
}

/**
 * @brief UI状态机ACT102状态处理
 * 光标在"返回"的位置
 * 
 */
void ui_proc102(void)
{
    uint8_t i = 0;
    // 当"左"键按下：光标移到"工作参数"，下一状态ACT101
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[1]->row_page[2], act[1]->col_page[2] * 8 - 7, act[1]->str[2], 0);
        display_GB2312_string(act[1]->row_page[1], act[1]->col_page[1] * 8 - 7, act[1]->str[1], 1);
        ui_state = 0x101;
    }
    // 当"右"键按下：光标移到"工作模式"，下一状态ACT100
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[1]->row_page[2], act[1]->col_page[2] * 8 - 7, act[1]->str[2], 0);
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 1);
        ui_state = 0x100;
    }
    // 当"确定"键按下：显示开机初始画面，下一状态ACT0
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // 清屏，显示ACT0的画面
        clear_screen();
        for (i = 0; i < act[0]->SIZE; ++i)
        {
            display_GB2312_string(act[0]->row_page[i], act[0]->col_page[i] * 8 - 7, act[0]->str[i], 0);
        }
        ui_state = 0x0;
    }
    else if (key_INCREASE_flag || key_DECREASE_flag)
    {
        key_DECREASE_flag = key_INCREASE_flag = 0;
    }
}

/**
 * @brief UI状态机ACT201状态处理
 * 光标在"模式#"的位置
 * 
 */
void ui_proc201(void)
{
    // 当"左"键按下：光标移到"取消"，下一状态ACT203
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 0);
        display_GB2312_string(act[2]->row_page[3], act[2]->col_page[3] * 8 - 7, act[2]->str[3], 1);
        ui_state = 0x203;
    }
    // 当"右"键按下：光标移到"确定"，下一状态ACT202
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 0);
        display_GB2312_string(act[2]->row_page[2], act[2]->col_page[2] * 8 - 7, act[2]->str[2], 1);
        ui_state = 0x202;
    }
    // 当"+"键按下："模式#"按A、B、C、A正序循环切换，留在本状态
    else if (key_INCREASE_flag)
    {
        key_INCREASE_flag = 0;
        if (++((act[2]->str[1])[strlen((const char *)(act[2]->str[1])) - 1]) > 'C')
        {
            (act[2]->str[1])[strlen((const char *)(act[2]->str[1])) - 1] = 'A';
        }
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 1);
    }
    // 当"-"键按下："模式#"按C、B、A、C逆序循环切换，留在本状态
    else if (key_DECREASE_flag)
    {
        key_DECREASE_flag = 0;
        if (--((act[2]->str[1])[strlen((const char *)(act[2]->str[1])) - 1]) < 'A')
        {
            (act[2]->str[1])[strlen((const char *)(act[2]->str[1])) - 1] = 'C';
        }
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 1);
    }
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
    }
}

/**
 * @brief UI状态机ACT202状态处理
 * 光标在"确定"的位置
 * 
 */
void ui_proc202(void)
{
    uint8_t i = 0;
    // 当"左"键按下：光标移到"模式#"，下一状态ACT201
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[2]->row_page[2], act[2]->col_page[2] * 8 - 7, act[2]->str[2], 0);
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 1);
        ui_state = 0x201;
    }
    // 当"右"键按下：光标移到"取消"，下一状态ACT203
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[2]->row_page[2], act[2]->col_page[2] * 8 - 7, act[2]->str[2], 0);
        display_GB2312_string(act[2]->row_page[3], act[2]->col_page[3] * 8 - 7, act[2]->str[3], 1);
        ui_state = 0x203;
    }
    // 当"确定"键按下：将当前更改同步到ACT0画面的"模式#"，光标移到ACT1的"工作模式"，下一状态ACT100
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // 将当前更改同步到ACT0画面的"模式#"
        strcpy((char *)(act[0]->str[0]), (const char *)(act[2]->str[1]));
        // 清屏，显示ACT1的画面
        clear_screen();
        for (i = 0; i < act[1]->SIZE; ++i)
        {
            display_GB2312_string(act[1]->row_page[i], act[1]->col_page[i] * 8 - 7, act[1]->str[i], 0);
        }
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 1);
        ui_state = 0x100;
    }
    else if (key_INCREASE_flag || key_DECREASE_flag)
    {
        key_DECREASE_flag = key_INCREASE_flag = 0;
    }
}

/**
 * @brief UI状态机ACT203状态处理
 * 光标在"取消"的位置
 * 
 */
void ui_proc203(void)
{
    uint8_t i = 0;
    // 当"左"键按下：光标移到"确定"，下一状态ACT202
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[2]->row_page[3], act[2]->col_page[3] * 8 - 7, act[2]->str[3], 0);
        display_GB2312_string(act[2]->row_page[2], act[2]->col_page[2] * 8 - 7, act[2]->str[2], 1);
        ui_state = 0x202;
    }
    // 当"右"键按下：光标移到"模式#"，下一状态ACT201
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[2]->row_page[3], act[2]->col_page[3] * 8 - 7, act[2]->str[3], 0);
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 1);
        ui_state = 0x201;
    }
    // 当"确定"键按下：撤销对"模式#"的更改，光标移到ACT1的"工作模式"，下一状态ACT100
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // 撤销对"模式#"的更改
        strcpy((char *)(act[2]->str[1]), (const char *)(act[0]->str[0]));
        // 清屏，显示ACT1的画面，光标移到ACT1的"工作模式"
        clear_screen();
        for (i = 0; i < act[1]->SIZE; ++i)
        {
            display_GB2312_string(act[1]->row_page[i], act[1]->col_page[i] * 8 - 7, act[1]->str[i], 0);
        }
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 1);
        ui_state = 0x100;
    }
    else if (key_INCREASE_flag || key_DECREASE_flag)
    {
        key_DECREASE_flag = key_INCREASE_flag = 0;
    }
}

/**
 * @brief UI状态机ACT301状态处理
 * 光标在工作参数个位的位置
 * 
 */
void ui_proc301(void)
{
    // 当"左"键按下：光标移到"取消"，下一状态ACT306
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 0);
        display_GB2312_string(act[3]->row_page[6], act[3]->col_page[6] * 8 - 7, act[3]->str[6], 1);
        ui_state = 0x306;
    }
    // 当"右"键按下：光标移到工作参数的十分位的位置，下一状态ACT303
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 0);
        display_GB2312_string(act[3]->row_page[3], act[3]->col_page[3] * 8 - 7, act[3]->str[3], 1);
        ui_state = 0x303;
    }
    // 当"+"键按下：个位数按1、2、...、9、0、1、...正序循环切换，留在本状态
    else if (key_INCREASE_flag)
    {
        key_INCREASE_flag = 0;
        if (++((act[3]->str[1])[0]) > '9')
        {
            (act[3]->str[1])[0] = '0';
        }
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 1);
    }
    // 当"-"键按下：个位数按9、8、...、0、9、8、...逆序循环切换，留在本状态
    else if (key_DECREASE_flag)
    {
        key_DECREASE_flag = 0;
        if (--((act[3]->str[1])[0]) < '0')
        {
            (act[3]->str[1])[0] = '9';
        }
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 1);
    }
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
    }
}

/**
 * @brief UI状态机ACT303状态处理
 * 光标在工作参数十分位的位置
 * 
 */
void ui_proc303(void)
{
    // 当"左"键按下：光标移到工作参数个位的位置，下一状态ACT301
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[3]->row_page[3], act[3]->col_page[3] * 8 - 7, act[3]->str[3], 0);
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 1);
        ui_state = 0x301;
    }
    // 当"右"键按下：光标移到"确定"，下一状态ACT305
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[3]->row_page[3], act[3]->col_page[3] * 8 - 7, act[3]->str[3], 0);
        display_GB2312_string(act[3]->row_page[5], act[3]->col_page[5] * 8 - 7, act[3]->str[5], 1);
        ui_state = 0x305;
    }
    // 当"+"键按下：十分位数按1、2、...、9、0、1、...正序循环切换，留在本状态
    else if (key_INCREASE_flag)
    {
        key_INCREASE_flag = 0;
        if (++((act[3]->str[3])[0]) > '9')
        {
            (act[3]->str[3])[0] = '0';
        }
        display_GB2312_string(act[3]->row_page[3], act[3]->col_page[3] * 8 - 7, act[3]->str[3], 1);
    }
    // 当"-"键按下：十分位数按9、8、...、0、9、8、...逆序循环切换，留在本状态
    else if (key_DECREASE_flag)
    {
        key_DECREASE_flag = 0;
        if (--((act[3]->str[3])[0]) < '0')
        {
            (act[3]->str[3])[0] = '9';
        }
        display_GB2312_string(act[3]->row_page[3], act[3]->col_page[3] * 8 - 7, act[3]->str[3], 1);
    }
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
    }
}

/**
 * @brief UI状态机ACT305状态处理
 * 光标在"确定"的位置
 * 
 */
void ui_proc305(void)
{
    // 当"左"键按下：光标移到工作参数十分位的位置，下一状态ACT303
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[3]->row_page[5], act[3]->col_page[5] * 8 - 7, act[3]->str[5], 0);
        display_GB2312_string(act[3]->row_page[3], act[3]->col_page[3] * 8 - 7, act[3]->str[3], 1);
        ui_state = 0x303;
    }
    // 当"右"键按下：光标移到"取消"，下一状态ACT306
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[3]->row_page[5], act[3]->col_page[5] * 8 - 7, act[3]->str[5], 0);
        display_GB2312_string(act[3]->row_page[6], act[3]->col_page[6] * 8 - 7, act[3]->str[6], 1);
        ui_state = 0x306;
    }
    // 当"确定"键按下：检查参数值合法性，若非法，则显示警示画面ACT4，下一状态ACT4；若合法，则将当前更改同步到ACT0画面，光标移到ACT1的"工作参数"，下一状态ACT101
    else if (key_ENTER_flag)
    {
        uint8_t i;
        uint8_t num = ((act[3]->str[1])[0] - '0') * 10 + (act[3]->str[3])[0] - '0';

        key_ENTER_flag = 0;
        // 非法参数：显示警示画面ACT4，下一状态ACT4
        if (num < 10 || num > 90)
        {
            // 显示ACT4的画面，下一状态ACT4
            clear_screen();
            for (i = 0; i < act[4]->SIZE; ++i)
            {
                display_GB2312_string(act[4]->row_page[i], act[4]->col_page[i] * 8 - 7, act[4]->str[i], 0);
            }
            ACT4_clock5s_flag = ACT4_clock5s = 0; // ACT4计数器初始化
            ui_state = 0x4;
        }
        // 合法参数：将当前更改同步到ACT0画面，显示ACT1的画面，光标移到ACT1的"工作参数"，下一状态ACT101
        else
        {
            strcpy((char *)(act[0]->str[1]), (const char *)(act[3]->str[1]));
            strcpy((char *)(act[0]->str[3]), (const char *)(act[3]->str[3]));
            clear_screen();
            for (i = 0; i < act[1]->SIZE; ++i)
            {
                display_GB2312_string(act[1]->row_page[i], act[1]->col_page[i] * 8 - 7, act[1]->str[i], 0);
            }
            display_GB2312_string(act[1]->row_page[1], act[1]->col_page[1] * 8 - 7, act[1]->str[1], 1);
            ui_state = 0x101;
        }
    }
    else if (key_INCREASE_flag || key_DECREASE_flag)
    {
        key_DECREASE_flag = key_INCREASE_flag = 0;
    }
}

/**
 * @brief UI状态机ACT306状态处理
 * 光标在"取消"的位置
 * 
 */
void ui_proc306(void)
{
    // 当"左"键按下：光标移到"确定"，下一状态ACT305
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[3]->row_page[6], act[3]->col_page[6] * 8 - 7, act[3]->str[6], 0);
        display_GB2312_string(act[3]->row_page[5], act[3]->col_page[5] * 8 - 7, act[3]->str[5], 1);
        ui_state = 0x305;
    }
    // 当"右"键按下：光标移到工作参数个位的位置，下一状态ACT301
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[3]->row_page[6], act[3]->col_page[6] * 8 - 7, act[3]->str[6], 0);
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 1);
        ui_state = 0x301;
    }
    // 当"确定"键按下：撤销对工作参数的更改，光标移到ACT1的"工作参数"，下一状态ACT101
    else if (key_ENTER_flag)
    {
        uint8_t i = 0;

        key_ENTER_flag = 0;
        // 撤销对工作参数的更改
        strcpy((char *)(act[3]->str[1]), (const char *)(act[0]->str[1]));
        strcpy((char *)(act[3]->str[3]), (const char *)(act[0]->str[3]));
        // 显示ACT1的画面，光标移到ACT1的"工作参数"，下一状态ACT101
        clear_screen();
        for (i = 0; i < act[1]->SIZE; ++i)
        {
            display_GB2312_string(act[1]->row_page[i], act[1]->col_page[i] * 8 - 7, act[1]->str[i], 0);
        }
        display_GB2312_string(act[1]->row_page[1], act[1]->col_page[1] * 8 - 7, act[1]->str[1], 1);
        ui_state = 0x101;
    }
    else if (key_INCREASE_flag || key_DECREASE_flag)
    {
        key_DECREASE_flag = key_INCREASE_flag = 0;
    }
}

/**
 * @brief UI状态机ACT4状态处理
 * 显示警示画面，维持5秒
 * 
 */
void ui_proc4(void)
{
    uint8_t i = 0;
    // 五秒时间到：显示ACT3的界面，光标移到ACT3的工作参数个位的位置，下一状态ACT301
    if (ACT4_clock5s_flag)
    {
        ACT4_clock5s_flag = 0;
        // 光标移到ACT3的工作参数个位的位置，下一状态ACT301
        clear_screen();
        for (i = 0; i < act[3]->SIZE; ++i)
        {
            display_GB2312_string(act[3]->row_page[i], act[3]->col_page[i] * 8 - 7, act[3]->str[i], 0);
        }
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 1);
        ui_state = 0x301;
    }

    if (key_ENTER_flag || key_LEFT_flag || key_RIGHT_flag || key_INCREASE_flag || key_DECREASE_flag)
    {
        key_ENTER_flag = key_LEFT_flag = key_RIGHT_flag = key_DECREASE_flag = key_INCREASE_flag = 0;
    }
}

/**
 * @brief "确定"键按键检测
 * 
 */
void ENTER_detect(void)
{
    if (pre_key_code != 5 && key_code == 5)
    {
        key_ENTER_flag = 1;
    }
}

/**
 * @brief "+"键按键检测
 * 
 */
void INCREASE_detect(void)
{
    if (pre_key_code != 2 && key_code == 2)
    {
        key_INCREASE_flag = 1;
    }
}

/**
 * @brief "-"键按键检测
 * 
 */
void DECREASE_detect(void)
{
    if (pre_key_code != 8 && key_code == 8)
    {
        key_DECREASE_flag = 1;
    }
}

/**
 * @brief "左"键按键检测
 * 
 */
void LEFT_detect(void)
{
    if (pre_key_code != 4 && key_code == 4)
    {
        key_LEFT_flag = 1;
    }
}

/**
 * @brief "右"键按键检测
 * 
 */
void RIGHT_detect(void)
{
    if (pre_key_code != 6 && key_code == 6)
    {
        key_RIGHT_flag = 1;
    }
}
