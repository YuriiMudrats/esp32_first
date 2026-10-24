#include "ssd1306.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver/i2c_master.h"

#define SSD1306_I2C_TIMEOUT_MS 1000

static const uint8_t SSD1306_DEFAULT_ADDRESS = 0x3C;

#define SSD1306_PAGE_WIDTH 128
#define SSD1306_PAGE_HEIGHT 64

struct ssd1306_s {
    uint8_t width;
    uint8_t height;
    uint8_t *framebuffer;
    uint8_t contrast;
    bool enabled;
    bool flip;
    uint8_t offset_x;
    i2c_master_dev_handle_t i2c_dev;
    uint8_t i2c_address;
};

static esp_err_t ssd1306_write_command(ssd1306_handle_t handle, uint8_t command)
{
    if (handle == NULL || handle->i2c_dev == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buffer[2] = {0x00, command};
    return i2c_master_transmit(handle->i2c_dev, buffer, sizeof(buffer), SSD1306_I2C_TIMEOUT_MS);
}

static esp_err_t ssd1306_write_data(ssd1306_handle_t handle, const uint8_t *data, size_t size)
{
    if (handle == NULL || handle->i2c_dev == NULL || data == NULL || size == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t *tx = static_cast<uint8_t *>(malloc(size + 1U));
    if (tx == NULL) {
        return ESP_ERR_NO_MEM;
    }

    tx[0] = 0x40;
    memcpy(&tx[1], data, size);
    const esp_err_t err = i2c_master_transmit(handle->i2c_dev, tx, size + 1U, SSD1306_I2C_TIMEOUT_MS);
    free(tx);
    return err;
}

static uint8_t ssd1306_bit_mask(uint8_t y)
{
    return (uint8_t)(1u << (y % 8));
}

static void ssd1306_put_pixel_raw(struct ssd1306_s *dev, uint8_t x, uint8_t y, bool set)
{
    if (dev == NULL || dev->framebuffer == NULL || x >= dev->width || y >= dev->height) {
        return;
    }

    const uint8_t page = (uint8_t)(y / 8);
    const uint8_t bit = ssd1306_bit_mask(y);
    const size_t index = (size_t)page * dev->width + x;
    if (set) {
        dev->framebuffer[index] |= bit;
    } else {
        dev->framebuffer[index] &= (uint8_t)(~bit);
    }
}

static void ssd1306_draw_hline(struct ssd1306_s *dev, int x0, int x1, int y, bool set)
{
    if (x0 > x1) {
        int tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    for (int x = x0; x <= x1; ++x) {
        ssd1306_put_pixel_raw(dev, (uint8_t)x, (uint8_t)y, set);
    }
}

static void ssd1306_draw_vline(struct ssd1306_s *dev, int y0, int y1, int x, bool set)
{
    if (y0 > y1) {
        int tmp = y0;
        y0 = y1;
        y1 = tmp;
    }
    for (int y = y0; y <= y1; ++y) {
        ssd1306_put_pixel_raw(dev, (uint8_t)x, (uint8_t)y, set);
    }
}

static void ssd1306_draw_char(struct ssd1306_s *dev, int x, int y, char ch, bool invert)
{
    static const uint8_t font5x7[95][5] = {
        {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},{0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},
        {0x23,0x13,0x08,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1C,0x22,0x41,0x00},{0x00,0x41,0x22,0x1C,0x00},
        {0x14,0x08,0x3E,0x08,0x14},{0x08,0x08,0x3E,0x08,0x08},{0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
        {0x20,0x10,0x08,0x04,0x02},{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x72,0x49,0x49,0x49,0x36},{0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},                {0x00,0x56,0x36,0x00,0x00},{0x00,0x36,0x36,0x00,0x00},{0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},{0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3E},{0x7E,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x41,0x49,0x49,0x49,0x3E},{0x7F,0x09,0x09,0x09,0x01},{0x3E,0x41,0x41,0x41,0x22},{0x7F,0x09,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x3E},{0x7F,0x09,0x09,0x09,0x01},{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00},    };    const uint8_t code = (uint8_t)ch;
    if (code < 32 || code > 126) {
        return;
    }
    const uint8_t *glyph = font5x7[(size_t)(code - 32)];
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if (glyph[col] & (1u << row)) {
                ssd1306_put_pixel_raw(dev, (uint8_t)(x + col), (uint8_t)(y + row), !invert);
            } else if (invert) {
                ssd1306_put_pixel_raw(dev, (uint8_t)(x + col), (uint8_t)(y + row), true);
            }
        }
    }
}

esp_err_t ssd1306_init(i2c_master_bus_handle_t bus, const ssd1306_config_t *config, ssd1306_handle_t *out_handle)
{
    if (bus == NULL || config == NULL || out_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    struct ssd1306_s *dev = static_cast<struct ssd1306_s *>(calloc(1, sizeof(*dev)));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }

    switch (config->panel_size) {
    case SSD1306_PANEL_128x32:
        dev->width = 128;
        dev->height = 32;
        break;
    case SSD1306_PANEL_128x128:
        dev->width = 128;
        dev->height = 128;
        break;
    default:
        dev->width = 128;
        dev->height = 64;
        break;
    }

    const size_t framebuffer_size = (size_t)(dev->width * dev->height / 8U);
    dev->framebuffer = static_cast<uint8_t *>(calloc(1, framebuffer_size));
    if (dev->framebuffer == NULL) {
        free(dev);
        return ESP_ERR_NO_MEM;
    }

    dev->contrast = 0x7F;
    dev->enabled = config->display_enabled;
    dev->flip = config->flip_enabled;
    dev->offset_x = config->offset_x;
    dev->i2c_address = (config->i2c_address != 0U) ? config->i2c_address : SSD1306_DEFAULT_ADDRESS;

    i2c_device_config_t device_cfg = {};
    device_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device_cfg.device_address = dev->i2c_address;
    device_cfg.scl_speed_hz = (config->i2c_clock_speed != 0U) ? config->i2c_clock_speed : 400000U;

    esp_err_t err = i2c_master_bus_add_device(bus, &device_cfg, &dev->i2c_dev);
    if (err != ESP_OK) {
        free(dev->framebuffer);
        free(dev);
        return err;
    }

    err = ssd1306_write_command(dev, 0xAE); // display off
    if (err != ESP_OK) {
        goto init_fail;
    }

    err = ssd1306_write_command(dev, 0xD5);
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x80);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0xA8);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, (uint8_t)(dev->height - 1U));
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0xD3);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x00);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x40);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0xA1);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0xC8);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0xDA);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x12);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x81);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x7F);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0xD9);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x22);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0xDB);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x40);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x20);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x00);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x8D);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0x14);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0xA6);
    }
    if (err == ESP_OK) {
        err = ssd1306_write_command(dev, 0xAF);
    }

    if (err != ESP_OK) {
init_fail:
        if (dev->i2c_dev != NULL) {
            i2c_master_bus_rm_device(dev->i2c_dev);
            dev->i2c_dev = NULL;
        }
        free(dev->framebuffer);
        free(dev);
        return err;
    }

    *out_handle = dev;
    return ESP_OK;
}

