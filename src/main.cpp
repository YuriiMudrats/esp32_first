#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "encoder.h"
#include "pwm.h"
#include "safebox.h"
#include "servo.h"
#include "sine.h"

#define ENCODER_A_INPUT GPIO_NUM_17
#define ENCODER_B_INPUT GPIO_NUM_16
#define ACTION_BUTTON_INPUT GPIO_NUM_15
#define RESET_BUTTON_INPUT GPIO_NUM_14
#define SERVO_GPIO GPIO_NUM_18
#define LED_GREEN_GPIO GPIO_NUM_12
#define LED_RED_GPIO GPIO_NUM_13
#define BUZZER_PIN GPIO_NUM_11
#define ENCODER_DEBOUNCE_NS 1000
#define BUTTON_DEBOUNCE_MS 30

static const char *TAG = "safebox";
static const int SECRET_CODE[SECRET_CODE_LENGTH] = {2, 6, 8, 6};


enum class Note : uint32_t {
    C4 = 262,
    D4 = 294,
    E4 = 330,
    F4 = 349,
    G4 = 392,
    A4 = 440,
    B4 = 494,
    C5 = 523,
};

struct NoteEvent {
    Note note;
    uint32_t duration_ms;
};

static void play_note(sine_t *sine, NoteEvent event)
{
    sine_set_frequency(sine, static_cast<uint32_t>(event.note));
    sine_start(sine);
    vTaskDelay(pdMS_TO_TICKS(event.duration_ms));
    sine_stop(sine);
    vTaskDelay(pdMS_TO_TICKS(40));
}
static void play_jingle_bells(sine_t *sine)
{
    play_note(sine, {Note::E4, 250});
    play_note(sine, {Note::E4, 250});
    play_note(sine, {Note::E4, 500});
    play_note(sine, {Note::E4, 250});
    play_note(sine, {Note::E4, 250});
    play_note(sine, {Note::E4, 500});
    play_note(sine, {Note::E4, 250});
    play_note(sine, {Note::G4, 250});
    play_note(sine, {Note::C4, 250});
    play_note(sine, {Note::D4, 250});
    play_note(sine, {Note::E4, 600});
}

static void update_safebox_leds(const safebox_state_t *safebox)
{
    const bool is_open = safebox->state == OPEN;
    gpio_set_level(LED_GREEN_GPIO, is_open ? 1 : 0);
    gpio_set_level(LED_RED_GPIO, is_open ? 0 : 1);
}

typedef struct {
    bool raw_pressed;
    bool stable_pressed;
    TickType_t changed_at;
} ButtonDebouncer;

static int encoder_pulses_to_digit(int32_t pulses)
{
    int digit = pulses % 10;
    return digit < 0 ? digit + 10 : digit;
}

static void button_debouncer_init(ButtonDebouncer *button, bool pressed)
{
    button->raw_pressed = pressed;
    button->stable_pressed = pressed;
    button->changed_at = xTaskGetTickCount();
}

static bool button_press_event(ButtonDebouncer *button, bool raw_pressed)
{
    TickType_t now = xTaskGetTickCount();
    if (raw_pressed != button->raw_pressed) {
        button->raw_pressed = raw_pressed;
        button->changed_at = now;
    }

    if (button->raw_pressed != button->stable_pressed &&
        (TickType_t)(now - button->changed_at) >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS)) {
        button->stable_pressed = button->raw_pressed;
        return button->stable_pressed;
    }
    return false;
}

