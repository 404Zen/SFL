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



对于 SEGGER 的相关接口，可以查看代码中的具体实现， 后面或许会有讲解。



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
- **PrgCode 必须必须必须 是 第一个 section**, 如果你遇到各种奇怪的问题，请先检查这一条





## 功能实现

#### SEGGER_FL_Prepare()

在 `SEGGER_FL_Prepare()` 中，我们要实现 MCU 时钟初始化，SPI 接口的初始化这些必须的工作。

所以，这个函数看起来至少是这样的

```C
int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Prepare(uint32_t PreparePara0, uint32_t PreparePara1, uint32_t PreparePara2)
{   
    /* System init */
    SystemInit();
    
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();


    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_OCTOSPI1_Init();

    return  0;
}
```







# How to use

固件编写完成之后，需要添加对应的设备才能使用这个 elf 对你的外部 Flash 进行编程。具体的实现流程看[这个链接](https://kb.segger.com/J-Link_Device_Support_Kit)。

简单说，就是在 J-Link 的 central folder 添加一个设备，设备由一个 XML 文件描述，里面会包含芯片信息以及前面所编译出来的 elf 文件路径。有了这个文件之后，J-Link 就可以识别到你添加的设备了。

The location of the central folder depends on the host OS, as well as on the active user on that OS:

| OS      | Location                                              |
| ------- | ----------------------------------------------------- |
| Windows | C:\Users\<USER>\AppData\Roaming\SEGGER\JLinkDevices   |
| Linux   | $HOME/.config/SEGGER/JLinkDevices                     |
| macOS   | $HOME/Library/Application Support/SEGGER/JLinkDevices |



**这里是 [参考模板](../Code/SFL/JLinkDevices/FK_ST/FKH7B0)。**

















