/**
 *******************************************************************************
 * @file  i2c_iap_app.c
 * @brief I2C IAP for APP — partial protocol (JUMP_TO_BOOT only)
 *******************************************************************************
 */

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "i2c_iap_app.h"
#include "boot_param.h"
#include "hsi2c.h"
#include "gpio.h"
#include "sysctrl.h"
#include "hc32l021.h"

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
typedef struct
{
    uint8_t  u8ErrCode;
    uint8_t  au8Payload[16];
    uint16_t u16PayloadLen;
} stc_cmd_result_t;

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void APP_SetError(uint8_t u8ErrCode);
static void APP_BuildResponse(uint8_t u8Cmd, uint8_t u8Seq, uint8_t u8ErrCode,
                              const uint8_t *pu8Payload, uint16_t u16PayloadLen);
static void APP_ParseAndExecute(void);

static stc_cmd_result_t CmdJumpToBoot(void);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/* Virtual registers */
static volatile uint8_t  s_u8RegStatus;
static volatile uint8_t  s_u8RegError;
static volatile uint16_t s_u16RegTxLen;

/* MAILBOX buffers */
static uint8_t  s_au8RxMailbox[IAP_MAILBOX_SIZE];
static uint8_t  s_au8TxMailbox[IAP_MAILBOX_SIZE];

/* I2C transaction state */
static volatile uint16_t s_u16SubAddr;
static volatile uint16_t s_u16RxIdx;
static volatile uint16_t s_u16TxIdx;
static volatile boolean_t s_bSubAddrValid;
static volatile uint8_t  s_u8TxLenByteIdx;

/* Control flags */
static volatile boolean_t s_bCtrlCommit;
static volatile boolean_t s_bJumpPending;

/* Received frame byte count */
static volatile uint16_t s_u16RxFrameLen;

/*******************************************************************************
 * I2C Slave Interrupt Handler (overrides weak Hsi2c_IRQHandler)
 ******************************************************************************/
void Hsi2c_IRQHandler(void)
{
    uint32_t u32Flags = HSI2C->SSR;

    /* Clear pending flags (write 1-to-clear) */
    HSI2C->SSCR = u32Flags & HSI2C_SLAVE_FLAG_CLR_ALL;

    /* ---- Receive data (Master → APP) ---- */
    if (u32Flags & HSI2C_SLAVE_FLAG_RDF)
    {
        uint8_t u8Data;
        if (Ok == HSI2C_SlaveReadData(HSI2C, &u8Data))
        {
            if (!s_bSubAddrValid)
            {
                /* First byte after address match = sub-address */
                s_u16SubAddr    = u8Data;
                s_bSubAddrValid = TRUE;

                if ((s_u16SubAddr >= REG_MAILBOX_START) && (s_u16SubAddr <= REG_MAILBOX_END))
                {
                    s_u16RxIdx = (uint16_t)(s_u16SubAddr - REG_MAILBOX_START);
                    s_u16TxIdx = (uint16_t)(s_u16SubAddr - REG_MAILBOX_START);
                }
            }
            else
            {
                /* Sub-address already received — route data */
                switch (s_u16SubAddr)
                {
                    case REG_CTRL:
                        if (CTRL_COMMIT == u8Data)
                        {
                            if (STATUS_IDLE == s_u8RegStatus)
                            {
                                s_bCtrlCommit = TRUE;
                            }
                        }
                        else if (CTRL_CLEAR == u8Data)
                        {
                            if ((STATUS_RESP_READY == s_u8RegStatus) ||
                                (STATUS_ERROR == s_u8RegStatus))
                            {
                                s_u8RegStatus = STATUS_IDLE;
                                s_u8RegError  = ERROR_CODE_OK;
                                s_u16RegTxLen = 0u;
                            }
                        }
                        else if (CTRL_ABORT == u8Data)
                        {
                            s_u8RegStatus = STATUS_IDLE;
                            s_u8RegError  = ERROR_CODE_OK;
                            s_u16RegTxLen = 0u;
                            s_bJumpPending = FALSE;
                        }
                        break;

                    default:
                        if ((s_u16SubAddr >= REG_MAILBOX_START) &&
                            (s_u16SubAddr <= REG_MAILBOX_END))
                        {
                            if (STATUS_IDLE == s_u8RegStatus)
                            {
                                if (s_u16RxIdx < IAP_MAILBOX_SIZE)
                                {
                                    s_au8RxMailbox[s_u16RxIdx++] = u8Data;
                                }
                            }
                            s_u16SubAddr++;
                        }
                        break;
                }
            }
        }
    }

    /* ---- Transmit data (APP → Master) ---- */
    if (u32Flags & HSI2C_SLAVE_FLAG_TDF)
    {
        uint8_t u8TxData = 0x00u;

        switch (s_u16SubAddr)
        {
            case REG_STATUS:
                u8TxData = s_u8RegStatus;
                break;

            case REG_ERROR:
                u8TxData = s_u8RegError;
                break;

            case REG_TX_LEN:
                if (STATUS_RESP_READY == s_u8RegStatus)
                {
                    if (0u == s_u8TxLenByteIdx)
                    {
                        u8TxData = (uint8_t)s_u16RegTxLen;
                        s_u8TxLenByteIdx = 1u;
                    }
                    else
                    {
                        u8TxData = (uint8_t)(s_u16RegTxLen >> 8);
                    }
                }
                break;

            default:
                if ((s_u16SubAddr >= REG_MAILBOX_START) &&
                    (s_u16SubAddr <= REG_MAILBOX_END))
                {
                    if (s_u16TxIdx < s_u16RegTxLen)
                    {
                        u8TxData = s_au8TxMailbox[s_u16TxIdx++];
                    }
                    s_u16SubAddr++;
                }
                break;
        }

        HSI2C_SlaveWriteData(HSI2C, u8TxData);
    }

    /* ---- STOP or Repeated START ---- */
    if (u32Flags & (HSI2C_SLAVE_FLAG_RSF | HSI2C_SLAVE_FLAG_SDF))
    {
        if (u32Flags & HSI2C_SLAVE_FLAG_SDF)
        {
            /* STOP: save received frame length for COMMIT */
            if ((s_u16SubAddr >= REG_MAILBOX_START) &&
                (s_u16SubAddr <= REG_MAILBOX_END))
            {
                s_u16RxFrameLen = s_u16RxIdx;
            }
            s_bSubAddrValid  = FALSE;
            s_u8TxLenByteIdx = 0u;
        }
        HSI2C->STAR = 0u;
    }

    /* ---- Address Valid / Address Match ---- */
    if (u32Flags & (HSI2C_SLAVE_FLAG_AVF | HSI2C_SLAVE_FLAG_AM0F | HSI2C_SLAVE_FLAG_AM1F))
    {
        (void)HSI2C->SASR;
    }

    /* ---- Transmit ACK: release SCL ---- */
    if (u32Flags & HSI2C_SLAVE_FLAG_TAF)
    {
        HSI2C->STAR = 0u;
    }
}

