/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdbool.h>
#include "stepper.h"
#include "tim.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PACKET_START 0x3C3C3C3C // "<<<<"
#define PACKET_END   0x3E3E3E3E // ">>>>"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* Active stepper motor object */
stepper_t my_stepper0;
stepper_t my_stepper1;
stepper_t my_stepper2;
stepper_t my_stepper3;
stepper_t my_stepper4;

// Message Queues to handle serial data and speed data
typedef struct {
	uint8_t data_pack[28];
} Message_t;

osMessageQueueId_t messageQueueHandle;

/////////////////////////////
typedef struct {
	float speed[5];
} Command_t;

osMessageQueueId_t commandQueueHandle;

// Task Identifiers

osThreadId_t parsingID;
osThreadId_t settingID;

// Task Attributes

const osThreadAttr_t parseTask_attributes = { .name = "parseTask", .stack_size =
		128 * 4, .priority = (osPriority_t) osPriorityNormal, };

const osThreadAttr_t setTimerTask_attributes = { .name = "setSpeedTask",
		.stack_size = 128 * 4, .priority = (osPriority_t) osPriorityHigh, };
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* Primary Tasks to handle signal and pwm*/

// Processing signal task
void parsePacketTask(void *argument) {
	Message_t rx_msg;
	uint32_t header;
	uint32_t tail;
	Command_t cmd_msg;
	for (;;) {
		if (osMessageQueueGet(messageQueueHandle, &rx_msg, NULL, osWaitForever)
				== osOK) {

			memcpy(&header, &rx_msg, 4);

			memcpy(&tail, &rx_msg.data_pack[24], 4);

			if (header == PACKET_START && tail == PACKET_END) {

				memcpy(cmd_msg.speed, &rx_msg.data_pack[4],
						sizeof(cmd_msg.speed));

				osMessageQueuePut(commandQueueHandle, &cmd_msg, 0, 0);

			}
		}
	}

}
// Setting PWM Signal task
void setTimersTask(void *argument) {
	Command_t cmd_msg;

	for (;;) {
		osStatus_t status = osMessageQueueGet(commandQueueHandle, &cmd_msg,
		NULL, 500);
		if (status == osOK) {

			// Set the direction accordingly
			float speed[5];

			for (int i = 0; i < 5; i++) {
				speed[i] = cmd_msg.speed[i];
			}
			// stepper 0
			if (speed[0] < 0) {
				stepper_set_direction(&my_stepper0, false);
				speed[0] = -speed[0];
			}
			else {
				stepper_set_direction(&my_stepper0, true);
			}
			stepper_move_speed(&my_stepper0, speed[0]);

			// stepper 1
			if (speed[1] < 0) {
				stepper_set_direction(&my_stepper1, false);
				speed[1] = -speed[1];
			}
			else {
				stepper_set_direction(&my_stepper1, true);
			}
			stepper_move_speed(&my_stepper1, speed[1]);

			// stepper 2
			if (speed[2] < 0) {
				stepper_set_direction(&my_stepper2, false);
				speed[2] = -speed[2];
			}
			else {
				stepper_set_direction(&my_stepper2, true);
			}
			stepper_move_speed(&my_stepper2, speed[2]);

			// stepper 3

			if (speed[3] < 0) {
				stepper_set_direction(&my_stepper3, false);
				speed[3] = -speed[3];
			}
			else {
				stepper_set_direction(&my_stepper3, true);
			}
			stepper_move_speed(&my_stepper3, speed[3]);
			// stepper 4

			if (speed[4] < 0) {
				stepper_set_direction(&my_stepper4, false);
				speed[4] = -speed[4];
			}
			else {
				stepper_set_direction(&my_stepper4, true);
			}
			stepper_move_speed(&my_stepper4, speed[4]);

		} else if (status == osErrorTimeout) {
			stepper_stop(&my_stepper0);
			stepper_stop(&my_stepper1);
			stepper_stop(&my_stepper2);
			stepper_stop(&my_stepper3);
			stepper_stop(&my_stepper4);
		}
	}
}

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	stepper_init(&my_stepper0, GPIOE, GPIO_PIN_2, GPIO_PIN_3, &htim2);
	stepper_init(&my_stepper1, GPIOE, GPIO_PIN_4, GPIO_PIN_5, &htim3);
	stepper_init(&my_stepper2, GPIOE, GPIO_PIN_1, GPIO_PIN_0, &htim4);
	stepper_init(&my_stepper3, GPIOB, GPIO_PIN_9, GPIO_PIN_8, &htim5);
	stepper_init(&my_stepper4, GPIOC, GPIO_PIN_13, GPIO_PIN_14, &htim8);
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	messageQueueHandle = osMessageQueueNew(16, sizeof(Message_t), NULL);
	commandQueueHandle = osMessageQueueNew(16, sizeof(Command_t), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	parsingID = osThreadNew(parsePacketTask, NULL, &parseTask_attributes);
	settingID = osThreadNew(setTimersTask, NULL, &setTimerTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
	/* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	/* Infinite loop */
	for (;;) {
		osDelay(1);
	}
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

