/**
 * @file gnss.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief GNSS functions and task
 * @version 0.3
 * @date 2022-01-29
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "main.h"

/** Instance for RAK1910 GNSS sensor */
TinyGPSPlus my_rak12501_gnss;
/** Instance for RAK12500 GNSS sensor */
SFE_UBLOX_GNSS my_gnss;

/** GNSS polling function */
bool poll_gnss(void);

/** Flag if location was found */
volatile bool last_read_ok = false;

/** Flag if GNSS is serial or I2C */
bool i2c_gnss = false;

/** The GPS module to use */
uint8_t gnss_option = 0;

int64_t latitude = 0;
int64_t longitude = 0;
int32_t altitude = 0;
int32_t accuracy = 0;

byte fix_type = 0; // Get the fix type
char fix_type_str[32] = {0};

/**
 * @brief Initialize GNSS module
 *
 * @return true if GNSS module was found
 * @return false if no GNSS module was found
 */
bool init_gnss(void)
{
	bool gnss_found = false;

	// Power on the GNSS module
	digitalWrite(WB_IO2, HIGH);

	// Give the module some time to power up
	delay(500);

	if (gnss_option == NO_GNSS_INIT)
	{
		gnss_option = NO_GNSS_FOUND;
		Serial1.begin(9600);
		if (!my_gnss.begin())
		{
			time_t timeout = millis();
			while ((millis() - timeout) < 20000)
			{
				char gnss = Serial1.read();
				// Serial.printf("%02x\n",gnss);
				if ((gnss >= 0x20) && (gnss <= 0x7F))
				{
					gnss_option = RAK12501_GNSS;
					MYLOG("GNSS", "Got data from RAK12501 after %ld", (uint32_t)(millis() - timeout));
					return true;
				}
				delay(500);
			}
			MYLOG("GNSS", "Got no data from RAK1910 on Serial1 in %ld", (uint32_t)(millis() - timeout));
		}
		else
		{
			MYLOG("GNSS", "UBLOX found on I2C");
			i2c_gnss = true;
			gnss_found = true;
			my_gnss.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)
			gnss_option = RAK12500_GNSS;
		}

		if (gnss_found && (gnss_option == RAK12500_GNSS))
		{
			my_gnss.begin();
			my_gnss.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)

			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GPS);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GALILEO);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GLONASS);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_SBAS);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_BEIDOU);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_IMES);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_QZSS);
			// my_gnss.setMeasurementRate(500);
			my_gnss.setNavigationFrequency(10); // Produce two solutions per second
			my_gnss.setAutoPVT(true, false);	// Tell the GNSS to "send" each solution and the lib not to update stale data implicitly

			my_gnss.saveConfiguration(); // Save the current settings to flash and BBR

			return true;
		}
	}
	else // Already initialized and GNSS found
	{
		if (gnss_option == RAK12500_GNSS)
		{
			my_gnss.begin();
			my_gnss.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)

			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GPS);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GALILEO);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_GLONASS);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_SBAS);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_BEIDOU);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_IMES);
			my_gnss.enableGNSS(true, SFE_UBLOX_GNSS_ID_QZSS);
			// my_gnss.setMeasurementRate(500);
			my_gnss.setNavigationFrequency(10); // Produce two solutions per second
			my_gnss.setAutoPVT(true, false);	// Tell the GNSS to "send" each solution and the lib not to update stale data implicitly

			my_gnss.saveConfiguration(); // Save the current settings to flash and BBR
		}
		else if (gnss_option == RAK12501_GNSS)
		{
			my_gnss.begin(Serial1);
			MYLOG("GNSS", "UBLOX found on Serial1 with 9600");
			my_gnss.setUART1Output(COM_TYPE_UBX); // Set the UART port to output UBX only
		}
		else
		{
			return false;
		}
		return true;
	}
	return false;
}

/**
 * @brief Check GNSS module for position
 *
 * @return true Valid position found
 * @return false No valid position
 */