esp_err_t ssd1306_delete(ssd1306_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->i2c_dev != NULL) {
        i2c_master_bus_rm_device(handle->i2c_dev);
    }
    free(handle->framebuffer);
    free(handle);
    return ESP_OK;
}

esp_err_t ssd1306_clear_display(ssd1306_handle_t handle, bool invert)
{
    if (handle == NULL || handle->framebuffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(handle->framebuffer, invert ? 0xFF : 0x00, (size_t)(handle->width * handle->height / 8U));
    return ESP_OK;
}

esp_err_t ssd1306_set_pixel(ssd1306_handle_t handle, uint8_t x, uint8_t y, bool invert)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    ssd1306_put_pixel_raw(handle, x, y, !invert);
    return ESP_OK;
}

esp_err_t ssd1306_set_line(ssd1306_handle_t handle, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool invert)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int dx = abs((int)x1 - (int)x0);
    int dy = abs((int)y1 - (int)y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        ssd1306_put_pixel_raw(handle, x0, y0, !invert);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int err2 = err * 2;
        if (err2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (err2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
    return ESP_OK;
}

esp_err_t ssd1306_set_circle(ssd1306_handle_t handle, uint8_t x, uint8_t y, uint8_t radius, bool invert)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int cx = (int)x;
    int cy = (int)y;
    int r = (int)radius;

    for (int yy = -r; yy <= r; ++yy) {
        for (int xx = -r; xx <= r; ++xx) {
            if ((xx * xx) + (yy * yy) <= (r * r)) {
                ssd1306_put_pixel_raw(handle, (uint8_t)(cx + xx), (uint8_t)(cy + yy), !invert);
            }
        }
    }
    return ESP_OK;
}

esp_err_t ssd1306_display_filled_circle(ssd1306_handle_t handle, uint8_t x, uint8_t y, uint8_t radius, bool invert)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int cx = (int)x;
    int cy = (int)y;
    int r = (int)radius;

    for (int yy = -r; yy <= r; ++yy) {
        for (int xx = -r; xx <= r; ++xx) {
            if ((xx * xx) + (yy * yy) <= (r * r)) {
                ssd1306_put_pixel_raw(handle, (uint8_t)(cx + xx), (uint8_t)(cy + yy), !invert);
            }
        }
    }
    return ESP_OK;
}

