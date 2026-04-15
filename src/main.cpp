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

/** LoRa radio events */
static RadioEvents_t RadioEvents;
/** LoRa tx/rx buffer size*/
static uint16_t BufferSize = BUFFER_SIZE;
/** LoRa RX buffer */
static uint8_t RcvBuffer[BUFFER_SIZE];
/** LoRa TX buffer */
static uint8_t TxdBuffer[BUFFER_SIZE];

/** Set the device name, max length is 10 characters */
char g_ble_dev_name[10] = "RAK-TEST";

/** Flag if RAK1921 was found */
bool has_rak1921 = false;

/** Flag if RAK12500 was found */
bool has_rak12500 = false;
/** Flag if RAK12501 was found */
bool has_rak12501 = false;

/** Buffer for RAK1921 OLED text */
char disp_txt[256];

/** Flag if RAK14000 was found */
bool has_rak14000 = false;

/** Flag is LoRa init was successfull */
uint8_t lora_success = 2;

/** Flag is Flash read/write was successfull */
bool flash_success = false;

// Timers
#ifdef NRF52_SERIES
SoftwareTimer blink_leds_timer;
#endif
#ifdef ESP32
Ticker blink_leds_timer;
#endif

// For battery readings
float batt_level_f = 0.0;

// For Flash test
s_lorawan_settings g_lorawan_settings;

/** Bandwidths as char arrays */
char *bandwidths[] = {(char *)"125", (char *)"250", (char *)"500", (char *)"062", (char *)"041", (char *)"031", (char *)"020", (char *)"015", (char *)"010", (char *)"007"};

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
 * @brief Initial setup of the application (before LoRaWAN and BLE setup)
 *
 */
void setup(void)
{
	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, LOW);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, LOW);
	MYLOG("APP", "Initialize application");
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);

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

	// Initialize BLE
	init_ble();

	// Wait some time for a BLE connection
	while (!g_ble_uart_is_connected)
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

	// Initialize battery functions
	init_batt();
	Serial.println("=====================================");
#ifdef NRF52_SERIES
	Serial.println("WisMesh RAK4631/RAK3401 HW test");
#endif
#ifdef ESP32
	Serial.println("WisMesh RAK3312 HW test");
#endif
	Serial.println("=====================================");

#ifdef HIGH_FREQ
#ifdef _VARIANT_RAK3400_
#warning "1W version"
	Serial.println("Test for 1W transceiver 8xx/9xx Mhz");
#else
	Serial.println("Test for 8xx/9xx Mhz");
#endif
#else
	Serial.println("Test for 4xx Mhz");
#endif

	// Start timer for LED blinking
#ifdef NRF52_SERIES
	blink_leds_timer.begin(250, toggle_led, NULL, true);
	blink_leds_timer.start();
#endif
#ifdef ESP32
	blink_leds_timer.attach_ms(250, toggle_led);
#endif
	return;
}

uint8_t loop_counter = 0;

/**
 * @brief Loop, runs RX/TX tests, reads battery, tries to get location
 *
 */
