/**
 * @file exp4_2.c
 * @author �Ϻ���ͨ��ѧ���ӹ���ϵʵ���ѧ����; Guorui Wei (313017602@qq.com)
 * @brief ʵ��4_2
 * @version 0.1
 * @date 2021-06-05
 * 
 * @copyright 2020-2021, �Ϻ���ͨ��ѧ���ӹ���ϵʵ���ѧ����
 * 
 */

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
#include "JLX12864.h"            // �����JLX12864G�йصĺ���
#include "tm1638.h"              // �����TM1638оƬ�йصĺ���
#include "string.h"              //

//*****************************************************************************
//
// �궨��
//
//*****************************************************************************
#define SYSTICK_FREQUENCY 50  // SysTickƵ��Ϊ50Hz����ѭ����ʱ����20ms
#define V_T100ms 5            // 0.1s�����ʱ�����ֵ��5��20ms
#define V_T500ms 25           // 0.5s�����ʱ�����ֵ��25��20ms
#define V_T2s 100             // 2.0s��ʱ�����ֵ��100��20ms
#define V_T10s 500            // 10.0s��ʱ�����ֵ��500��20ms
#define LCD_MAX_BLOCK 15      // ��ʾ���ϵ����8x8������
#define LCD_MAX_BLOCK_CHAR 15 // ��ʾ����ÿ������������ַ���

//*****************************************************************************
//
// ����ԭ������
//
//*****************************************************************************
void GPIOInit(void);    // GPIO��ʼ��
void SysTickInit(void); // ����SysTick�ж�
void DevicesInit(void); // MCU������ʼ����ע���������������

/** 
 * UI״̬����غ���
 */

void ui_state_proc(uint16_t ui_state);
void ui_proc0(void);
void ui_proc001(void);
void ui_proc003(void);
void ui_proc005(void);
void ENTER_detect(void);
void LEFT_detect(void);
void RIGHT_detect(void);
void INCREASE_detect(void);
void DECREASE_detect(void);

//*****************************************************************************
//
// ��������
//
//*****************************************************************************

// �����ʱ������
uint8_t clock100ms = 0;
uint8_t clock500ms = 0;
uint8_t clock2s = 0;
uint16_t NOKEY_clock10s = 0;

// �����ʱ�������־
uint8_t clock100ms_flag = 0;
uint8_t clock500ms_flag = 0;
uint8_t clock2s_flag = 0;
uint8_t NOKEY_clock10s_flag = 0;

/**
 * �����¼���־
 */
uint8_t key_LEFT_flag = 0;
uint8_t key_RIGHT_flag = 0;
uint8_t key_INCREASE_flag = 0;
uint8_t key_DECREASE_flag = 0;
uint8_t key_ENTER_flag = 0;

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
uint8_t pre_key_code; // ��һ����ֵ

// ϵͳʱ��Ƶ��
uint32_t ui32SysClock;

/**
 * �û����棨UI��״̬����ر�������
 */

uint16_t ui_state = 0x0; // �û����棨UI��״̬����ǰ״̬

/**
 * @brief ״̬�����ṹ��
 * 
 */
struct ACT_T
{
    uint8_t row_page[LCD_MAX_BLOCK];                      // ��ʾ����ÿ����������ʼ��ҳ��
    uint8_t col_page[LCD_MAX_BLOCK];                      // ��ʾ����ÿ����������ʼ��ҳ��
    unsigned char str[LCD_MAX_BLOCK][LCD_MAX_BLOCK_CHAR]; // ��ʾ����ÿ����������ʾ����
    const uint8_t SIZE;                                   // ��ʾ������Ч����������
};

struct ACT_T act0 = {
    {3, 3, 5, 5, 5, 5, 5},
    {1, 11, 1, 11, 12, 13, 14},
    {"����ģʽ��", "ģʽA", "����������", "1", ".", "1", "Hz"},
    7};

struct ACT_T *act[] = {&act0};

