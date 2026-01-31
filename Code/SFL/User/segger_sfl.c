/**
 * @file segger_sfl.c
 * @brief Brief description of the source file
 * @author 404zen
 * @date 2026-01-28
 * @version 1.0
 */

#include <stdint.h>
#include <stdio.h>
#include "segger_sfl.h"
#include "ext_flash.h"
#include "main.h"
#include "gpio.h"
#include "octospi.h"
#include "WB_Command.h"
#include "segger_sfl_def.h"
#include "usart.h"

struct FlashDevice const FlashDevice __attribute__ ((section ("DevDscr"))) = 
{
    SEGGER_SFL_ALGO_VERSION,
    "W25Q64JV SPI Flash",
    1,                          // Flash device type. Currently ignored. Set to 1 to get max. compatibility.
    0x90000000,                 // Base Address
    0x00080000,                 // Total Size: 8 MB
    256,                        // Page Size: 256 Bytes
    0,                          // Reserved
    0xFF,                       // Erased Value
    30,                         // Timeout Prog: 3 mS
    4000,                       // Timeout Erase: 400 mS

    /* Flash Sector information */
    {
        { 0x00001000, 0x00000000 },         // Sector Size: 4 KB (128 Sectors)
        { 0xFFFFFFFF, 0xFFFFFFFF },         // Indicates the end of the flash sector layout. Must be present.
    }
};



int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Prepare(uint32_t PreparePara0, uint32_t PreparePara1, uint32_t PreparePara2)
{   
#if 1
    /* System init */
    SystemInit();
    MPU_Config();
    

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();


    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();

    MX_OCTOSPI1_Init();
    MX_USART1_UART_Init();

    return  OSPI_Get_FlashID();
#else


    return 0;
#endif
}

int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Restore(uint32_t RestorePara0, uint32_t RestorePara1, uint32_t RestorePara2)
{
    __HAL_UART_DISABLE(&huart1);
    HAL_UART_DeInit(&huart1);

    HAL_OSPI_MspDeInit(&hospi1);

    return 0;
}

int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Program(uint32_t DestAddr, uint32_t NumBytes, uint8_t *pSrcBuff)
{
    return OSPI_W25Qxx_WriteBuffer(DestAddr, NumBytes, pSrcBuff);
}

int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Erase(uint32_t SectorAddr, uint32_t SectorIndex, uint32_t NumSectors)
{
    uint32_t addr = 0;
    uint32_t i = 0; 

    if(SectorAddr >= FlashDevice.BaseAddr)
    {
        addr = SectorAddr - FlashDevice.BaseAddr;
    }
    
    for(i = SectorIndex; i < (SectorIndex + NumSectors); i++)
    {

        // if(OSPI_W25Qxx_Erase(addr+(i*flash_descriptor.SectorInfo[0].SectorSize), WB_Flash_CMD_Sector_Erase) != EXT_FLASH_RET_OK)
        if(OSPI_W25Qxx_Erase(addr+(i*FlashDevice.SectorInfo->SectorSize), WB_Flash_CMD_Sector_Erase) != EXT_FLASH_RET_OK)
        {
            return -1;
        }
    }

    return 0;
}

int __attribute__ ((section ("PrgCode"))) SEGGER_FL_EraseChip(void)
{
    return OSPI_W25Qxx_Erase(0, WB_Flash_CMD_Chip_Erase);
}


int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Read(uint32_t Addr, uint32_t NumBytes, uint8_t *pDestBuff)
{
    uint32_t addr = 0;
    int8_t ret = -1;

    if(Addr >= FlashDevice.BaseAddr)
    {
        addr = Addr - FlashDevice.BaseAddr;
    }

    ret = OSPI_W25Qxx_ReadBuffer(addr, NumBytes, pDestBuff);

    if(ret != EXT_FLASH_RET_OK)
    {
        return ret;
    }
    else 
    {
        return NumBytes;
    }


}

/* END OF FILE */