#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c.h"
#include "logger.h"

static const char *TAG = "logger_example";

#define I2C_MASTER_SCL_IO GPIO_NUM_9
#define I2C_MASTER_SDA_IO GPIO_NUM_8
#define I2C_MASTER_NUM I2C_NUM_0

extern "C" void app_main()
{
    // Set up the shared I2C bus used by the AT24C32 EEPROM.
    i2c_master_bus_handle_t bus = NULL;
    i2c_bus_settings_t bus_settings = {};
    bus_settings.port = I2C_MASTER_NUM;
    bus_settings.sda_pin = I2C_MASTER_SDA_IO;
    bus_settings.scl_pin = I2C_MASTER_SCL_IO;
    bus_settings.enable_internal_pullup = true;

    esp_err_t err = i2c_bus_init(&bus_settings, &bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus initialization failed: %s", esp_err_to_name(err));
        return;
    }

    // Probe the EEPROM and scan its pages to restore the last log number
    // and the next ring-buffer slot after a reboot.
    logger_handle_t logger = NULL;
    err = logger_init(bus, AT24C32_DEFAULT_ADDRESS, &logger);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AT24C32 logger initialization failed: %s",
                 esp_err_to_name(err));
        (void)i2c_bus_deinit(bus);
        return;
    }

    static const char *sample_logs[] = {
        "System started",
        "Temperature: 25 C",
        "Pressure: 1013 hPa",
    };
    constexpr size_t SAMPLE_LOG_COUNT = sizeof(sample_logs) / sizeof(sample_logs[0]);

    // Write a few example events; the logger assigns sequential numbers and
    // automatically wraps from #999 back to #0.
    for (size_t index = 0U; index < SAMPLE_LOG_COUNT; ++index) {
        err = logger_write(logger, sample_logs[index]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write example log: %s", esp_err_to_name(err));
            (void)logger_deinit(logger);
            (void)i2c_bus_deinit(bus);
            return;
        }
    }

    uint32_t test_write_count = 0U;

    // Periodically write a test entry, read it back from EEPROM, and report
    // whether the round trip succeeded.
    while (true) {
        logger_info_t info = {};
        err = logger_get_info(logger, &info);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get logger state: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        const uint16_t number_to_read = (info.count == 0U)
                                            ? 0U
                                            : static_cast<uint16_t>(
                                                  (info.last_number + 1U) %
                                                  LOGGER_NUMBER_COUNT);
        err = logger_printf(logger, "Loop test write %u",
                            static_cast<unsigned int>(test_write_count++));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "EEPROM log write failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        logger_record_t record = {};
        err = logger_read(logger, number_to_read, &record);
        if (err == ESP_OK) {
            printf("WRITE/READ OK: %s\n", record.text);
        } else {
            ESP_LOGE(TAG, "EEPROM read of log #%u failed: %s",
                     static_cast<unsigned int>(number_to_read), esp_err_to_name(err));
        }

        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