//*****************************************************************************
//
// ������
//
//*****************************************************************************
int main(void)
{
    uint8_t temp, i;
    DevicesInit(); //  MCU������ʼ��

    while (clock100ms < 3)
        ;           // ��ʱ>60ms,�ȴ�TM1638�ϵ����
    TM1638_Init();  // ��ʼ��TM1638
    initial_lcd();  // ��ʼ��JLX12864
    clear_screen(); //clear all dots

    while (1)
    {
        if (clock100ms_flag == 1) // ���0.1�붨ʱ�Ƿ�
        {
            clock100ms_flag = 0;
            // ÿ0.1���ۼӼ�ʱֵ�����������ʮ������ʾ���м�����ʱ��ͣ��ʱ
            if (key_code == 0)
            {
                if (++test_counter >= 10000)
                    test_counter = 0;
                digit[0] = test_counter / 1000;     // �����λ��
                digit[1] = test_counter / 100 % 10; // ����ʮλ��
                digit[2] = test_counter / 10 % 10;  // �����λ��
                digit[3] = test_counter % 10;       // ����ٷ�λ��
            }
        }

        if (clock500ms_flag == 1) // ���0.5�붨ʱ�Ƿ�
        {
            clock500ms_flag = 0;
            // 8��ָʾ��������Ʒ�ʽ��ÿ0.5�����ң�ѭ�����ƶ�һ��
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
    pre_key_code = key_code;          // ������һ����ֵ
    key_code = TM1638_Readkeyboard(); // ���µ�ǰ����ֵ

    digit[5] = key_code;

    ENTER_detect();
    LEFT_detect();
    RIGHT_detect();
    INCREASE_detect();
    DECREASE_detect();

    // 10.0������ʱ������
    if (!key_code && ++NOKEY_clock10s >= V_T10s) // ���޼�����ʱ
    {
        NOKEY_clock10s_flag = 1; // ��10.0�뵽ʱ�������־��1
        NOKEY_clock10s = 0;
    }
    // ���м����£���10.0���������
    if (key_code)
    {
        NOKEY_clock10s = 0;
    }
}

/**
 * @brief UI״̬��������
 * 
 * @param ui_state UI״̬����ǰ״̬
 */
void ui_state_proc(uint16_t ui_state)
{
    switch (ui_state)
    {
    case 0x0: // ACT0
        ui_proc0();
        break;
    case 0x001: //ACT001
        ui_proc001();
        break;
    case 0x003: // ACT003
        ui_proc003();
        break;
    case 0x005: // ACT005
        ui_proc005();
        break;
    default:
        ui_state = 0;
        break;
    }
}

/**
 * @brief UI״̬��ACT0״̬����
 * ������ʼ���棬����ʾ���
 * 
 */
void ui_proc0(void)
{
    uint8_t i = 0;
    // ��ʾ������ʼ���棬�޹��
    for (i = 0; i < act[0]->SIZE; ++i)
    {
        display_GB2312_string(act[0]->row_page[i], act[0]->col_page[i] * 8 - 7, act[0]->str[i], 0);
    }

    // �������ⰴ�������£�"ģʽ#"������Ч������꣩��ת����״̬ACT001
    if (!pre_key_code && key_code)
    {
        key_LEFT_flag = key_RIGHT_flag = key_INCREASE_flag = key_DECREASE_flag = key_ENTER_flag = 0;
        display_GB2312_string(act[0]->row_page[1], act[0]->col_page[1] * 8 - 7, act[0]->str[1], 1);
        ui_state = 0x001;
    }
}

/**
 * @brief UI״̬��ACT001״̬����
 * ����ڹ���ģʽѡ��λ��
 * 
 */
void ui_proc001(void)
{
    // ��"��"�����£�����Ƶ����������ĸ�λ����һ״̬ACT003
    if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[0]->row_page[1], act[0]->col_page[1] * 8 - 7, act[0]->str[1], 0);
        display_GB2312_string(act[0]->row_page[3], act[0]->col_page[3] * 8 - 7, act[0]->str[3], 1);
        ui_state = 0x003;
    }
    // ��"��"�����£�����Ƶ�����������ʮ��λ����һ״̬ACT005
    else if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[0]->row_page[1], act[0]->col_page[1] * 8 - 7, act[0]->str[1], 0);
        display_GB2312_string(act[0]->row_page[5], act[0]->col_page[5] * 8 - 7, act[0]->str[5], 1);
        ui_state = 0x005;
    }
    // ��"+"�����£�"ģʽ#"��A��B��C��A����ѭ���л������ڱ�״̬
    else if (key_INCREASE_flag)
    {
        key_INCREASE_flag = 0;
        if (++((act[0]->str[1])[strlen((const char *)(act[0]->str[1])) - 1]) > 'C')
        {
            (act[0]->str[1])[strlen((const char *)(act[0]->str[1])) - 1] = 'A';
        }
        display_GB2312_string(act[0]->row_page[1], act[0]->col_page[1] * 8 - 7, act[0]->str[1], 1);
    }
    // ��"-"�����£�"ģʽ#"��C��B��A��C����ѭ���л������ڱ�״̬
    else if (key_DECREASE_flag)
    {
        key_DECREASE_flag = 0;
        if (--((act[0]->str[1])[strlen((const char *)(act[0]->str[1])) - 1]) < 'A')
        {
            (act[0]->str[1])[strlen((const char *)(act[0]->str[1])) - 1] = 'C';
        }
        display_GB2312_string(act[0]->row_page[1], act[0]->col_page[1] * 8 - 7, act[0]->str[1], 1);
    }

    // ��10���޲�����"ģʽ#"����Ч���������һ״̬ACT0
    if (NOKEY_clock10s_flag)
    {
        NOKEY_clock10s_flag = 0;
        display_GB2312_string(act[0]->row_page[1], act[0]->col_page[1] * 8 - 7, act[0]->str[1], 0);
        ui_state = 0x0;
    }
}

