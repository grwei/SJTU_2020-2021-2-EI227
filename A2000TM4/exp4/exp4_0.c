//*****************************************************************************
//
// Copyright: 2020-2021, �Ϻ���ͨ��ѧ���ӹ���ϵʵ���ѧ����
// File name: exp4_0.c
// Description:
//    1.������λ�󣬵װ����ұ�4λ������Զ���ʾ��ʱ��ֵ�����λ��Ӧ��λ��0.1�룻
//    2.������λ�󣬵װ���8��LED�����������ʽ��������ѭ���任��Լ0.5��任1�Σ�
//    3.��û�а�������ʱ����������ڶ�λ�������ʾ��0����
//      ���˹�����ĳ�����������ʾ�ü��ı�ţ�
//      �˿���λ��ʱ�������ͣ�仯��ֹͣ��ʱ��ֱ���ſ��������Զ�������ʱ��
// Author:	�Ϻ���ͨ��ѧ���ӹ���ϵʵ���ѧ����
// Version: 1.0.0.20201228
// Date��2020-12-28
// History��
//
//*****************************************************************************

//*****************************************************************************
//
// ͷ�ļ�
//
//*****************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"       // ��ַ�궨��
#include "inc/hw_types.h"        // �������ͺ궨�壬�Ĵ������ʺ���
#include "driverlib/debug.h"     // ������
#include "driverlib/gpio.h"      // ͨ��IO�ں궨��
#include "driverlib/pin_map.h"   // TM4Cϵ��MCU��Χ�豸�ܽź궨��
#include "driverlib/sysctl.h"    // ϵͳ���ƶ���
#include "driverlib/systick.h"   // SysTick Driver ԭ��
#include "driverlib/interrupt.h" // NVIC Interrupt Controller Driver ԭ��
#include "JLX12864.h"
#include "tm1638.h" // �����TM1638оƬ�йصĺ���
//*****************************************************************************
//
// �궨��
//
//*****************************************************************************
#define SYSTICK_FREQUENCY 50 // SysTickƵ��Ϊ50Hz����ѭ����ʱ����20ms

#define V_T100ms 5  // 0.1s�����ʱ�����ֵ��5��20ms
#define V_T500ms 25 // 0.5s�����ʱ�����ֵ��25��20ms
#define V_T2s 100   // 2.0s��ʱ�����ֵ��100��20ms

//*****************************************************************************
//
// ����ԭ������
//
//*****************************************************************************
void GPIOInit(void);    // GPIO��ʼ��
void SysTickInit(void); // ����SysTick�ж�
void DevicesInit(void); // MCU������ʼ����ע���������������
//*****************************************************************************
//
// ��������
//
//*****************************************************************************

// �����ʱ������
uint8_t clock100ms = 0;
uint8_t clock500ms = 0;
uint8_t clock2s = 0;

// �����ʱ�������־
uint8_t clock100ms_flag = 0;
uint8_t clock500ms_flag = 0;
uint8_t clock2s_flag = 0;

// �����ü�����
uint32_t test_counter = 0;

// 8λ�������ʾ�����ֻ���ĸ����
// ע����������λ�������������Ϊ4��5��6��7��0��1��2��3
uint8_t digit[8] = {' ', ' ', ' ', ' ', '_', ' ', '_', ' '};

// 8λС���� 1��  0��
// ע����������λС����������������Ϊ4��5��6��7��0��1��2��3
uint8_t pnt = 0x04;

// 8��LEDָʾ��״̬��0��1��
// ע������ָʾ�ƴ������������Ϊ7��6��5��4��3��2��1��0
//     ��ӦԪ��LED8��LED7��LED6��LED5��LED4��LED3��LED2��LED1
uint8_t led[] = {1, 1, 1, 1, 1, 1, 1, 0};

// ��ǰ����ֵ
uint8_t key_code = 0;

// ϵͳʱ��Ƶ��
uint32_t ui32SysClock;

