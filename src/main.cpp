#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "Adc.h"
#include "pwm.h"
#include "sine.h"

#define BUZZER_PIN GPIO_NUM_17

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

static void play_baby_shark(sine_t *sine)
{
    play_note(sine, {Note::C4, 250});
    play_note(sine, {Note::D4, 250});
    play_note(sine, {Note::E4, 400});
    play_note(sine, {Note::C4, 250});
    play_note(sine, {Note::D4, 250});
    play_note(sine, {Note::E4, 400});
    play_note(sine, {Note::C4, 250});
    play_note(sine, {Note::D4, 250});
    play_note(sine, {Note::E4, 500});
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

static void play_we_will_rock_you(sine_t *sine)
{
    play_note(sine, {Note::C4, 300});
    play_note(sine, {Note::C4, 300});
    play_note(sine, {Note::D4, 500});
    play_note(sine, {Note::C4, 300});
    play_note(sine, {Note::C4, 300});
    play_note(sine, {Note::D4, 500});
    play_note(sine, {Note::C4, 300});
    play_note(sine, {Note::C4, 300});
    play_note(sine, {Note::D4, 500});
    play_note(sine, {Note::E4, 700});
}

static void play_twinkle_twinkle(sine_t *sine)
{
    play_note(sine, {Note::C4, 300});
    play_note(sine, {Note::C4, 300});
    play_note(sine, {Note::G4, 300});
    play_note(sine, {Note::G4, 300});
    play_note(sine, {Note::A4, 300});
    play_note(sine, {Note::A4, 300});
    play_note(sine, {Note::G4, 600});
}

extern "C" void app_main()
{
    pwm_t pwm = {};
    sine_t sine = {};

    pwm_config_t pwm_config = {
        .gpio = BUZZER_PIN,
        .channel = LEDC_CHANNEL_0,
        .timer = LEDC_TIMER_0,
        .frequency_hz = 1000,
        .resolution = LEDC_TIMER_12_BIT,
        .duty = 0,
        .inverted = false,
    };

    pwm_init(&pwm, &pwm_config);
    sine_init(&sine, &pwm, 440);

    while (true) {
        play_baby_shark(&sine);
        vTaskDelay(pdMS_TO_TICKS(500));
        play_jingle_bells(&sine);
        vTaskDelay(pdMS_TO_TICKS(500));
        play_we_will_rock_you(&sine);
        vTaskDelay(pdMS_TO_TICKS(500));
        play_twinkle_twinkle(&sine);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
