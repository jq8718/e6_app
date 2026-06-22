/**
 *******************************************************************************
 * @file  i2c_iap_app.h
 * @brief I2C IAP protocol header for APP side (partial protocol)
 *******************************************************************************
 */

#ifndef __I2C_IAP_APP_H__
#define __I2C_IAP_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "base_types.h"

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/* I2C slave address */
#define I2C_SLAVE_ADDR       (0x20u)

/*===================================================================
 *  Protocol Constants
 *===================================================================*/
#define IAP_MAILBOX_SIZE      (530u)
#define IAP_FLASH_DATA_MAX    (512u)
#define IAP_PAYLOAD_MAX       (4u + IAP_FLASH_DATA_MAX)  /* 516 */
#define IAP_HEADER_SIZE       (8u)
#define IAP_CRC_SIZE          (2u)
#define IAP_FRAME_MIN         (10u)   /* Header(8) + CRC(2) */
#define IAP_FRAME_MAX         (526u)
#define IAP_PROTOCOL_VERSION  (0x01u)

#define FRAME_MAGIC0          (0x6Du)
#define FRAME_MAGIC1          (0xACu)

/* Frame header offsets */
#define HDR_OFFSET_MAGIC0     (0u)
#define HDR_OFFSET_MAGIC1     (1u)
#define HDR_OFFSET_VERSION    (2u)
#define HDR_OFFSET_CMD        (3u)
#define HDR_OFFSET_SEQ        (4u)
#define HDR_OFFSET_FLAGS      (5u)
#define HDR_OFFSET_PAYLOADLEN (6u)
#define HDR_OFFSET_PAYLOAD    (8u)

/*===================================================================
 *  Virtual Register Addresses
 *===================================================================*/
#define REG_STATUS        (0x00u)
#define REG_ERROR         (0x01u)
#define REG_CTRL          (0x02u)
#define REG_TX_LEN        (0x06u)
#define REG_MAILBOX_START (0x20u)
#define REG_MAILBOX_END   (0x231u)

/*===================================================================
 *  STATUS Values
 *===================================================================*/
#define STATUS_IDLE       (0x00u)
#define STATUS_BUSY       (0x02u)
#define STATUS_RESP_READY (0x03u)
#define STATUS_ERROR      (0x04u)

/*===================================================================
 *  CTRL Values
 *===================================================================*/
#define CTRL_COMMIT (0xA5u)
#define CTRL_CLEAR  (0x5Au)
#define CTRL_ABORT  (0xC3u)

/*===================================================================
 *  ERROR Codes
 *===================================================================*/
#define ERROR_CODE_OK            (0x00u)
#define ERROR_CODE_CRC           (0x01u)
#define ERROR_CODE_FRAME         (0x02u)
#define ERROR_CODE_UNSUPPORTED   (0x03u)
#define ERROR_CODE_ADDR          (0x04u)
#define ERROR_CODE_FLASH         (0x05u)
#define ERROR_CODE_BUSY          (0x06u)
#define ERROR_CODE_SEQ           (0x07u)
#define ERROR_CODE_APP_INVALID   (0x08u)

/*===================================================================
 *  Command Codes (only JUMP_TO_BOOT supported by APP)
 *===================================================================*/
#define CMD_HANDSHAKE    (0x20u)
#define CMD_JUMP_TO_APP  (0x21u)
#define CMD_APP_DOWNLOAD (0x22u)
#define CMD_JUMP_TO_BOOT (0x23u)    /* APP: jump to bootloader */
#define CMD_ERASE_FLASH  (0x24u)
#define CMD_CRC_FLASH    (0x25u)

/*******************************************************************************
 * Global function prototypes
 ******************************************************************************/
void IAP_APP_Init(void);
void IAP_APP_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_IAP_APP_H__ */
