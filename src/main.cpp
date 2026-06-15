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

/** Send Fail counter **/
uint8_t send_fail = 0;

/** Set the device name, max length is 10 characters */
char g_ble_dev_name[10] = "RAK-TEST";

/** Flag if RAK1921 was found */
bool has_rak1921 = false;

/** Flag if RAK12500 was found */
bool has_rak12500 = false;
/** Flag if RAK12501 was found */
bool has_rak12501 = false;

/** Flag if Ethernet module was found */
bool has_rak13800 = false;

/** Flag if LORa transceiver init failed */
bool sx1262_ok = false;

/** Buffer for RAK1921 OLED text */
char disp_txt[256];

/** Flag if RAK14000 was found */
bool has_rak14000 = false;

/** Flag is LoRa init was successfull */
uint8_t lora_success = 2;

/** Flag is Flash read/write was successfull */
bool flash_success = false;

#ifdef NRF52_SERIES
SoftwareTimer blink_leds_timer;
#endif
#ifdef ESP32
Ticker blink_leds_timer;
#endif

// Run tests on every 5th wakeup
uint8_t loop_counter = 0;

// LED toggle control
#ifdef NRF52_SERIES
void toggle_led(TimerHandle_t unused)
{
	digitalToggle(LED_BLUE);
	digitalToggle(LED_GREEN);
}
#endif
#ifdef ESP32
void toggle_led(void)
{
	digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
	digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
}
#endif

/**
 * @brief Run basic HW tests, except for LoRa transceiver check and LoRa initialization
 *
 */
