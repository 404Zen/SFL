# 项目环境

- **Toolchain** : Cmake + GCC
- MCU : STM32H7B0VB; actually use FK7B0M1-VBT6 board
- SPI Flash : W25Q64JV



Use STM32CubeMX generate project, and VSCode with STM32CubeMX extension pack to build and debug.



## IO defintion

| IO   | Function       | Comment                                   |
| ---- | -------------- | ----------------------------------------- |
| PH0  | RCC_OSC_IN     | External high speed clock, crystal @25MHz |
| PH1  | RCC_OSC_OUT    |                                           |
| PA13 | DEBUG_SWDIO    |                                           |
| PA14 | DEBUG_SWCLK    |                                           |
| PB6  | OCTOSPI_P1_NSS |                                           |
| PB2  | OCTOSPI_P1_CLK |                                           |
| PD11 | OCTOSPI_P1_IO0 |                                           |
| PD12 | OCTOSPI_P1_IO1 |                                           |
| PD13 | OCTOSPI_P1_IO2 |                                           |
| PD14 | OCTOSPI_P1_IO3 |                                           |
| PC1  | LED0           | Active low                                |

this part also can check in project.ioc file.



# 什么是 Segger Flash Loader

请参考：https://kb.segger.com/SEGGER_Flash_Loader



在 J-Link 下载的时候，需要先将一个可执行文件下载到 MCU 的 RAM 中，然后 J-Link 会调用这个可执行文件中的一些接口，对 MCU 内部/外部 的 Flash 进行擦写和编程；一般情况下，J-Link 软件已经包含了 MCU 内部的 Flash 的下载算法，甚至还有一些 MCU 会提供某些特定配置的外部 Flash 的下载算法。但当 J-Link 软件中未包含你的 MCU + 外部 Flash 配置的算法实现时，就可以根据以上文档自行编写 Segger Flash Loader, 实现在你的板子上使用 J-Link 进行外部 Flash 的编程。



# 实现过程

根据 [Segger Flash Loader](https://kb.segger.com/SEGGER_Flash_Loader) , 我们至少需要对前 4 个函数进行实现。

The following table gives an overview about the mandatory and optional entry functions, of a SEGGER Flash Loader:

| Entry function                                               | Type      |
| ------------------------------------------------------------ | --------- |
| [SEGGER_FL_Prepare()](https://kb.segger.com/SEGGER_Flash_Loader#SEGGER_FL_Prepare) | Mandatory |
| [SEGGER_FL_Restore()](https://kb.segger.com/SEGGER_Flash_Loader#SEGGER_FL_Restore) | Mandatory |
| [SEGGER_FL_Program()](https://kb.segger.com/SEGGER_Flash_Loader#SEGGER_FL_Program) | Mandatory |
| [SEGGER_FL_Erase()](https://kb.segger.com/SEGGER_Flash_Loader#SEGGER_FL_Erase) | Mandatory |
| [SEGGER_FL_EraseChip()](https://kb.segger.com/SEGGER_Flash_Loader#SEGGER_FL_EraseChip) | Optional  |
| [SEGGER_FL_CheckBlank()](https://kb.segger.com/SEGGER_Flash_Loader#SEGGER_FL_CheckBlank) | Optional  |
| [SEGGER_FL_Verify()](https://kb.segger.com/SEGGER_Flash_Loader#SEGGER_FL_Verify) | Optional  |
| [SEGGER_FL_Read()](https://kb.segger.com/SEGGER_Flash_Loader#SEGGER_FL_Read) | Optional  |
| [SEGGER_FL_CalcCRC()](https://kb.segger.com/SEGGER_Flash_Loader#SEGGER_FL_CalcCRC) | Optional  |
| [SEGGER_FL_Start()](https://kb.segger.com/SEGGER_Flash_Loader#SEGGER_FL_Start) | Optional  |
| [SEGGER_FL_GetFlashInfo()](https://kb.segger.com/SEGGER_Flash_Loader#SEGGER_FL_GetFlashInfo) | Optional  |



对于不同的 Flash, 实现这些功能的过程可能略有差异，可以到相关厂商的官网中获取对应的 Flash  型号的手册来编写你的代码。

在使用 STM32CubeMX 生成驱动代码之后，编写 Flash 相关驱动代码并进行测试，在测试通过之后，我们再进行下一步的工作。



对于 SEGGER 的相关接口，可以查看代码中的具体实现。



## 变成一个可以加载到 RAM 中执行的程序

在 Flash 中对我们的代码测试完成之后，需要修改链接文件，将代码/数据放到 RAM 中；因为 J-Link 在调用这个文件的时候，是将其放到 RAM 中执行的。

SFL 对于 Section Layout 是有如下要求的：

The J-Link software expects a special layout when it comes to RO code, RO data, RW data placement in the SFL binary.
The reference algorithms and templates provided with the J-Link DSK already take care of that layout, so there is nothing special to be considered by the user.

The section layout is as expected:

```
section PrgCode                     // Marks the start of the SFL. Must be the very first section
sections .text, .rodata, ...        // In any order
section PrgData                     // Marks the end of the code + rodata region (functions, const data, ...) and the start of the data region (static + global variables)
sections .textrw, .data, .fast, ... // In any order
section DevDscr                     // Marks the location of the <FlashDevice> structure variable and also the end of the loader. Must(!!!) be the very last section
```



#### 修改链接文件

这里我选择 STM32H7B0 的第一段 AXI SRAM，这一段 SRAM 总共有 256KBytes，完全够用。根据要求适配我们的链接文件。

可以参考修改完成的链接文件 [sfl.ld](../Code/SFL/sfl.ld) 

一些需要注意的小细节：

- 这里的段名称，前面都没有带 .
- FlashDevice 结构体变量的名称 必须是 FlashDevice



#### 修改 VTOR 指向 SRAM

这部分在 `SystemInit()` 中有想当完善的实现，我们只需要在 `CMakeLists.txt` 中添加对应的全局宏定义即可。

```cmake
+# STM32CubeMX generated symbols (macros)
+set(MX_Defines_Syms
+       USE_PWR_LDO_SUPPLY
+       USE_HAL_DRIVER
+       STM32H7B0xx
+    USER_VECT_TAB_ADDRESS
+    VECT_TAB_SRAM
+    $<$<CONFIG:Debug>:DEBUG>
+)
```











