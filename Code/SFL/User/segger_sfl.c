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
    0x00800000,                 // Total Size: 8 MB
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

// Seems no used.
const uint32_t SEGGER_FL_Flags __attribute__((used)) = ((SEGGER_FL_FLAG_SUPPORT_AUTO_ERASE) | (SEGGER_FL_FLAG_GO_INT_EN));
const uint32_t SEGGER_FL_MaxBlocksizeErase __attribute__((used)) = (0x00010000); // Overrides maximum block size to 64 KB

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

    SFL_DEBUG("Call _%s\r\n", __FUNCTION__);
    return  OSPI_Get_FlashID();
#else


    return 0;
#endif
}

int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Restore(uint32_t RestorePara0, uint32_t RestorePara1, uint32_t RestorePara2)
{
    SFL_DEBUG("Call _%s\r\n", __FUNCTION__);

    __HAL_UART_DISABLE(&huart1);
    HAL_UART_DeInit(&huart1);

    HAL_OSPI_MspDeInit(&hospi1);

    return 0;
}

int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Program(uint32_t DestAddr, uint32_t NumBytes, uint8_t *pSrcBuff)
{
    uint32_t address = 0;

    SFL_DEBUG("Call %s\r\n", __FUNCTION__);
    SFL_DEBUG("DestAddr: 0x%08X, NumBytes: %d\r\n", DestAddr, NumBytes);
    if(DestAddr >= FlashDevice.BaseAddr)
    {
        address = DestAddr - FlashDevice.BaseAddr;
    }

    return OSPI_W25Qxx_WriteBuffer(address, NumBytes, pSrcBuff);
}

int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Erase(uint32_t SectorAddr, uint32_t SectorIndex, uint32_t NumSectors)
{
    uint32_t addr = 0;

    uint32_t block_num = 0;
    uint32_t half_block_num = 0;
    uint32_t sector_num = 0;

    SFL_DEBUG("Call %s\r\n", __FUNCTION__);
    SFL_DEBUG("SectorAddr: 0x%08X, SectorIndex: %d, NumSectors: %d\r\n", SectorAddr, SectorIndex, NumSectors);

    if(SectorAddr >= FlashDevice.BaseAddr)
    {
        addr = SectorAddr - FlashDevice.BaseAddr;
    }

    block_num = NumSectors / SECTOR_NUM_OF_BLOCK;
    sector_num = NumSectors % SECTOR_NUM_OF_BLOCK;
    half_block_num = sector_num / SECTOR_NUM_OF_HALF_BLOCK;
    sector_num = sector_num % SECTOR_NUM_OF_HALF_BLOCK;

    if(block_num)
    {
        SFL_DEBUG("Erase %d blocks\r\n", block_num);
        do
        {
            if(OSPI_W25Qxx_Erase(addr, WB_Flash_CMD_Block_Erase) != EXT_FLASH_RET_OK)
            {
                return -1;
            }
            addr += BLOCK_SIZE;
        }while(block_num--);
    }

    if(half_block_num)
    {
        SFL_DEBUG("Erase %d half blocks\r\n", half_block_num);
        do
        {
            if(OSPI_W25Qxx_Erase(addr, WB_Flash_CMD_Block_Erase_32KB) != EXT_FLASH_RET_OK)
            {
                return -1;
            }
            addr += HALF_BLOCK_SIZE;
        }while(half_block_num--);
    }

    if(sector_num)
    {
        SFL_DEBUG("Erase %d sectors\r\n", sector_num);
        do
        {
            if(OSPI_W25Qxx_SectorErase(addr) != EXT_FLASH_RET_OK)
            {
                return -1;
            }
            addr += SECTOR_SIZE;
        }while(sector_num--);
    }
    


    return 0;
}

int __attribute__ ((section ("PrgCode"))) SEGGER_FL_EraseChip(void)
{
    SFL_DEBUG("Call %s\r\n", __FUNCTION__);
    return OSPI_W25Qxx_Erase(0, WB_Flash_CMD_Chip_Erase);
}

