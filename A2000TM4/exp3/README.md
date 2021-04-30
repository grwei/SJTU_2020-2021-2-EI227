# 实验3

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