bool poll_gnss(void)
{
	char oled_buff[128];

	MYLOG("GNSS", "poll_gnss");

	last_read_ok = false;

	time_t time_out = millis();

	time_t check_limit = 30000;

	MYLOG("GNSS", "GNSS timeout %ld", (long int)check_limit);

	MYLOG("GNSS", "Using %s", gnss_option == RAK12500_GNSS ? "RAK12500" : "RAK12501");

	uint8_t sat_num = 0;

	bool has_pos = false;
	bool has_alt = false;

	sprintf(fix_type_str, "None");
	// RAK12500
	if (gnss_option == RAK12500_GNSS)
	{
		while ((millis() - time_out) < check_limit)
		{
			latitude = my_gnss.getLatitude();
			longitude = my_gnss.getLongitude();
			altitude = my_gnss.getAltitude();
			accuracy = my_gnss.getHorizontalDOP();
			sat_num = my_gnss.getSIV();
			fix_type = my_gnss.getFixType(); // Get the fix type
			if (fix_type == 1)
				sprintf(fix_type_str, "Dead reckoning");
			else if (fix_type == 2)
				sprintf(fix_type_str, "Fix type 2D");
			else if (fix_type == 3)
				sprintf(fix_type_str, "Fix type 3D");
			else if (fix_type == 4)
				sprintf(fix_type_str, "GNSS fix");
			else if (fix_type == 5)
				sprintf(fix_type_str, "Time fix");
			else
			{
				sprintf(fix_type_str, "No Fix");
				fix_type = 0;
			}

			MYLOG("GNSS", "Sat: %d Fix: %s", sat_num, fix_type_str);
			MYLOG("GNSS", "Lat: %.4f Lon: %.4f", latitude / 10000000.0, longitude / 10000000.0);
			MYLOG("GNSS", "Alt: %.2f", altitude / 1000.0);
			MYLOG("GNSS", "HDOP: %.2f ", accuracy / 100.0);

			if ((accuracy < 300) && (sat_num > 5))
			{
				last_read_ok = true;
				// Break the while()
				break;
			}
			// if (my_gnss.getGnssFixOk())
			// {
			// 	fix_type = my_gnss.getFixType(); // Get the fix type
			// 	if (fix_type == 1)
			// 		sprintf(fix_type_str, "Dead reckoning");
			// 	else if (fix_type == 2)
			// 		sprintf(fix_type_str, "Fix type 2D");
			// 	else if (fix_type == 3)
			// 		sprintf(fix_type_str, "Fix type 3D");
			// 	else if (fix_type == 4)
			// 		sprintf(fix_type_str, "GNSS fix");
			// 	else if (fix_type == 5)
			// 		sprintf(fix_type_str, "Time fix");
			// 	else
			// 	{
			// 		sprintf(fix_type_str, "No Fix");
			// 		fix_type = 0;
			// 	}
			// 	bool fix_sufficient = false;
			// 	sat_num = my_gnss.getSIV();
			// 	accuracy = my_gnss.getHorizontalDOP();

			// 	MYLOG("GNSS", "L Fixtype: %d %s", fix_type, fix_type_str);
			// 	MYLOG("GNSS", "L Sat: %d ", sat_num);
			// 	if (fix_type >= 3) /** Fix type 3D */
			// 	{
			// 		fix_sufficient = true;
			// 	}

			// 	if (fix_sufficient) /** Fix type 3D */
			// 	{
			// 		last_read_ok = true;
			// 		latitude = my_gnss.getLatitude();
			// 		longitude = my_gnss.getLongitude();
			// 		altitude = my_gnss.getAltitude();
			// 		accuracy = my_gnss.getHorizontalDOP();

			// 		MYLOG("GNSS", "Fixtype: %d %s", my_gnss.getFixType(), fix_type_str);
			// 		MYLOG("GNSS", "Lat: %.4f Lon: %.4f", latitude / 10000000.0, longitude / 10000000.0);
			// 		MYLOG("GNSS", "Alt: %.2f", altitude / 1000.0);
			// 		MYLOG("GNSS", "HDOP: %.2f ", accuracy / 100.0);

			// 		// Break the while()
			// 		break;
			// 	}
			// }
			else
			{
				delay(1000);
			}
		}
	}
	else
	{
		uint32_t start_time = millis();
		while ((millis() - start_time) < 20000)
		{
			if (Serial1.available() > 0)
			{
				// char gnss = Serial1.read();
				// Serial.print(gnss);
				// if (my_rak12501_gnss.encode(gnss))
				if (my_rak12501_gnss.encode(Serial1.read()))
				{
					if (my_rak12501_gnss.location.isUpdated() && my_rak12501_gnss.location.isValid())
					{
						MYLOG("GNSS", "Location valid");
						has_pos = true;
						latitude = (my_rak12501_gnss.location.lat() * 10000000.0);
						longitude = (my_rak12501_gnss.location.lng() * 10000000.0);
					}
					else if (my_rak12501_gnss.altitude.isUpdated() && my_rak12501_gnss.altitude.isValid())
					{
						MYLOG("GNSS", "Altitude valid");
						has_alt = true;
						altitude = (my_rak12501_gnss.altitude.meters() * 1000);
					}
					else if (my_rak12501_gnss.hdop.isUpdated() && my_rak12501_gnss.hdop.isValid())
					{
						accuracy = my_rak12501_gnss.hdop.hdop() * 100;
					}
				}
				if (has_pos && has_alt)
				{
					MYLOG("GNSS", "Lat: %.4f Lon: %.4f", latitude / 10000000.0, longitude / 10000000.0);
					MYLOG("GNSS", "Alt: %.2f", altitude / 1000.0);
					MYLOG("GNSS", "Acy: %.2f ", accuracy / 100.0);
					last_read_ok = true;
					break;
				}
				delay(10);
			}
		}
		if (has_pos && has_alt)
		{
			last_read_ok = true;
		}
	}
	if (last_read_ok)
	{
		if ((latitude == 0) && (longitude == 0))
		{
			last_read_ok = false;
			return false;
		}

		if (has_rak1921)
		{
			snprintf(oled_buff, 127, "Fix: %s Sat: %d", fix_type_str, sat_num);
			rak1921_add_line(oled_buff);
			snprintf(oled_buff, 127, "L: %.6f:%.6f", latitude / 10000000.0, longitude / 10000000.0);
			rak1921_add_line(oled_buff);
			snprintf(oled_buff, 127, "Alt: %.2f, Acry %.2f", altitude / 1000.0, accuracy / 100.0);
			rak1921_add_line(oled_buff);
		}
		return true;
	}
	else
	{
		if (has_rak1921)
		{
			rak1921_add_line((char *)"No location fix");
			snprintf(oled_buff, 127, "Fix: %s Sat: %d", fix_type_str, sat_num);
			rak1921_add_line(oled_buff);
		}
		// No location found
	}

	MYLOG("GNSS", "No valid location found");
	last_read_ok = false;

	return false;
}
