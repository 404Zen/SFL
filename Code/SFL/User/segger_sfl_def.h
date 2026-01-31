/**
 * @file segger_sfl_def.h
 * @brief Brief description of the header file
 * @author 404zen
 * @date 2026-01-29
 * @version 1.0
 */

#ifndef __SEGGER_SFL_DEF_H__
#define __SEGGER_SFL_DEF_H__


/* REF: https://kb.segger.com/SEGGER_Flash_Loader?utm_source=chatgpt.com#FlashDevice_struct */

const uint32_t SEGGER_FL_MaxBlocksizeErase __attribute__((used)) = (0x00010000); // Overrides maximum block size to 64 KB
#define MAX_NUM_SECTORS             (512)           // Max. number of sectors, must not be modified.
#define SEGGER_SFL_ALGO_VERSION      0x0101         /* Set to 0x0101. Do not set to anything else! */


struct SECTOR_INFO  {
  uint32_t SectorSize;       // Sector Size in bytes
  uint32_t SectorStartAddr;  // Start address of the sector area (relative to the "BaseAddr" of the flash)
};


struct FlashDevice  {
  uint16_t AlgoVer;
  uint8_t  Name[128];
  uint16_t Type;                    // Flash device type. Currently ignored. Set to 1 to get max. compatibility.
  uint32_t BaseAddr;                // Flash base address. It is recommended to always use the real address of the flash here, even if the flash is also available at other addresses (via an alias / remap), depending on the current settings of the device.
  uint32_t TotalSize;               // Total flash device size in bytes. 
  uint32_t PageSize;                // This field describes in what chunks J-Link feeds the flash loader. 
  uint32_t Reserved;                // 	Set this element to 0
  uint8_t  ErasedVal;               // Most flashes have an erased value of 0xFF (set this element to 0xFF in such cases). 
  uint32_t TimeoutProg;             // Timeout in milliseconds (ms) to program one chunk of <PageSize>.
  uint32_t TimeoutErase;            // Timeout in milliseconds (ms) to erase one sector.
  struct SECTOR_INFO SectorInfo[MAX_NUM_SECTORS];       // 	This element is actually a list of different sector sizes present on target flash.Having a flash with uniform sectors will result in only SectorInfo[0] being used for sectorization information.
};


#endif /* __SEGGER_SFL_DEF_H__ */
