#ifndef PROJECT_TYPES_H
#define PROJECT_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#define PROJECT_PROTOCOL_HEADER0           0x55U
#define PROJECT_PROTOCOL_HEADER1           0xAAU
#define PROJECT_PROTOCOL_MAX_PAYLOAD       96U
#define PROJECT_HISTORY_BURST_MAX          6U

#define PROJECT_DEFAULT_SAMPLE_PERIOD_MS   100U
#define PROJECT_DEFAULT_DISPLAY_PERIOD_MS  200U
#define PROJECT_DEFAULT_RECORD_PERIOD_MS   1000U
#define PROJECT_IDLE_SLEEP_TIMEOUT_MS      15000U

#define SAMPLE_STATUS_SENSOR_OPEN          0x01U
#define SAMPLE_STATUS_SENSOR_SHORT         0x02U
#define SAMPLE_STATUS_STORAGE_FULL         0x04U
#define SAMPLE_STATUS_UART_ACTIVE          0x08U

typedef enum {
    APP_STATE_INIT = 0,
    APP_STATE_MONITOR,
    APP_STATE_RECORD,
    APP_STATE_REPLAY,
    APP_STATE_CONFIG,
    APP_STATE_SLEEP,
    APP_STATE_BOOT
} app_state_t;

typedef enum {
    APP_PAGE_MAIN = 0,
    APP_PAGE_EXTENDED = 1
} app_page_t;

typedef enum {
    APP_RESULT_OK = 0,
    APP_RESULT_ERROR = 1,
    APP_RESULT_ARG = 2,
    APP_RESULT_BUSY = 3,
    APP_RESULT_EMPTY = 4
} app_result_t;

typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_SHORT,
    KEY_EVENT_LONG
} key_event_t;

typedef enum {
    APP_CMD_READ_SAMPLE = 0x01,
    APP_CMD_SET_DAC = 0x02,
    APP_CMD_SET_TIME = 0x03,
    APP_CMD_GET_TIME = 0x04,
    APP_CMD_START_RECORD = 0x05,
    APP_CMD_STOP_RECORD = 0x06,
    APP_CMD_READ_HISTORY = 0x07,
    APP_CMD_ERASE_HISTORY = 0x08,
    APP_CMD_QUERY_STATUS = 0x09,
    APP_CMD_ENTER_BOOT = 0x0A,
    APP_CMD_SWITCH_MODE = 0x0B
} app_command_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t day_of_week;
} rtc_datetime_t;

typedef struct {
    rtc_datetime_t timestamp;
    uint16_t raw_adc;
    uint16_t voltage_mv;
    int16_t temperature_centi_c;
    uint16_t dac_raw;
    uint8_t status;
} sample_record_t;

typedef struct {
    uint32_t sample_period_ms;
    uint32_t display_period_ms;
    uint32_t record_period_ms;
    uint16_t dac_raw;
    app_page_t display_page;
} system_config_t;

typedef struct {
    app_state_t current_state;
    app_state_t previous_state;
    bool recording_enabled;
    bool storage_full;
    bool sleep_armed;
    bool boot_requested;
    bool uart_online;
    uint8_t last_error;
    uint16_t record_count;
    uint32_t last_activity_ms;
} system_status_t;

#endif
