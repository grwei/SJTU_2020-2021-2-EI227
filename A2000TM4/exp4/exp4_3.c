/**
 * @file exp4_3.c
 * @author �Ϻ���ͨ��ѧ���ӹ���ϵʵ���ѧ����; Guorui Wei (313017602@qq.com)
 * @brief ʵ��4_3
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
#define V_T5s 250             // 5.0s��ʱ�����ֵ��250��20ms
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
// ��������
//
//*****************************************************************************

// �����ʱ������
uint8_t clock100ms = 0;
uint8_t clock500ms = 0;
uint8_t clock2s = 0;
uint8_t NOKEY_clock5s = 0;
uint8_t ACT4_clock5s = 0;

// �����ʱ�������־
uint8_t clock100ms_flag = 0;
uint8_t clock500ms_flag = 0;
uint8_t clock2s_flag = 0;
uint8_t NOKEY_clock5s_flag = 0;
uint8_t ACT4_clock5s_flag = 0;

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
    {3, 3, 3, 3, 3, 7},
    {3, 11, 12, 13, 14, 1},
    {"ģʽA", "1", ".", "1", "Hz", "����"},
    6};

struct ACT_T act1 = {
    {3, 5, 7},
    {3, 3, 13},
    {"����ģʽ", "��������", "����"},
    3};

struct ACT_T act2 = {
    {3, 3, 7, 7},
    {1, 11, 1, 13},
    {"����ģʽ��", "ģʽA", "ȷ��", "ȡ��"},
    4};

struct ACT_T act3 = {
    {3, 3, 3, 3, 3, 7, 7},
    {1, 11, 12, 13, 14, 1, 13},
    {"����������", "1", ".", "1", "Hz", "ȷ��", "ȡ��"},
    7};

struct ACT_T act4 = {
    {3},
    {1},
    {"�����������Ϸ�"},
    1};

struct ACT_T *act[] = {&act0, &act1, &act2, &act3, &act4};

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

    // 5.0������ʱ������
    if (!key_code && ++NOKEY_clock5s >= V_T5s) // ���޼�����ʱ
    {
        NOKEY_clock5s_flag = 1; // ��5.0�뵽ʱ�������־��1
        NOKEY_clock5s = 0;
    }
    // ���м����£���5.0���������
    if (key_code)
    {
        NOKEY_clock5s = 0;
    }

    // ACT4����ʾ���棩��5.0������ʱ������
    if (ui_state == 0x4 && ++ACT4_clock5s >= V_T5s) // ���޼�����ʱ
    {
        ACT4_clock5s_flag = 1; // ��5.0�뵽ʱ�������־��1
        ACT4_clock5s = 0;
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

    // �������ⰴ�������£�"����"������Ч������꣩��ת����״̬ACT005
    if (!pre_key_code && key_code)
    {
        key_LEFT_flag = key_RIGHT_flag = key_INCREASE_flag = key_DECREASE_flag = key_ENTER_flag = 0;
        display_GB2312_string(act[0]->row_page[5], act[0]->col_page[5] * 8 - 7, act[0]->str[5], 1);
        ui_state = 0x005;
    }
}

/**
 * @brief UI״̬��ACT005״̬����
 * �����"����"��λ��
 * 
 */
void ui_proc005(void)
{
    uint8_t i = 0;
    // ��"ȷ��"�����£�����Ƶ�ACT1��"��������"����һ״̬ACT100
    if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // ��������ʾACT1�Ļ���
        clear_screen();
        for (i = 0; i < act[1]->SIZE; ++i)
        {
            display_GB2312_string(act[1]->row_page[i], act[1]->col_page[i] * 8 - 7, act[1]->str[i], 0);
        }
        // ��ʾ���
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 1);
        ui_state = 0x100;
    }
    else if (key_LEFT_flag || key_RIGHT_flag || key_INCREASE_flag || key_DECREASE_flag)
    {
        key_LEFT_flag = key_RIGHT_flag = key_DECREASE_flag = key_INCREASE_flag = 0;
    }

    // ��5���޲�����"����"����Ч���������һ״̬ACT0
    if (NOKEY_clock5s_flag)
    {
        NOKEY_clock5s_flag = 0;
        display_GB2312_string(act[0]->row_page[5], act[0]->col_page[5] * 8 - 7, act[0]->str[5], 0);
        ui_state = 0x0;
    }
}

/**
 * @brief UI״̬��ACT100״̬����
 * �����"����ģʽ"��λ��
 * 
 */
