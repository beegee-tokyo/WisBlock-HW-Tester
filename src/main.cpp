/**
 * @file main.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Low power test
 * @version 0.1
 * @date 2023-02-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "main.h"
#include <radio/radio.h>

/** Send Fail counter **/
uint8_t send_fail = 0;

/** Set the device name, max length is 10 characters */
char g_ble_dev_name[10] = "RAK-TEST";

/** Flag if RAK1921 was found */
bool has_rak1921 = false;

/** Flag if RAK12500 was found */
bool has_rak12500 = false;

/** Buffer for RAK1921 OLED text */
char disp_txt[256];

/** Flag if RAK14000 was found */
bool has_rak14000 = false;

/** Flag is LoRa init was successfull */
uint8_t lora_success = 2;

/** Flag is Flash read/write was successfull */
bool flash_success = false;

SoftwareTimer blink_leds_timer;

void toggle_led(TimerHandle_t unsused)
{
	digitalToggle(LED_BLUE);
	digitalToggle(LED_GREEN);
}

/**
 * @brief Initial setup of the application (before LoRaWAN and BLE setup)
 *
 */
void setup_app(void)
{
	// Set firmware version
	api_set_version(SW_VERSION_1, SW_VERSION_2, SW_VERSION_3);

	// Read LoRaWAN settings from flash
	api_read_credentials();

	if (g_lorawan_settings.lorawan_enable != false)
	{
		// Change LoRaWAN settings
		g_lorawan_settings.lorawan_enable = false;	 // Force LoRa P2P
		g_lorawan_settings.send_repeat_time = 30000; // Force 30 seconds send interval
		g_lorawan_settings.auto_join = false;		 // Disable automatic join ==> enable BLE advertising
		// Save LoRaWAN settings
		api_set_credentials();
	}

	// MYLOG("APP", "Setup application");
	g_enable_ble = true;
}

/**
 * @brief Final setup of application  (after LoRaWAN and BLE setup)
 *
 * @return true
 * @return false
 */