/**
 * @brief UI״̬��ACT003״̬����
 * ����ڹ���������λ��λ��
 * 
 */
void ui_proc003(void)
{
    // ��"��"�����£�����Ƶ�ʮ��λ����һ״̬ACT005
    if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[0]->row_page[3], act[0]->col_page[3] * 8 - 7, act[0]->str[3], 0);
        display_GB2312_string(act[0]->row_page[5], act[0]->col_page[5] * 8 - 7, act[0]->str[5], 1);
        ui_state = 0x005;
    }
    // ��"��"�����£�����Ƶ�"ģʽ#"����һ״̬ACT001
    else if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[0]->row_page[3], act[0]->col_page[3] * 8 - 7, act[0]->str[3], 0);
        display_GB2312_string(act[0]->row_page[1], act[0]->col_page[1] * 8 - 7, act[0]->str[1], 1);
        ui_state = 0x001;
    }
    // ��"+"�����£���λ����1��2��...��9��0��1��...����ѭ���л������ڱ�״̬
    else if (key_INCREASE_flag)
    {
        key_INCREASE_flag = 0;
        if (++((act[0]->str[3])[0]) > '9')
        {
            (act[0]->str[3])[0] = '0';
        }
        display_GB2312_string(act[0]->row_page[3], act[0]->col_page[3] * 8 - 7, act[0]->str[3], 1);
    }
    // ��"-"�����£���λ����9��8��...��0��9��8��...����ѭ���л������ڱ�״̬
    else if (key_DECREASE_flag)
    {
        key_DECREASE_flag = 0;
        if (--((act[0]->str[3])[0]) < '0')
        {
            (act[0]->str[3])[0] = '9';
        }
        display_GB2312_string(act[0]->row_page[3], act[0]->col_page[3] * 8 - 7, act[0]->str[3], 1);
    }
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
    }

    // ��10���޲�������λ����Ч���������һ״̬ACT0
    if (NOKEY_clock10s_flag)
    {
        NOKEY_clock10s_flag = 0;
        display_GB2312_string(act[0]->row_page[3], act[0]->col_page[3] * 8 - 7, act[0]->str[3], 0);
        ui_state = 0x0;
    }
}

/**
 * @brief UI״̬��ACT005״̬����
 * ����ڹ�������ʮ��λ��λ��
 * 
 */
void ui_proc005(void)
{
    // ��"��"�����£�����Ƶ�"ģʽ#"����һ״̬ACT001
    if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[0]->row_page[5], act[0]->col_page[5] * 8 - 7, act[0]->str[5], 0);
        display_GB2312_string(act[0]->row_page[1], act[0]->col_page[1] * 8 - 7, act[0]->str[1], 1);
        ui_state = 0x001;
    }
    // ��"��"�����£�����Ƶ���λ����һ״̬ACT003
    else if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[0]->row_page[5], act[0]->col_page[5] * 8 - 7, act[0]->str[5], 0);
        display_GB2312_string(act[0]->row_page[3], act[0]->col_page[3] * 8 - 7, act[0]->str[3], 1);
        ui_state = 0x003;
    }
    // ��"+"�����£�ʮ��λ����1��2��...��9��0��1��...����ѭ���л������ڱ�״̬
    else if (key_INCREASE_flag)
    {
        key_INCREASE_flag = 0;
        if (++((act[0]->str[5])[0]) > '9')
        {
            (act[0]->str[5])[0] = '0';
        }
        display_GB2312_string(act[0]->row_page[5], act[0]->col_page[5] * 8 - 7, act[0]->str[5], 1);
    }
    // ��"-"�����£�ʮ��λ����9��8��...��0��9��8��...����ѭ���л������ڱ�״̬
    else if (key_DECREASE_flag)
    {
        key_DECREASE_flag = 0;
        if (--((act[0]->str[5])[0]) < '0')
        {
            (act[0]->str[5])[0] = '9';
        }
        display_GB2312_string(act[0]->row_page[5], act[0]->col_page[5] * 8 - 7, act[0]->str[5], 1);
    }
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
    }

    // ��10���޲�������������ʮ��λ����Ч���������һ״̬ACT0
    if (NOKEY_clock10s_flag)
    {
        NOKEY_clock10s_flag = 0;
        display_GB2312_string(act[0]->row_page[5], act[0]->col_page[5] * 8 - 7, act[0]->str[5], 0);
        ui_state = 0x0;
    }
}

/**
 * @brief "ȷ��"���������
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
 * @brief "+"���������
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
 * @brief "-"���������
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
 * @brief "��"���������
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
 * @brief "��"���������
 * 
 */
void RIGHT_detect(void)
{
    if (pre_key_code != 6 && key_code == 6)
    {
        key_RIGHT_flag = 1;
    }
}