void ui_proc100(void)
{
    uint8_t i = 0;
    // ��"��"�����£�����Ƶ�"����"����һ״̬ACT102
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 0);
        display_GB2312_string(act[1]->row_page[2], act[1]->col_page[2] * 8 - 7, act[1]->str[2], 1);
        ui_state = 0x102;
    }
    // ��"��"�����£�����Ƶ�"��������"����һ״̬ACT101
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 0);
        display_GB2312_string(act[1]->row_page[1], act[1]->col_page[1] * 8 - 7, act[1]->str[1], 1);
        ui_state = 0x101;
    }
    // ��"ȷ��"�����£�����Ƶ�ACT2��"ģʽ#"����һ״̬ACT201
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // ��������ʾACT2�Ļ���
        clear_screen();
        for (i = 0; i < act[2]->SIZE; ++i)
        {
            display_GB2312_string(act[2]->row_page[i], act[2]->col_page[i] * 8 - 7, act[2]->str[i], 0);
        }
        // ��ʾ���
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 1);
        ui_state = 0x201;
    }
    else if (key_INCREASE_flag || key_DECREASE_flag)
    {
        key_DECREASE_flag = key_INCREASE_flag = 0;
    }
}

/**
 * @brief UI״̬��ACT101״̬����
 * �����"��������"��λ��
 * 
 */
void ui_proc101(void)
{
    uint8_t i = 0;
    // ��"��"�����£�����Ƶ�"����ģʽ"����һ״̬ACT100
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[1]->row_page[1], act[1]->col_page[1] * 8 - 7, act[1]->str[1], 0);
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 1);
        ui_state = 0x100;
    }
    // ��"��"�����£�����Ƶ�"����"����һ״̬ACT102
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[1]->row_page[1], act[1]->col_page[1] * 8 - 7, act[1]->str[1], 0);
        display_GB2312_string(act[1]->row_page[2], act[1]->col_page[2] * 8 - 7, act[1]->str[2], 1);
        ui_state = 0x102;
    }
    // ��"ȷ��"�����£�����Ƶ�ACT3�Ĺ���������λ��λ�ã���һ״̬ACT301
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // ��������ʾACT3�Ļ���
        clear_screen();
        for (i = 0; i < act[3]->SIZE; ++i)
        {
            display_GB2312_string(act[3]->row_page[i], act[3]->col_page[i] * 8 - 7, act[3]->str[i], 0);
        }
        // ��ʾ���
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 1);
        ui_state = 0x301;
    }
    else if (key_INCREASE_flag || key_DECREASE_flag)
    {
        key_DECREASE_flag = key_INCREASE_flag = 0;
    }
}

/**
 * @brief UI״̬��ACT102״̬����
 * �����"����"��λ��
 * 
 */
void ui_proc102(void)
{
    uint8_t i = 0;
    // ��"��"�����£�����Ƶ�"��������"����һ״̬ACT101
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[1]->row_page[2], act[1]->col_page[2] * 8 - 7, act[1]->str[2], 0);
        display_GB2312_string(act[1]->row_page[1], act[1]->col_page[1] * 8 - 7, act[1]->str[1], 1);
        ui_state = 0x101;
    }
    // ��"��"�����£�����Ƶ�"����ģʽ"����һ״̬ACT100
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[1]->row_page[2], act[1]->col_page[2] * 8 - 7, act[1]->str[2], 0);
        display_GB2312_string(act[1]->row_page[0], act[1]->col_page[0] * 8 - 7, act[1]->str[0], 1);
        ui_state = 0x100;
    }
    // ��"ȷ��"�����£���ʾ������ʼ���棬��һ״̬ACT0
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // ��������ʾACT0�Ļ���
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
 * @brief UI״̬��ACT201״̬����
 * �����"ģʽ#"��λ��
 * 
 */
void ui_proc201(void)
{
    // ��"��"�����£�����Ƶ�"ȡ��"����һ״̬ACT203
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 0);
        display_GB2312_string(act[2]->row_page[3], act[2]->col_page[3] * 8 - 7, act[2]->str[3], 1);
        ui_state = 0x203;
    }
    // ��"��"�����£�����Ƶ�"ȷ��"����һ״̬ACT202
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 0);
        display_GB2312_string(act[2]->row_page[2], act[2]->col_page[2] * 8 - 7, act[2]->str[2], 1);
        ui_state = 0x202;
    }
    // ��"+"�����£�"ģʽ#"��A��B��C��A����ѭ���л������ڱ�״̬
    else if (key_INCREASE_flag)
    {
        key_INCREASE_flag = 0;
        if (++((act[2]->str[1])[strlen((const char *)(act[2]->str[1])) - 1]) > 'C')
        {
            (act[2]->str[1])[strlen((const char *)(act[2]->str[1])) - 1] = 'A';
        }
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 1);
    }
    // ��"-"�����£�"ģʽ#"��C��B��A��C����ѭ���л������ڱ�״̬
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
 * @brief UI״̬��ACT202״̬����
 * �����"ȷ��"��λ��
 * 
 */
