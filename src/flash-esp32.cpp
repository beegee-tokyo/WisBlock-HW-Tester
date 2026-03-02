/**
 * @file flash-esp32.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Initialize, read and write parameters from/to internal flash memory
 * @version 0.1
 * @date 2021-01-10
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifdef ESP32

#include "main.h"
#include <Preferences.h>

/** ESP32 preferences */
Preferences lora_prefs;

/**
 * @brief Initialize access to ESP32 preferences
 *
 */
void init_flash(void)
{
	lora_prefs.begin("LoRaCred", false);

	bool hasPref = lora_prefs.getBool("valid", false);
	if (hasPref)
	{
		MYLOG("FLASH", "Found preferences");
		g_lorawan_settings.auto_join = lora_prefs.getBool("a_j", false);
		g_lorawan_settings.otaa_enabled = lora_prefs.getBool("o_e", true);
		lora_prefs.getBytes("d_e", g_lorawan_settings.node_device_eui, 8);
		lora_prefs.getBytes("a_e", g_lorawan_settings.node_app_eui, 8);
		lora_prefs.getBytes("a_k", g_lorawan_settings.node_app_key, 16);
		lora_prefs.getBytes("n_k", g_lorawan_settings.node_nws_key, 16);
		lora_prefs.getBytes("s_k", g_lorawan_settings.node_apps_key, 16);
		g_lorawan_settings.node_dev_addr = lora_prefs.getLong("d_a", 0x26021FB4);
		g_lorawan_settings.send_repeat_time = lora_prefs.getLong("s_r", 120000);
		g_lorawan_settings.adr_enabled = lora_prefs.getBool("a_d", false);
		g_lorawan_settings.public_network = lora_prefs.getBool("p_n", true);
		g_lorawan_settings.duty_cycle_enabled = lora_prefs.getBool("d_c", false);
		g_lorawan_settings.join_trials = lora_prefs.getShort("j_t", 5);
		g_lorawan_settings.tx_power = lora_prefs.getShort("t_p", 0);
		g_lorawan_settings.data_rate = lora_prefs.getShort("d_r", 5);
		g_lorawan_settings.lora_class = lora_prefs.getShort("l_c", 0);
		g_lorawan_settings.subband_channels = lora_prefs.getShort("s_c", 1);
		g_lorawan_settings.app_port = lora_prefs.getShort("a_p", 2);
		g_lorawan_settings.confirmed_msg_enabled = (lmh_confirm)lora_prefs.getShort("c_m", LMH_UNCONFIRMED_MSG);
		g_lorawan_settings.resetRequest = lora_prefs.getBool("r_r", true);
		g_lorawan_settings.lora_region = lora_prefs.getShort("l_r", 10);

		g_lorawan_settings.lorawan_enable = lora_prefs.getBool("l_e", true);
		g_lorawan_settings.p2p_frequency = lora_prefs.getLong("p_f", 916000000);
		g_lorawan_settings.p2p_tx_power = lora_prefs.getShort("p_t", 22);
		g_lorawan_settings.p2p_bandwidth = lora_prefs.getShort("p_b", 0);
		g_lorawan_settings.p2p_sf = lora_prefs.getShort("p_s", 7);
		g_lorawan_settings.p2p_cr = lora_prefs.getShort("p_c", 1);
		g_lorawan_settings.p2p_preamble_len = lora_prefs.getShort("p_p", 8);
		g_lorawan_settings.p2p_symbol_timeout = lora_prefs.getShort("p_x", 0);

		lora_prefs.end();
	}
	else
	{
		lora_prefs.end();
		MYLOG("FLASH", "No valid preferences");
		save_settings();
	}
}

/**
 * @brief Save changed settings if required
 *
 * @return boolean
 * 			result of saving (always true)
 */
boolean save_settings(void)
{
	lora_prefs.begin("LoRaCred", false);

	lora_prefs.putBool("a_j", g_lorawan_settings.auto_join);
	lora_prefs.putBool("o_e", g_lorawan_settings.otaa_enabled);
	lora_prefs.putBytes("d_e", g_lorawan_settings.node_device_eui, 8);
	lora_prefs.putBytes("a_e", g_lorawan_settings.node_app_eui, 8);
	lora_prefs.putBytes("a_k", g_lorawan_settings.node_app_key, 16);
	lora_prefs.putBytes("n_k", g_lorawan_settings.node_nws_key, 16);
	lora_prefs.putBytes("s_k", g_lorawan_settings.node_apps_key, 16);
	lora_prefs.putLong("d_a", g_lorawan_settings.node_dev_addr);
	lora_prefs.putLong("s_r", g_lorawan_settings.send_repeat_time);
	lora_prefs.putBool("a_d", g_lorawan_settings.adr_enabled);
	lora_prefs.putBool("p_n", g_lorawan_settings.public_network);
	lora_prefs.putBool("d_c", g_lorawan_settings.duty_cycle_enabled);
	lora_prefs.putShort("j_t", g_lorawan_settings.join_trials);
	lora_prefs.putShort("t_p", g_lorawan_settings.tx_power);
	lora_prefs.putShort("d_r", g_lorawan_settings.data_rate);
	lora_prefs.putShort("l_c", g_lorawan_settings.lora_class);
	lora_prefs.putShort("s_c", g_lorawan_settings.subband_channels);
	lora_prefs.putShort("a_p", g_lorawan_settings.app_port);
	lora_prefs.putShort("c_m", g_lorawan_settings.confirmed_msg_enabled);
	lora_prefs.putBool("r_r", g_lorawan_settings.resetRequest);
	lora_prefs.putShort("l_r", g_lorawan_settings.lora_region);

	lora_prefs.putBool("l_e", g_lorawan_settings.lorawan_enable);
	lora_prefs.putLong("p_f", g_lorawan_settings.p2p_frequency);
	lora_prefs.putShort("p_t", g_lorawan_settings.p2p_tx_power);
	lora_prefs.putShort("p_b", g_lorawan_settings.p2p_bandwidth);
	lora_prefs.putShort("p_s", g_lorawan_settings.p2p_sf);
	lora_prefs.putShort("p_c", g_lorawan_settings.p2p_cr);
	lora_prefs.putShort("p_p", g_lorawan_settings.p2p_preamble_len);
	lora_prefs.putShort("p_x", g_lorawan_settings.p2p_symbol_timeout);

	lora_prefs.putBool("valid", true);
	lora_prefs.end();

	return true;
}

/**
 * @brief Reset content of the filesystem
 *
 */
void flash_reset(void)
{
	MYLOG("FLASH", "flash_reset remove file");
	s_lorawan_settings default_settings;
	memcpy((void *)&g_lorawan_settings, (void *)&default_settings, sizeof(s_lorawan_settings));
	save_settings();
}

#endif // ESP32