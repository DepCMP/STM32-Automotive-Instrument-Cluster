#ifndef ST7789_H
#define ST7789_H


#include "main.h"
#include "font5x7.h"


#define ST7789_WIDTH   240
#define ST7789_HEIGHT  240


#define ST7789_BLACK   	0x0000
#define ST7789_WHITE   	0xFFFF
#define ST7789_RED     	0xF800
#define ST7789_GREEN   	0x07E0
#define ST7789_BLUE    	0x001F  
#define ST7789_YELLOW	 	0xFFE0
void ST7789_Init(void);
void ST7789_FillScreen(uint16_t color);
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bgColor, uint8_t size);
void ST7789_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bgColor, uint8_t size);
void ST7789_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void ST7789_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void ST7789_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void ST7789_DrawSpeedometer(int16_t cx, int16_t cy, int16_t radius, uint16_t speed, uint16_t maxSpeed);
void ST7789_DrawRPMMeter(int16_t cx, int16_t cy, int16_t radius, uint16_t speed, uint16_t maxRPM);

void ST7789_DrawCarIconTop(int16_t cx, int16_t cy, uint16_t color, uint16_t colorLevel);
void ST7789_DrawSignalBars(int16_t x, int16_t y, uint8_t level, uint16_t color);
void ST7789_DrawParkingScreen(uint16_t distanceCm);

void ST7789_DrawTempHumiPage(int8_t temp, uint8_t humi);

void ST7789_DrawGearIcon(int16_t cx, int16_t cy, int16_t radius, uint16_t color);
void ST7789_DrawSettingScreen(void);

void ST7789_DrawDashboard();
void ST7789_DrawDashboardLabels(void);
void ST7789_UpdateDashboardValues(uint16_t speed, uint16_t distance, int8_t temp, uint8_t humi);
#endif