/*******************************************************************************
 * Set error state
 ******************************************************************************/
static void APP_SetError(uint8_t u8ErrCode)
{
    s_u8RegStatus = STATUS_ERROR;
    s_u8RegError  = u8ErrCode;
}

/*******************************************************************************
 * Build response frame in TX MAILBOX
 ******************************************************************************/
static void APP_BuildResponse(uint8_t u8Cmd, uint8_t u8Seq, uint8_t u8ErrCode,
                              const uint8_t *pu8Payload, uint16_t u16PayloadLen)
{
    uint16_t u16TotalPayloadLen = 1u + u16PayloadLen;
    uint16_t u16CrcOffset;
    uint16_t u16Crc;

    s_au8TxMailbox[HDR_OFFSET_MAGIC0]     = FRAME_MAGIC0;
    s_au8TxMailbox[HDR_OFFSET_MAGIC1]     = FRAME_MAGIC1;
    s_au8TxMailbox[HDR_OFFSET_VERSION]    = IAP_PROTOCOL_VERSION;
    s_au8TxMailbox[HDR_OFFSET_CMD]        = u8Cmd;
    s_au8TxMailbox[HDR_OFFSET_SEQ]        = u8Seq;
    s_au8TxMailbox[HDR_OFFSET_FLAGS]      = 0x00u;
    s_au8TxMailbox[HDR_OFFSET_PAYLOADLEN] = (uint8_t)u16TotalPayloadLen;
    s_au8TxMailbox[HDR_OFFSET_PAYLOADLEN + 1u] = (uint8_t)(u16TotalPayloadLen >> 8);

    s_au8TxMailbox[HDR_OFFSET_PAYLOAD] = u8ErrCode;
    if ((NULL != pu8Payload) && (u16PayloadLen > 0u))
    {
        (void)memcpy(&s_au8TxMailbox[HDR_OFFSET_PAYLOAD + 1u], pu8Payload, u16PayloadLen);
    }

    u16CrcOffset = IAP_HEADER_SIZE + u16TotalPayloadLen;
    u16Crc       = (uint16_t)HC32_CalCrc16(s_au8TxMailbox, 0u, u16CrcOffset);

    s_au8TxMailbox[u16CrcOffset]       = (uint8_t)u16Crc;
    s_au8TxMailbox[u16CrcOffset + 1u]  = (uint8_t)(u16Crc >> 8);

    s_u16RegTxLen  = u16CrcOffset + IAP_CRC_SIZE;
    s_u16TxIdx     = 0u;
}