void loop(void)
{
	if (loop_counter == 0)
	{
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

		MYLOG("SCAN", "Start I2C scan");

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

		// Init GNSS only if not done yet
		if (gnss_option == NO_GNSS_INIT)
		{ 
			// If it has RAK12500, setup the GNSS module with RAK specific settings
			if (has_rak12500)
			{
				has_rak12500 = init_gnss();
			}
			// If it has RAK12501, setup the GNSS module with RAK specific settings
			else if (gnss_option == NO_GNSS_INIT)
			{
				has_rak12501 = init_gnss();
			}
		}
		// Initialize flash file system
		init_flash();

		// Erase flash file system
		flash_reset();

		// Change LoRaWAN settings
		g_lorawan_settings.lorawan_enable = false;	 // Force LoRa P2P
		g_lorawan_settings.send_repeat_time = 10000; // Force 30 seconds send interval
		g_lorawan_settings.auto_join = false;		 // Disable automatic join ==> enable BLE advertising
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

		// Save LoRaWAN settings (in case they were still there on top of Meshtastic settings)
		save_settings();

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

		// Setup connection to LoRa transceiver
		uint32_t init_result = -1;

#ifdef NRF52_SERIES
#ifdef HIGH_FREQ
#ifdef _VARIANT_RAK3400_
		init_result = lora_rak3400_init();
#else
		init_result = lora_rak4630_init();
#endif
#else
		init_result = lora_rak4630_init();
#endif
#endif
#ifdef ESP32
		init_result = lora_rak3112_init();
#endif
		if (init_result != 0)
		{
			MYLOG("SX1262", "Initiation of LoRa Transceiver failed, potential problem in SPI setup or LoRa transceiver");
		}

		// Initialize the Radio callbacks
		RadioEvents.TxDone = OnTxDone;
		RadioEvents.RxDone = OnRxDone;
		RadioEvents.TxTimeout = OnTxTimeout;
		RadioEvents.RxTimeout = OnRxTimeout;
		RadioEvents.RxError = OnRxError;
		RadioEvents.CadDone = OnCadDone;

		// Initialize the Radio
		Radio.Init(&RadioEvents);

		// Set Radio channel
		Radio.SetChannel(RF_FREQUENCY);

		// Set Radio TX configuration
		Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
						  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
						  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD,
						  true, 0, 0, LORA_IQ_INVERSION, TX_TIMEOUT_VALUE);

		// Set Radio RX configuration
		Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
						  LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
						  LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD,
						  0, true, 0, 0, LORA_IQ_INVERSION, true);

		// Start LoRa
		Serial.println("Starting Radio.Rx");
		Radio.Rx(0);

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
			// Write syncword
			Radio.SetCustomSyncWord(0x2414);
			// Readback syncword
			SX126xReadRegisters(REG_LR_SYNCWORD, (uint8_t *)&readSyncWord, 2);

			MYLOG("SX1262", "SyncWord after reset = %04X", readSyncWord);
		}

		// Start listening
		Radio.Rx(0);

		// Read battery values
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

		// restart_advertising(60);

		Serial.flush();
		delay(500);
	}

	delay(10000);
	digitalWrite(WB_IO2, HIGH);

	// restart_advertising(60);
	MYLOG("APP", "Timer wakeup");
	if (has_rak1921)
	{
		sprintf(disp_txt, "Timer wakeup");
		rak1921_add_line(disp_txt);
	}

	// Check GNSS location
	if (has_rak12500 || has_rak12501)
	{
		MYLOG("APP", "Try GNSS");
		if (has_rak1921)
		{
			sprintf(disp_txt, "Try GNSS");
			rak1921_add_line(disp_txt);
		}
		poll_gnss();
	}

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
	uint16_t batt_level = (uint16_t)(batt_level_f);
	TxdBuffer[0] = 0x01;
	TxdBuffer[0] = 0x74;
	TxdBuffer[2] = (uint8_t)(batt_level >> 8);
	TxdBuffer[3] = (uint8_t)(batt_level);

	MYLOG("APP", "Send P2P packet");
	MYLOG("APP", "F:%ld MHz TXP: %d dbm BW: %s", g_lorawan_settings.p2p_frequency / 1000000, g_lorawan_settings.p2p_tx_power, bandwidths[g_lorawan_settings.p2p_bandwidth]);
	MYLOG("APP", "SF:%d CR: %d/5 PPL: %d", g_lorawan_settings.p2p_sf, g_lorawan_settings.p2p_cr + 3, g_lorawan_settings.p2p_preamble_len);

	if (has_rak1921)
	{
		sprintf(disp_txt, "Send P2P packet");
		rak1921_add_line(disp_txt);
	}
	Radio.Send(TxdBuffer, 4);

	if (has_rak14000)
	{
		refresh_rak14000();
		delay(3000);
	}

	loop_counter++;
	if (loop_counter == 5)
	{
		loop_counter = 0;
		lora_hardware_uninit();
	}
}

/**@brief Function to be executed on Radio Tx Done event
 */
void OnTxDone(void)
{
	Serial.println("OnTxDone");

	// Back to RX
	Radio.Rx(0);
}

/**@brief Function to be executed on Radio Rx Done event
 */
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
	Serial.println("OnRxDone");

	delay(10);
	BufferSize = size;
	memcpy(RcvBuffer, payload, BufferSize);

	Serial.printf("RssiValue=%d dBm, SnrValue=%d\n", rssi, snr);

	for (int idx = 0; idx < size; idx++)
	{
		Serial.printf("%02X ", RcvBuffer[idx]);
	}
	Serial.println("");

	digitalWrite(LED_GREEN, LOW);

	// Back to RX
	Radio.Rx(0);
}

/**@brief Function to be executed on Radio Tx Timeout event
 */
void OnTxTimeout(void)
{
	// Radio.Sleep();
	Serial.println("OnTxTimeout");

	digitalWrite(LED_GREEN, LOW);

	// Back to RX
	Radio.Rx(0);
}

/**@brief Function to be executed on Radio Rx Timeout event
 */
void OnRxTimeout(void)
{
	Serial.println("OnRxTimeout");

	digitalWrite(LED_GREEN, LOW);

	// Back to RX
	Radio.Rx(0);
}

/**@brief Function to be executed on Radio Rx Error event
 */
void OnRxError(void)
{
	Serial.println("OnRxError");

	digitalWrite(LED_GREEN, LOW);

	// Back to RX
	Radio.Rx(0);
}

/**@brief Function to be executed on CAD Done event
 */
void OnCadDone(bool cadResult)
{
	if (cadResult)
	{
		Serial.printf("CAD returned channel busy\n");

		// Back to RX
		Radio.Rx(0);
	}
	else
	{
		Serial.printf("CAD returned channel free\n");

		uint16_t batt_level = (uint16_t)(batt_level_f);
		TxdBuffer[0] = 0x01;
		TxdBuffer[0] = 0x74;
		TxdBuffer[2] = (uint8_t)(batt_level >> 8);
		TxdBuffer[3] = (uint8_t)(batt_level);

		Radio.Send(TxdBuffer, 4);
	}
}
