#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>
#include <stdint.h>

#include "at24c32.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOGGER_RECORD_SIZE AT24C32_PAGE_SIZE
#define LOGGER_MAX_RECORDS (AT24C32_CAPACITY_BYTES / LOGGER_RECORD_SIZE)
#define LOGGER_NUMBER_COUNT 1000U

typedef struct logger_s *logger_handle_t;

typedef struct {
    uint16_t number;
    char text[LOGGER_RECORD_SIZE];
} logger_record_t;

typedef struct {
    uint16_t count;
    uint16_t first_number;
    uint16_t last_number;
} logger_info_t;

esp_err_t logger_init(i2c_master_bus_handle_t bus,
                      uint16_t eeprom_address,
                      logger_handle_t *logger);
esp_err_t logger_deinit(logger_handle_t logger);
esp_err_t logger_write(logger_handle_t logger, const char *message);
esp_err_t logger_printf(logger_handle_t logger, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
esp_err_t logger_get_info(logger_handle_t logger, logger_info_t *info);
esp_err_t logger_read(logger_handle_t logger,
                      uint16_t number,
                      logger_record_t *record);
esp_err_t logger_print_number(logger_handle_t logger, uint16_t number);
esp_err_t logger_print_all(logger_handle_t logger);
esp_err_t logger_print_range(logger_handle_t logger,
                             uint16_t first_number,
                             uint16_t last_number);

#ifdef __cplusplus
}
#endif

#endif