/*******************************************************************************
 * Parse and execute command from MAILBOX
 ******************************************************************************/
static void APP_ParseAndExecute(void)
{
    uint16_t u16PayloadLen;
    uint16_t u16FrameLen;
    uint16_t u16CrcOffset;
    uint16_t u16CrcRecv;
    uint16_t u16CrcCalc;
    uint8_t  u8Cmd;
    uint8_t  u8Seq;

    /* Basic frame size check */
    if (s_u16RxFrameLen < IAP_FRAME_MIN)
    {
        APP_BuildResponse(0u, 0u, ERROR_CODE_FRAME, NULL, 0u);
        APP_SetError(ERROR_CODE_FRAME);
        return;
    }

    u16PayloadLen = (uint16_t)(s_au8RxMailbox[HDR_OFFSET_PAYLOADLEN] |
                    ((uint16_t)s_au8RxMailbox[HDR_OFFSET_PAYLOADLEN + 1u] << 8));
    u16FrameLen   = (uint16_t)(IAP_HEADER_SIZE + u16PayloadLen + IAP_CRC_SIZE);

    if ((u16PayloadLen > IAP_PAYLOAD_MAX) || (u16FrameLen > IAP_FRAME_MAX) ||
        (s_u16RxFrameLen < u16FrameLen))
    {
        u8Cmd = s_au8RxMailbox[HDR_OFFSET_CMD];
        u8Seq = s_au8RxMailbox[HDR_OFFSET_SEQ];
        APP_BuildResponse(u8Cmd, u8Seq, ERROR_CODE_FRAME, NULL, 0u);
        APP_SetError(ERROR_CODE_FRAME);
        return;
    }

    /* Validate header */
    if ((s_au8RxMailbox[HDR_OFFSET_MAGIC0] != FRAME_MAGIC0) ||
        (s_au8RxMailbox[HDR_OFFSET_MAGIC1] != FRAME_MAGIC1) ||
        (s_au8RxMailbox[HDR_OFFSET_VERSION] != IAP_PROTOCOL_VERSION) ||
        (s_au8RxMailbox[HDR_OFFSET_FLAGS] != 0x00u))
    {
        u8Cmd = s_au8RxMailbox[HDR_OFFSET_CMD];
        u8Seq = s_au8RxMailbox[HDR_OFFSET_SEQ];
        APP_BuildResponse(u8Cmd, u8Seq, ERROR_CODE_FRAME, NULL, 0u);
        APP_SetError(ERROR_CODE_FRAME);
        return;
    }

    /* Validate CRC16 */
    u16CrcOffset = (uint16_t)(IAP_HEADER_SIZE + u16PayloadLen);
    u16CrcRecv   = (uint16_t)(s_au8RxMailbox[u16CrcOffset] |
                   ((uint16_t)s_au8RxMailbox[u16CrcOffset + 1u] << 8));
    u16CrcCalc   = (uint16_t)HC32_CalCrc16(s_au8RxMailbox, 0u, u16CrcOffset);

    if (u16CrcRecv != u16CrcCalc)
    {
        u8Cmd = s_au8RxMailbox[HDR_OFFSET_CMD];
        u8Seq = s_au8RxMailbox[HDR_OFFSET_SEQ];
        APP_BuildResponse(u8Cmd, u8Seq, ERROR_CODE_CRC, NULL, 0u);
        APP_SetError(ERROR_CODE_CRC);
        return;
    }

    /* Frame valid — execute command */
    s_u8RegStatus = STATUS_BUSY;
    s_u8RegError  = ERROR_CODE_OK;

    u8Cmd = s_au8RxMailbox[HDR_OFFSET_CMD];
    u8Seq = s_au8RxMailbox[HDR_OFFSET_SEQ];

    stc_cmd_result_t stcResult;

    switch (u8Cmd)
    {
        case CMD_JUMP_TO_BOOT:
            stcResult = CmdJumpToBoot();
            break;

        default:
            stcResult.u8ErrCode     = ERROR_CODE_UNSUPPORTED;
            stcResult.u16PayloadLen = 0u;
            break;
    }

    APP_BuildResponse(u8Cmd, u8Seq, stcResult.u8ErrCode,
                      stcResult.au8Payload, stcResult.u16PayloadLen);

    if (ERROR_CODE_OK == stcResult.u8ErrCode)
    {
        s_u8RegStatus = STATUS_RESP_READY;
    }
    else
    {
        s_u8RegStatus = STATUS_ERROR;
        s_u8RegError  = stcResult.u8ErrCode;
    }
}

