/**
 *******************************************************************************
 * @file  board_stkhc32l021.c
 * @brief Source file for BSP functions.
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

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "board_stkhc32l021.h"
#include "gpio.h"
#include "hc32l021.h"
#include "sysctrl.h"
#include "btim.h"
/**
 * @addtogroup HC32L021_DDL 驱动库
 * @{
 */

/**
 * @defgroup DDL_STK STK模块驱动库
 * @brief STK Driver Library STK模块驱动库
 * @{
 */
/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/
/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/
/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/
/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/
/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @defgroup STK_Global_Functions STK全局函数定义
 * @brief STK Function Library STK功能函数库
 * @{
 */

/**
 * @brief  帧同步信号触发 初始化
 * @retval None
 */
void STK_EvsyncConfig(void)
{
    stc_gpio_init_t stcGpioInit = {0};

    GPIO_StcInit(&stcGpioInit);                 /* 结构体变量初始值初始化 */
    SYSCTRL_PeriphClockEnable(PeriphClockGpio); /* 打开GPIO外设时钟门控 */
    stcGpioInit.u32Mode   = GPIO_MD_INT_INPUT;      /* 端口方向配置 */
    stcGpioInit.u32PullUp    = GPIO_PULL_NONE;       /* 端口上拉配置 */
    stcGpioInit.u32ExternInt = GPIO_EXTI_RISING;        /* 端口外部中断触发方式配置 */
    stcGpioInit.u32Pin    = STK_EVSYNC_PIN;       /* 端口引脚配置 */
    GPIOA_Init(&stcGpioInit);                   /* GPIO 端口初始化 */
		EnableNvic(PORTA_IRQn, IrqPriorityLevel2, TRUE); /* 使能端口PORTA系统中断 */
}


/**
 * @brief  AIM初始化
 * @retval None
 */
void STK_AIMConfig(void)
{
    stc_gpio_init_t stcGpioInit = {0};

    GPIO_StcInit(&stcGpioInit);                 /* 结构体变量初始值初始化 */
    SYSCTRL_PeriphClockEnable(PeriphClockGpio); /* 打开GPIO外设时钟门控 */
    stcGpioInit.u32Mode   = GPIO_MD_OUTPUT_OD;  /* 端口方向配置 */
    stcGpioInit.u32PullUp = GPIO_PULL_NONE;     /* 端口上拉配置 */
    stcGpioInit.u32Pin    = STK_AIM_PIN;        /* 端口引脚配置 */
    GPIOA_Init(&stcGpioInit);                   /* GPIO 端口初始化 */
}



 /* @brief  LED初始化
 * @retval None
 */
void STK_LEDConfig(void)
{
    stc_gpio_init_t stcGpioInit = {0};

    GPIO_StcInit(&stcGpioInit);                 /* 结构体变量初始值初始化 */
    SYSCTRL_PeriphClockEnable(PeriphClockGpio); /* 打开GPIO外设时钟门控 */
    stcGpioInit.u32Mode   = GPIO_MD_OUTPUT_OD;  /* 端口方向配置 */
    stcGpioInit.u32PullUp = GPIO_PULL_NONE;     /* 端口上拉配置 */
    stcGpioInit.u32Pin    = STK_LED_PIN;        /* 端口引脚配置 */
    GPIOA_Init(&stcGpioInit);                   /* GPIO 端口初始化 */
}


/**
 * @brief  获取芯片相关信息
 * @param  [in] pstcChipInfo 芯片信息结构体指针 @ref stc_stk_chip_info_t
 * @retval None
 */
void STK_ChipInfoGet(stc_stk_chip_info_t *pstcChipInfo)
{
    memcpy((void *)(&pstcChipInfo->u8UID[0]), (uint8_t *)0x001008B4, 10);
    pstcChipInfo->pcProductNum       = (char_t *)0x00100740u;
    pstcChipInfo->u32FlashSize       = *((uint32_t *)0x00100760u);
    pstcChipInfo->u32RamSize         = *((uint32_t *)0x00100764u);
    pstcChipInfo->u16FlashSectorSize = *((uint16_t *)0x00100768u);
    pstcChipInfo->u16PinCount        = *((uint16_t *)0x0010076Au);
}



/**
 * @brief  初始化BTIM0
 * @retval None
 */
 void Btim0Config(uint16_t u16Period)
{
    stc_btim_init_t stcBtimInit = {0};
		u16Period = u16Period * 6.25;
    /* 配置BTIM0/1/2有效，GTIM0无效 */
    SYSCTRL_FuncEnable(SYSCTRL_FUNC_CTIMER0_USE_BTIM); /* 配置BTIM0/1/2有效，GTIM0无效 */
    SYSCTRL_PeriphClockEnable(PeriphClockCtim0);       /* CTIM0 时钟使能 */

    BTIM_StcInit(&stcBtimInit);                               /* 结构体变量初始值初始化 */
    stcBtimInit.u32Mode            = BTIM_MD_PCLK;            /* 工作模式:定时器模式，计数时钟源来自PCLK */
    stcBtimInit.u32OneShotEn       = BTIM_CONTINUOUS_COUNTER; /* 连续计数模式 */
    stcBtimInit.u32Prescaler       = BTIM_COUNTER_CLK_DIV64; /* 对计数时钟进行预除频 */
    stcBtimInit.u32ToggleEn        = BTIM_TOGGLE_DISABLE;     /* TOG输出不使能 */
    stcBtimInit.u32AutoReloadValue = u16Period - 1;           /* 自动重载寄存ARR赋值,计数周期为PRS*(ARR+1)*TPCLK */
    BTIM_Init(BTIM0, &stcBtimInit);

    BTIM_IntFlagClear(BTIM0, BTIM_FLAG_UI);          /* 清除溢出中断标志位 */
    BTIM_IntEnable(BTIM0, BTIM_INT_UI);              /* 允许BTIM0溢出中断    */
    EnableNvic(CTIM0_IRQn, IrqPriorityLevel3, TRUE); /* NVIC使能 */
}

/**
 * @brief  初始化BTIM0
 * @retval None
 */
 void Btim1Config(uint16_t u16Period)
{
    stc_btim_init_t stcBtimInit = {0};
		u16Period = u16Period * 6.25;
    /* 配置BTIM0/1/2有效，GTIM0无效 */
    SYSCTRL_FuncEnable(SYSCTRL_FUNC_CTIMER0_USE_BTIM); /* 配置BTIM0/1/2有效，GTIM0无效 */
    SYSCTRL_PeriphClockEnable(PeriphClockCtim0);       /* CTIM0 时钟使能 */

    BTIM_StcInit(&stcBtimInit);                               /* 结构体变量初始值初始化 */
    stcBtimInit.u32Mode            = BTIM_MD_PCLK;            /* 工作模式:定时器模式，计数时钟源来自PCLK */
    stcBtimInit.u32OneShotEn       = BTIM_CONTINUOUS_COUNTER; /* 连续计数模式 */
    stcBtimInit.u32Prescaler       = BTIM_COUNTER_CLK_DIV64; /* 对计数时钟进行预除频 */
    stcBtimInit.u32ToggleEn        = BTIM_TOGGLE_DISABLE;     /* TOG输出不使能 */
    stcBtimInit.u32AutoReloadValue = u16Period - 1;           /* 自动重载寄存ARR赋值,计数周期为PRS*(ARR+1)*TPCLK */
    BTIM_Init(BTIM1, &stcBtimInit);

    BTIM_IntFlagClear(BTIM1, BTIM_FLAG_UI);          /* 清除溢出中断标志位 */
    BTIM_IntEnable(BTIM1, BTIM_INT_UI);              /* 允许BTIM0溢出中断    */
    EnableNvic(CTIM0_IRQn, IrqPriorityLevel3, TRUE); /* NVIC使能 */
}


/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
