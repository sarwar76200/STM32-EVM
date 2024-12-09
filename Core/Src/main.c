/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "liquidcrystal_i2c.h"
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
I2C_HandleTypeDef hi2c2;

/* USER CODE BEGIN PV */

GPIO_InitTypeDef GPIO_InitStructPrivate = { 0 };
uint32_t previousMillis = 0;
uint32_t currentMillis = 0;
uint8_t keyPressed = 0;

int32_t last_when_pressed = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void clear_console() {
	HD44780_Clear();
}

void show_text(int x, int y, const char *text, int clear) {
	if (clear) {
		clear_console();
	}

	HD44780_SetCursor(x, y);
	HD44780_PrintStr(text);
}

void show_int(int x, int y, int64_t text, int clear) {
	if (clear) {
		HD44780_Clear();
	}
	char str[20];
	itoa(text, str, 0);
	HD44780_SetCursor(x, y);
	HD44780_PrintStr(str);
}


#define HOME_STATE 0
#define MENU_STATE 1
#define VOTE_STATE 2
#define STAT_STATE 3
#define COUNT_STATE 4
#define SUCCESS_STATE 5
#define ADMIN_LOGIN_STATE 6
#define PUBLISH_STATE 7
#define PROLOGUE_STATE 8


int admin_authorized = 0;


int transition_stack[100] = {0};
int stack_ptr = 1;

int stack_size() {
  return stack_ptr;
}
int stack_top() {
  assert(stack_size() > 0);
  return transition_stack[stack_ptr - 1];
}

void stack_push(int value) {
  transition_stack[stack_ptr++] = value;
}

void stack_pop() {
  if (stack_size() <= 1) {
    return;
  }
  --stack_ptr;
}

void stack_reset() {
  stack_ptr = 1;
}

int current_state = PROLOGUE_STATE;
const char* headings[][2] = {{"Welcome voter", "1.Vote #.Exit"}, {"Main menu", "1.Stats #.Exit"}, {"Select candidate", "1.A 2.B 3.C 4.D"}, {"See stat", "1.Vote count #.Exit"}, {"", ""}, {"Vote Success!", "Thank you"}, {"Admin Logged In", "1.Menu #.Exit"}, {"Voting finished", "Winner: "}, {"Pending", "Admin Login"}};
const char* headings_admin[][2] = {{"Welcome admin", "1.Menu #.Exit"}, {"1.Stats 2.Take", "3.End vote"}, {"Select candidate", "1.A 2.B 3.C 4.D"}, {"See stat", "1.Vote count #.Exit"}, {"", ""}, {"Vote Success!", "#.Exit"}, {"Admin Logged In", "1.Menu #.Exit"}, {"Voting finished", "Winner: "}};

int counts[4];

void cast_vote(int option) {
  ++counts[option - 1];
}
char get_winner() {
  int max_val = 0;
  char winner = '?';
  for (int i = 0; i < 4; ++i) {
    if (counts[i] > max_val) {
      max_val = counts[i];
      winner = (char) ('A' + i);
    }
  }
  return winner;
}

void reset_all() {
  admin_authorized = 1;
  current_state = HOME_STATE;
  counts[0] = counts[1] = counts[2] = counts[3] = 1;
  stack_reset();
}

void show_heading() {
  if (current_state == PUBLISH_STATE) {
    char winner[2] = {get_winner()};
    char text[] = "Winner: ";
    strcat(text, winner);
    show_text(0, 1, text, 0);
  } else if (current_state == COUNT_STATE) {
    char a_label[] = "A: ";
    char a_count[5];
    itoa(counts[0], a_count, 10);
    strcat(a_label, a_count);
    show_text(0, 0, a_label, 0);

    char b_label[] = "B: ";
    char b_count[5];
    itoa(counts[1], b_count, 10);
    strcat(b_label, b_count);
    show_text(8, 0, b_label, 0);

    char c_label[] = "C: ";
    char c_count[5];
    itoa(counts[2], c_count, 10);
    strcat(c_label, c_count);
    show_text(0, 1, c_label, 0);

    char d_label[] = "D: ";
    char d_count[5];
    itoa(counts[3], d_count, 10);
    strcat(d_label, d_count);
    show_text(8, 1, d_label, 0);
  } else {
    if (admin_authorized) {
      show_text(0, 0, headings_admin[current_state][0], 0);
      show_text(0, 1, headings_admin[current_state][1], 0);
    } else {
      show_text(0, 0, headings[current_state][0], 0);
      show_text(0, 1, headings[current_state][1], 0);
    }
  }
}


void go_back() {
  stack_pop();
  current_state = stack_top();
}

char key_presses[1000];
int key_press_count = 0;


int scan_for_admin() {
  char pass[] = "699";
  if (key_press_count < strlen(pass)) {
    return 0;
  }
  if (key_presses[key_press_count - 1] == pass[2] && key_presses[key_press_count - 2] == pass[1] && key_presses[key_press_count - 3] == pass[0]) {
    return 1;
  }
  return 0;
}

