#ifndef SAFEBOX_H
#define SAFEBOX_H

#include "Led.h"
#include <stdint.h>
#define SECRET_CODE_LENGTH 4


typedef enum {
    OPEN,
    CLOSE,
} safebox_state;

typedef struct {
    safebox_state state;
    Led_t led_green;
    Led_t led_red;
    int secret_code[SECRET_CODE_LENGTH];
    int secret_code_current[SECRET_CODE_LENGTH];
    uint8_t entered_count;
    void* arg;
} safebox_state_t;

int safebox_state_init(safebox_state_t *ctx);
int safebox_state_set_current_number(safebox_state_t *ctx, int value, int position);
int safebox_state_get_current_number(const safebox_state_t *ctx, int position);
bool safebox_state_is_open(safebox_state_t *ctx);
int safebox_state_reset(safebox_state_t *ctx);


#endif