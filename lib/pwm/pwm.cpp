#include "pwm.h"

#include <string.h>

#include "esp_err.h"

int Pwm_Init(Pwm_t *ctx, gpio_num_t pin)
{
	if (ctx == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	ctx->max_duty = (1U << LEDC_TIMER_12_BIT) - 1U;
	memset(&ctx->timer_config, 0, sizeof(ctx->timer_config));
	ctx->timer_config.speed_mode = LEDC_LOW_SPEED_MODE;
	ctx->timer_config.duty_resolution = LEDC_TIMER_12_BIT;
	ctx->timer_config.timer_num = LEDC_TIMER_0;
	ctx->timer_config.freq_hz = 1000;
	ctx->timer_config.clk_cfg = LEDC_AUTO_CLK;

	esp_err_t err = ledc_timer_config(&ctx->timer_config);
	if (err != ESP_OK) {
		return err;
	}

	memset(&ctx->channel_config, 0, sizeof(ctx->channel_config));
	ctx->channel_config.gpio_num = pin;
	ctx->channel_config.speed_mode = LEDC_LOW_SPEED_MODE;
	ctx->channel_config.channel = LEDC_CHANNEL_0;
	ctx->channel_config.intr_type = LEDC_INTR_DISABLE;
	ctx->channel_config.timer_sel = LEDC_TIMER_0;
	ctx->channel_config.duty = 0;
	ctx->channel_config.hpoint = 0;
	ctx->channel_config.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
	ctx->channel_config.flags.output_invert = 0;

	return ledc_channel_config(&ctx->channel_config);
}

int Pwm_UpdateDuty(Pwm_t *ctx, uint32_t duty)
{
	if (ctx == nullptr || duty > ctx->max_duty) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err = ledc_set_duty(
		ctx->channel_config.speed_mode,
		ctx->channel_config.channel,
		duty);
	if (err != ESP_OK) {
		return err;
	}

	return ledc_update_duty(
		ctx->channel_config.speed_mode,
		ctx->channel_config.channel);
}

int Pwm_UpdatePercent(Pwm_t *ctx, uint32_t percent)
{
	if (ctx == nullptr || percent > 100U) {
		return ESP_ERR_INVALID_ARG;
	}

	uint32_t duty = (percent * ctx->max_duty) / 100U;
	return Pwm_UpdateDuty(ctx, duty);
}
