/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY_UP_Pin GPIO_PIN_3
#define KEY_UP_GPIO_Port GPIOA
#define KEY_OK_Pin GPIO_PIN_5
#define KEY_OK_GPIO_Port GPIOA
#define ENC_KEY_Pin GPIO_PIN_8
#define ENC_KEY_GPIO_Port GPIOA
#define ENC_B_Pin GPIO_PIN_14
#define ENC_B_GPIO_Port GPIOB
#define ENC_A_Pin GPIO_PIN_15
#define ENC_A_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#ifndef KEY_NEXT_Pin
#define KEY_NEXT_Pin GPIO_PIN_1
#define KEY_NEXT_GPIO_Port GPIOA
#endif
#ifndef ENC_KEY_Pin
#define ENC_KEY_Pin GPIO_PIN_8
#define ENC_KEY_GPIO_Port GPIOA
#endif
#ifndef ENC_A_Pin
#define ENC_A_Pin GPIO_PIN_15
#define ENC_A_GPIO_Port GPIOB
#endif
#ifndef ENC_B_Pin
#define ENC_B_Pin GPIO_PIN_14
#define ENC_B_GPIO_Port GPIOB
#endif

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
