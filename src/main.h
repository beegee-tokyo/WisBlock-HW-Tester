/**
 * @file main.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Defines and includes
 * @version 0.1
 * @date 2024-02-28
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef _MAIN_H_
#define _MAIN_H_
#include <Arduino.h>
#include <WisBlock-API-V2.h>
#include "RAK1921_oled.h"

// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 1
#endif

#if MY_DEBUG > 0
#define MYLOG(tag, ...)                     \
	do                                      \
	{                                       \
		if (tag)                            \
			PRINTF("[%s] ", tag);           \
		PRINTF(__VA_ARGS__);                \
		PRINTF("\n");                       \
		if (g_ble_uart_is_connected)        \
		{                                   \
			g_ble_uart.printf(__VA_ARGS__); \
			g_ble_uart.printf("\n");        \
		}                                   \
	} while (0)
#else
#define MYLOG(...)
#endif

#ifndef SW_VERSION_1
/** Define the version of your SW */
#define SW_VERSION_1 1 // major version increase on API change / not backwards compatible
#define SW_VERSION_2 0 // minor version increase on API change / backward compatible
#define SW_VERSION_3 0 // patch version increase on bugfix, no affect on API
#endif

/** Application function definitions */
void setup_app(void);
bool init_app(void);
void app_event_handler(void);
void ble_data_handler(void) __attribute__((weak));
void lora_data_handler(void);

// EPD
void status_rak14000(void);
bool init_rak14000(void);
void rak14000_text(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size);
void rak14000_logo(int16_t x, int16_t y);
void clear_rak14000(void);
void refresh_rak14000(void);
#define POWER_ENABLE WB_IO2

// GNSS functions
#define NO_GNSS_INIT 0
#define RAK1910_GNSS 1
#define RAK12500_GNSS 2
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
bool init_gnss(void);
bool poll_gnss(void);
void gnss_task(void *pvParameters);
extern SemaphoreHandle_t g_gnss_sem;
extern TaskHandle_t gnss_task_handle;
extern volatile bool last_read_ok;
extern uint8_t gnss_option;
extern bool gnss_ok;
extern bool has_rak12500;

extern bool flash_success;
extern uint8_t lora_success;
extern bool has_rak1921;

#endif // _MAIN_H_