void ui_proc202(void)
{
    uint8_t i = 0;
    // ��"��"�����£�����Ƶ�"ģʽ#"����һ״̬ACT201
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[2]->row_page[2], act[2]->col_page[2] * 8 - 7, act[2]->str[2], 0);
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 1);
        ui_state = 0x201;
    }
    // ��"��"�����£�����Ƶ�"ȡ��"����һ״̬ACT203
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[2]->row_page[2], act[2]->col_page[2] * 8 - 7, act[2]->str[2], 0);
        display_GB2312_string(act[2]->row_page[3], act[2]->col_page[3] * 8 - 7, act[2]->str[3], 1);
        ui_state = 0x203;
    }
    // ��"ȷ��"�����£�����ǰ����ͬ����ACT0�����"ģʽ#"������Ƶ�ACT1��"����ģʽ"����һ״̬ACT100
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // ����ǰ����ͬ����ACT0�����"ģʽ#"
        strcpy((char *)(act[0]->str[0]), (const char *)(act[2]->str[1]));
        // ��������ʾACT1�Ļ���
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
 * @brief UI״̬��ACT203״̬����
 * �����"ȡ��"��λ��
 * 
 */
void ui_proc203(void)
{
    uint8_t i = 0;
    // ��"��"�����£�����Ƶ�"ȷ��"����һ״̬ACT202
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[2]->row_page[3], act[2]->col_page[3] * 8 - 7, act[2]->str[3], 0);
        display_GB2312_string(act[2]->row_page[2], act[2]->col_page[2] * 8 - 7, act[2]->str[2], 1);
        ui_state = 0x202;
    }
    // ��"��"�����£�����Ƶ�"ģʽ#"����һ״̬ACT201
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[2]->row_page[3], act[2]->col_page[3] * 8 - 7, act[2]->str[3], 0);
        display_GB2312_string(act[2]->row_page[1], act[2]->col_page[1] * 8 - 7, act[2]->str[1], 1);
        ui_state = 0x201;
    }
    // ��"ȷ��"�����£�������"ģʽ#"�ĸ��ģ�����Ƶ�ACT1��"����ģʽ"����һ״̬ACT100
    else if (key_ENTER_flag)
    {
        key_ENTER_flag = 0;
        // ������"ģʽ#"�ĸ���
        strcpy((char *)(act[2]->str[1]), (const char *)(act[0]->str[0]));
        // ��������ʾACT1�Ļ��棬����Ƶ�ACT1��"����ģʽ"
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
 * @brief UI״̬��ACT301״̬����
 * ����ڹ���������λ��λ��
 * 
 */
void ui_proc301(void)
{
    // ��"��"�����£�����Ƶ�"ȡ��"����һ״̬ACT306
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 0);
        display_GB2312_string(act[3]->row_page[6], act[3]->col_page[6] * 8 - 7, act[3]->str[6], 1);
        ui_state = 0x306;
    }
    // ��"��"�����£�����Ƶ���������ʮ��λ��λ�ã���һ״̬ACT303
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 0);
        display_GB2312_string(act[3]->row_page[3], act[3]->col_page[3] * 8 - 7, act[3]->str[3], 1);
        ui_state = 0x303;
    }
    // ��"+"�����£���λ����1��2��...��9��0��1��...����ѭ���л������ڱ�״̬
    else if (key_INCREASE_flag)
    {
        key_INCREASE_flag = 0;
        if (++((act[3]->str[1])[0]) > '9')
        {
            (act[3]->str[1])[0] = '0';
        }
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 1);
    }
    // ��"-"�����£���λ����9��8��...��0��9��8��...����ѭ���л������ڱ�״̬
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
 * @brief UI״̬��ACT303״̬����
 * ����ڹ�������ʮ��λ��λ��
 * 
 */
void ui_proc303(void)
{
    // ��"��"�����£�����Ƶ�����������λ��λ�ã���һ״̬ACT301
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[3]->row_page[3], act[3]->col_page[3] * 8 - 7, act[3]->str[3], 0);
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 1);
        ui_state = 0x301;
    }
    // ��"��"�����£�����Ƶ�"ȷ��"����һ״̬ACT305
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[3]->row_page[3], act[3]->col_page[3] * 8 - 7, act[3]->str[3], 0);
        display_GB2312_string(act[3]->row_page[5], act[3]->col_page[5] * 8 - 7, act[3]->str[5], 1);
        ui_state = 0x305;
    }
    // ��"+"�����£�ʮ��λ����1��2��...��9��0��1��...����ѭ���л������ڱ�״̬
    else if (key_INCREASE_flag)
    {
        key_INCREASE_flag = 0;
        if (++((act[3]->str[3])[0]) > '9')
        {
            (act[3]->str[3])[0] = '0';
        }
        display_GB2312_string(act[3]->row_page[3], act[3]->col_page[3] * 8 - 7, act[3]->str[3], 1);
    }
    // ��"-"�����£�ʮ��λ����9��8��...��0��9��8��...����ѭ���л������ڱ�״̬
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
 * @brief UI״̬��ACT305״̬����
 * �����"ȷ��"��λ��
 * 
 */
