# 实验4

- 课程文档：[README.md](../../README.md)
- 补充实验记录：[实验四记录.docx](doc/实验四记录.docx)

## 任务4_0

- 源文件编码：GB 2312
- C语言不支持函数参数的默认值，不支持函数重载

## 任务4_1

- 使用[前后台架构（前台-中断，后台-主循环）+状态机逻辑](https://vshare.sjtu.edu.cn/play/5e55d9f7a4e0b58856fb5b9dd7aa359f)

## 任务4-2

- 遵循指导，将显示屏划分为 8x16 个 8x8 的点阵。
- 指导视频中出现了类似下面的语句。其中一种实现方案是：`act`为结构体类型`ACT_T`的指针数组，`ACT_T`结构体包含`uint8_t row_page[]`, `uint8_t col_page`, `unsigned char str[][]`等字段。

  ```cpp
    display_GB2312_string(act[0]->row_page[1], act[0]->col_page[1] * 8 - 7, act[0]->str[1], 1);
  ```

- 务必随时做单元测试，尊重工程规律。若跳过单元测试直接做集成测试，将导致调试难度极大。
  - 例如，写完一个状态处理函数，就检查该状态是否已正确实现。
- 通过调试，可发现一个中文占 2 字节。可以使用类似下面的语句修改类似字符串"模式A"中的尾字符：

  ```cpp
    if (++((act[0]->str[1])[strlen((const char *)(act[0]->str[1])) - 1]) > 'C')
    {
        (act[0]->str[1])[strlen((const char *)(act[0]->str[1])) - 1] = 'A';
    }
  ```

## 任务4_3

- 共5个画面，分别对应状态号前缀ACT0~4.
- 状态号含义举例：
  - ACT305表示显示ACT3画面，且光标在块号05区域上
  - ACT0表示显示ACT0画面，且不显示光标
- 共5个画面。切换画面时，应先全屏清屏。
- 在ACT2和ACT3中，若用户点击"取消"，则要撤销对相应参数的更改。
  - 实现方法：只有在点击"确定"时，才会将当前更改同步到ACT0中的相应参数；当点击"取消"时，用ACT0中的相应参数覆盖当前更改。
- 细节补充：
  - 若用户在ACT3给出非法参数，则会进入ACT4警示界面，但不会撤销更改。
