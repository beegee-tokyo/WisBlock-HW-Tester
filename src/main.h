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
// #ifdef NRF52_SERIES
// #include <Adafruit_TinyUSB.h>
// #endif
#include <SX126x-Arduino.h>
#include <SPI.h>
#include <LoRaWan-Arduino.h>
#include "RAK1921_oled.h"

#ifndef SW_VERSION_1
/** Define the version of your SW */
#define SW_VERSION_1 1 // major version increase on API change / not backwards compatible
#define SW_VERSION_2 0 // minor version increase on API change / backward compatible
#define SW_VERSION_3 0 // patch version increase on bugfix, no affect on API
#endif

/** LoRa definitions */
// Function declarations
void OnTxDone(void);
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnTxTimeout(void);
void OnRxTimeout(void);
void OnRxError(void);
void OnCadDone(bool cadResult);

// Define LoRa parameters
#define RF_FREQUENCY 910000000		  // Hz
#define TX_OUTPUT_POWER 22			  // dBm
#define LORA_BANDWIDTH 0			  // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 7		  // [SF7..SF12]
#define LORA_CODINGRATE 1			  // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8		  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0		  // Symbols
#define LORA_FIX_LENGTH_PAYLOAD false // No fixed payload length
#define LORA_IQ_INVERSION false		  // No IQ inversion
#define TX_TIMEOUT_VALUE 120000
#define RX_TIMEOUT_VALUE 30000
#define BUFFER_SIZE 64 // Define the payload size here

// EPD (only RAK4630/RAK3400)
bool init_rak14000(void);
void rak14000_text(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size);
void rak14000_logo(int16_t x, int16_t y);
void clear_rak14000(void);
void refresh_rak14000(void);
#define POWER_ENABLE WB_IO2

// GNSS functions
#define NO_GNSS_INIT 0
#define RAK12501_GNSS 1
#define RAK12500_GNSS 2
#define NO_GNSS_FOUND 3

#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <TinyGPS++.h>
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

// Flash
#define LORAWAN_DATA_MARKER 0x55
struct s_lorawan_settings
{
	uint8_t valid_mark_1 = 0xAA;				// Just a marker for the Flash
	uint8_t valid_mark_2 = LORAWAN_DATA_MARKER; // Just a marker for the Flash
												// OTAA Device EUI MSB
	uint8_t node_device_eui[8] = {0x00, 0x0D, 0x75, 0xE6, 0x56, 0x4D, 0xC1, 0xF3};
	// OTAA Application EUI MSB
	uint8_t node_app_eui[8] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x02, 0x01, 0xE1};
	// OTAA Application Key MSB
	uint8_t node_app_key[16] = {0x2B, 0x84, 0xE0, 0xB0, 0x9B, 0x68, 0xE5, 0xCB, 0x42, 0x17, 0x6F, 0xE7, 0x53, 0xDC, 0xEE, 0x79};
	// ABP Device Address MSB
	uint32_t node_dev_addr = 0x26021FB4;
	// ABP Network Session Key MSB
	uint8_t node_nws_key[16] = {0x32, 0x3D, 0x15, 0x5A, 0x00, 0x0D, 0xF3, 0x35, 0x30, 0x7A, 0x16, 0xDA, 0x0C, 0x9D, 0xF5, 0x3F};
	// ABP Application Session key MSB
	uint8_t node_apps_key[16] = {0x3F, 0x6A, 0x66, 0x45, 0x9D, 0x5E, 0xDC, 0xA6, 0x3C, 0xBC, 0x46, 0x19, 0xCD, 0x61, 0xA1, 0x1E};
	// Flag for OTAA or ABP
	bool otaa_enabled = true;
	// Flag for ADR on or off
	bool adr_enabled = false;
	// Flag for public or private network
	bool public_network = true;
	// Flag to enable duty cycle
	bool duty_cycle_enabled = false;
	// Default is off
	uint32_t send_repeat_time = 10000;
	// Number of join retries
	uint8_t join_trials = 5;
	// TX power 0 .. 10
	uint8_t tx_power = 0;
	// Data rate 0 .. 15 (validity depnends on Region)
	uint8_t data_rate = 3;
	// LoRaWAN class 0: A, 2: C, 1: B is not supported
	uint8_t lora_class = 0;
	// Subband channel selection 1 .. 9
	uint8_t subband_channels = 1;
	// Flag if node joins automatically after reboot
	bool auto_join = false;
	// Data port to send data
	uint8_t app_port = 2;
	// Flag to enable confirmed messages
	lmh_confirm confirmed_msg_enabled = LMH_UNCONFIRMED_MSG;
	// Fixed LoRaWAN lorawan_region (depends on compiler option)
	uint8_t lora_region = 1;
	// Flag for LoRaWAN or LoRa P2P
	bool lorawan_enable = false;
	// Frequency in Hz
	uint32_t p2p_frequency = 910000000;
	// Tx power 0 .. 22
	uint8_t p2p_tx_power = 22;
	// Bandwidth 0: 125, 1: 250, 2: 500, 3: 62.5, 4: 41.67, 5: 31.25, 6: 20.83, 7: 15.63, 8: 10.4, 9: 7.8
	uint8_t p2p_bandwidth = 0;
	// Spreading Factor SF7..SF12
	uint8_t p2p_sf = 7;
	// Coding Rate 1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8
	uint8_t p2p_cr = 1;
	// Preamble length
	uint8_t p2p_preamble_len = 8;
	// Symbol timeout
	uint16_t p2p_symbol_timeout = 0;
	// Command from BLE to reset device
	bool resetRequest = true;
};
extern s_lorawan_settings g_lorawan_settings;

void init_flash(void);
bool save_settings(void);
void log_settings(void);
void flash_reset(void);

// Battery
void init_batt(void);
float read_batt(void);
uint8_t get_lora_batt(void);
uint8_t mv_to_percent(float mvolts);

// BLE
#ifdef NRF52_SERIES
#include <bluefruit.h>
extern BLEUart g_ble_uart;
#endif
#ifdef ESP32
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLE2902.h>
extern BLECharacteristic *uart_tx_characteristic;
#endif
extern char g_ble_dev_name[];
void init_ble();
extern bool g_ble_uart_is_connected;

// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 1
#endif

#ifdef NRF52_SERIES
#if MY_DEBUG > 0
#define MYLOG(tag, ...)                 \
	if (tag)                            \
		PRINTF("[%s] ", tag);           \
	PRINTF(__VA_ARGS__);                \
	PRINTF("\n");                       \
	if (g_ble_uart_is_connected)        \
	{                                   \
		g_ble_uart.printf(__VA_ARGS__); \
		g_ble_uart.printf("\r\n");      \
		g_ble_uart.flush();             \
	}
#else
#define MYLOG(...)
#endif
#endif

#ifdef ESP32
#if MY_DEBUG > 0
#define MYLOG(tag, ...)                           \
	if (tag)                                      \
		Serial.printf("[%s] ", tag);              \
	Serial.printf(__VA_ARGS__);                   \
	Serial.printf("\n");                          \
	if (g_ble_uart_is_connected)                  \
	{                                             \
		char buff[255];                           \
		int len = sprintf(buff, __VA_ARGS__);     \
		std::string buff_s(buff);                 \
		uart_tx_characteristic->setValue(buff_s); \
		uart_tx_characteristic->notify();         \
		delay(50);                                \
	}
#else
#define MYLOG(...)
#endif
#endif

#endif // _MAIN_H_