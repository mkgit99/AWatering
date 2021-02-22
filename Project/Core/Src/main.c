/* USER CODE BEGIN Header */
/*!
 * @file main.c
 *
 * @mainpage Automatic Watering System
 *
 * @section intro_sec Introduction
 *
 * This is documentation for AWatering project.
 *
 * The project uses microcontroller from STM32, mini water pomp, soil moisture sensor,
 * DHT 11 sensor, display OLED SSD1306 and HC-05 Bluetooth module.
 *
 * @section author Author
 *
 * Written by Michał Kobuszewski and Szymon Kasiński for studies.
 */
/**
  ******************************************************************************
  * @file main.c
  * @brief
  * This is the main file of AutoWatering project.
  * It contains functions to measure, water, and display the results of measurements.
  * Also, it has the timer function implemented.
  *
  * @author: Szymon Kasiński
  * 		 Michał Kobuszewski
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "DHT.h"
#include "GFX.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t moisture = 0;
float percent_moisture;  		//Zmienna przechowująca wylgotność gleby, wyrażona w %
char msg[60];					//Zmienna przechowująca wiadomość z danymi, wysyłana przez BT
char msg_oled[58];		 		//Zmienna przechowująca wiadomość z danymi, wysyłana na OLED
char buff[6];			 		//Zmienna przechowująca aktualną komendę wysłaną przez BT.

uint8_t measure_counter_flag; 	//Flaga decydująca, czy należy wykonać pomiar z czujników
uint8_t measure_counter;		//Licznik wykonujący inkrementację po danym czasie (6 sekund)

uint8_t pump_flag;				//Flaga decydująca, czy należy uruchomić pompę wodną
uint8_t pump_counter;			//Licznik który decyduje o wyłączeniu pompy po danym czasie (6 sekund)

DHT_DataTypedef DHT;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void watering(void);
void measuring(void);
void handle_main(void);
void handle_bluetooth(void);
void handle_oled(void);
void init_oled(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC2_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  SSD1306_init();
  HAL_UART_Receive_IT(&huart2, (uint8_t *) buff , sizeof(buff));
  init_oled();
  HAL_TIM_Base_Start_IT(&htim2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		handle_main();
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_ADC12;
  PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
 * This function turns on the water pomp. It sets the water pin on STM
 * to high and logic flag to true.
 */
void watering(void) {
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
	//Informacja na wyświetlaczu o podlewaniu
	SSD1306_display_clear();
	SSD1306_display_repaint();
	GFX_draw_string(0, 0, (unsigned char*) "WATERING", WHITE, BLACK, 1, 2);
	SSD1306_display_repaint();
	pump_flag = 1;
}

/**
 * This function measures, transforms soil moisture value to percents
 * and displays the results in bluetooth terminal and on display
 */
void measuring(void) {
	//Odczytanie wilgotności gleby z czujnika M335
	HAL_ADC_Start(&hadc2);
	HAL_ADC_PollForConversion(&hadc2, HAL_MAX_DELAY);
	moisture = HAL_ADC_GetValue(&hadc2);
	//Odczytanie temperatury i wilgotności z czujnika DHT11
	DHT_GetData(&DHT);
	percent_moisture = (1550 / (float) moisture - 0.5) * 100;
	//Wyświetlanie pomiarów na urządzeniu bluetooth
	sprintf(msg,
			"Moisture = %0.1f%%\r\nTemperature = %0.0f st.C\r\nHumidity = %0.0f%%\r\n\n",
			percent_moisture, DHT.Temperature, DHT.Humidity);
	sprintf(msg_oled, "Moisture = %0.1f%%", percent_moisture);
	HAL_UART_Transmit(&huart2, (uint8_t*) msg, sizeof(msg), HAL_MAX_DELAY);
	handle_oled();
}

/**
 * This function initializes default message on the OLED in a moment of turning on the STM.
 */
void init_oled(void) {
	GFX_draw_string(0, 0, (unsigned char*) "Starting", WHITE, BLACK, 1, 2);
	SSD1306_start_scroll_right(0, 20);
	SSD1306_display_repaint();
	HAL_Delay(3000);
	SSD1306_display_clear();
	SSD1306_display_repaint();
	measuring();
}

/**
 * This function displays measurement on the OLED SSD1306
 */
void handle_oled(void) {
	GFX_draw_string(0, 0, (char*) msg_oled, WHITE, BLACK, 1, 2);
	SSD1306_start_scroll_right(0, 20);
	SSD1306_display_repaint();
}

/**
 * This is function of the main functionality of project.
 * It measures, waters, shows results in BT and starts timer after one loop.
 */
void handle_main(void) {
	//Wykonanie pomiarów, jeśli flaga jest włączona
	if (measure_counter_flag) {
		measuring();
		//Podlewanie, gdy wilgotność gleby spadnie poniżej 40%
		if (percent_moisture <= 40) {
			watering();
		}
		measure_counter_flag = 0;
		HAL_TIM_Base_Start_IT(&htim2);
	}
}

/**
 * @brief This function is responsible for timer, it sets logic flags,
 * dependent on the amount of seconds.
 * @param htim
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM2) {
		measure_counter++;

		//Warunek odpowiadający za wykonywanie pomiarów
		//po danym czasie
		if (measure_counter >= 200) {
			measure_counter_flag = 1;
			measure_counter = 0;
			HAL_TIM_Base_Stop_IT(htim);
		}
		//Warunek odpowiadający za uruchamianie pompy
		//i utrzymywanie jej wysokiego stanu przez dany czas
		if (pump_flag) {
			pump_counter++;
			if (pump_counter >= 1) {
				pump_flag = 0;
				pump_counter = 0;
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
				SSD1306_display_clear();
				SSD1306_display_repaint();
				handle_oled();
			}
		}
	}
}

/**
 * @brief This function manages bluetooth commands
 * @param huart Communication module
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART2) {
		if (strcmp(buff, "woda\r\n") == 0) {
			watering();
		}
		if (strcmp(buff, "dane\r\n") == 0) {
			measuring();
		}
		HAL_UART_Receive_IT(&huart2, (uint8_t*) buff, sizeof(buff));
	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
