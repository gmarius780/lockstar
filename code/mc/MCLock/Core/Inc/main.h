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

#define Receive_Pid_Vars 			0x0001
#define Send_Measured_Data 			0x0002
#define Measure_by_Mode				0x0003
#define Receive_AOM_Data 			0x0004
#define Check_AOM_data 				0x0005
#define Measure_AOM_Data 			0x0006
#define Start_Signal_Output 		0x0007
#define Stop_Signal_Output 			0x0008
#define Start_Old_Lock 				0x0009
#define Set_Scan_Range	 			0x000a
#define Save_Waypoints_Lock 		0x000b
#define Send_AOM_Data		 		0x000c


#define PID_Save_Data 				0x0100
#define Ziegler_Nichols 			0x0101
#define AOM_Save_Data 				0x0102
#define Test_AOM_Data 				0x0103
#define Continuous_PID 				0x0104
#define Measure_Both 				0x0105
#define Continuous_Signal 			0x0106
#define Old_Lock 					0x0107
#define Lock_And_Measure 			0x0108
#define Continuous_Lock 			0x0109

//DEFINE WHICH MODULE TO RUN!!
#define IO_TEST_MODULE
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
