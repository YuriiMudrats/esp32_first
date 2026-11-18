#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "logger";
static constexpr uint16_t INVALID_SLOT = UINT16_MAX;
static constexpr uint16_t NUMBER_MODULUS = LOGGER_NUMBER_COUNT;

struct logger_s {
    at24c32_handle_t eeprom;
    uint16_t sequence[LOGGER_MAX_RECORDS];
    bool valid[LOGGER_MAX_RECORDS];
    int16_t slot_by_number[LOGGER_NUMBER_COUNT];
    uint16_t count;
    uint16_t newest_slot;
    uint16_t newest_number;
    uint16_t oldest_age;
    uint16_t next_slot;
    uint16_t next_number;
};

static bool parse_record(const uint8_t bytes[LOGGER_RECORD_SIZE],
                         uint16_t *number,
                         char text[LOGGER_RECORD_SIZE])
{
    if (bytes[0] != static_cast<uint8_t>('#')) {
        return false;
    }

    uint16_t parsed_number = 0U;
    size_t index = 1U;
    size_t digit_count = 0U;
    while (index < LOGGER_RECORD_SIZE &&
           bytes[index] >= static_cast<uint8_t>('0') &&
           bytes[index] <= static_cast<uint8_t>('9')) {
        parsed_number = static_cast<uint16_t>(
            parsed_number * 10U + (bytes[index] - static_cast<uint8_t>('0')));
        ++index;
        ++digit_count;
    }
    if (digit_count == 0U || digit_count > 3U || parsed_number >= NUMBER_MODULUS ||
        index >= LOGGER_RECORD_SIZE || bytes[index] != static_cast<uint8_t>(' ')) {
        return false;
    }

    size_t terminator = index + 1U;
    while (terminator < LOGGER_RECORD_SIZE && bytes[terminator] != 0U) {
        ++terminator;
    }
    if (terminator == LOGGER_RECORD_SIZE) {
        return false;
    }
    for (size_t padding = terminator + 1U; padding < LOGGER_RECORD_SIZE; ++padding) {
        if (bytes[padding] != 0U) {
            return false;
        }
    }

    memcpy(text, bytes, LOGGER_RECORD_SIZE);
    *number = parsed_number;
    return true;
}

static uint16_t sequence_age(uint16_t newest, uint16_t number)
{
    return static_cast<uint16_t>((newest + NUMBER_MODULUS - number) % NUMBER_MODULUS);
}

static esp_err_t read_slot(logger_handle_t logger,
                           uint16_t slot,
                           logger_record_t *record)
{
    uint8_t bytes[LOGGER_RECORD_SIZE];
    const esp_err_t err = at24c32_read(
        logger->eeprom, static_cast<uint16_t>(slot * LOGGER_RECORD_SIZE),
        bytes, sizeof(bytes));
    if (err != ESP_OK) {
        return err;
    }
    if (!parse_record(bytes, &record->number, record->text)) {
        return ESP_ERR_NOT_FOUND;
    }
    return ESP_OK;
}

