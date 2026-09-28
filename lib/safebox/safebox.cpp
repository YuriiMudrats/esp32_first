#include "safebox.h"

#include <string.h>

static bool safebox_code_matches(const safebox_state_t *ctx)
{
	for (int position = 0; position < SECRET_CODE_LENGTH; ++position) {
		if (ctx->secret_code_current[position] != ctx->secret_code[position]) {
			return false;
		}
	}
	return true;
}

int safebox_state_init(safebox_state_t *ctx)
{
	if (ctx == nullptr) {
		return -1;
	}

	memset(ctx, 0, sizeof(*ctx));
	ctx->state = CLOSE;
	ctx->led_green.state = OFF;
	ctx->led_red.state = ON;
	return 0;
}

int safebox_state_set_current_number(safebox_state_t *ctx,
									int value,
									int position)
{
	if (ctx == nullptr || position < 0 || position >= SECRET_CODE_LENGTH ||
		value < 0 || value > 9 || position != ctx->entered_count ||
		ctx->entered_count >= SECRET_CODE_LENGTH || ctx->state == OPEN) {
		return -1;
	}

	ctx->secret_code_current[position] = value;
	++ctx->entered_count;
	if (ctx->entered_count == SECRET_CODE_LENGTH) {
		ctx->state = safebox_code_matches(ctx) ? OPEN : CLOSE;
	}
	ctx->led_green.state = ctx->state == OPEN ? ON : OFF;
	ctx->led_red.state = ctx->state == OPEN ? OFF : ON;
	return 0;
}

int safebox_state_get_current_number(const safebox_state_t *ctx, int position)
{
	if (ctx == nullptr || position < 0 || position >= SECRET_CODE_LENGTH) {
		return -1;
	}
	return ctx->secret_code_current[position];
}

bool safebox_state_is_open(safebox_state_t *ctx)
{
	return ctx != nullptr && ctx->state == OPEN;
}

int safebox_state_reset(safebox_state_t *ctx)
{
	if (ctx == nullptr) {
		return -1;
	}

	memset(ctx->secret_code_current, 0, sizeof(ctx->secret_code_current));
	ctx->entered_count = 0;
	ctx->state = CLOSE;
	ctx->led_green.state = OFF;
	ctx->led_red.state = ON;
	return 0;
}
