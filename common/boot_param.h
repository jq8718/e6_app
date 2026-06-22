/**
 *******************************************************************************
 * @file  boot_param.h
 * @brief Boot parameter area management for APP IAP
 *******************************************************************************
 */

#ifndef __BOOT_PARAM_H__
#define __BOOT_PARAM_H__

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "base_types.h"

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/**
 * @brief  Boot parameter area structure (32 bytes, fits in one Flash row)
 */
typedef struct
{
    uint32_t magic;        /*!< BOOT_PARAM_MAGIC (0x48434C42 = "HCLB") */
    uint32_t state;        /*!< State machine value */
    uint32_t app_addr;     /*!< APP start address in Flash */
    uint32_t app_size;     /*!< APP firmware size in bytes */
    uint32_t app_crc;      /*!< APP firmware CRC16 */
    uint32_t boot_count;   /*!< Reserved */
    uint32_t app_version;  /*!< APP firmware version number */
    uint32_t header_crc;   /*!< CRC16 of the first 28 bytes */
} stc_boot_param_t;

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/* Boot parameter area magic ("HCLB" = HC32L Boot) */
#define BOOT_PARAM_MAGIC        ((uint32_t)0x48434C42u)

/* State machine values */
#define BOOT_PARAM_STATE_EMPTY            ((uint32_t)0xFFFFFFFFu)
#define BOOT_PARAM_STATE_UPDATE_REQUEST   ((uint32_t)0xA5A55A5Au)
#define BOOT_PARAM_STATE_IMAGE_PENDING    ((uint32_t)0x5AA55AA5u)
#define BOOT_PARAM_STATE_IMAGE_VALID      ((uint32_t)0x55AAAA55u)
#define BOOT_PARAM_STATE_IMAGE_INVALID    ((uint32_t)0xDEAD0001u)

/* Flash layout */
#define FLASH_SECTOR_SIZE (0x200u)
#define FLASH_SECTOR_NUM  (0x80u)
#define FLASH_START_ADDR  ((uint32_t)0x00000000u)
#define FLASH_SIZE        (FLASH_SECTOR_NUM * FLASH_SECTOR_SIZE)
#define FLASH_END_ADDR    ((uint32_t)(FLASH_START_ADDR + FLASH_SIZE - 1u))

/* Bootloader region: first 16 sectors (8KB) */
#define BOOT_SIZE         (16u * FLASH_SECTOR_SIZE)
#define BOOT_PARAM_ADDR   (FLASH_START_ADDR + BOOT_SIZE - FLASH_SECTOR_SIZE)

/* APP region */
#define APP_ADDR          (FLASH_START_ADDR + BOOT_SIZE)
#define APP_MAX_SIZE      (FLASH_SIZE - BOOT_SIZE - FLASH_SECTOR_SIZE)

/*******************************************************************************
 * Global function prototypes
 ******************************************************************************/
void        BootParam_Read(stc_boot_param_t *pstcParam);
en_result_t BootParam_WriteState(uint32_t u32State);
uint16_t    BootParam_CalcHeaderCrc(const stc_boot_param_t *pstcParam);

/* Low-level Flash helpers */
en_result_t HC32_FlashWriteBytes(uint32_t u32Addr, uint8_t *pu8Data, uint32_t u32Len);
void        HC32_FlashReadBytes(uint32_t u32Addr, uint8_t *pu8ReadBuff, uint32_t u32ByteLength);
uint32_t    HC32_CalCrc16(uint8_t *pu8Data, uint32_t u32Offset, uint32_t u32Size);
en_result_t HC32_FlashEraseSector(uint32_t u32SectorAddr);

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_PARAM_H__ */
