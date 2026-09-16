/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mcp2515.h"
#include "dht11.h"
#include "hcsr04.h"
#include "adc.h"
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
uint16_t simSpeed = 0;  // giữ giả lập tạm cho speed
uint32_t lastDHTReadTime = 0;
volatile uint32_t adc_debug = 0;
volatile uint16_t speed_debug = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void DWT_Delay_Init(void);
void DWT_Delay_us(uint32_t us);
uint16_t ReadPotentiometer_Speed(void);
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
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  DWT_Delay_Init();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitTypeDef gpioLed = {0};
  gpioLed.Pin = GPIO_PIN_13;
  gpioLed.Mode = GPIO_MODE_OUTPUT_PP;
  gpioLed.Pull = GPIO_NOPULL;
  gpioLed.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &gpioLed);


  HAL_Delay(300); // giữ sáng rõ 500ms để bạn chắc chắn quan sát được

  MCP2515_t canDev;
  canDev.hspi = &hspi1;
  canDev.cs_port = CAN_CS_GPIO_Port;
  canDev.cs_pin = CAN_CS_Pin;
  MCP2515_Init(&canDev);

  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADC_Start(&hadc1);


  CAN_Frame_t speedFrame, distFrame, tempHumiFrame;
  uint32_t lastSendTime = 0;
  uint32_t lastDistanceReadTime = 0;
  uint16_t distance = 999;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


	  uint16_t speed = ReadPotentiometer_Speed();


	  if (HAL_GetTick() - lastDistanceReadTime >= 100)
	  	  {
	      	  lastDistanceReadTime = HAL_GetTick();
	          uint16_t newDist = HCSR04_ReadDistance();
	          if (newDist > 0 && newDist <= 400)
	          	  {
	                  distance = newDist;
	              }
	          if (distance < 30)
	        	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // sáng
	          else
	        	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   // tắt
	      }


	      // ---- Temp/Humi: đọc thật từ DHT11, nhưng CHỈ mỗi 2 giây (cảm biến rất chậm) ----
	      static int8_t  temp = 25;
	      static uint8_t humi = 60;
	      if (HAL_GetTick() - lastDHTReadTime >= 2000)
	      {
	          lastDHTReadTime = HAL_GetTick();
	          int8_t newTemp;
	          uint8_t newHumi;
	          if (DHT11_Read(&newTemp, &newHumi))
	          {
	              temp = newTemp;
	              humi = newHumi;
	          }
	          // Nếu đọc lỗi (checksum sai/timeout) -> giữ nguyên giá trị cũ, không cập nhật giá trị rác
	      }

	      // ---- Gửi CAN mỗi 100ms như cũ ----
	      if (HAL_GetTick() - lastSendTime >= 100)
	      {
	          lastSendTime = HAL_GetTick();

	          speedFrame.id = 0x100;
	          speedFrame.dlc = 2;
	          speedFrame.data[0] = speed >> 8;
	          speedFrame.data[1] = speed & 0xFF;
	          MCP2515_SendFrame(&canDev, &speedFrame);

	          distFrame.id = 0x101;
	          distFrame.dlc = 2;
	          distFrame.data[0] = distance >> 8;
	          distFrame.data[1] = distance & 0xFF;
	          MCP2515_SendFrame(&canDev, &distFrame);

	          tempHumiFrame.id = 0x102;
	          tempHumiFrame.dlc = 2;
	          tempHumiFrame.data[0] = (uint8_t)temp;
	          tempHumiFrame.data[1] = humi;
	          MCP2515_SendFrame(&canDev, &tempHumiFrame);


	      }

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void DWT_Delay_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void DWT_Delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

uint16_t ReadPotentiometer_Speed(void)
{
    HAL_ADC_Start(&hadc1);

    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);
        return 0;
    }

    adc_debug = HAL_ADC_GetValue(&hadc1);

    HAL_ADC_Stop(&hadc1);

    speed_debug = (uint16_t)((adc_debug * 180U) / 4095U);

    return speed_debug;
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
#ifdef USE_FULL_ASSERT
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