bool init_app(void)
{
	Serial.begin(115200);
	time_t serial_timeout = millis();
	// On nRF52840 the USB serial is not available immediately
	while (!Serial)
	{
		if ((millis() - serial_timeout) < 5000)
		{
			delay(100);
			digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
		}
		else
		{
			break;
		}
	}
	digitalWrite(LED_GREEN, LOW);

	MYLOG("APP", "Initialize application");
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);
	// restart_advertising(30);

	pinMode(LED_BLUE, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);

	digitalWrite(LED_BLUE, HIGH);
	digitalWrite(LED_GREEN, LOW);

	blink_leds_timer.begin(250, toggle_led, NULL, true);
	blink_leds_timer.start();

	// Test OLED
	has_rak1921 = init_rak1921();
	if (has_rak1921)
	{
		rak1921_write_header((char *)"WisBlock Node");
	}
	else
	{
		MYLOG("OLED", "No OLED found");
	}

	// Initialize EPD
	has_rak14000 = init_rak14000();
	if (has_rak1921)
	{
		if (has_rak14000)
		{
			sprintf(disp_txt, "Found RAK14000 EPD");
			rak1921_add_line(disp_txt);
		}
		else
		{
			sprintf(disp_txt, "No RAK14000 EPD");
			rak1921_add_line(disp_txt);
		}
	}

	if (has_rak14000)
	{
		MYLOG("EPD", "Found RAK14000 EPD");
	}
	else
	{
		MYLOG("EPD", "No RAK14000 EPD");
	}

	// Scan the I2C interfaces for devices
	byte error;
	uint8_t num_dev = 0;

	Wire.begin();
	// Some modules support only 100kHz
	Wire.setClock(100000);
	for (byte address = 1; address < 127; address++)
	{
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		if (error == 0)
		{
			MYLOG("SCAN", "Found sensor at I2C1 0x%02X", address);
			if (address == 0x3c)
			{
				has_rak1921 = true;
			}
			if (has_rak1921)
			{
				sprintf(disp_txt, "Found I2C device 0x%02X", address);
				rak1921_add_line(disp_txt);
			}
			if (address == 0x42)
			{
				has_rak12500 = true;
			}
			num_dev++;
		}
	}
	MYLOG("SCAN", "Found %d I2C devices", num_dev);
	if (has_rak1921)
	{
		sprintf(disp_txt, "Found %d I2C devices", num_dev);
		rak1921_add_line(disp_txt);
	}

	// If it has RAK12500, setup the GNSS module with RAK specific settings
	if (has_rak12500)
	{
		has_rak12500 = init_gnss();
	}

	// Erase flash file system
	flash_reset();
	// Save LoRaWAN settings (in case they were still there on top of Meshtastic settings)
	api_set_credentials();

	// Write-Read Flash test
	MYLOG("FLASH", "Flash Write-Read test #1");

	if (save_settings())
	{
		flash_success = true;
		MYLOG("FLASH", "Flash Write-Read test #1 success");
		if (has_rak1921)
		{
			sprintf(disp_txt, "Flash Write-Read test #1 success");
			rak1921_add_line(disp_txt);
		}
	}
	else
	{
		flash_success = false;
		MYLOG("FLASH", "Flash Write-Read test #1 failed");
		if (has_rak1921)
		{
			sprintf(disp_txt, "Flash Write-Read test #1 failed");
			rak1921_add_line(disp_txt);
		}
	}
	MYLOG("FLASH", "Read send time from flash %ld", g_lorawan_settings.send_repeat_time);

	MYLOG("FLASH", "Flash Write-Read test #2");
	g_lorawan_settings.send_repeat_time = 60000;

	if (save_settings())
	{
		flash_success = true;
		MYLOG("FLASH", "Flash Write-Read test #2 success");
		if (has_rak1921)
		{
			sprintf(disp_txt, "Flash Write-Read test #2 success");
			rak1921_add_line(disp_txt);
		}
	}
	else
	{
		flash_success = false;
		MYLOG("FLASH", "Flash Write-Read test #2 failed");
		if (has_rak1921)
		{
			sprintf(disp_txt, "Flash Write-Read test #2 failed");
			rak1921_add_line(disp_txt);
		}
	}

	// Check connection to SX126x
	// After power on the sync word should be 2414. 4434 could be possible on a restart (private network syncword)
	// If we got something else, something is wrong.
	uint16_t readSyncWord = 0;

	SX126xReadRegisters(REG_LR_SYNCWORD, (uint8_t *)&readSyncWord, 2);

	MYLOG("SX1262", "SyncWord = %04X", readSyncWord);
	if (has_rak1921)
	{
		sprintf(disp_txt, "SyncWord = %04X", readSyncWord);
		rak1921_add_line(disp_txt);
	}

	if ((readSyncWord == 0x2414) || (readSyncWord == 0x4434))
	{
		MYLOG("SX1262", "LoRa transceiver ok");
		if (has_rak1921)
		{
			sprintf(disp_txt, "LoRa transceiver ok");
			rak1921_add_line(disp_txt);
		}
	}
	else
	{
		MYLOG("SX1262", "SyncWord is incorrect, potential problem in SPI setup or LoRa transceiver");
		if (has_rak1921)
		{
			sprintf(disp_txt, "SX1262 problem (SPI or LoRa chip");
			rak1921_add_line(disp_txt);
		}
	}

	// Read battery values
	float batt_level_f = 0.0;
	for (int readings = 0; readings < 10; readings++)
	{
		batt_level_f += read_batt();
	}
	batt_level_f = batt_level_f / 10;
	MYLOG("APP", "Battery %.2f V", batt_level_f / 1000);

	if (has_rak14000)
	{
		refresh_rak14000();
	}
	blink_leds_timer.start();

	// restart_advertising(60);

	Serial.flush();
	delay(500);

	return true;
}

/**
 * @brief Handle events
 * 		Events can be
 * 		- timer (setup with AT+SENDINT=xxx)
 * 		- interrupt events
 * 		- wake-up signals from other tasks
 */
