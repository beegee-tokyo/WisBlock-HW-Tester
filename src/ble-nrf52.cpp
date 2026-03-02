/**
 * @file ble.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief BLE initialization & device configuration
 * @version 0.1
 * @date 2021-01-10
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifdef NRF52_SERIES

#include "main.h"
#include <bluefruit.h>

/** BLE UART service */
BLEUart g_ble_uart;
/** Device information service */
BLEDis ble_dis;
/** OTA DFU service */
BLEDfu ble_dfu;

// Connect callback
void connect_callback(uint16_t conn_handle);
// Disconnect callback
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
// Uart RX callback
void bleuart_rx_callback(uint16_t conn_handle);
// Restart advertising
void restart_advertising(uint16_t timeout);

/** Flag if BLE UART is connected */
bool g_ble_uart_is_connected = false;

/**
 * @brief Initialize BLE and start advertising
 *
 */
void init_ble(void)
{
	// Config the peripheral connection with maximum bandwidth
	// more SRAM required by SoftDevice
	// Note: All config***() function must be called before begin()
	Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
	Bluefruit.configPrphConn(250, BLE_GAP_EVENT_LENGTH_MIN, 16, 16);

	// Start BLE
	Bluefruit.begin(1, 0);

	// Set max power. Accepted values are: (min) -40, -20, -16, -12, -8, -4, 0, 2, 3, 4, 5, 6, 7, 8 (max)
	Bluefruit.setTxPower(0);

#if NO_BLE_LED > 0
	Bluefruit.autoConnLed(false);
	digitalWrite(LED_BLUE, LOW);
#endif

	// Create device name
	char helper_string[256] = {0};

	/** Device name for RAK4631 */
	sprintf(helper_string, "%s-%02X%02X%02X%02X%02X%02X", g_ble_dev_name,
			(uint8_t)(g_lorawan_settings.node_device_eui[2]), (uint8_t)(g_lorawan_settings.node_device_eui[3]),
			(uint8_t)(g_lorawan_settings.node_device_eui[4]), (uint8_t)(g_lorawan_settings.node_device_eui[5]), (uint8_t)(g_lorawan_settings.node_device_eui[6]), (uint8_t)(g_lorawan_settings.node_device_eui[7]));

	Bluefruit.setName(helper_string);

	// Set connection/disconnect callbacks
	Bluefruit.Periph.setConnectCallback(connect_callback);
	Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

	// Configure and Start Device Information Service
	ble_dis.setManufacturer("RAKwireless");

	ble_dis.setModel("RAK4631");

	sprintf(helper_string, "%d.%d.%d", SW_VERSION_1, SW_VERSION_2, SW_VERSION_3);
	ble_dis.setSoftwareRev(helper_string);

	ble_dis.setHardwareRev("52840");

	ble_dis.begin();

	// Start the DFU service
	ble_dfu.begin();

	// Start the UART service
	g_ble_uart.begin();
	g_ble_uart.setRxCallback(bleuart_rx_callback);

	// Advertising packet
	Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE); //
	Bluefruit.Advertising.addName();
	Bluefruit.Advertising.addTxPower();

	/* Start Advertising
	 * - Enable auto advertising if disconnected
	 * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
	 * - Timeout for fast mode is 30 seconds
	 * - Start(timeout) with timeout = 0 will advertise forever (until connected)
	 *
	 * For recommended advertising interval
	 * https://developer.apple.com/library/content/qa/qa1931/_index.html
	 */
	Bluefruit.Advertising.restartOnDisconnect(true);
	Bluefruit.Advertising.setInterval(32, 244); // in unit of 0.625 ms
	Bluefruit.Advertising.setFastTimeout(15);	// number of seconds in fast mode

	// On custom boards advertising has to be started automatically
	// Bluefruit.Advertising.start(60);			// 0 = Don't stop advertising
	if (g_lorawan_settings.auto_join)
	{
		restart_advertising(60);
	}
	else
	{
		restart_advertising(0);
	}
}

/**
 * @brief Restart advertising for a certain time
 *
 * @param timeout timeout in seconds
 */
void restart_advertising(uint16_t timeout)
{
	Bluefruit.Advertising.start(timeout);
}

/**
 * @brief  Callback when client connects
 * @param  conn_handle: Connection handle id
 */
void connect_callback(uint16_t conn_handle)
{
	(void)conn_handle;
	g_ble_uart_is_connected = true;
	Bluefruit.setTxPower(8);
}

/**
 * @brief  Callback invoked when a connection is dropped
 * @param  conn_handle: connection handle id
 * @param  reason: disconnect reason
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
	(void)conn_handle;
	(void)reason;
	g_ble_uart_is_connected = false;
	Bluefruit.setTxPower(0);
}

/**
 * Callback if data has been sent from the connected client
 * @param conn_handle
 * 		The connection handle
 */
void bleuart_rx_callback(uint16_t conn_handle)
{
	(void)conn_handle;

	while (g_ble_uart.available() > 0)
	{
		Serial.print(uint8_t(g_ble_uart.read()));
		delay(5);
	}
	Serial.println("");
}
#endif