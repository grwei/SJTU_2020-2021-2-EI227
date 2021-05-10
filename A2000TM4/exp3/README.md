# 实验3

- 课程文档：[README.md](../../README.md)

## 任务3-0

- 要加预编译指令 `#define PART_TM4C1294NCPDT`. 可在源代码 `#include "driverlib/pin_map.h"` 之前加，也可在编译指令中指明（课程指导视频采用此方式）. API 函数 [GPIOPinConfigure()](inc/hw_gpio.h) 的源码注释节选：
  > The available mappings are supplied on a per-device basis in [`pin_map.h`](driverlib/pin_map.h). The PART_\<partno> defines controls which set of defines are included so that they match the device that is being used. For example, `PART_TM4C129XNCZAD` must be defined in order to get the correct pin mappings for the TM4C129XNCZAD device.
- 例程 [`exp3_0.c`](exp3_0.c) 将按键状态变化的检测写在 ISR `void SysTick_Handler(void)` 中，用 FSM 实现（状态号记录在全局变量 `key_state` 中，在其状态转移动作 C01 中修改全局变量 `run_state`.
  - 也可在主循环中实现按键状态变化的检测，参见[实验2文档](../exp2/README.md).

## 任务3-1

- 各 ISR 函数名定义见 [`startup_TM4C129.s`](RTE/Device/TM4C1294NCPDT/startup_TM4C129.s). 查出 UART0 的 ISR 名为 `UART0_Handler`.

```asm
DCD     UART0_Handler             ;   5: UART0 Rx and Tx
```

中断源见 [`inc/hw_ints.h`](inc/hw_ints.h).

- 可能需要 `#include "inc/tm4c1294ncpdt.h"` (TM4C1294NCPDT Register Definitions).
  - 官方解决方案：包含 [`hw_ints.h`](inc/hw_ints.h).
- UART 串口通信原理参见：[工程实践与科技创新II-A_3.1UART串口通信 (sjtu.edu.cn)](https://vshare.sjtu.edu.cn/play/f1717b5a1c71ecd3d7eaacd6d6ffac31)，提供了以非阻塞方式收发信息的流程。

## 任务3-2

- 不支持动态申请内存？`calloc`？
- 在主循环中处理接收到的命令。
- FSM 规则：
  - 状态定义. 0-IDLE（空闲）；1-BUSY_RX（接收忙）；2-BUSY_TX（处理或发送忙）.
  - 状态动作. 0-无；1-接收时间窗计数进行，把进入 UART Rx FIFO 的数据以[**非阻塞方式**](https://vshare.sjtu.edu.cn/play/f1717b5a1c71ecd3d7eaacd6d6ffac31)全部存入命令接收缓存区（直至缓存区满）；2-执行命令（若合法）、决定 UART 回传内容、将回传内容（若有）写入命令发送缓存区、置命令发送缓存区相关标志并关写允许（若有写入）、以[**非阻塞方式**](https://vshare.sjtu.edu.cn/play/f1717b5a1c71ecd3d7eaacd6d6ffac31)将命令发送缓存区的数据（若有）送到 UART Tx FIFO.
    - 注：接收时间窗应大于一次接收所需的时间，否则将出现难以预料的错误
    - UART串口非阻塞式通信实现的是[课程视频](https://vshare.sjtu.edu.cn/play/f1717b5a1c71ecd3d7eaacd6d6ffac31)提供的思路
  - 状态转移条件. C01-发生 `UART_RX` 或 `UART_RT` 中断；C12-接收计时结束；C20-命令（若合法）执行完毕，且回传内容（若有）已全部送到 UART Tx FIFO.
  - 状态转移动作. C01-接收时间计数器复位、接收缓存区复位、将第一个字符存入接收缓存区；C12-关命令接收缓存区写允许，发送缓存区复位；C20-无.
- 选 20ms 的接收时间，经实验，可以承受 24ms 以上的命令发送间隔。

## 任务3-3

- 如何收发中文？
- 可添加到任务3-2中。FSM 规则作以下修改：
  - 新增状态转移条件C02：定时1秒到；
  - 新增状态转移动作C02：关命令接收缓存区写允许、发送缓存区复位、将报时信息写入发送缓存区、关发送缓存区写允许；
  - 修改状态转移条件C20：命令（**若有且**合法）执行完毕，且回传内容（若有）已全部送到 UART Tx FIFO.
- FSM 规则：
  - 状态定义. 0-IDLE（空闲）；1-BUSY_RX（接收忙）；2-BUSY_TX（处理或发送忙）.
  - 状态动作. 0-无；1-接收时间窗计数进行，把进入 UART Rx FIFO 的数据全部存入命令接收缓存区（直至缓存区满）；2-执行命令（若合法）、决定 UART 回传内容、将回传内容（若有）写入命令发送缓存区、置命令发送缓存区相关标志并关写允许（若有写入）、以**非阻塞方式**将命令发送缓存区的数据（若有）送到 UART Tx FIFO.
  - 状态转移条件. C01-发生 `UART_RX` 或 `UART_RT` 中断；C12-接收计时结束；C20-命令（**若有且**合法）执行完毕，且回传内容（若有）已全部送到 UART Tx FIFO；C02-定时1秒到.
  - 状态转移动作. C01-接收时间计数器复位、接收缓存区复位、将第一个字符存入接收缓存区；C12-关命令接收缓存区写允许，发送缓存区复位；C20-无；C02-关命令接收缓存区写允许、发送缓存区复位、将报时信息写入发送缓存区、关发送缓存区写允许.
- FSM 细节：
  - 报时信息已准备好，但未发送完毕时（状态2），会忽略对时指令，且不会产生新的报时信息
  - 报时信息准备中，收到对时指令，会转而执行对时，造成本次报时被跳过

### 表3-4 不同的波特率对UART通信的影响

| 序号 | UART波特率（Baud） | 显示结果是否正确 | 原因 |
|:-:|:-:|:-:|:-:|
| 1 | 230400 | 否 | 我的PC不支持这波特率 |
| 2 | 115200 | 是 | 通信速率足够 |
| 3 | 9600 | 是，发对时指令有时导致忽略报时 | 通信速率尚可；FSM逻辑 |
| 4 | 1200 | 是，可见逐字效果 | 通信速率较低 |
| 5 | 300 | 约两秒一次报时，经常忽略、有时误判对时指令 | 通信速率低，导致一秒内发不完报时信息；FSM逻辑 |
| 6 | 110 | 否 | 我的PC不支持这波特率 |
