/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32h7xx_hal.h"

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
#define LED6_Pin GPIO_PIN_9
#define LED6_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

#define _20BIT_MIN					0xfff80000
#define _20BIT_MAX					0x0007ffff
#define _16Bit_MIN					0x8000
#define _16Bit_MAX					0x7fff

//#define Receive_Pid_Vars 			0x0001
//#define Send_Measured_Data 			0x0002
//#define Measure_by_Mode				0x0003
//#define Receive_AOM_Data 			0x0004
//#define Check_AOM_data 				0x0005
//#define Measure_AOM_Data 			0x0006
////#define Start_Signal_Output 		0x0007
//#define Stop_Signal_Output 			0x0008
//#define Start_Old_Lock 				0x0009
//#define Set_Scan_Range	 			0x000a
//#define Save_Waypoints_Lock 		0x000b
//#define Send_AOM_Data		 		0x000c


//#define PID_Save_Data 				0x0100
//#define Ziegler_Nichols 			0x0101
//#define AOM_Save_Data 				0x0102
//#define Test_AOM_Data 				0x0103
//#define Continuous_PID 				0x0104
//#define Measure_Both 				0x0105
//#define Continuous_Signal 			0x0106
//#define Old_Lock 					0x0107
//#define Lock_And_Measure 			0x0108
//#define Continuous_Lock 			0x0109

//DEFINE WHICH MODULE TO RUN!!
//#define AWG_PID_MODULE
//#define AWG_MODULE
//#define SINGLE_PID_MODULE
//#define LINEARIZATION_MODULE
#define ANALOG_OUTPUT_MODULE
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
