#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <stdio.h>
#include <string.h>
#include "ds1307.h"
#include "i2c.h"
#include "clib/u8g2.h"

static const char *TAG = "app";

// Піни та порт шини I2C
#define I2C_MASTER_SCL_IO  GPIO_NUM_9
#define I2C_MASTER_SDA_IO  GPIO_NUM_8
#define I2C_MASTER_NUM     I2C_NUM_0

static constexpr size_t OLED_TX_BUFFER_SIZE = 256U;
static i2c_master_dev_handle_t oled_i2c_device = NULL;
static uint8_t oled_tx_buffer[OLED_TX_BUFFER_SIZE];
static size_t oled_tx_size = 0U;

static uint8_t u8g2_i2c_byte_callback(u8x8_t *u8x8, uint8_t message,
                                      uint8_t argument, void *data)
{
    (void)u8x8;

    switch (message) {
    case U8X8_MSG_BYTE_INIT:
        return 1U;
    case U8X8_MSG_BYTE_START_TRANSFER:
        oled_tx_size = 0U;
        return 1U;
    case U8X8_MSG_BYTE_SEND:
        if (data == NULL || argument > OLED_TX_BUFFER_SIZE - oled_tx_size) {
            ESP_LOGE(TAG, "U8g2 I2C transaction exceeds transmit buffer");
            return 0U;
        }
        memcpy(&oled_tx_buffer[oled_tx_size], data, argument);
        oled_tx_size += argument;
        return 1U;
    case U8X8_MSG_BYTE_END_TRANSFER: {
        if (oled_tx_size == 0U) {
            return 1U;
        }
        const esp_err_t err = i2c_master_transmit(oled_i2c_device, oled_tx_buffer,
                                                   oled_tx_size, 1000);
        oled_tx_size = 0U;
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "U8g2 I2C transfer failed: %s", esp_err_to_name(err));
            return 0U;
        }
        return 1U;
    }
    default:
        return 0U;
    }
}

static uint8_t u8g2_gpio_and_delay_callback(u8x8_t *u8x8, uint8_t message,
                                            uint8_t argument, void *data)
{
    (void)u8x8;
    (void)data;

    switch (message) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        return 1U;
    case U8X8_MSG_DELAY_MILLI:
        vTaskDelay(pdMS_TO_TICKS(argument));
        return 1U;
    case U8X8_MSG_DELAY_10MICRO:
        esp_rom_delay_us(10U * argument);
        return 1U;
    case U8X8_MSG_DELAY_100NANO:
    case U8X8_MSG_DELAY_NANO:
        return 1U;
    default:
        return 1U;
    }
}

static void display_message(u8g2_t *display, const char *message)
{
    u8g2_ClearBuffer(display);
    u8g2_SetFont(display, u8g2_font_6x10_tf);
    u8g2_DrawStr(display, 0, 32, message);
    u8g2_SendBuffer(display);
}

static bool get_firmware_datetime(ds1307_datetime_t *datetime)
{
    static const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };
    char month_name[4] = {};
    int day;
    int year;
    int hour;
    int minute;
    int second;

    if (datetime == NULL ||
        sscanf(__DATE__, "%3s %d %d", month_name, &day, &year) != 3 ||
        sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second) != 3) {
        return false;
    }

    uint8_t month = 0;
    for (uint8_t index = 0; index < 12; ++index) {
        if (strncmp(month_name, months[index], 3) == 0) {
            month = (uint8_t)(index + 1U);
            break;
        }
    }
    if (month == 0U || day < 1 || day > 31 || year < 2000 || year > 2099 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return false;
    }

    datetime->year = (uint16_t)year;
    datetime->month = month;
    datetime->day = (uint8_t)day;
    datetime->weekday = 1U;
    datetime->hour = (uint8_t)hour;
    datetime->minute = (uint8_t)minute;
    datetime->second = (uint8_t)second;
    return true;
}

static void sanitize_rtc_datetime(ds1307_datetime_t *datetime)
{
    if (datetime == NULL) {
        return;
    }

    if (datetime->year < 2000U || datetime->year > 2099U) {
        datetime->year = 2000U;
    }
    if (datetime->month < 1U || datetime->month > 12U) {
        datetime->month = 1U;
    }
    if (datetime->day < 1U || datetime->day > 31U) {
        datetime->day = 1U;
    }
    if (datetime->weekday < 1U || datetime->weekday > 7U) {
        datetime->weekday = 1U;
    }
    if (datetime->hour > 23U) {
        datetime->hour = 0U;
    }
    if (datetime->minute > 59U) {
        datetime->minute = 0U;
    }
    if (datetime->second > 59U) {
        datetime->second = 0U;
    }
}

