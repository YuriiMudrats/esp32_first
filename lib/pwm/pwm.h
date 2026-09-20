#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include "driver/gpio.h"
#include "driver/ledc.h"

typedef struct {
	ledc_timer_config_t timer_config;
	ledc_channel_config_t channel_config;
	uint32_t max_duty;
} Pwm_t;

int Pwm_Init(Pwm_t *ctx, gpio_num_t pin);
int Pwm_UpdateDuty(Pwm_t *ctx, uint32_t duty);
int Pwm_UpdatePercent(Pwm_t *ctx, uint32_t percent);

#endif
