/**
 * @file segger_sfl.h
 * @brief Brief description of the header file
 * @author 404zen
 * @date 2026-01-28
 * @version 1.0
 */

#ifndef __SEGGER_SFL_H__
#define __SEGGER_SFL_H__

#include "usart.h"
#include <stdint.h>

#define SFL_DEBUG_ENABLE                0

#if SFL_DEBUG_ENABLE
    #define SFL_DEBUG(...)              usart1_printf(__VA_ARGS__)
#else
    #define SFL_DEBUG(...)              (void)0
#endif


#define SFL_FORCE_ERASE                 0

#define SEGGER_FL_FLAG_SHIFT_SUPPORT_AUTO_ERASE     (0)
#define SEGGER_FL_FLAG_SUPPORT_AUTO_ERASE           (1 << SEGGER_FL_FLAG_SHIFT_SUPPORT_AUTO_ERASE)

#define SEGGER_FL_FLAG_SHIFT_GO_INT_EN              (1)
#define SEGGER_FL_FLAG_GO_INT_EN                    (1 << SEGGER_FL_FLAG_SHIFT_GO_INT_EN)


/* Mandory function */
int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Prepare(uint32_t PreparePara0, uint32_t PreparePara1, uint32_t PreparePara2);
int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Restore(uint32_t RestorePara0, uint32_t RestorePara1, uint32_t RestorePara2);
int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Program(uint32_t DestAddr, uint32_t NumBytes, uint8_t *pSrcBuff);
int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Erase(uint32_t SectorAddr, uint32_t SectorIndex, uint32_t NumSectors);

/* Optional Function */
int __attribute__ ((section ("PrgCode"))) SEGGER_FL_EraseChip(void);
int __attribute__ ((section ("PrgCode"))) SEGGER_FL_CheckBlank(uint32_t Addr, uint32_t NumBytes, uint8_t BlankValue);
uint32_t __attribute__ ((section ("PrgCode"))) SEGGER_FL_Verify(uint32_t Addr, uint32_t NumBytes, uint8_t* pData);
int __attribute__ ((section ("PrgCode"))) SEGGER_FL_Read(uint32_t Addr, uint32_t NumBytes, uint8_t *pDestBuff);
// uint32_t SEGGER_FL_CalcCRC(uint32_t CRC, uint32_t Addr, uint32_t NumBytes, uint32_t Polynom);
// void SEGGER_FL_Start(volatile struct SEGGER_FL_CMD_INFO* pInfo);
// int SEGGER_FL_GetFlashInfo(SEGGER_FL_FLASH_INFO* pInfo, U32 InfoAreaSize);

#endif /* __SEGGER_SFL_H__ */
