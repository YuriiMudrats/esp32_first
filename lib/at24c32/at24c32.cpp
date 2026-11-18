#include "at24c32.h"

#include <stdlib.h>

#include "esp_log.h"
#include "i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define AT24C32_I2C_SPEED_HZ 100000U
#define AT24C32_TIMEOUT_MS 1000U
#define AT24C32_WRITE_POLL_ATTEMPTS 20U

static const char *TAG = "at24c32";

struct at24c32_s {
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t device;
    uint16_t address;
};

static esp_err_t wait_for_write_complete(at24c32_handle_t eeprom)
{
    for (uint32_t attempt = 0U; attempt < AT24C32_WRITE_POLL_ATTEMPTS; ++attempt) {
        const esp_err_t err = i2c_bus_probe(eeprom->bus, eeprom->address, 10U);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        if (err != ESP_ERR_NOT_FOUND && err != ESP_ERR_TIMEOUT) {
            return err;
        }
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
    ESP_LOGE(TAG, "EEPROM write acknowledgement timed out");
    return ESP_ERR_TIMEOUT;
}

esp_err_t at24c32_init(i2c_master_bus_handle_t bus,
                       uint16_t address,
                       at24c32_handle_t *eeprom)
{
    if (bus == NULL || eeprom == NULL || address > 0x7FU) {
        return ESP_ERR_INVALID_ARG;
    }
    *eeprom = NULL;

    at24c32_handle_t instance = static_cast<at24c32_handle_t>(
        calloc(1U, sizeof(*instance)));
    if (instance == NULL) {
        return ESP_ERR_NO_MEM;
    }
    instance->bus = bus;
    instance->address = address;

    i2c_device_settings_t settings = {};
    settings.address = address;
    settings.scl_speed_hz = AT24C32_I2C_SPEED_HZ;
    esp_err_t err = i2c_device_add(bus, &settings, &instance->device);
    if (err == ESP_OK) {
        err = i2c_bus_probe(bus, address, AT24C32_TIMEOUT_MS);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AT24C32 at 0x%02X is not available: %s",
                 static_cast<unsigned int>(address), esp_err_to_name(err));
        if (instance->device != NULL) {
            (void)i2c_device_remove(instance->device);
        }
        free(instance);
        return err;
    }

    *eeprom = instance;
    return ESP_OK;
}

esp_err_t at24c32_deinit(at24c32_handle_t eeprom)
{
    if (eeprom == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_err_t err = i2c_device_remove(eeprom->device);
    free(eeprom);
    return err;
}

esp_err_t at24c32_read(at24c32_handle_t eeprom,
                       uint16_t address,
                       uint8_t *data,
                       size_t size)
{
    if (eeprom == NULL || data == NULL || size == 0U ||
        address >= AT24C32_CAPACITY_BYTES ||
        size > AT24C32_CAPACITY_BYTES - address) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t offset = 0U;
    while (offset < size) {
        const uint16_t current_address = static_cast<uint16_t>(address + offset);
        const size_t page_remaining = AT24C32_PAGE_SIZE -
                                     (current_address % AT24C32_PAGE_SIZE);
        const size_t chunk = (size - offset < page_remaining)
                                 ? size - offset
                                 : page_remaining;
        const uint8_t address_bytes[] = {
            static_cast<uint8_t>(current_address >> 8U),
            static_cast<uint8_t>(current_address & 0xFFU),
        };
        const esp_err_t err = i2c_write_read(eeprom->device, address_bytes,
                                             sizeof(address_bytes), &data[offset],
                                             chunk, AT24C32_TIMEOUT_MS);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Read failed at address 0x%04X: %s",
                     static_cast<unsigned int>(current_address), esp_err_to_name(err));
            return err;
        }
        offset += chunk;
    }
    return ESP_OK;
}

esp_err_t at24c32_write_page(at24c32_handle_t eeprom,
                             uint16_t address,
                             const uint8_t data[AT24C32_PAGE_SIZE])
{
    if (eeprom == NULL || data == NULL ||
        address >= AT24C32_CAPACITY_BYTES ||
        (address % AT24C32_PAGE_SIZE) != 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer[2U + AT24C32_PAGE_SIZE];
    buffer[0] = static_cast<uint8_t>(address >> 8U);
    buffer[1] = static_cast<uint8_t>(address & 0xFFU);
    for (size_t index = 0U; index < AT24C32_PAGE_SIZE; ++index) {
        buffer[2U + index] = data[index];
    }

    esp_err_t err = i2c_write(eeprom->device, buffer, sizeof(buffer), AT24C32_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Page write failed at address 0x%04X: %s",
                 static_cast<unsigned int>(address), esp_err_to_name(err));
        return err;
    }
    return wait_for_write_complete(eeprom);
}
