# Environment

- **Toolchain**: Cmake + GCC
- **MCU**: STM32H7B0VB; actually using FK7B0M1-VBT6 board
- **SPI Flash**: W25Q64JV

The project was generated using STM32CubeMX, and built/debugged with VSCode using the STM32CubeMX extension pack.

## IO Definition

| IO   | Function       | Comment                                   |
| ---- | -------------- | ----------------------------------------- |
| PH0  | RCC_OSC_IN     | External high-speed clock, crystal @25MHz |
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

This information can also be verified in the `project.ioc`file.

# What is Segger Flash Loader

Please refer to: https://kb.segger.com/SEGGER_Flash_Loader

When downloading with J-Link, an executable file first needs to be loaded into the MCU's RAM. J-Link then calls interfaces within this executable to erase, write, and program the internal/external Flash memory. Typically, the J-Link software includes download algorithms for the MCU's internal Flash, and sometimes even for specific configurations of external Flash. However, when the J-Link software lacks the algorithm for your specific MCU + external Flash configuration, you can write your own Segger Flash Loader based on the documentation above. This enables programming the external Flash on your board using J-Link.

# Implementation Process

According to the Segger Flash Loaderdocumentation, we need to implement at least the first 4 functions.

The following table gives an overview about the mandatory and optional entry functions of a SEGGER Flash Loader:

| Entry Function           | Type      |
| ------------------------ | --------- |
| SEGGER_FL_Prepare()      | Mandatory |
| SEGGER_FL_Restore()      | Mandatory |
| SEGGER_FL_Program()      | Mandatory |
| SEGGER_FL_Erase()        | Mandatory |
| SEGGER_FL_EraseChip()    | Optional  |
| SEGGER_FL_CheckBlank()   | Optional  |
| SEGGER_FL_Verify()       | Optional  |
| SEGGER_FL_Read()         | Optional  |
| SEGGER_FL_CalcCRC()      | Optional  |
| SEGGER_FL_Start()        | Optional  |
| SEGGER_FL_GetFlashInfo() | Optional  |

The implementation of these functions may vary slightly depending on the specific Flash memory. It is recommended to obtain the datasheet for your Flash model from the manufacturer's website to guide your code development.

After generating the driver code with STM32CubeMX, write and test the Flash-related driver code. Once testing is successful, proceed to the next steps.

For details on the SEGGER interfaces, refer to the specific implementations in the code, which may be explained later.

## Creating a RAM-executable Program

After testing the code in Flash, the linker script needs to be modified to place the code/data into RAM. This is because J-Link loads this file into RAM for execution.

The SFL has specific requirements for the Section Layout:

The J-Link software expects a special layout when it comes to RO code, RO data, and RW data placement in the SFL binary. The reference algorithms and templates provided with the J-Link DSK already take care of that layout, so there is nothing special to be considered by the user.

The expected section layout is:

```markdown
section PrgCode                     // Marks the start of the SFL. Must be the very first section
sections .text, .rodata, ...        // In any order
section PrgData                     // Marks the end of the code + rodata region (functions, const data, ...) and the start of the data region (static + global variables)
sections .textrw, .data, .fast, ... // In any order
section DevDscr                     // Marks the location of the <FlashDevice> structure variable and also the end of the loader. Must(!!!) be the very last section
```

#### Modifying the Linker Script

The first segment of the STM32H7B0's AXI SRAM (256KBytes) was selected for this purpose, as it provides sufficient space. Adapt the linker script according to the requirements.

You can refer to the modified linker script: sfl.ld

Important details to note:

- The section names specified here do not start with a period (`.`).
- The name of the `FlashDevice`structure variable **must** be `FlashDevice`.
- The **`PrgCode`section MUST, MUST, MUST be the very first section**. If you encounter strange issues, check this first.

## Function Implementation

#### SEGGER_FL_Prepare()

In `SEGGER_FL_Prepare()`, we need to implement essential initializations such as MCU clock configuration and SPI interface initialization.

Therefore, this function should look at least like this:

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

# How to Use

After the firmware is written, a corresponding device needs to be added to use the generated `.elf`file for programming your external Flash. The specific process is described in this link.

In short, a device is added to J-Link's central folder via an XML file. This file describes the chip information and includes the path to the compiled `.elf`file. Once this file is in place, J-Link can recognize your added device.

The location of the central folder depends on the host OS and the active user:

| OS      | Location                                              |
| ------- | ----------------------------------------------------- |
| Windows | C:\Users<USER>\AppData\Roaming\SEGGER\JLinkDevices    |
| Linux   | $HOME/.config/SEGGER/JLinkDevices                     |
| macOS   | $HOME/Library/Application Support/SEGGER/JLinkDevices |

**Refer to the template here.**
