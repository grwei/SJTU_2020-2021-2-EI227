# 实验3

- 课程文档：[README.md](../../README.md)

## Todo

1. （**任务3-2**）修改收发和命令处理逻辑. FSM 规则：
   1. 状态定义. 0-IDLE（空闲）；1-BUSY_RX（接收忙）；2-BUSY_TX（处理或发送忙）.
   2. 状态动作. 0-无；1-接收时间窗计数器递减，把进入 UART Rx FIFO 的数据全部存入命令接收缓存区（直至缓存区满）；2-关命令接收缓存区写允许，决定 UART 回传内容，将回传内容（若有）写入命令发送缓存区，关命令发送缓存区写允许，以**非阻塞方式**将命令发送缓存区的数据（若有）送到 UART Tx FIFO.
   3. 状态转移条件. C01-发生`UART_RX` 或 `UART_RT` 中断；C12-接收忙标志被清除；C20-命令（若合法）执行完毕，且回传内容（若有）已全部送到 UART Tx FIFO.
   4. 状态转移动作. C01-通过置接收忙标志启动接收时间窗计数器；C12-关命令接收缓存区写允许；C20-开命令接收缓存区写允许.
2. （任务3-3）实现中文收发.
   - 参考思路：获取串口通信软件 [SerialProV1.04](../../SerialProV1.04.exe) 发送的中文的十六进制编码，然后直接创建这些编码的字符串.

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

## 任务3-2

- 不支持动态申请内存？`calloc`？
- 极端情况下，会误判非法输入.
- 可在 ISR `UART0_Handler` 中处理接收到的命令，也可在主循环中处理。

## 任务3-3

- 如何收发中文？
