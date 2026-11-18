#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SSD1306_PANEL_128x32 = 0,
    SSD1306_PANEL_128x64,
    SSD1306_PANEL_128x128,
} ssd1306_panel_sizes_t;

typedef struct {
    uint16_t i2c_address;
    uint32_t i2c_clock_speed;
    ssd1306_panel_sizes_t panel_size;
    uint8_t offset_x;
    bool flip_enabled;
    bool display_enabled;
} ssd1306_config_t;

typedef struct ssd1306_s *ssd1306_handle_t;

esp_err_t ssd1306_init(i2c_master_bus_handle_t bus, const ssd1306_config_t *config, ssd1306_handle_t *out_handle);
esp_err_t ssd1306_delete(ssd1306_handle_t handle);
esp_err_t ssd1306_clear_display(ssd1306_handle_t handle, bool invert);
esp_err_t ssd1306_set_pixel(ssd1306_handle_t handle, uint8_t x, uint8_t y, bool invert);
esp_err_t ssd1306_set_line(ssd1306_handle_t handle, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool invert);
esp_err_t ssd1306_set_circle(ssd1306_handle_t handle, uint8_t x, uint8_t y, uint8_t radius, bool invert);
esp_err_t ssd1306_display_filled_circle(ssd1306_handle_t handle, uint8_t x, uint8_t y, uint8_t radius, bool invert);
esp_err_t ssd1306_set_rectangle(ssd1306_handle_t handle, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert);
esp_err_t ssd1306_display_filled_rectangle(ssd1306_handle_t handle, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert);
esp_err_t ssd1306_set_contrast(ssd1306_handle_t handle, uint8_t contrast);
esp_err_t ssd1306_enable_display(ssd1306_handle_t handle);
esp_err_t ssd1306_disable_display(ssd1306_handle_t handle);
esp_err_t ssd1306_display_pages(ssd1306_handle_t handle);
esp_err_t ssd1306_display_text(ssd1306_handle_t handle, uint8_t row, const char *text, bool invert);
esp_err_t ssd1306_display_text_x2(ssd1306_handle_t handle, uint8_t row, const char *text, bool invert);
esp_err_t ssd1306_display_text_x3(ssd1306_handle_t handle, uint8_t row, const char *text, bool invert);
esp_err_t ssd1306_display_bdf_text(ssd1306_handle_t handle, const uint8_t *font, const char *text, int x, int y);

#ifdef __cplusplus
}
#endif

#endif