/**
 * @brief  Checks if a memory region is blank / erased / unprogrammed.
 * @param  Addr
 * @retval HAL status or other return value description
 * @note   W25Q64 without check blank command, so read and check manually; it may not be efficient sometime.
 */

uint8_t blank_check_buf[4096];
int __attribute__ ((section ("PrgCode"))) SEGGER_FL_CheckBlank(uint32_t Addr, uint32_t NumBytes, uint8_t BlankValue)
{
    SFL_DEBUG("Call %s\r\n", __FUNCTION__);
#if SFL_FORCE_ERASE
    return 1;
#else
    uint32_t address = 0;
    uint32_t read_len = 0;

    if(Addr >= FlashDevice.BaseAddr)
    {
        address = Addr - FlashDevice.BaseAddr;
    }

    if(NumBytes < sizeof(blank_check_buf))
    {
        read_len = NumBytes;
    }
    else 
    {
        read_len = sizeof(blank_check_buf);
    }
    
    while (NumBytes != 0) 
    {
        if(OSPI_W25Qxx_ReadBuffer(address, read_len, blank_check_buf) != EXT_FLASH_RET_OK)
        {
            return -1;
        }
        
        for(int i = 0; i < read_len; i++)
        {
            if(blank_check_buf[i] != BlankValue)
            {
                return 1;
            }
        }
        
        address += read_len;
        NumBytes -= read_len;
        
        if(NumBytes < sizeof(blank_check_buf))
        {
            read_len = NumBytes;
        }
    }

    return 0;
#endif
}

uint8_t verify_buf[8*1024];             // Verify buffer
uint32_t __attribute__ ((section ("PrgCode"))) SEGGER_FL_Verify(uint32_t Addr, uint32_t NumBytes, uint8_t* pData)
{
    
    uint32_t address = 0;
    uint32_t read_len = 0;
    uint32_t j = 0;

    SFL_DEBUG("Call %s\r\n", __FUNCTION__);
    if(Addr >= FlashDevice.BaseAddr)
    {
        address = Addr - FlashDevice.BaseAddr;
    }

    if(NumBytes < sizeof(verify_buf))
    {
        read_len = NumBytes;
    }
    else 
    {
        read_len = sizeof(verify_buf);
    }
    
    while (NumBytes != 0) 
    {
        if(OSPI_W25Qxx_ReadBuffer(address, read_len, verify_buf) != EXT_FLASH_RET_OK)
        {
            SFL_DEBUG("Read failed at 0x%08X\r\n", address + FlashDevice.BaseAddr);
            return -1;
        }
        
        for(int i = 0; i < read_len; i++)
        {
            if(verify_buf[i] != pData[i + j * sizeof(verify_buf)])
            {
                SFL_DEBUG("Verify failed at 0x%08X\r\n", address + i + FlashDevice.BaseAddr);
                SFL_DEBUG("Read: 0x%02X, Expected: 0x%02X\r\n", verify_buf[i], pData[i]);
                return address + i + FlashDevice.BaseAddr;
            }
        }
        
        j++;
        address += read_len;
        NumBytes -= read_len;
        
        if(NumBytes < sizeof(blank_check_buf))
        {
            read_len = NumBytes;
        }
    }

    return address + FlashDevice.BaseAddr;
}


int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Read(uint32_t Addr, uint32_t NumBytes, uint8_t *pDestBuff)
{
    uint32_t address = 0;
    int8_t ret = -1;

    SFL_DEBUG("Call %s\r\n", __FUNCTION__);
    SFL_DEBUG("Addr: 0x%08X, NumBytes: %d\r\n", Addr, NumBytes);

    if(Addr >= FlashDevice.BaseAddr)
    {
        address = Addr - FlashDevice.BaseAddr;
    }

    ret = OSPI_W25Qxx_ReadBuffer(address, NumBytes, pDestBuff);

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