extern "C" void app_main()
{
    ESP_LOGI(TAG, "ESP32 booted");

    // 1. Створюємо шину I2C
    i2c_master_bus_handle_t bus = NULL;
    i2c_bus_settings_t bus_cfg = {};
    bus_cfg.port = I2C_MASTER_NUM;
    bus_cfg.sda_pin = I2C_MASTER_SDA_IO;
    bus_cfg.scl_pin = I2C_MASTER_SCL_IO;
    bus_cfg.enable_internal_pullup = true;
    ESP_LOGI(TAG, "Initializing I2C bus...");
    ESP_ERROR_CHECK(i2c_bus_init(&bus_cfg, &bus));
    ESP_LOGI(TAG, "I2C bus ready");

    // Ініціалізуємо U8g2 для SSD1306 128x64 на I2C-адресі 0x3C.
    i2c_device_settings_t oled_settings = {};
    oled_settings.address = 0x3CU;
    oled_settings.scl_speed_hz = 400000U;
    esp_err_t err = i2c_device_add(bus, &oled_settings, &oled_i2c_device);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OLED I2C setup failed: %s", esp_err_to_name(err));
        return;
    }

    u8g2_t display = {};
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&display, U8G2_R0,
                                           u8g2_i2c_byte_callback,
                                           u8g2_gpio_and_delay_callback);
    u8g2_InitDisplay(&display);
    u8g2_SetPowerSave(&display, 0U);
    display_message(&display, "DS1307 CLOCK");
    ESP_LOGI(TAG, "U8g2 SSD1306 initialized");

    ds1307_handle_t rtc = NULL;
    err = ds1307_init(bus, &rtc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "DS1307 init failed: %s", esp_err_to_name(err));
        display_message(&display, "RTC INIT ERROR");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    bool oscillator_checked = false;

    while (1) {
        ds1307_datetime_t datetime = {};
        err = ds1307_get_datetime(rtc, &datetime);
        if (err == ESP_OK) {
            if (datetime.year == 2000U && datetime.month == 1U && datetime.day == 1U) {
                ds1307_datetime_t firmware_datetime = {};
                if (get_firmware_datetime(&firmware_datetime)) {
                    err = ds1307_set_datetime(rtc, &firmware_datetime);
                    if (err == ESP_OK) {
                        datetime = firmware_datetime;
                        ESP_LOGW(TAG, "RTC was at its default date; initialized from firmware build time");
                    }
                }
            }

            if (err != ESP_OK) {
                ESP_LOGE(TAG, "DS1307 initialization failed: %s", esp_err_to_name(err));
                display_message(&display, "RTC SET ERROR");
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            if (!oscillator_checked) {
                bool rtc_running = false;
                err = ds1307_is_running(rtc, &rtc_running);
                if (err == ESP_OK && !rtc_running) {
                    ESP_LOGW(TAG, "DS1307 oscillator was stopped; starting it");
                    err = ds1307_set_running(rtc, true);
                }
                oscillator_checked = (err == ESP_OK);
            }

            if (err != ESP_OK) {
                ESP_LOGE(TAG, "DS1307 status check failed: %s", esp_err_to_name(err));
                display_message(&display, "CHECK DS1307");
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            sanitize_rtc_datetime(&datetime);

            char time_buf[16];
            char date_buf[16];
            snprintf(time_buf, sizeof(time_buf), "%02u:%02u:%02u",
                     datetime.hour, datetime.minute, datetime.second);
            snprintf(date_buf, sizeof(date_buf), "%04u-%02u-%02u",
                     datetime.year, datetime.month, datetime.day);

            u8g2_ClearBuffer(&display);
            u8g2_SetFont(&display, u8g2_font_6x10_tf);
            u8g2_DrawStr(&display, 0, 10, "DS1307 CLOCK");
            u8g2_SetFont(&display, u8g2_font_ncenB14_tr);
            u8g2_DrawStr(&display, 0, 34, time_buf);
            u8g2_SetFont(&display, u8g2_font_6x10_tf);
            u8g2_DrawStr(&display, 0, 54, date_buf);
            u8g2_SendBuffer(&display);
            ESP_LOGI(TAG, "RTC %s %s", date_buf, time_buf);
        } else {
            ESP_LOGE(TAG, "DS1307 time read failed: %s", esp_err_to_name(err));
            display_message(&display, "RTC READ ERROR");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
