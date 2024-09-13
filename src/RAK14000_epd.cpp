/**
 * @file RAK14000_epd.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialization and functions for EPD display
 * @version 0.1
 * @date 2022-06-25
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "main.h"

#include <rak14000.h> //Click here to get the library: http://librarymanager/All#RAK14000

#include "RAK14000_epd.h"

#define LEFT_BUTTON WB_IO6
#define MIDDLE_BUTTON WB_IO5
#define RIGHT_BUTTON WB_IO3

char disp_text[60];

unsigned char image[4000];
EPD_213_BW epd;
Paint paint(image, 122, 250);

uint16_t bg_color = UNCOLORED;
uint16_t txt_color = COLORED;

void rak14000_text(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size);

bool init_rak14000(void)
{
	digitalWrite(POWER_ENABLE, HIGH);

	// set left button 
	pinMode(LEFT_BUTTON, INPUT_PULLDOWN);
	MYLOG("EPD","Left %s", digitalRead(LEFT_BUTTON) == HIGH ? "HIGH" : "LOW");

	// set middle button 
	pinMode(MIDDLE_BUTTON, INPUT_PULLDOWN);
	MYLOG("EPD","Middle %s", digitalRead(MIDDLE_BUTTON) == HIGH ? "HIGH" : "LOW");

	// set right button 
	pinMode(RIGHT_BUTTON, INPUT_PULLDOWN);
	MYLOG("EPD","Right %s", digitalRead(RIGHT_BUTTON) == HIGH ? "HIGH" : "LOW");

	// Use button GPIO to test if RAK14000 is present
	if ((digitalRead(LEFT_BUTTON) == LOW) || (digitalRead(MIDDLE_BUTTON) == LOW) || (digitalRead(RIGHT_BUTTON) == LOW))
	{
		// All three buttons should be in high state when not pressed
		// return false;
		MYLOG("EPD", "Button status suggests no EPD");
		return false;
	}

	clear_rak14000();
	// paint.drawBitmap(5, 5, (uint8_t *)rak_img, 59, 56);
	rak14000_logo(5, 5);
	rak14000_text(60, 65, (char *)"RAKWireless", (uint16_t)txt_color, 2);
	rak14000_text(60, 85, (char *)"IoT Made Easy", (uint16_t)txt_color, 2);

	epd.Display(image);
	delay(500);
	return true;
}

void rak14000_logo(int16_t x, int16_t y)
{
	paint.drawBitmap(x, y, (uint8_t *)rak_img, 59, 56);
}

/**
   @brief Write a text on the display

   @param x x position to start
   @param y y position to start
   @param text text to write
   @param text_color color of text
   @param text_size size of text
*/
void rak14000_text(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size)
{
	sFONT *use_font;
	if (text_size == 1)
	{
		use_font = &Font12;
	}
	else
	{
		use_font = &Font20;
	}
	paint.DrawStringAt(x, y, text, use_font, COLORED);
}

void clear_rak14000(void)
{
	paint.SetRotate(ROTATE_270);
	epd.Init(FULL);
	// epd.Clear();
	paint.Clear(UNCOLORED);
}

void refresh_rak14000(void)
{
	// Clear display buffer
	clear_rak14000();

	paint.Clear(UNCOLORED);
	rak14000_text(0, 4, (char *)"RAK Test Firmware", txt_color, 2);

	rak14000_logo(0, 31);

	snprintf(disp_text, 59, "Flash R/W %s", flash_success ? "OK" : "NOK");
	rak14000_text(70, 30, disp_text, (uint16_t)txt_color, 2);

	switch (lora_success)
	{
	case 0:
		snprintf(disp_text, 59, "LoRa OK");
		rak14000_text(70, 50, disp_text, (uint16_t)txt_color, 2);
		break;
	case 1:
		snprintf(disp_text, 59, "LoRa failed");
		rak14000_text(70, 50, disp_text, (uint16_t)txt_color, 2);
		break;
	case 2:
		snprintf(disp_text, 59, "LoRa test");
		rak14000_text(70, 50, disp_text, (uint16_t)txt_color, 2);
		break;
	}

	snprintf(disp_text, 59, "OLED %s", has_rak1921 ? "OK" : "NA");
	rak14000_text(70, 70, disp_text, (uint16_t)txt_color, 2);

	// Get Battery status
	float batt_level_f = 0.0;
	for (int readings = 0; readings < 10; readings++)
	{
		batt_level_f += read_batt();
	}
	batt_level_f = batt_level_f / 10;

	snprintf(disp_text, 59, "Batt %.2fV", batt_level_f/1000);
	rak14000_text(70, 90, disp_text, (uint16_t)txt_color, 2);

	epd.Init(FULL);
	epd.Display(image);
}
