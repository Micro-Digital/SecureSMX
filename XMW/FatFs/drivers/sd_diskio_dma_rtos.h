/**
  ******************************************************************************
  * @file    sd_diskio_dma_rtos_template.h
  * @author  MCD Application Team
  * @brief   Header for sd_diskio_dma_rtos.c module. This is template file that
             needs to be adjusted and copied into the application project.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics. All rights reserved.
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                       opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
**/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SD_DISKIO_H
#define __SD_DISKIO_H

/* Includes ------------------------------------------------------------------*/
#if defined(SB_BRD_STMICRO_STM32746GEVAL)
#include "stm32756g_eval_sd.h"
#elif defined(SB_BRD_STMICRO_STM32F746G_DISCOVERY)
#include "stm32746g_discovery_sd.h"
#endif
#include "cmsis_os2.h"
#define  osCMSIS  0x20001U  //MDI:DM: Added since cmsis_os2.h doesn't define but FatFs needs
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
extern const Diskio_drvTypeDef  SD_Driver;

#endif /* __SD_DISKIO_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

