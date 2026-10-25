# ESP32-S3 application with AT24C32 logger

This project logs text messages to an AT24C32 I2C EEPROM. Each log occupies one
32-byte EEPROM page and has the format `#<number> <message>`, followed by a NUL
terminator and zero padding. Message text is truncated to fit the page.

The logger stores up to 128 records in a ring buffer. Numbers range from `#0`
to `#999` and wrap back to `#0`. On startup it scans the EEPROM to restore the
newest record and the next write position. Logger output is sent to `printf()`.

## I2C setup

The AT24C32 uses the shared I2C bus configured in `src/main.cpp` (SDA GPIO 8,
SCL GPIO 9) and defaults to address `0x50`. The OLED and DS1307 continue to use
the same bus.

## Logger API

Include `logger.h` and initialize the logger after initializing the I2C bus:

```cpp
logger_handle_t logger = nullptr;
esp_err_t err = logger_init(bus, AT24C32_DEFAULT_ADDRESS, &logger);
```

Use `logger_write()` or `logger_printf()` to append a record.
`logger_get_info()` returns the number of records and the first/last record
numbers. `logger_read()` reads one record by number,
`logger_print_number()` prints one record, `logger_print_range()` prints an
inclusive range (including ranges that wrap through `#999` to `#0`), and
`logger_print_all()` prints records in chronological order.