/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void cppmain(void);

//HAS to be implemented by the modules
void start(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CLR5_Pin GPIO_PIN_2
#define CLR5_GPIO_Port GPIOE
#define DAC_2_Sync_Pin GPIO_PIN_6
#define DAC_2_Sync_GPIO_Port GPIOF
#define ADC_CNV_Pin GPIO_PIN_4
#define ADC_CNV_GPIO_Port GPIOA
#define DigitalOut_Pin GPIO_PIN_0
#define DigitalOut_GPIO_Port GPIOB
#define DigitalIn_Pin GPIO_PIN_1
#define DigitalIn_GPIO_Port GPIOB
#define DigitalIn_EXTI_IRQn EXTI1_IRQn
#define LED1_Pin GPIO_PIN_8
#define LED1_GPIO_Port GPIOD
#define LED2_Pin GPIO_PIN_9
#define LED2_GPIO_Port GPIOD
#define LED3_Pin GPIO_PIN_10
#define LED3_GPIO_Port GPIOD
#define LED4_Pin GPIO_PIN_11
#define LED4_GPIO_Port GPIOD
#define DAC_1_Sync_Pin GPIO_PIN_8
#define DAC_1_Sync_GPIO_Port GPIOG
#define LED5_Pin GPIO_PIN_8
#define LED5_GPIO_Port GPIOA
#define LED6_Pin GPIO_PIN_9
#define LED6_GPIO_Port GPIOA
#define Timing_Pin GPIO_PIN_1
#define Timing_GPIO_Port GPIOD
#define Pi_Int_Pin GPIO_PIN_6
#define Pi_Int_GPIO_Port GPIOD
#define Pi_Int_EXTI_IRQn EXTI9_5_IRQn
#define CLR6_Pin GPIO_PIN_11
#define CLR6_GPIO_Port GPIOG
/* USER CODE BEGIN Private defines */

#define _20BIT_MIN					0xfff80000
#define _20BIT_MAX					0x0007ffff
#define _16Bit_MIN					0x8000
#define _16Bit_MAX					0x7fff

//CAN BE DIFFERENT FOR EACH MODULE:
#define SCOPE_BUFFER_LENGTH 2000 //total number of points the scope can record for the inputs and outputs together. The user can choose how to distribute those points
//DEFINE WHICH MODULE TO RUN!!
#define DOUBLE_DITHER_LOCK_MODULE
//#define SINGLE_PID_MODULE
//#define LINEARIZATION_MODULE

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