esp_err_t logger_init(i2c_master_bus_handle_t bus,
                      uint16_t eeprom_address,
                      logger_handle_t *logger)
{
    if (bus == NULL || logger == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *logger = NULL;

    logger_handle_t instance = static_cast<logger_handle_t>(
        calloc(1U, sizeof(*instance)));
    if (instance == NULL) {
        return ESP_ERR_NO_MEM;
    }
    for (size_t index = 0U; index < LOGGER_NUMBER_COUNT; ++index) {
        instance->slot_by_number[index] = -1;
    }

    esp_err_t err = at24c32_init(bus, eeprom_address, &instance->eeprom);
    if (err != ESP_OK) {
        free(instance);
        return err;
    }

    uint8_t bytes[LOGGER_RECORD_SIZE];
    for (uint16_t slot = 0U; slot < LOGGER_MAX_RECORDS; ++slot) {
        err = at24c32_read(instance->eeprom,
                           static_cast<uint16_t>(slot * LOGGER_RECORD_SIZE),
                           bytes, sizeof(bytes));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to scan EEPROM slot %u: %s",
                     static_cast<unsigned int>(slot), esp_err_to_name(err));
            (void)at24c32_deinit(instance->eeprom);
            free(instance);
            return err;
        }
        logger_record_t record = {};
        if (parse_record(bytes, &record.number, record.text)) {
            instance->valid[slot] = true;
            instance->sequence[slot] = record.number;
        }
    }

    uint16_t best_count = 0U;
    uint16_t best_oldest_age = 0U;
    uint16_t best_slot = INVALID_SLOT;
    for (uint16_t candidate = 0U; candidate < LOGGER_MAX_RECORDS; ++candidate) {
        if (!instance->valid[candidate]) {
            continue;
        }
        uint16_t candidate_count = 0U;
        uint16_t candidate_oldest_age = 0U;
        for (uint16_t slot = 0U; slot < LOGGER_MAX_RECORDS; ++slot) {
            if (!instance->valid[slot]) {
                continue;
            }
            const uint16_t age = sequence_age(instance->sequence[candidate],
                                              instance->sequence[slot]);
            if (age < LOGGER_MAX_RECORDS) {
                ++candidate_count;
                if (age > candidate_oldest_age) {
                    candidate_oldest_age = age;
                }
            }
        }
        if (candidate_count > best_count ||
            (candidate_count == best_count && candidate_oldest_age < best_oldest_age)) {
            best_count = candidate_count;
            best_oldest_age = candidate_oldest_age;
            best_slot = candidate;
        }
    }

    if (best_slot != INVALID_SLOT) {
        instance->count = best_count;
        instance->newest_slot = best_slot;
        instance->newest_number = instance->sequence[best_slot];
        instance->oldest_age = best_oldest_age;
        for (uint16_t slot = 0U; slot < LOGGER_MAX_RECORDS; ++slot) {
            if (instance->valid[slot] &&
                sequence_age(instance->newest_number, instance->sequence[slot]) <
                    LOGGER_MAX_RECORDS) {
                instance->slot_by_number[instance->sequence[slot]] =
                    static_cast<int16_t>(slot);
            }
        }
        instance->next_slot = static_cast<uint16_t>(
            (instance->newest_slot + 1U) % LOGGER_MAX_RECORDS);
        instance->next_number = static_cast<uint16_t>(
            (instance->newest_number + 1U) % NUMBER_MODULUS);
    }

    ESP_LOGI(TAG, "Initialized with %u stored log(s); next number is #%u",
             static_cast<unsigned int>(instance->count),
             static_cast<unsigned int>(instance->next_number));
    *logger = instance;
    return ESP_OK;
}