extern "C" void app_main(void)
{
    encoder_ctx_t encoder = {};
    safebox_state_t safebox = {};
    pwm_t servo_pwm = {};
    servo_t servo = {};

    pwm_t pwm = {};
    sine_t sine = {};

    pwm_config_t pwm_config = {
        .gpio = BUZZER_PIN,
        .channel = LEDC_CHANNEL_1,
        .timer = LEDC_TIMER_1,
        .frequency_hz = 1000,
        .resolution = LEDC_TIMER_12_BIT,
        .duty = 0,
        .inverted = false,
    };

    ESP_ERROR_CHECK(pwm_init(&pwm, &pwm_config));
    ESP_ERROR_CHECK(sine_init(&sine, &pwm, 440));

    ESP_ERROR_CHECK(encoder_init(&encoder,
                                ENCODER_A_INPUT,
                                ENCODER_B_INPUT,
                                ACTION_BUTTON_INPUT,
                                ENCODER_DEBOUNCE_NS));

    gpio_config_t reset_button_config = {};
    reset_button_config.pin_bit_mask = 1ULL << RESET_BUTTON_INPUT;
    reset_button_config.mode = GPIO_MODE_INPUT;
    reset_button_config.pull_up_en = GPIO_PULLUP_ENABLE;
    reset_button_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    reset_button_config.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&reset_button_config));

    gpio_config_t led_config = {};
    led_config.pin_bit_mask = (1ULL << LED_GREEN_GPIO) | (1ULL << LED_RED_GPIO);
    led_config.mode = GPIO_MODE_OUTPUT;
    led_config.pull_up_en = GPIO_PULLUP_DISABLE;
    led_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    led_config.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&led_config));

    pwm_config_t servo_config = {
        .gpio = SERVO_GPIO,
        .channel = LEDC_CHANNEL_0,
        .timer = LEDC_TIMER_0,
        .frequency_hz = SERVO_FREQUENCY_HZ,
        .resolution = LEDC_TIMER_14_BIT,
        .duty = 0,
        .inverted = false,
    };
    ESP_ERROR_CHECK(pwm_init(&servo_pwm, &servo_config));
    ESP_ERROR_CHECK(servo_init(&servo, &servo_pwm));
    ESP_ERROR_CHECK(servo_set_angle(&servo, SERVO_MIN_ANGLE));

    if (safebox_state_init(&safebox) != 0) {
        ESP_LOGE(TAG, "Safebox initialization failed");
        return;
    }
    memcpy(safebox.secret_code, SECRET_CODE, sizeof(SECRET_CODE));
    update_safebox_leds(&safebox);

    bool action_pressed = false;
    ESP_ERROR_CHECK(encoder_get_button(&encoder, &action_pressed));
    ButtonDebouncer action_button;
    ButtonDebouncer reset_button;
    button_debouncer_init(&action_button, action_pressed);
    button_debouncer_init(&reset_button, gpio_get_level(RESET_BUTTON_INPUT) == 0);

    int32_t last_pulses = 0;
    int32_t pulses = 0;
    ESP_ERROR_CHECK(encoder_get_pulses(&encoder, &last_pulses));
    int selected_digit = encoder_pulses_to_digit(last_pulses);
    ESP_LOGI(TAG, "Selected digit: %d; enter a 4-digit code", selected_digit);

    while (true) {
        ESP_ERROR_CHECK(encoder_get_pulses(&encoder, &pulses));
        if (pulses != last_pulses) {
            selected_digit = encoder_pulses_to_digit(pulses);
            last_pulses = pulses;
            ESP_LOGI(TAG, "Selected digit: %d", selected_digit);
        }

        ESP_ERROR_CHECK(encoder_get_button(&encoder, &action_pressed));
        const bool reset_pressed = gpio_get_level(RESET_BUTTON_INPUT) == 0;
        const bool reset_event = button_press_event(&reset_button, reset_pressed);
        const bool action_event = button_press_event(&action_button, action_pressed);

        if (reset_event) {
            if (safebox_state_reset(&safebox) != 0) {
                ESP_LOGE(TAG, "Safebox reset failed");
            }
            update_safebox_leds(&safebox);
            ESP_ERROR_CHECK(servo_set_angle(&servo, SERVO_MIN_ANGLE));
            ESP_LOGI(TAG, "Safebox reset; servo returned to 0 degrees");
        } else if (action_event && !safebox_state_is_open(&safebox)) {
            const int position = safebox.entered_count;
            if (safebox_state_set_current_number(&safebox,
                                                 selected_digit,
                                                 position) != 0) {
                ESP_LOGE(TAG, "Could not submit digit at position %d", position);
            } else {
                ESP_LOGI(TAG, "Digit %d submitted at position %d",
                         selected_digit, position + 1);
                if (safebox.entered_count == SECRET_CODE_LENGTH) {
                    if (safebox_state_is_open(&safebox)) {
                        update_safebox_leds(&safebox);
                        ESP_ERROR_CHECK(servo_set_angle(&servo, SERVO_MAX_ANGLE));
                        ESP_LOGI(TAG, "Code matched; opening safebox");
                        play_jingle_bells(&sine);
                    } else {
                        ESP_LOGW(TAG, "Incorrect code; entry cleared");
                        safebox_state_reset(&safebox);
                        update_safebox_leds(&safebox);
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}