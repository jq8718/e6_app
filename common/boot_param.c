/**
 *******************************************************************************
 * @file  boot_param.c
 * @brief Boot parameter area management
 *******************************************************************************
 */

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "boot_param.h"
#include "hc32l021.h"
#include "ddl.h"

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

#define BOOT_PARAM_PAYLOAD_SIZE (7u * sizeof(uint32_t)) /* 28 bytes */
#define FLASH_TIMEOUT           (0xFFFFu)

#define FLASH_BYPASS() \
    do { \
        FLASH->BYPASS = 0x5A5A; \
        FLASH->BYPASS = 0xA5A5; \
    } while (0)

#define FLASH_UNLOCK_ALL() \
    do { \
        FLASH_BYPASS(); \
        FLASH->SLOCK0 = 0xFFFFFFFFu; \
    } while (0)

#define FLASH_LOCK_ALL() \
    do { \
        FLASH_BYPASS(); \
        FLASH->SLOCK0 = 0; \
    } while (0)

/*******************************************************************************
 * Function implementation
 ******************************************************************************/

uint16_t BootParam_CalcHeaderCrc(const stc_boot_param_t *pstcParam)
{
    return (uint16_t)HC32_CalCrc16((uint8_t *)pstcParam, 0u, BOOT_PARAM_PAYLOAD_SIZE);
}

void BootParam_Read(stc_boot_param_t *pstcParam)
{
    HC32_FlashReadBytes(BOOT_PARAM_ADDR, (uint8_t *)pstcParam, sizeof(stc_boot_param_t));
}

en_result_t BootParam_WriteState(uint32_t u32State)
{
    en_result_t enRet;
    stc_boot_param_t stcParam;

    BootParam_Read(&stcParam);

    stcParam.magic  = BOOT_PARAM_MAGIC;
    stcParam.state  = u32State;
    stcParam.header_crc = BootParam_CalcHeaderCrc(&stcParam);

    /* Must erase sector before write */
    enRet = HC32_FlashEraseSector(BOOT_PARAM_ADDR);
    if (Ok != enRet)
    {
        return enRet;
    }

    return HC32_FlashWriteBytes(BOOT_PARAM_ADDR, (uint8_t *)&stcParam, sizeof(stc_boot_param_t));
}

/*******************************************************************************
 * CRC16
 ******************************************************************************/
uint32_t HC32_CalCrc16(uint8_t *pu8Data, uint32_t u32Offset, uint32_t u32Size)
{
    uint8_t  u8Cnt;
    uint16_t u16CrcResult = 0xA28C;

    while (u32Size != 0)
    {
        u16CrcResult ^= pu8Data[u32Offset++];
        for (u8Cnt = 0; u8Cnt < 8; u8Cnt++)
        {
            if ((u16CrcResult & 0x1) == 0x1)
            {
                u16CrcResult >>= 1;
                u16CrcResult  ^= 0x8408;
            }
            else
            {
                u16CrcResult >>= 1;
            }
        }
        u32Size--;
    }
    u16CrcResult = (uint16_t)(~u16CrcResult);

    return u16CrcResult;
}

/*******************************************************************************
 * Flash helpers
 ******************************************************************************/
en_result_t HC32_FlashWriteBytes(uint32_t u32Addr, uint8_t *pu8Data, uint32_t u32Len)
{
    volatile uint32_t u32Timeout = FLASH_TIMEOUT;
    uint32_t          u32Index   = 0u;

    FLASH_BYPASS();
    FLASH->CR_f.RO = 0u;

    FLASH_UNLOCK_ALL();

    if (FLASH_END_ADDR < (u32Addr + u32Len - 1u))
    {
        return ErrorInvalidParameter;
    }

    /* FLASH写模式 */
    u32Timeout = FLASH_TIMEOUT;
    while (FLASH->CR_f.OP != 1)
    {
        FLASH_BYPASS();
        FLASH->CR_f.OP = 1;
        if (0x0u == u32Timeout--)
        {
            return ErrorTimeout;
        }
    }

    /* busy */
    u32Timeout = FLASH_TIMEOUT;
    while (READ_REG32_BIT(FLASH->CR, FLASH_CR_BUSY_Msk))
    {
        if (0x0u == u32Timeout--)
        {
            return ErrorTimeout;
        }
    }

    /* write data byte */
    for (u32Index = 0u; u32Index < u32Len; u32Index++)
    {
        *((volatile uint8_t *)u32Addr) = pu8Data[u32Index];

        u32Timeout = FLASH_TIMEOUT;
        while (READ_REG32_BIT(FLASH->CR, FLASH_CR_BUSY_Msk))
        {
            if (0x0u == u32Timeout--)
            {
                return ErrorTimeout;
            }
        }

        /* verify */
        if (pu8Data[u32Index] != *((volatile uint8_t *)u32Addr))
        {
            return Error;
        }
        u32Addr++;
    }

    /* FLASH读模式 */
    u32Timeout = FLASH_TIMEOUT;
    while (FLASH->CR_f.OP != 0)
    {
        FLASH_BYPASS();
        FLASH->CR_f.OP = 0;
        if (0x0u == u32Timeout--)
        {
            return ErrorTimeout;
        }
    }

    FLASH_LOCK_ALL();
    FLASH_BYPASS();
    FLASH->CR_f.RO = 1u;
    return Ok;
}

void HC32_FlashReadBytes(uint32_t u32Addr, uint8_t *pu8ReadBuff, uint32_t u32ByteLength)
{
    uint32_t i;
    for (i = 0; i < u32ByteLength; i++)
    {
        pu8ReadBuff[i] = *((unsigned char *)(u32Addr + i));
    }
}

en_result_t HC32_FlashEraseSector(uint32_t u32SectorAddr)
{
    volatile uint32_t u32Timeout = FLASH_TIMEOUT;

    if (FLASH_END_ADDR < u32SectorAddr)
    {
        return ErrorInvalidParameter;
    }

    FLASH_BYPASS();
    FLASH->CR_f.RO = 0u;

    FLASH_UNLOCK_ALL();

    /* Sector erase mode (OP=2) */
    u32Timeout = FLASH_TIMEOUT;
    while (FLASH->CR_f.OP != 2)
    {
        FLASH_BYPASS();
        FLASH->CR_f.OP = 2;
        if (0x0u == u32Timeout--)
        {
            return ErrorTimeout;
        }
    }

    /* Wait busy */
    u32Timeout = FLASH_TIMEOUT;
    while (READ_REG32_BIT(FLASH->CR, FLASH_CR_BUSY_Msk))
    {
        if (0x0u == u32Timeout--)
        {
            return ErrorTimeout;
        }
    }

    /* Trigger erase */
    *((volatile uint32_t *)u32SectorAddr) = 0u;

    /* Wait busy */
    u32Timeout = FLASH_TIMEOUT;
    while (READ_REG32_BIT(FLASH->CR, FLASH_CR_BUSY_Msk))
    {
        if (0x0u == u32Timeout--)
        {
            return ErrorTimeout;
        }
    }

    /* Back to read mode */
    u32Timeout = FLASH_TIMEOUT;
    while (FLASH->CR_f.OP != 0)
    {
        FLASH_BYPASS();
        FLASH->CR_f.OP = 0;
        if (0x0u == u32Timeout--)
        {
            return ErrorTimeout;
        }
    }

    FLASH_LOCK_ALL();
    FLASH_BYPASS();
    FLASH->CR_f.RO = 1u;
    return Ok;
}