esp_err_t logger_deinit(logger_handle_t logger)
{
    if (logger == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_err_t err = at24c32_deinit(logger->eeprom);
    free(logger);
    return err;
}

esp_err_t logger_write(logger_handle_t logger, const char *message)
{
    if (logger == NULL || message == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t bytes[LOGGER_RECORD_SIZE] = {};
    const int prefix_size = snprintf(reinterpret_cast<char *>(bytes),
                                     sizeof(bytes), "#%u ",
                                     static_cast<unsigned int>(logger->next_number));
    if (prefix_size < 0 || static_cast<size_t>(prefix_size) >= sizeof(bytes)) {
        return ESP_FAIL;
    }
    const size_t message_capacity = sizeof(bytes) - static_cast<size_t>(prefix_size) - 1U;
    const size_t message_size = strnlen(message, message_capacity);
    memcpy(&bytes[prefix_size], message, message_size);

    const uint16_t slot = logger->next_slot;
    const esp_err_t err = at24c32_write_page(
        logger->eeprom, static_cast<uint16_t>(slot * LOGGER_RECORD_SIZE), bytes);
    if (err != ESP_OK) {
        return err;
    }

    if (logger->valid[slot]) {
        const uint16_t previous_number = logger->sequence[slot];
        if (logger->slot_by_number[previous_number] == static_cast<int16_t>(slot)) {
            logger->slot_by_number[previous_number] = -1;
        }
    } else if (logger->count < LOGGER_MAX_RECORDS) {
        ++logger->count;
    }

    logger->valid[slot] = true;
    logger->sequence[slot] = logger->next_number;
    logger->slot_by_number[logger->next_number] = static_cast<int16_t>(slot);
    logger->newest_slot = slot;
    logger->newest_number = logger->next_number;
    if (logger->count == 1U) {
        logger->oldest_age = 0U;
    } else if (logger->oldest_age < LOGGER_MAX_RECORDS - 1U) {
        ++logger->oldest_age;
    }
    logger->next_slot = static_cast<uint16_t>((slot + 1U) % LOGGER_MAX_RECORDS);
    logger->next_number = static_cast<uint16_t>(
        (logger->next_number + 1U) % NUMBER_MODULUS);

    printf("%s\n", reinterpret_cast<const char *>(bytes));
    fflush(stdout);
    return ESP_OK;
}

esp_err_t logger_printf(logger_handle_t logger, const char *format, ...)
{
    if (logger == NULL || format == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char message[LOGGER_RECORD_SIZE];
    va_list args;
    va_start(args, format);
    const int written = vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    if (written < 0) {
        return ESP_FAIL;
    }
    return logger_write(logger, message);
}

esp_err_t logger_get_info(logger_handle_t logger, logger_info_t *info)
{
    if (logger == NULL || info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    info->count = logger->count;
    info->last_number = (logger->count == 0U) ? 0U : logger->newest_number;
    info->first_number = (logger->count == 0U)
                             ? 0U
                             : static_cast<uint16_t>(
                                   (logger->newest_number + NUMBER_MODULUS -
                                    logger->oldest_age) % NUMBER_MODULUS);
    return ESP_OK;
}

esp_err_t logger_read(logger_handle_t logger,
                      uint16_t number,
                      logger_record_t *record)
{
    if (logger == NULL || record == NULL || number >= NUMBER_MODULUS) {
        return ESP_ERR_INVALID_ARG;
    }
    if (logger->slot_by_number[number] < 0) {
        return ESP_ERR_NOT_FOUND;
    }
    const uint16_t slot = static_cast<uint16_t>(logger->slot_by_number[number]);
    const esp_err_t err = read_slot(logger, slot, record);
    if (err != ESP_OK || record->number != number) {
        return (err == ESP_OK) ? ESP_ERR_NOT_FOUND : err;
    }
    return ESP_OK;
}

esp_err_t logger_print_number(logger_handle_t logger, uint16_t number)
{
    logger_record_t record = {};
    const esp_err_t err = logger_read(logger, number, &record);
    if (err != ESP_OK) {
        return err;
    }
    printf("%s\n", record.text);
    return ESP_OK;
}

esp_err_t logger_print_all(logger_handle_t logger)
{
    if (logger == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    for (int age = logger->oldest_age; age >= 0 && logger->count > 0U; --age) {
        const uint16_t number = static_cast<uint16_t>(
            (logger->newest_number + NUMBER_MODULUS -
             static_cast<uint16_t>(age)) % NUMBER_MODULUS);
        logger_record_t record = {};
        const esp_err_t err = logger_read(logger, number, &record);
        if (err == ESP_OK) {
            printf("%s\n", record.text);
        } else if (err != ESP_ERR_NOT_FOUND) {
            return err;
        }
    }
    return ESP_OK;
}

esp_err_t logger_print_range(logger_handle_t logger,
                             uint16_t first_number,
                             uint16_t last_number)
{
    if (logger == NULL || first_number >= NUMBER_MODULUS ||
        last_number >= NUMBER_MODULUS) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t number = first_number;
    for (uint16_t scanned = 0U; scanned < NUMBER_MODULUS; ++scanned) {
        logger_record_t record = {};
        const esp_err_t err = logger_read(logger, number, &record);
        if (err == ESP_OK) {
            printf("%s\n", record.text);
        } else if (err != ESP_ERR_NOT_FOUND) {
            return err;
        }
        if (number == last_number) {
            return ESP_OK;
        }
        number = static_cast<uint16_t>((number + 1U) % NUMBER_MODULUS);
    }
    return ESP_OK;
}