esp_err_t ssd1306_set_rectangle(ssd1306_handle_t handle, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    ssd1306_draw_hline(handle, x, x + width, y, !invert);
    ssd1306_draw_hline(handle, x, x + width, y + height, !invert);
    ssd1306_draw_vline(handle, y, y + height, x, !invert);
    ssd1306_draw_vline(handle, y, y + height, x + width, !invert);
    return ESP_OK;
}

esp_err_t ssd1306_display_filled_rectangle(ssd1306_handle_t handle, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool invert)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    for (uint8_t yy = y; yy <= y + height; ++yy) {
        ssd1306_draw_hline(handle, x, x + width, yy, !invert);
    }
    return ESP_OK;
}

esp_err_t ssd1306_set_contrast(ssd1306_handle_t handle, uint8_t contrast)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    handle->contrast = contrast;
    return ESP_OK;
}

esp_err_t ssd1306_enable_display(ssd1306_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    handle->enabled = true;
    return ESP_OK;
}

esp_err_t ssd1306_disable_display(ssd1306_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    handle->enabled = false;
    return ESP_OK;
}

esp_err_t ssd1306_display_pages(ssd1306_handle_t handle)
{
    if (handle == NULL || handle->framebuffer == NULL || handle->i2c_dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const size_t page_count = (size_t)(handle->height / 8U);
    for (size_t page = 0; page < page_count; ++page) {
        const size_t offset = page * (size_t)handle->width;

        esp_err_t err = ssd1306_write_command(handle, (uint8_t)(0xB0U | page));
        if (err != ESP_OK) {
            return err;
        }
        err = ssd1306_write_command(handle, 0x00U);
        if (err != ESP_OK) {
            return err;
        }
        err = ssd1306_write_command(handle, 0x10U);
        if (err != ESP_OK) {
            return err;
        }

        const uint8_t *page_data = &handle->framebuffer[offset];
        err = ssd1306_write_data(handle, page_data, (size_t)handle->width);
        if (err != ESP_OK) {
            return err;
        }
    }

    return ESP_OK;
}

esp_err_t ssd1306_display_text(ssd1306_handle_t handle, uint8_t row, const char *text, bool invert)
{
    if (handle == NULL || text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int x = 0;
    for (const char *p = text; *p != '\0'; ++p) {
        ssd1306_draw_char(handle, x, row * 8, *p, invert);
        x += 6;
    }
    return ESP_OK;
}

esp_err_t ssd1306_display_text_x2(ssd1306_handle_t handle, uint8_t row, const char *text, bool invert)
{
    if (handle == NULL || text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int x = 0;
    for (const char *p = text; *p != '\0'; ++p) {
        ssd1306_draw_char(handle, x, row * 8, *p, invert);
        ssd1306_draw_char(handle, x + 1, row * 8, *p, invert);
        ssd1306_draw_char(handle, x, row * 8 + 1, *p, invert);
        ssd1306_draw_char(handle, x + 1, row * 8 + 1, *p, invert);
        x += 12;
    }
    return ESP_OK;
}

esp_err_t ssd1306_display_text_x3(ssd1306_handle_t handle, uint8_t row, const char *text, bool invert)
{
    if (handle == NULL || text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int x = 0;
    for (const char *p = text; *p != '\0'; ++p) {
        for (int sy = 0; sy < 3; ++sy) {
            for (int sx = 0; sx < 3; ++sx) {
                ssd1306_draw_char(handle, x + sx * 2, row * 8 + sy * 2, *p, invert);
            }
        }
        x += 18;
    }
    return ESP_OK;
}

esp_err_t ssd1306_display_bdf_text(ssd1306_handle_t handle, const uint8_t *font, const char *text, int x, int y)
{
    (void)font;
    if (handle == NULL || text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    int cursor_x = x;
    for (const char *p = text; *p != '\0'; ++p) {
        ssd1306_draw_char(handle, cursor_x, y, *p, false);
        cursor_x += 6;
    }
    return ESP_OK;
}
