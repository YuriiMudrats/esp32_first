#ifndef AT24C32_H
#define AT24C32_H

#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AT24C32_CAPACITY_BYTES 4096U
#define AT24C32_PAGE_SIZE 32U
#define AT24C32_DEFAULT_ADDRESS 0x50U

typedef struct at24c32_s *at24c32_handle_t;

esp_err_t at24c32_init(i2c_master_bus_handle_t bus,
                       uint16_t address,
                       at24c32_handle_t *eeprom);
esp_err_t at24c32_deinit(at24c32_handle_t eeprom);
esp_err_t at24c32_read(at24c32_handle_t eeprom,
                       uint16_t address,
                       uint8_t *data,
                       size_t size);
esp_err_t at24c32_write_page(at24c32_handle_t eeprom,
                             uint16_t address,
                             const uint8_t data[AT24C32_PAGE_SIZE]);

#ifdef __cplusplus
}
#endif

#endif