void transition(char key_pressed) {
  char pressed[2] = {key_pressed};
  show_text(15, 0, pressed, 0);
  HAL_Delay(100);

  char old_state = current_state;

  if (scan_for_admin()) {
    current_state = HOME_STATE;
    admin_authorized = 1;
    clear_console();
    stack_reset();
    return;
  }

  if (key_pressed == '?') {
    exit(0);
    return;
  }
  if (key_pressed == '*') {
    if (current_state == SUCCESS_STATE || current_state == PROLOGUE_STATE) return;
    go_back();
    clear_console();
    return;
  }

  if (key_pressed == '#') {
    if (current_state != PROLOGUE_STATE && current_state != SUCCESS_STATE) {
      current_state = HOME_STATE;
    }
    clear_console();
    stack_reset();
    return;
  }
  if (current_state == HOME_STATE) {
    if (key_pressed == '1') {
      current_state = admin_authorized ? MENU_STATE : VOTE_STATE;
    } else if (key_pressed == '#') {
      current_state = HOME_STATE;
    }
  } else if (current_state == MENU_STATE) {
    if (key_pressed == '1') {
      current_state = STAT_STATE;
    } else if (key_pressed == '#') {
      current_state = HOME_STATE;
    } else if (key_pressed == '2') {
      current_state = HOME_STATE;
      admin_authorized = 0;
    } else if (key_pressed == '3') {
      current_state = PUBLISH_STATE;
    }
  } else if (current_state == VOTE_STATE) {
    cast_vote(key_pressed - '0');
    current_state = SUCCESS_STATE;

  } else if (current_state == STAT_STATE) {
    if (key_pressed == '1') {
      current_state = COUNT_STATE;
    } else if (key_pressed == '#') {
      current_state = HOME_STATE;
    }
  }
  stack_push(current_state);
  if (current_state != old_state) {
    clear_console();
  }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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
	MX_I2C2_Init();
	/* USER CODE BEGIN 2 */
	HD44780_Init(2);

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, 1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 1);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		show_heading();
		char c = keyPressed;
		if (currentMillis > last_when_pressed + 500) {
			key_presses[key_press_count++] = c;
			transition(c);
			last_when_pressed = currentMillis;
		}
	}
	/* USER CODE END WHILE */
	/* USER CODE BEGIN 3 */
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C2_Init(void) {

	/* USER CODE BEGIN I2C2_Init 0 */

	/* USER CODE END I2C2_Init 0 */

	/* USER CODE BEGIN I2C2_Init 1 */

	/* USER CODE END I2C2_Init 1 */
	hi2c2.Instance = I2C2;
	hi2c2.Init.ClockSpeed = 100000;
	hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c2.Init.OwnAddress1 = 0;
	hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c2.Init.OwnAddress2 = 0;
	hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C2_Init 2 */

	/* USER CODE END I2C2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5,
			GPIO_PIN_RESET);

	/*Configure GPIO pin : PA15 */
	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : PB3 PB4 PB5 */
	GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : PB6 PB7 PB8 PB9 */
	GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	currentMillis = HAL_GetTick();
	if (currentMillis - previousMillis > 10) {
		/*Configure GPIO pins : PB6 PB7 PB8 PB9 to GPIO_INPUT*/
		GPIO_InitStructPrivate.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8
				| GPIO_PIN_9;
		GPIO_InitStructPrivate.Mode = GPIO_MODE_INPUT;
		GPIO_InitStructPrivate.Pull = GPIO_NOPULL;
		GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_LOW;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStructPrivate);

		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, 1);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 0);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 0);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 0);
		if (GPIO_Pin == GPIO_PIN_6 && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6)) {
			keyPressed = 68; //ASCII value of D
		} else if (GPIO_Pin == GPIO_PIN_7
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)) {
			keyPressed = 67; //ASCII value of C
		} else if (GPIO_Pin == GPIO_PIN_8
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8)) {
			keyPressed = 66; //ASCII value of B
		} else if (GPIO_Pin == GPIO_PIN_9
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9)) {
			keyPressed = 65; //ASCII value of A
		}

		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, 0);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 1);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 0);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 0);
		if (GPIO_Pin == GPIO_PIN_6 && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6)) {
			keyPressed = 35; //ASCII value of #
		} else if (GPIO_Pin == GPIO_PIN_7
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)) {
			keyPressed = 57; //ASCII value of 9
		} else if (GPIO_Pin == GPIO_PIN_8
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8)) {
			keyPressed = 54; //ASCII value of 6
		} else if (GPIO_Pin == GPIO_PIN_9
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9)) {
			keyPressed = 51; //ASCII value of 3
		}

		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, 0);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 0);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 1);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 0);
		if (GPIO_Pin == GPIO_PIN_6 && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6)) {
			keyPressed = 48; //ASCII value of 0
		} else if (GPIO_Pin == GPIO_PIN_7
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)) {
			keyPressed = 56; //ASCII value of 8
		} else if (GPIO_Pin == GPIO_PIN_8
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8)) {
			keyPressed = 53; //ASCII value of 5
		} else if (GPIO_Pin == GPIO_PIN_9
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9)) {
			keyPressed = 50; //ASCII value of 2
		}

		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, 0);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 0);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 0);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 1);
		if (GPIO_Pin == GPIO_PIN_6 && HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6)) {
			keyPressed = 42; //ASCII value of *
		} else if (GPIO_Pin == GPIO_PIN_7
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)) {
			keyPressed = 55; //ASCII value of 7
		} else if (GPIO_Pin == GPIO_PIN_8
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8)) {
			keyPressed = 52; //ASCII value of 4
		} else if (GPIO_Pin == GPIO_PIN_9
				&& HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9)) {
			keyPressed = 49; //ASCII value of 1
		}

		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, 1);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 1);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 1);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 1);
		/*Configure GPIO pins : PB6 PB7 PB8 PB9 back to EXTI*/
		GPIO_InitStructPrivate.Mode = GPIO_MODE_IT_RISING;
		GPIO_InitStructPrivate.Pull = GPIO_PULLDOWN;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStructPrivate);
		previousMillis = currentMillis;
	}
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
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