void run_tests(bool init_eth_now = false)
{
	MYLOG("APP", "=========================================");
#ifdef HIGH_FREQ
#ifdef _VARIANT_RAK3400_
#warning "1W version"
	MYLOG("APP", "Test for 1W transceiver 8xx/9xx Mhz");
#else
	MYLOG("APP", "Test for 8xx/9xx Mhz");
#endif
#else
	MYLOG("APP", "Test for 4xx Mhz");
#endif
	MYLOG("APP", "=========================================");
	MYLOG("APP", "Start tests");
	// restart_advertising(30);

	pinMode(LED_BLUE, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);

	digitalWrite(LED_BLUE, HIGH);
	digitalWrite(LED_GREEN, LOW);

	// Test OLED
	has_rak1921 = init_rak1921();
	if (has_rak1921)
	{
#ifdef HIGH_FREQ
	#ifdef _VARIANT_RAK3400_
			// #warning "1W version"
			rak1921_write_header((char *)"RAK3401 1W Test");
	#elif defined _VARIANT_RAK3112_
			rak1921_write_header((char *)"RAK3312 8xx/9xx");
	#else
			rak1921_write_header((char *)"RAK4631 8xx/9xx");
	#endif
#else
#ifdef _VARIANT_RAK3400_
		// #warning "1W version"
		rak1921_write_header((char *)"RAK3401 4xx 1W Test");
#else
		rak1921_write_header((char *)"RAK4631 4xx");
#endif
#endif // HIGH_FREQ
	}
	else
	{
		MYLOG("OLED", "No OLED found");
	}

	if (init_eth_now)
	{
		// Check Ethernet module
		has_rak13800 = init_eth();
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

	if (has_rak13800)
	{
		check_ip();
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
}

/**
 * @brief Initial setup of the application (before LoRaWAN and BLE setup)
 *
 */
void setup_app(void)
{
	// Set firmware version
	api_set_version(SW_VERSION_1, SW_VERSION_2, SW_VERSION_3);

	g_lora_p2p_rx_mode = RX_MODE_RX;

	// MYLOG("APP", "Setup application");
	g_enable_ble = true;

	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);
}

/**
 * @brief Final setup of application  (after LoRaWAN and BLE setup)
 *
 * @return true
 * @return false
 */
bool init_app(void)
{
	// Check if settings is for LoRa P2P
	if (g_lorawan_settings.lorawan_enable != false)
	{
		MYLOG("APP", "Detected LoRaWAN, switch to LoRa P2P");
		// Change LoRaWAN settings
		g_lorawan_settings.lorawan_enable = false;	 // Force LoRa P2P
		g_lorawan_settings.send_repeat_time = 10000; // Force 30 seconds send interval
		g_lorawan_settings.auto_join = true;		 // Disable automatic join ==> enable BLE advertising
#ifdef HIGH_FREQ
#warning "H version"
		g_lorawan_settings.p2p_frequency = 910000000;
#else
#warning "L version"
		g_lorawan_settings.p2p_frequency = 433100000;
#endif
		g_lorawan_settings.p2p_tx_power = 22;
		g_lorawan_settings.p2p_bandwidth = 0;
		g_lorawan_settings.p2p_sf = 7;
		g_lorawan_settings.p2p_cr = 1;
		g_lorawan_settings.p2p_preamble_len = 8;
		g_lorawan_settings.p2p_symbol_timeout = 0;

		// Save LoRaWAN settings
		api_set_credentials();

		// Reset device and restart in P2P mode
		api_reset();
	}

	g_lora_p2p_rx_mode = RX_MODE_RX;

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

	run_tests(true);

	// Start timer for LED blinking
#ifdef NRF52_SERIES
	blink_leds_timer.begin(250, toggle_led, NULL, true);
	blink_leds_timer.start();
#endif
#ifdef ESP32
	blink_leds_timer.attach_ms(250, toggle_led);
#endif

	// Erase flash file system
	flash_reset();

	// Change LoRaWAN settings
	g_lorawan_settings.lorawan_enable = false;	 // Force LoRa P2P
	g_lorawan_settings.send_repeat_time = 10000; // Force 30 seconds send interval
	g_lorawan_settings.auto_join = true;		 // Disable automatic join ==> enable BLE advertising
#ifdef HIGH_FREQ
#warning "H version"
	g_lorawan_settings.p2p_frequency = 910000000;
#else
#warning "L version"
	g_lorawan_settings.p2p_frequency = 433100000;
#endif
	g_lorawan_settings.p2p_tx_power = 22;
	g_lorawan_settings.p2p_bandwidth = 0;
	g_lorawan_settings.p2p_sf = 7;
	g_lorawan_settings.p2p_cr = 1;
	g_lorawan_settings.p2p_preamble_len = 8;
	g_lorawan_settings.p2p_symbol_timeout = 0;

	g_lora_p2p_rx_mode = RX_MODE_RX;

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
	g_lorawan_settings.send_repeat_time = 10000;

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
		sx1262_ok = true;
		if (has_rak1921)
		{
			sprintf(disp_txt, "LoRa transceiver ok");
			rak1921_add_line(disp_txt);
		}
	}
	else
	{
		MYLOG("SX1262", "SyncWord is incorrect, potential problem in SPI setup or LoRa transceiver");
		sx1262_ok = false;
		if (has_rak1921)
		{
			sprintf(disp_txt, "SX1262 problem (SPI or LoRa chip");
			rak1921_add_line(disp_txt);
		}
		// Write syncword
		Radio.SetCustomSyncWord(0x2414);
		// Readback syncword
		SX126xReadRegisters(REG_LR_SYNCWORD, (uint8_t *)&readSyncWord, 2);

		MYLOG("SX1262", "SyncWord after reset = %04X", readSyncWord);
	}

	// Init LoRa
	init_lora();

	g_lora_p2p_rx_mode = RX_MODE_RX;

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
#ifdef NRF52_SERIES
	blink_leds_timer.start();
#endif

	// Timer triggered event
	if ((g_task_event_type & STATUS) == STATUS)
	{
		g_task_event_type &= N_STATUS;

		digitalWrite(WB_IO2, HIGH);

		loop_counter++;
		if (loop_counter == 10)
		{
			loop_counter = 0;
			MYLOG("APP", "De-init LoRa");
		}

		if (loop_counter == 0)
		{
			if (sx1262_ok)
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

			run_tests();
		}

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

		digitalWrite(WB_IO2, HIGH);

		// Dummy packet
		uint8_t dummy_packet[] = {0x01, 0x74, 0x00, 0x55};
		uint16_t batt_level = (uint16_t)(batt_level_f);
		dummy_packet[2] = (uint8_t)(batt_level >> 8);
		dummy_packet[3] = (uint8_t)(batt_level);

		MYLOG("APP", "Send P2P packet");
		if (has_rak1921)
		{
			sprintf(disp_txt, "Send P2P packet");
			rak1921_add_line(disp_txt);
		}
		// Radio.Send(dummy_packet, 4);
		/// \todo Not sure why default function does not work
		if (send_p2p_packet(dummy_packet, 4))
		{
			lora_success = 0;
			MYLOG("APP", "Send P2P initiated");
		}
		else
		{
			lora_success = 1;
			MYLOG("APP", "Send P2P error");
		}
		g_lora_p2p_rx_mode = RX_MODE_RX;
	}

	if (has_rak14000)
	{
		refresh_rak14000();
		delay(3000);
	}
}

#ifdef NRF52_SERIES
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
#endif

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

		if (has_rak1921)
		{
			sprintf(disp_txt, "RX: RSSI %d", g_last_rssi);
			rak1921_add_line(disp_txt);
			sprintf(disp_txt, "%s", log_buff);
			rak1921_add_line(disp_txt);
		}
		Radio.Rx(0);
		g_lora_p2p_rx_mode = RX_MODE_RX;
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
			Radio.Rx(0);
		}
	}
}