void app_event_handler(void)
{
	blink_leds_timer.start();
	// Timer triggered event
	if ((g_task_event_type & STATUS) == STATUS)
	{
		g_task_event_type &= N_STATUS;
		// restart_advertising(60);
		MYLOG("APP", "Timer wakeup");
		if (has_rak1921)
		{
			sprintf(disp_txt, "Timer wakeup");
			rak1921_add_line(disp_txt);
		}

		// Check GNSS location
		if (has_rak12500)
		{
			MYLOG("APP", "Try GNSS");
			if (has_rak1921)
			{
				sprintf(disp_txt, "Try GNSS");
				rak1921_add_line(disp_txt);
			}
			poll_gnss();
		}

		// Restart BLE advertising
		// restart_advertising(30);

		// Get Battery status
		float batt_level_f = 0.0;
		for (int readings = 0; readings < 10; readings++)
		{
			batt_level_f += read_batt();
		}
		batt_level_f = batt_level_f / 10;
		MYLOG("APP", "Battery %.2f V", batt_level_f / 1000);

		// Dummy packet
		uint8_t dummy_packet[] = {0x01, 0x74, 0x00, 0x55};
		uint16_t batt_level = (uint16_t)(batt_level_f);
		dummy_packet[2] = (uint8_t)(batt_level >> 8);
		dummy_packet[3] = (uint8_t)(batt_level);

		if (g_lorawan_settings.lorawan_enable)
		{
			if (g_lpwan_has_joined)
			{

				lmh_error_status result = send_lora_packet(dummy_packet, 4, 2);
				switch (result)
				{
				case LMH_SUCCESS:
					MYLOG("APP", "Packet enqueued");
					lora_success = 0;
					break;
				case LMH_BUSY:
					MYLOG("APP", "LoRa transceiver is busy");
					lora_success = 1;
					break;
				case LMH_ERROR:
					MYLOG("APP", "Packet error, too big to send with current DR");
					lora_success = 1;
					break;
				}
			}
			else
			{
				MYLOG("APP", "Network not joined, skip sending");
			}
		}
		else
		{
			MYLOG("APP", "Send P2P packet");
			if (has_rak1921)
			{
				sprintf(disp_txt, "Send P2P packet");
				rak1921_add_line(disp_txt);
			}
			if (send_p2p_packet(dummy_packet, 4))
			{
				lora_success = 0;
			}
			else
			{
				lora_success = 1;
			}
		}

		if (has_rak14000)
		{
			refresh_rak14000();
			delay(3000);
		}
	}
}

/**
 * @brief Handle BLE events
 *
 */
void ble_data_handler(void)
{
	if (g_enable_ble)
	{
		/**************************************************************/
		/**************************************************************/
		/// \todo BLE UART data arrived
		/// \todo or forward them to the AT command interpreter
		/// \todo parse them here
		/**************************************************************/
		/**************************************************************/
		if ((g_task_event_type & BLE_DATA) == BLE_DATA)
		{
			MYLOG("AT", "RECEIVED BLE");
			// BLE UART data arrived
			// in this example we forward it to the AT command interpreter
			g_task_event_type &= N_BLE_DATA;

			while (g_ble_uart.available() > 0)
			{
				at_serial_input(uint8_t(g_ble_uart.read()));
				delay(5);
			}
			at_serial_input(uint8_t('\n'));
		}
	}
}

/**
 * @brief Handle LoRa events
 *
 */
void lora_data_handler(void)
{
	// LoRa Join finished handling
	if ((g_task_event_type & LORA_JOIN_FIN) == LORA_JOIN_FIN)
	{
		g_task_event_type &= N_LORA_JOIN_FIN;
		if (g_join_result)
		{
			MYLOG("APP", "Successfully joined network");
		}
		else
		{
			MYLOG("APP", "Join network failed");
			/// \todo here join could be restarted.
			// lmh_join();
		}
	}

	// LoRa data handling
	if ((g_task_event_type & LORA_DATA) == LORA_DATA)
	{
		/**************************************************************/
		/**************************************************************/
		/// \todo LoRa data arrived
		/// \todo parse them here
		/**************************************************************/
		/**************************************************************/
		g_task_event_type &= N_LORA_DATA;
		MYLOG("APP", "Received package over LoRa");
		MYLOG("APP", "Last RSSI %d", g_last_rssi);

		char log_buff[g_rx_data_len * 3] = {0};
		uint8_t log_idx = 0;
		for (int idx = 0; idx < g_rx_data_len; idx++)
		{
			sprintf(&log_buff[log_idx], "%02X ", g_rx_lora_data[idx]);
			log_idx += 3;
		}
		MYLOG("APP", "%s", log_buff);
	}

	// LoRa TX finished handling
	if ((g_task_event_type & LORA_TX_FIN) == LORA_TX_FIN)
	{
		g_task_event_type &= N_LORA_TX_FIN;

		if (g_lorawan_settings.lorawan_enable)
		{
			if (g_lorawan_settings.confirmed_msg_enabled == LMH_UNCONFIRMED_MSG)
			{
				MYLOG("APP", "LPWAN TX cycle finished");
			}
			else
			{
				MYLOG("APP", "LPWAN TX cycle %s", g_rx_fin_result ? "finished ACK" : "failed NAK");
			}
			if (!g_rx_fin_result)
			{
				// Increase fail send counter
				send_fail++;

				if (send_fail == 10)
				{
					// Too many failed sendings, reset node and try to rejoin
					delay(100);
					api_reset();
				}
			}
		}
		else
		{
			MYLOG("APP", "P2P TX finished");
			if (has_rak1921)
			{
				sprintf(disp_txt, "P2P TX finished");
				rak1921_add_line(disp_txt);
			}
		}
	}
}