//*****************************************************************************
//
// ������
//
//*****************************************************************************
int main(void)
{
    DevicesInit(); //  MCU������ʼ��
    while (clock100ms < 3)
        ;           // ��ʱ>60ms,�ȴ�TM1638�ϵ����
    TM1638_Init();  // ��ʼ��TM1638
    initial_lcd();  // ��ʼ��JLX12864
    clear_screen(); //clear all dots

    while (1)
    {
        display_128x64(xiaohui);
        LCD_delay(200000);
        clear_screen();

        display_GB2312_string(1, 1, "12864,�������ֿ�", false);  //�ڵ� 1 ҳ���� 1 �У���ʾһ�� 16x16 �����ֻ� 8x16 �� ASCII ��
        display_GB2312_string(3, 1, "16X16 ���庺�ֿ�,", false); //��ʾһ�� 16x16 �����ֻ� 8x16 �� ASCII ��.������ͬ
        display_GB2312_string(5, 1, "�� 8X16 ���� ASCII,", false);
        display_GB2312_string(7, 1, "�� 5x8 ���� ASCII ��", false);
        LCD_delay(4000);
        clear_screen();

        display_GB2312_string(1, 1, "����Ѷ�����ڶ���", true);
        display_GB2312_string(3, 1, "������ʮһ������", true);
        display_GB2312_string(5, 1, "��Ҫ����Һ��ģ��", true);
        display_GB2312_string(7, 1, "Ʒ��������Ϸ���", true);
        LCD_delay(4000);
        clear_screen();

        display_GB2312_string(1, 1, "GB2312 �����ֿ⼰", true);
        display_GB2312_string(3, 1, "��ͼ�͹��ܣ�����", false);
        display_GB2312_string(5, 1, "����ֻ�ͼ�����", true);
        display_GB2312_string(7, 1, "Ƨ�֣����磺 ", false);
        display_graphic_16x16(7, 97, jiong1); //�ڵ� 7 ҳ���� 81 ����ʾ�����Ա���Ƨ���֡��塱
        display_graphic_16x16(7, 113, lei1);  //��ʾ�����Ա���Ƨ����"����
        LCD_delay(4000);
        clear_screen();

        display_GB2312_string(1, 1, "<!@#$%^&*()_-+]/", false);   //�ڵ� 1 ҳ���� 1 �У���ʾһ�� 16x16 �����ֻ� 8*16 �� ASCII ��
        display_string_5x8(3, 1, "<!@#$%^&*()_-+]/;.,?[", false); //�ڵ� 3 ҳ���� 1 �У���ʾһ�� 5x8 ����� ASCII ��
        display_string_5x8(4, 1, "JLX electronics Co., ", false); //��ʾһ�� 5x8 ����� ASCII ��
        display_string_5x8(5, 1, "Ltd. established at ", false);  //��ʾһ�� 5x8 ����� ASCII ��
        display_string_5x8(6, 1, "year 2004.Focus LCM. ", false); //��ʾһ�� 5x8 ����� ASCII ��
        display_string_5x8(7, 1, "TEL:0755-29784961 ", false);    //��ʾһ�� 5x8 ����� ASCII ��
        display_string_5x8(8, 1, "FAX:0755-29784964 ", false);    //��ʾһ�� 5x8 ����� ASCII ��
        LCD_delay(4000);
        clear_screen();

        display_GB2312_string(1, 1, "����������������", false);
        display_GB2312_string(3, 1, "����������������", false);
        display_GB2312_string(5, 1, "����������������", false);
        display_GB2312_string(7, 1, "����������������", false);
        LCD_delay(4000);
        clear_screen();
    }
}

//*****************************************************************************
//
// ����ԭ�ͣ�void GPIOInit(void)
// �������ܣ�GPIO��ʼ����ʹ��PortK������PK4,PK5Ϊ�����ʹ��PortM������PM0Ϊ�����
//          ��PK4����TM1638��STB��PK5����TM1638��DIO��PM0����TM1638��CLK��
// ������������
// ��������ֵ����
//
//*****************************************************************************
void GPIOInit(void)
{
    //����TM1638оƬ�ܽ�
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK); // ʹ�ܶ˿� K
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK))
    {
    }; // �ȴ��˿� K׼�����

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM); // ʹ�ܶ˿� M
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOM))
    {
    }; // �ȴ��˿� M׼�����

    // ���ö˿� K�ĵ�4,5λ��PK4,PK5��Ϊ�������		PK4-STB  PK5-DIO
    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    // ���ö˿� M�ĵ�0λ��PM0��Ϊ�������   PM0-CLK
    GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, GPIO_PIN_0);
}

//*****************************************************************************
//
// ����ԭ�ͣ�SysTickInit(void)
// �������ܣ�����SysTick�ж�
// ������������
// ��������ֵ����
//
//*****************************************************************************
void SysTickInit(void)
{
    SysTickPeriodSet(ui32SysClock / SYSTICK_FREQUENCY); // ������������,��ʱ����20ms
    SysTickEnable();                                    // SysTickʹ��
    SysTickIntEnable();                                 // SysTick�ж�����
}

//*****************************************************************************
//
// ����ԭ�ͣ�void DevicesInit(void)
// �������ܣ�CU������ʼ��������ϵͳʱ�����á�GPIO��ʼ����SysTick�ж�����
// ������������
// ��������ֵ����
//
//*****************************************************************************
void DevicesInit(void)
{
    // ʹ���ⲿ25MHz��ʱ��Դ������PLL��Ȼ���ƵΪ20MHz
    ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
                                       SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
                                      20000000);

    GPIOInit();        // GPIO��ʼ��
    SysTickInit();     // ����SysTick�ж�
    IntMasterEnable(); // ���ж�����
}

//*****************************************************************************
//
// ����ԭ�ͣ�void SysTick_Handler(void)
// �������ܣ�SysTick�жϷ������
// ������������
// ��������ֵ����
//
//*****************************************************************************
void SysTick_Handler(void) // ��ʱ����Ϊ20ms
{
    // 0.1������ʱ������
    if (++clock100ms >= V_T100ms)
    {
        clock100ms_flag = 1; // ��0.1�뵽ʱ�������־��1
        clock100ms = 0;
    }

    // 0.5������ʱ������
    if (++clock500ms >= V_T500ms)
    {
        clock500ms_flag = 1; // ��0.5�뵽ʱ�������־��1
        clock500ms = 0;
    }

    // ˢ��ȫ������ܺ�LEDָʾ��
    TM1638_RefreshDIGIandLED(digit, pnt, led);

    // ��鵱ǰ�������룬0�����޼�������1-9��ʾ�ж�Ӧ����
    // ������ʾ��һλ�������
    key_code = TM1638_Readkeyboard();

    digit[5] = key_code;
}
