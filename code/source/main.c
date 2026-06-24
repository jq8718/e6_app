/**
 *******************************************************************************
 * @file  main.c
 * @brief This file provides example of BTIM
 @verbatim
   Change Logs:
   Date             Author          Notes
   2024-12-02       MADS            First version
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2024, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */

/******************************************************************************
 * Include files
 ******************************************************************************/
#include "btim.h"
#include "ddl.h"
#include "gpio.h"
#include "board_stkhc32l021.h"
#include "i2c_iap_app.h"
#include "boot_param.h"
#include "lpuart.h"
#include "sysctrl.h"
/******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
static void APP_DebugUartInit(void);
static void APP_DebugLog(const char *pStr);
static void APP_DebugLogHex32(uint32_t u32Val);

uint16_t aim_delay_on=60;
uint16_t aim_duration_on=70;

uint16_t fill_delay_on=60;
uint16_t fill_duration_on=70;


/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @brief  Main function
 * @retval int32_t return value, if needed
 */
int32_t main(void)
{
    stc_boot_param_t stcBootParam;
    uint32_t u32BootState;

    IAP_APP_Init(); /* I2C IAP 初始化 */

    /* 读取启动前的参数区 state */
    BootParam_Read(&stcBootParam);
    u32BootState = stcBootParam.state;

    /* 每次启动打印 boot state */
    APP_DebugUartInit();
    APP_DebugLog("E6_APP boot 0x");
    APP_DebugLogHex32(u32BootState);
    if (BOOT_PARAM_STATE_IMAGE_VALID == u32BootState)
    {
        APP_DebugLog(" IMAGE_VALID - last boot OK");
    }
    else if (BOOT_PARAM_STATE_IMAGE_PENDING == u32BootState)
    {
        APP_DebugLog(" IMAGE_PENDING - last JUMP_TO_APP crash");
    }
    else if (BOOT_PARAM_STATE_UPDATE_REQUEST == u32BootState)
    {
        APP_DebugLog(" UPDATE_REQUEST - enter IAP mode");
    }
    else if (BOOT_PARAM_STATE_EMPTY == u32BootState)
    {
        APP_DebugLog(" EMPTY - first boot");
    }
    APP_DebugLog("\r\n");

    STK_EvsyncConfig(); /* 帧同步输入 初始化 */
    STK_AIMConfig(); /* AIM控制输出 初始化 */
    STK_AIM_ON(); /* AIM点亮 */
    STK_LEDConfig(); /* AIM控制输出 初始化 */
    STK_LED_ON(); /* 补光开启 */
    Btim0Config(aim_delay_on); /* BTIM0初始化  帧同步后延时 时长*/
    Btim1Config(aim_duration_on); /* BTIM1初始化  瞄准灯亮的 时长*/

    /* APP 初始化完成，标记 IMAGE_VALID */
    if (u32BootState != BOOT_PARAM_STATE_IMAGE_VALID)
    {
        BootParam_WriteState(BOOT_PARAM_STATE_IMAGE_VALID);
        APP_DebugLog("E6_APP state -> IMAGE_VALID\r\n");
    }

    while (1)
    {
        IAP_APP_Task(); /* I2C IAP 后台轮询 */
    }
}

/*******************************************************************************
 * LPUART1 debug output (PA01 TXD, 115200 8N1)
 ******************************************************************************/
