/*
 * stepper.c
 *
 *  Created on: Jul 21, 2026
 *      Author: th3ka
 */

#include "stepper.h"
#include "gpio.h"

#define TIMER_CLOCK_HZ 84000000U   //TIM4

void stepper_init(stepper_t* stepper, GPIO_TypeDef* stepper_port, uint16_t ena, uint16_t dir, TIM_HandleTypeDef* htim)
{
	stepper->stepper_port = stepper_port;

	stepper->ena = ena;
	stepper->dir = dir;
	// stepper->pulse = GPIO_PIN_4; // <-- İPTAL: Artık bu pini CubeMX'ten "Timer Channel 1 (Alternate Function)" olarak seçmelisin.

	stepper->htim = htim;
	stepper->remaining_steps = 0;
	stepper->moving = false;
}

void stepper_enable(stepper_t *stepper)
{
	HAL_GPIO_WritePin(stepper->stepper_port, stepper->ena, GPIO_PIN_RESET);
}
void stepper_disable(stepper_t *stepper)
{
	HAL_GPIO_WritePin(stepper->stepper_port, stepper->ena, GPIO_PIN_SET);
}

void stepper_set_direction(stepper_t *stepper, bool cw)
{
	if (cw) {
		HAL_GPIO_WritePin(stepper->stepper_port, stepper->dir, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(stepper->stepper_port, stepper->dir, GPIO_PIN_RESET);
	}
}

// (Not: Bloklayıcı HAL_Delay içeren bu fonksiyon donanımsal PWM mimarisinde genellikle kullanılmaz,
// yerine stepper_move_speed_with_steps kullanılması daha sağlıklıdır.)
void stepper_move_steps(stepper_t *stepper, uint32_t steps)
{
	for (uint32_t i = 0; i < steps; i++) {
		// HAL_GPIO_TogglePin(stepper->stepper_port, stepper->pulse);
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
		HAL_Delay(300);
	}
}

static bool stepper_calc_timer(uint32_t target_freq, uint32_t *psc_out, uint32_t *arr_out)
{
	if (target_freq == 0)
		return false;

	for (uint32_t psc = 0; psc <= 65535; psc++) {
		uint32_t divider = (psc + 1) * target_freq;
		if (divider == 0)
			continue;

		uint32_t arr = TIMER_CLOCK_HZ / divider;
		if (arr >= 1 && arr <= 65536) {
			*psc_out = psc;
			*arr_out = arr - 1;
			return true;
		}
	}
	return false;
}

void stepper_set_speed(stepper_t *stepper, float steps_per_sec)
{
	if (steps_per_sec <= 0.0f) {
		stepper_stop(stepper);
		return;
	}

	// YENİ MİMARİ: PWM her periyotta tam dalga ürettiği için çarpı 2 iptal edildi.
	uint32_t target_freq = (uint32_t) steps_per_sec;
	uint32_t psc, arr;

	if (!stepper_calc_timer(target_freq, &psc, &arr)) {
		return;
	}

	// Timer değerlerini güncelle
	__HAL_TIM_SET_PRESCALER(stepper->htim, psc);
	__HAL_TIM_SET_AUTORELOAD(stepper->htim, arr);

	// Duty Cycle'ı %50 olarak ayarla (ARR'nin yarısı)
	__HAL_TIM_SET_COMPARE(stepper->htim, TIM_CHANNEL_1, arr / 2);

	if (!stepper->moving) {
		__HAL_TIM_SET_COUNTER(stepper->htim, 0);

		// 1. Donanımsal PWM'i başlat (Pini bu sürecek)
		HAL_TIM_PWM_Start(stepper->htim, TIM_CHANNEL_1);

		// 2. Adım sayabilmek için kesmeyi (Update Interrupt) başlat
		HAL_TIM_Base_Start_IT(stepper->htim);

		stepper->moving = true;
	}
}

void stepper_move_speed_with_steps(stepper_t *stepper, uint32_t steps, float speed)
{
	if (steps <= 0)
		return;

	// YENİ MİMARİ: PWM her periyotta 1 adım attığı için "steps * 2" yapmamıza gerek yok.
	stepper->remaining_steps = steps;
	stepper_set_speed(stepper, speed);
}

void stepper_move_speed(stepper_t* stepper, float speed)
{
	if(speed <= 0.0f) {
		stepper_stop(stepper);
		return;
	}

	stepper->remaining_steps = 0; // Sonsuz dönüş
	stepper_set_speed(stepper, speed);
}

void stepper_stop(stepper_t *stepper) {
	// Hem PWM çıkışını hem de sayıcı kesmesini durdur
    HAL_TIM_PWM_Stop(stepper->htim, TIM_CHANNEL_1);
    HAL_TIM_Base_Stop_IT(stepper->htim);

    stepper->moving = false;
    stepper->remaining_steps = 0;
}

// Callback sadece adımları saymak için (Timer Update Interrupt / Period Elapsed)
// Not using in this version
void stepper_pulse_cb(stepper_t *stepper)
{

	// İPTAL: Pini artık donanım (PWM) çeviriyor, yazılımla Toggle yapmıyoruz.
    // HAL_GPIO_TogglePin(stepper->stepper_port, stepper->pulse);

    if (stepper->remaining_steps > 0) {
        stepper->remaining_steps--;
        if (stepper->remaining_steps == 0) {
        	// Adımlar bittiğinde motoru durdur
            stepper_stop(stepper);
        }
    }
}