void ui_proc305(void)
{
    // ��"��"�����£�����Ƶ���������ʮ��λ��λ�ã���һ״̬ACT303
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[3]->row_page[5], act[3]->col_page[5] * 8 - 7, act[3]->str[5], 0);
        display_GB2312_string(act[3]->row_page[3], act[3]->col_page[3] * 8 - 7, act[3]->str[3], 1);
        ui_state = 0x303;
    }
    // ��"��"�����£�����Ƶ�"ȡ��"����һ״̬ACT306
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[3]->row_page[5], act[3]->col_page[5] * 8 - 7, act[3]->str[5], 0);
        display_GB2312_string(act[3]->row_page[6], act[3]->col_page[6] * 8 - 7, act[3]->str[6], 1);
        ui_state = 0x306;
    }
    // ��"ȷ��"�����£�������ֵ�Ϸ��ԣ����Ƿ�������ʾ��ʾ����ACT4����һ״̬ACT4�����Ϸ����򽫵�ǰ����ͬ����ACT0���棬����Ƶ�ACT1��"��������"����һ״̬ACT101
    else if (key_ENTER_flag)
    {
        uint8_t i;
        uint8_t num = ((act[3]->str[1])[0] - '0') * 10 + (act[3]->str[3])[0] - '0';

        key_ENTER_flag = 0;
        // �Ƿ���������ʾ��ʾ����ACT4����һ״̬ACT4
        if (num < 10 || num > 90)
        {
            // ��ʾACT4�Ļ��棬��һ״̬ACT4
            clear_screen();
            for (i = 0; i < act[4]->SIZE; ++i)
            {
                display_GB2312_string(act[4]->row_page[i], act[4]->col_page[i] * 8 - 7, act[4]->str[i], 0);
            }
            ACT4_clock5s_flag = ACT4_clock5s = 0; // ACT4��������ʼ��
            ui_state = 0x4;
        }
        // �Ϸ�����������ǰ����ͬ����ACT0���棬��ʾACT1�Ļ��棬����Ƶ�ACT1��"��������"����һ״̬ACT101
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
 * @brief UI״̬��ACT306״̬����
 * �����"ȡ��"��λ��
 * 
 */
void ui_proc306(void)
{
    // ��"��"�����£�����Ƶ�"ȷ��"����һ״̬ACT305
    if (key_LEFT_flag)
    {
        key_LEFT_flag = 0;
        display_GB2312_string(act[3]->row_page[6], act[3]->col_page[6] * 8 - 7, act[3]->str[6], 0);
        display_GB2312_string(act[3]->row_page[5], act[3]->col_page[5] * 8 - 7, act[3]->str[5], 1);
        ui_state = 0x305;
    }
    // ��"��"�����£�����Ƶ�����������λ��λ�ã���һ״̬ACT301
    else if (key_RIGHT_flag)
    {
        key_RIGHT_flag = 0;
        display_GB2312_string(act[3]->row_page[6], act[3]->col_page[6] * 8 - 7, act[3]->str[6], 0);
        display_GB2312_string(act[3]->row_page[1], act[3]->col_page[1] * 8 - 7, act[3]->str[1], 1);
        ui_state = 0x301;
    }
    // ��"ȷ��"�����£������Թ��������ĸ��ģ�����Ƶ�ACT1��"��������"����һ״̬ACT101
    else if (key_ENTER_flag)
    {
        uint8_t i = 0;

        key_ENTER_flag = 0;
        // �����Թ��������ĸ���
        strcpy((char *)(act[3]->str[1]), (const char *)(act[0]->str[1]));
        strcpy((char *)(act[3]->str[3]), (const char *)(act[0]->str[3]));
        // ��ʾACT1�Ļ��棬����Ƶ�ACT1��"��������"����һ״̬ACT101
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
 * @brief UI״̬��ACT4״̬����
 * ��ʾ��ʾ���棬ά��5��
 * 
 */
void ui_proc4(void)
{
    uint8_t i = 0;
    // ����ʱ�䵽����ʾACT3�Ľ��棬����Ƶ�ACT3�Ĺ���������λ��λ�ã���һ״̬ACT301
    if (ACT4_clock5s_flag)
    {
        ACT4_clock5s_flag = 0;
        // ����Ƶ�ACT3�Ĺ���������λ��λ�ã���һ״̬ACT301
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