static void APP_DebugUartInit(void)
{
    stc_lpuart_init_t stcLpuart;

    /* Enable LPUART1 clock */
    SYSCTRL_PeriphClockEnable(PeriphClockLpuart1);
    SYSCTRL_PeriphReset(PeriphResetLpuart1);

    /* PA01 as LPUART1 TXD */
    GPIO_PA01_AF_LPUART1_TXD();

    stc_gpio_init_t stcGpio;
    GPIO_StcInit(&stcGpio);
    stcGpio.u32Pin  = GPIO_PIN_01;
    stcGpio.u32Mode = GPIO_MD_OUTPUT_PP;
    GPIOA_Init(&stcGpio);

    LPUART_StcInit(&stcLpuart);
    stcLpuart.u32TransMode              = LPUART_MODE_TX;
    stcLpuart.u32FrameLength            = LPUART_FRAME_LEN_8B_NOPAR;
    stcLpuart.u32Parity                 = LPUART_B8_MULTI_DATA_OR_ADDR;
    stcLpuart.u32StopBits               = LPUART_STOPBITS_1;
    stcLpuart.u32HwControl              = LPUART_HWCONTROL_NONE;
    stcLpuart.u32BaudRateGenSelect      = LPUART_BAUD_NORMAL;
    stcLpuart.stcBaudRate.u32SclkSelect = LPUART_SCLK_SEL_PCLK;
    stcLpuart.stcBaudRate.u32Sclk       = SYSCTRL_HclkFreqGet();
    stcLpuart.stcBaudRate.u32Baud       = 115200u;

    LPUART_Init(LPUART1, &stcLpuart);

    LPUART_FuncEnable(LPUART1, LPUART_FUN_REN);
}

static void APP_DebugLog(const char *pStr)
{
    while (*pStr != '\0')
    {
        while (!READ_REG_BIT(LPUART1->ISR, LPUART_ISR_TXE))
        {
            ;
        }
        WRITE_REG8(LPUART1->SBUF_f.DATA, (uint8_t)*pStr);
        pStr++;
    }
}

static void APP_DebugLogHex32(uint32_t u32Val)
{
    const char hex[] = "0123456789ABCDEF";
    char buf[11];
    uint8_t i;

    buf[0] = '\0';
    for (i = 0; i < 8; i++)
    {
        buf[7 - i] = hex[u32Val & 0xFu];
        u32Val >>= 4;
    }
    buf[8] = '\0';
    APP_DebugLog(buf);
}



/**
 * @brief  BTIM0中断服务函数
 * @retval None
 */

void Ctim0_IRQHandler(void)
{
    static boolean_t bFlag = FALSE;

    if (TRUE == BTIM_IntFlagGet(BTIM0, BTIM_FLAG_UI))
    {
        STK_AIM_ON(); /* AIM点亮 */
       // STK_LED_OFF(); /* 补光关闭 */
        BTIM_CounterSet(BTIM1,0);
				//Btim1Config(aim_duration_on); /* BTIM1初始化  瞄准灯亮的 时长*/
        BTIM_Enable(BTIM1);

        BTIM_Disable(BTIM0);
        BTIM_CounterSet(BTIM0,0);
        BTIM_IntFlagClear(BTIM0, BTIM_FLAG_UI); /* 清除BTIM0的溢出中断标志位 */
    }

 if (TRUE == BTIM_IntFlagGet(BTIM1, BTIM_FLAG_UI))
    {
        STK_AIM_OFF(); /* AIM关闭 */
       // STK_LED_ON(); /* 补光开启 */
        BTIM_Disable(BTIM1);
        BTIM_CounterSet(BTIM1,0);
        BTIM_IntFlagClear(BTIM1, BTIM_FLAG_UI); /* 清除BTIM1的溢出中断标志位 */
    }
}


/**
 * @brief  PortA中断服务函数
 * @retval None
 */
void PortA_IRQHandler(void)
{
    if (TRUE == GPIO_IntFlagGet(STK_EVSYNC_PORT, STK_EVSYNC_PIN)) /* 获取中断状态 */
    {
        GPIO_IntFlagClear(STK_EVSYNC_PORT, STK_EVSYNC_PIN); /* 清除中断标志位 */
        BTIM_CounterSet(BTIM0,0);
				//Btim0Config(aim_delay_on); /* BTIM0初始化  帧同步后延时 时长*/
        BTIM_Enable(BTIM0); /* 启动BTIM0运行 */
    }
}

/******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
