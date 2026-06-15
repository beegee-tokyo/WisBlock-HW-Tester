/**
 * @file ethernet-test.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialize Ethernet board and obtain IP address
 * @version 0.1
 * @date 2026-06-15
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "main.h"
#ifdef SUPPORTS_RAK13800

#include <SPI.h>
#include <RAK13800_W5100S.h> // Click to install library: http://librarymanager/All#RAKwireless_W5100S

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // Set the MAC address, do not repeat in a network.

bool init_eth(void)
{
	pinMode(WB_IO3, OUTPUT);
	digitalWrite(WB_IO3, LOW); // Reset Time.
	delay(100);
	digitalWrite(WB_IO3, HIGH); // Reset Time.

	Ethernet.init(SS);

	MYLOG("ETH", "Initialize Ethernet with DHCP."); // start the Ethernet connection.
	if (Ethernet.begin(mac, 10000) == 0)
	{
		MYLOG("ETH", "Failed to configure Ethernet using DHCP");
		if (Ethernet.hardwareStatus() == EthernetNoHardware) // Check for Ethernet hardware present.
		{
			MYLOG("ETH", "Ethernet shield was not found.");
			return false;
		}
	}
	else
	{
		MYLOG("ETH", "Module init success");
	}

	if (Ethernet.linkStatus() == LinkOFF)
	{
		MYLOG("ETH", "Ethernet module found, but cable not connected.");
		return true;
	}

	IPAddress eth_addr = Ethernet.localIP();
	MYLOG("ETH", "IP address obtained %02d:%02d:%02d:%02d", eth_addr[0], eth_addr[1], eth_addr[2], eth_addr[3]);
	return true;
}

void check_ip(void)
{
	if (Ethernet.linkStatus() == LinkOFF)
	{
		MYLOG("ETH", "Ethernet module found, but cable not connected.");
		return;
	}
	IPAddress eth_addr = Ethernet.localIP();
	MYLOG("ETH", "IP address obtained %02d:%02d:%02d:%02d", eth_addr[0], eth_addr[1], eth_addr[2], eth_addr[3]);
	// if (has_rak1921)
	// {
	// 	sprintf(disp_txt, "IP: %02d:%02d:%02d:%02d", eth_addr[0], eth_addr[1], eth_addr[2], eth_addr[3]);
	// 	rak1921_add_line(disp_txt);
	// }
}

#else // SUPPORTS_RAK13800
bool init_eth(void)
{
	return false;
}

void check_ip(void)
{}

#endif // SUPPORTS_RAK13800