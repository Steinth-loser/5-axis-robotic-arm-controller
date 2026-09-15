/*
 * stepper.h
 *
 *  Created on: Jul 21, 2026
 *      Author: th3ka
 */

#ifndef INC_STEPPER_H_
#define INC_STEPPER_H_

#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"

typedef struct {

	GPIO_TypeDef* stepper_port;

	uint16_t   ena;
	uint16_t   dir;



	TIM_HandleTypeDef* htim;
	volatile uint32_t remaining_steps;
	volatile bool moving;
}stepper_t;

void stepper_init(stepper_t* stepper, GPIO_TypeDef* stepper_port, uint16_t ena, uint16_t dir, TIM_HandleTypeDef* htim);

void stepper_enable(stepper_t* stepper);
void stepper_disable(stepper_t* stepper);

void stepper_set_direction(stepper_t* stepper, bool cw);

void stepper_move_steps(stepper_t* stepper, uint32_t steps);

void stepper_set_speed(stepper_t* stepper, float steps_per_sec);
void stepper_move_speed_with_steps(stepper_t* stepper, uint32_t steps, float speed);
void stepper_move_speed(stepper_t* stepper, float speed);
void stepper_stop(stepper_t* stepper);

// Will be called in timer callback function
void stepper_pulse_cb(stepper_t* stepper);

#endif /* INC_STEPPER_H_ */