/*******************************************************************************
 * JUMP_TO_BOOT command handler — marks UPDATE_REQUEST and requests reset
 ******************************************************************************/
static stc_cmd_result_t CmdJumpToBoot(void)
{
    stc_cmd_result_t stcResult;

    stcResult.u8ErrCode     = ERROR_CODE_OK;
    stcResult.u16PayloadLen = 0u;

    /* Write UPDATE_REQUEST to boot parameter area */
    if (Ok != BootParam_WriteState(BOOT_PARAM_STATE_UPDATE_REQUEST))
    {
        stcResult.u8ErrCode = ERROR_CODE_FLASH;
        return stcResult;
    }

    /* Signal main loop to reset after CLEAR */
    s_bJumpPending = TRUE;

    return stcResult;
}

/*******************************************************************************
 * I2C slave & GPIO initialization
 ******************************************************************************/
static void APP_I2cInit(void)
{
    stc_hsi2c_slave_init_t stcSlaveInit;

    /* Enable HSI2C clock */
    SYSCTRL_PeriphClockEnable(PeriphClockHsi2c);

    /* Configure PA06(SDA) and PA07(SCL) as open-drain output */
    SYSCTRL_PeriphClockEnable(PeriphClockGpio);

    stc_gpio_init_t stcGpioInit = {0};
    GPIO_StcInit(&stcGpioInit);
    stcGpioInit.bOutputValue = TRUE;
    stcGpioInit.u32Pin       = GPIO_PIN_06 | GPIO_PIN_07;
    stcGpioInit.u32Mode      = GPIO_MD_OUTPUT_OD;
    GPIOA_Init(&stcGpioInit);
    GPIO_PA06_AF_HSI2C_SDA();
    GPIO_PA07_AF_HSI2C_SCL();

    /* Slave init */
    HSI2C_SlaveStcInit(&stcSlaveInit);
    stcSlaveInit.u32SlaveAddr0 = I2C_SLAVE_ADDR;
    stcSlaveInit.u8SubAddrSize = 1u;
    stcSlaveInit.enDir         = Hsi2cMasterWriteSlaveRead;
    stcSlaveInit.stcSlaveConfig1.u32FuncSelect =
        HSI2C_SLAVE_TXDSTALL_ENABLE |
        HSI2C_SLAVE_RXSTALL_ENABLE  |
        HSI2C_SLAVE_ACKSTALL_ENABLE;

    HSI2C_SlaveInit(HSI2C, &stcSlaveInit, 48000000u);

    /* Clear flags & enable interrupts */
    HSI2C_SlaveFlagClear(HSI2C, HSI2C_SLAVE_FLAG_CLR_ALL);
    HSI2C_SlaveIntEnable(HSI2C,
                         HSI2C_SLAVE_INT_AVIE |
                         HSI2C_SLAVE_INT_SDIE |
                         HSI2C_SLAVE_INT_RDIE |
                         HSI2C_SLAVE_INT_TDIE |
                         HSI2C_SLAVE_INT_FEIE |
                         HSI2C_SLAVE_INT_BEIE);

    EnableNvic(HSI2C_IRQn, IrqPriorityLevel0, TRUE);
}

/*******************************************************************************
 * Public API
 ******************************************************************************/
void IAP_APP_Init(void)
{
    /* Clear all state */
    s_u8RegStatus     = STATUS_IDLE;
    s_u8RegError      = ERROR_CODE_OK;
    s_u16RegTxLen     = 0u;
    s_u16RxIdx        = 0u;
    s_u16TxIdx        = 0u;
    s_u16SubAddr      = 0u;
    s_u16RxFrameLen   = 0u;
    s_bSubAddrValid   = FALSE;
    s_u8TxLenByteIdx  = 0u;
    s_bCtrlCommit     = FALSE;
    s_bJumpPending    = FALSE;

    (void)memset(s_au8RxMailbox, 0, sizeof(s_au8RxMailbox));
    (void)memset(s_au8TxMailbox, 0, sizeof(s_au8TxMailbox));

    /* Init I2C slave hardware */
    APP_I2cInit();
}

void IAP_APP_Task(void)
{
    /* Handle COMMIT */
    if (s_bCtrlCommit)
    {
        s_bCtrlCommit = FALSE;

        if (STATUS_IDLE != s_u8RegStatus)
        {
            return;
        }

        APP_ParseAndExecute();
    }

    /* Handle JUMP request (after host CLEAR) */
    if (s_bJumpPending && (STATUS_IDLE == s_u8RegStatus))
    {
        s_bJumpPending = FALSE;

        /* Small delay before reset */
        volatile uint32_t i;
        for (i = 0u; i < 48000u; i++) { ; }

        NVIC_SystemReset();
    }
}
