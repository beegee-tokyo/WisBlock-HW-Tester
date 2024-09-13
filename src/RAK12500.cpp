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

// The GNSS object
SFE_UBLOX_GNSS my_gnss; // RAK12500_GNSS

/** GNSS polling function */
bool poll_gnss(void);

/** Flag if location was found */
volatile bool last_read_ok = false;

/** Flag if GNSS is serial or I2C */
bool i2c_gnss = false;

/** The GPS module to use */
uint8_t gnss_option = 0;

/** Switcher between different fake locations */
uint8_t fake_gnss_selector = 0;

int64_t fake_latitude[] = {144213730, 414861950, -80533010, -274789700};
int64_t fake_longitude[] = {1210069140, -816814860, -349049060, 1530410440};

// PH 144213730, 1210069140, 35.000 // Ohio 414861950, -816814860 // Recife -80533010, -349049060 // Brisbane -274789700, 1530410440

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
		if (!my_gnss.begin())
		{
			MYLOG("GNSS", "UBLOX did not answer on I2C, retry on Serial1");
			return false;
		}
		else
		{
			MYLOG("GNSS", "UBLOX found on I2C");
			gnss_found = true;
			my_gnss.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)
			gnss_option = RAK12500_GNSS;
		}

		if (gnss_found)
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
	else
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
		return true;
	}
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

	time_t check_limit = 15000;

	MYLOG("GNSS", "GNSS timeout %ld", (long int)check_limit);

	MYLOG("GNSS", "Using %s", gnss_option == RAK12500_GNSS ? "RAK12500" : "RAK1910");

	uint8_t sat_num = 0;
	sprintf(fix_type_str, "None");
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
#if FAKE_GPS > 0
		/// \todo Enable below to get a fake GPS position if no location fix could be obtained
		// PH 144213730, 1210069140, 35.000 // Ohio 414861950, -816814860 // Recife -80533010, -349049060 // Brisbane -274789700, 1530410440
		latitude = fake_latitude[fake_gnss_selector];
		longitude = fake_longitude[fake_gnss_selector];
		fake_gnss_selector++;
		if (fake_gnss_selector == 4)
		{
			fake_gnss_selector = 0;
		}
		altitude = 35000;
		accuracy = 100;

		if (!g_is_helium)
		{
			if (g_gps_prec_6)
			{
				// Save extended precision, not Cayenne LPP compatible
				g_data_packet.addGNSS_6(LPP_CHANNEL_GPS, latitude, longitude, altitude);
			}
			else
			{
				// Save default Cayenne LPP precision
				g_data_packet.addGNSS_4(LPP_CHANNEL_GPS, latitude, longitude, altitude);
			}
		}
		else
		{
			// Save default Cayenne LPP precision
			g_data_packet.addGNSS_H(latitude, longitude, altitude, accuracy, read_batt());
		}
		last_read_ok = true;
		return true;
#endif
	}

	MYLOG("GNSS", "No valid location found");
	last_read_ok = false;

	return false;
}
