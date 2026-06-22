/**
 *******************************************************************************
 * @file  board_stkhc32l021.h
 * @brief Header file for BSP functions
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

#ifndef __BOARD_STKHC32L021_H__
#define __BOARD_STKHC32L021_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "ddl.h"

/**
 * @addtogroup HC32L021_DDL 驱动库
 * @{
 */

/**
 * @addtogroup DDL_STK STK模块驱动库
 * @{
 */
/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup STK_Global_Types STK全局类型定义
 * @{
 */

/**
 * @brief 芯片信息结构体指针
 */
typedef struct
{
    uint8_t  u8UID[10];          /*!< 10字节UID(唯一识别号) */
    char_t  *pcProductNum;       /*!< 32字节产品型号"HC32xxx……" */
    uint32_t u32FlashSize;       /*!< FLASH容量(Byte) */
    uint32_t u32RamSize;         /*!< RAM容量(Byte) */
    uint16_t u16PinCount;        /*!< 管脚数量 */
    uint16_t u16FlashSectorSize; /*!< FLASH Sector大小(Byte) */
} stc_stk_chip_info_t;
/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup STK_Global_Macros STK全局宏定义
 * @{
 */

/**
 * @brief  USER KEY 
 */
#define E6_V1 

#ifdef E6_V1

//PA05   瞄准灯控制 输出
//PA09   补光灯控制 输出
//PA02   帧同步信号 输入 
//PA6   SDA
//PA7		SCL
//帧同步信号输入
#define STK_EVSYNC_PORT          GPIOA
#define STK_EVSYNC_PIN           GPIO_PIN_02
#define STK_EVSYNC_KEY_PRESSED() (GPIO_PA02_READ() ? FALSE : TRUE)

 //瞄准灯  控制IO
#define STK_AIM_PORT   GPIOA
#define STK_AIM_PIN    GPIO_PIN_05
#define STK_AIM_OFF()   GPIO_PA05_RESET()
#define STK_AIM_ON()  GPIO_PA05_SET()


 //补光灯  控制IO
#define STK_LED_PORT   GPIOA
#define STK_LED_PIN    GPIO_PIN_09
#define STK_LED_OFF()   GPIO_PA09_RESET()
#define STK_LED_ON()  GPIO_PA09_SET()

 //SDA  控制IO
#define STK_SDA_PORT   GPIOA
#define STK_SDA_PIN    GPIO_PIN_06
#define STK_SDA_LOW()   GPIO_PA06_RESET()
#define STK_SDA_HIG()  GPIO_PA06_SET()

 //SCL  控制IO
#define STK_SCL_PORT   GPIOA
#define STK_SCL_PIN    GPIO_PIN_07
#define STK_SCL_LOW()   GPIO_PA07_RESET()
#define STK_SCL_HIG()  GPIO_PA07_SET()

#else
//PA06   瞄准灯控制 输出
//PA12   帧同步信号 输入 

//帧同步信号输入
#define STK_EVSYNC_PORT          GPIOA
#define STK_EVSYNC_PIN           GPIO_PIN_12
#define STK_EVSYNC_KEY_PRESSED() (GPIO_PA12_READ() ? FALSE : TRUE)

 //瞄准灯  控制IO
#define STK_AIM_PORT   GPIOA
#define STK_AIM_PIN    GPIO_PIN_06
#define STK_AIM_ON()   GPIO_PA06_RESET()
#define STK_AIM_OFF()  GPIO_PA06_SET()


#endif



/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/
/*******************************************************************************
  Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup STK_Global_Functions STK全局函数定义
 * @{
 */
void STK_EvsyncConfig(void); /* STK 按键初始化 */
void STK_AIMConfig(void);     /* STK LED初始化 */
void STK_LEDConfig(void);
void Btim0Config(uint16_t u16Period);
void Btim1Config(uint16_t u16Period);
void STK_ChipInfoGet(stc_stk_chip_info_t *pstcChipInfo); /* 获取芯片相关信息 */
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_STKHC32L021_H__ */
/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
