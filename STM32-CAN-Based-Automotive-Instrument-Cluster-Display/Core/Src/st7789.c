#include "st7789.h"
#include "spi.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==== �?nh nghia l?nh ST7789 ====
#define ST7789_SWRESET  0x01
#define ST7789_SLPOUT   0x11
#define ST7789_COLMOD   0x3A
#define ST7789_MADCTL   0x36
#define ST7789_CASET    0x2A
#define ST7789_RASET    0x2B
#define ST7789_RAMWR    0x2C
#define ST7789_INVON    0x21
#define ST7789_DISPON   0x29
#define ST7789_X_OFFSET  	0
#define ST7789_Y_OFFSET 	0

#define PI 3.14159265f

// ==== H�m h? tr? di?u khi?n ch�n ====
static void ST7789_Reset(void)
{
    HAL_GPIO_WritePin(TFT_RES_GPIO_Port, TFT_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(50);      // tang t? 20ms l�n 50ms
    HAL_GPIO_WritePin(TFT_RES_GPIO_Port, TFT_RES_Pin, GPIO_PIN_SET);
    HAL_Delay(150);     // tang t? 120ms l�n 150ms
}

static void ST7789_WriteCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
}

static void ST7789_WriteData(uint8_t *data, uint16_t size)
{
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_SET); // DC=1 -> data
    HAL_SPI_Transmit(&hspi1, data, size, HAL_MAX_DELAY);
}

static void ST7789_WriteData8(uint8_t d)
{
    ST7789_WriteData(&d, 1);
}

// Set v�ng v? (window) tru?c khi ghi d? li?u pixel
static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    x0 += ST7789_X_OFFSET;
    x1 += ST7789_X_OFFSET;
    y0 += ST7789_Y_OFFSET;
    y1 += ST7789_Y_OFFSET;

    ST7789_WriteCommand(ST7789_CASET);
    uint8_t caset[4] = { x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF };
    ST7789_WriteData(caset, 4);

    ST7789_WriteCommand(ST7789_RASET);
    uint8_t raset[4] = { y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF };
    ST7789_WriteData(raset, 4);

    ST7789_WriteCommand(ST7789_RAMWR);
}

// ==== H�m kh?i t?o m�n h�nh ====
void ST7789_Init(void)
{
    ST7789_Reset();

    ST7789_WriteCommand(ST7789_SWRESET);
    HAL_Delay(150);

    ST7789_WriteCommand(ST7789_SLPOUT);
    HAL_Delay(120);

    ST7789_WriteCommand(ST7789_COLMOD);
    ST7789_WriteData8(0x55); // 16-bit color RGB565

    ST7789_WriteCommand(ST7789_MADCTL);
    ST7789_WriteData8(0x00);

    // ---- Porch setting ----
    ST7789_WriteCommand(0xB2);
    {
        uint8_t d[5] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
        ST7789_WriteData(d, 5);
    }

    // ---- Gate control ----
    ST7789_WriteCommand(0xB7);
    ST7789_WriteData8(0x35);

    // ---- VCOM setting ----
    ST7789_WriteCommand(0xBB);
    ST7789_WriteData8(0x19);

    // ---- LCM control ----
    ST7789_WriteCommand(0xC0);
    ST7789_WriteData8(0x2C);

    // ---- VDV and VRH command enable ----
    ST7789_WriteCommand(0xC2);
    ST7789_WriteData8(0x01);

    // ---- VRH set ----
    ST7789_WriteCommand(0xC3);
    ST7789_WriteData8(0x12);

    // ---- VDV set ----
    ST7789_WriteCommand(0xC4);
    ST7789_WriteData8(0x20);

    // ---- Frame rate control ----
    ST7789_WriteCommand(0xC6);
    ST7789_WriteData8(0x0F);

    // ---- Power control 1 ----
    ST7789_WriteCommand(0xD0);
    {
        uint8_t d[2] = {0xA4, 0xA1};
        ST7789_WriteData(d, 2);
    }

    // ---- Positive gamma correction ----
    ST7789_WriteCommand(0xE0);
    {
        uint8_t d[14] = {0xD0,0x04,0x0D,0x11,0x13,0x2B,0x3F,
                          0x54,0x4C,0x18,0x0D,0x0B,0x1F,0x23};
        ST7789_WriteData(d, 14);
    }

    // ---- Negative gamma correction ----
    ST7789_WriteCommand(0xE1);
    {
        uint8_t d[14] = {0xD0,0x04,0x0C,0x11,0x13,0x2C,0x3F,
                          0x44,0x51,0x2F,0x1F,0x1F,0x20,0x23};
        ST7789_WriteData(d, 14);
    }

    ST7789_WriteCommand(ST7789_INVON);
    HAL_Delay(10);

    ST7789_WriteCommand(0x13); // NORON - normal display mode on
    HAL_Delay(10);

    ST7789_WriteCommand(ST7789_DISPON);
    HAL_Delay(120);
}

// ==== T� to�n m�n h�nh 1 m�u ====
void ST7789_FillScreen(uint16_t color)
{
    ST7789_FillRect(0, 0, ST7789_WIDTH, ST7789_HEIGHT, color);
}

// ==== T� 1 pixel ====
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;

    ST7789_SetAddressWindow(x, y, x, y);
    uint8_t data[2] = { color >> 8, color & 0xFF };
    ST7789_WriteData(data, 2);
}

// ==== T� 1 v�ng h�nh ch? nh?t ====
void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if ((x + w) > ST7789_WIDTH) w = ST7789_WIDTH - x;
    if ((y + h) > ST7789_HEIGHT) h = ST7789_HEIGHT - y;

    ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t data[2] = { color >> 8, color & 0xFF };
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_SET);

    for (uint32_t i = 0; i < (uint32_t)w * h; i++)
    {
        HAL_SPI_Transmit(&hspi1, data, 2, HAL_MAX_DELAY);
    }
}

void ST7789_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bgColor, uint8_t size)
{
    // Chuyển chữ thường (a-z) thành chữ hoa (A-Z) vì bảng font chỉ có chữ hoa
    if (c >= 'a' && c <= 'z')
    {
        c = c - 32; // 'a' - 32 = 'A', đúng khoảng cách mã ASCII giữa hoa/thường
    }

    if (c < 32 || c > 90) c = 32;

    const uint8_t *charData = font5x7[c - 32];

    for (uint8_t col = 0; col < 5; col++)
    {
        uint8_t line = charData[col];
        for (uint8_t row = 0; row < 8; row++)
        {
            uint16_t pixelColor = (line & (1 << row)) ? color : bgColor;

            if (size == 1)
            {
                ST7789_DrawPixel(x + col, y + row, pixelColor);
            }
            else
            {
                ST7789_FillRect(x + col * size, y + row * size, size, size, pixelColor);
            }
        }
    }
}

void ST7789_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bgColor, uint8_t size)
{
    uint16_t cursorX = x;
    uint16_t cursorY = y;

    while (*str)
    {
        // Ki?m tra TRU?C khi v?: n?u k� t? n�y v? v�o s? vu?t qu� m�p ph?i m�n
        if (cursorX + 6 * size > ST7789_WIDTH)
        {
            cursorX = x;                   // v? l?i d?u h�ng (c?t x ban d?u)
            cursorY += 8 * size + 2;       // xu?ng h�ng ti?p theo
        }

        if (*str == '\n')
        {
            cursorX = x;
            cursorY += 8 * size + 2;
            str++;
            continue;
        }

        ST7789_DrawChar(cursorX, cursorY, *str, color, bgColor, size);
        cursorX += 6 * size;
        str++;
    }
}

void ST7789_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;

    while (1)
    {
        ST7789_DrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void ST7789_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ST7789_DrawLine(x, y, x + w - 1, y, color);              // c?nh tr�n
    ST7789_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color); // c?nh du?i
    ST7789_DrawLine(x, y, x, y + h - 1, color);               // c?nh tr�i
    ST7789_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color); // c?nh ph?i
}

void ST7789_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;

    ST7789_DrawPixel(x0, y0 + r, color);
    ST7789_DrawPixel(x0, y0 - r, color);
    ST7789_DrawPixel(x0 + r, y0, color);
    ST7789_DrawPixel(x0 - r, y0, color);

    while (x < y)
    {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;

        ST7789_DrawPixel(x0 + x, y0 + y, color);
        ST7789_DrawPixel(x0 - x, y0 + y, color);
        ST7789_DrawPixel(x0 + x, y0 - y, color);
        ST7789_DrawPixel(x0 - x, y0 - y, color);
        ST7789_DrawPixel(x0 + y, y0 + x, color);
        ST7789_DrawPixel(x0 - y, y0 + x, color);
        ST7789_DrawPixel(x0 + y, y0 - x, color);
        ST7789_DrawPixel(x0 - y, y0 - x, color);
    }
}

void ST7789_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    for (int16_t y = -r; y <= r; y++)
        for (int16_t x = -r; x <= r; x++)
            if (x * x + y * y <= r * r)
                ST7789_DrawPixel(x0 + x, y0 + y, color);
}


void ST7789_DrawSpeedometer(int16_t cx, int16_t cy, int16_t radius, uint16_t speed, uint16_t maxSpeed)
{
    const float startAngle = 135.0f;  // g�c b?t d?u (du?i-tr�i)
    const float sweep = 270.0f;       // qu�t 270 d?, h? ph�a du?i
		ST7789_DrawString(cx - 10, cy - 36, "KMH", ST7789_GREEN, ST7789_BLACK, 1);
    if (speed > maxSpeed) speed = maxSpeed;

    // V? v�ng cung vi?n ngo�i
    for (float a = 0; a <= sweep; a += 1.0f)
    {
        float rad = (startAngle + a) * PI / 180.0f;
        ST7789_DrawPixel(cx + radius * cosf(rad), cy + radius * sinf(rad), ST7789_WHITE);
    }

    // V? v?ch chia + s?, m?i 20 don v?
    for (int v = 0; v <= maxSpeed; v += 20)
    {
        float angleDeg = startAngle + (sweep * v / maxSpeed);
        float rad = angleDeg * PI / 180.0f;

        int16_t x1 = cx + (radius - 10) * cosf(rad);
        int16_t y1 = cy + (radius - 10) * sinf(rad);
        int16_t x2 = cx + radius * cosf(rad);
        int16_t y2 = cy + radius * sinf(rad);
        ST7789_DrawLine(x1, y1, x2, y2, ST7789_WHITE);

        int16_t tx = cx + (radius - 25) * cosf(rad);
        int16_t ty = cy + (radius - 25) * sinf(rad);
        char buf[4];
        sprintf(buf, "%d", v);
        ST7789_DrawString(tx - 6, ty - 4, buf, ST7789_WHITE, ST7789_BLACK, 1);
    }

    // V? kim ch? t?c d? hi?n t?i
    float needleDeg = startAngle + (sweep * speed / maxSpeed);
    float needleRad = needleDeg * PI / 180.0f;
    int16_t nx = cx + (radius - 15) * cosf(needleRad);
    int16_t ny = cy + (radius - 15) * sinf(needleRad);
    ST7789_DrawLine(cx, cy, nx, ny, ST7789_RED);

    // T�m kim
    ST7789_FillCircle(cx, cy, 4, ST7789_WHITE);

    // S? t?c d? hi?n t?i, hi?n th? du?i t�m
    char speedBuf[8];
    sprintf(speedBuf, "%3d", speed);
    ST7789_DrawString(cx - 18, cy + 20, speedBuf, ST7789_GREEN, ST7789_BLACK, 2);
		
}

void ST7789_DrawRPMMeter(int16_t cx, int16_t cy, int16_t radius, uint16_t rpm, uint16_t maxRPM)
{
    const float startAngle = 135.0f;  // g�c b?t d?u (du?i-tr�i)
    const float sweep = 270.0f;       // qu�t 270 d?, h? ph�a du?i
		
    if (rpm > maxRPM) rpm = maxRPM;

    // V? v�ng cung vi?n ngo�i
    for (float a = 0; a <= sweep; a += 1.0f)
    {
        float rad = (startAngle + a) * PI / 180.0f;
        ST7789_DrawPixel(cx + radius * cosf(rad), cy + radius * sinf(rad), ST7789_WHITE);
    }

    // V? v?ch chia + s?, m?i 20 don v?
    for (int v = 0; v <= maxRPM; v += 1)
    {
        float angleDeg = startAngle + (sweep * v / maxRPM);
        float rad = angleDeg * PI / 180.0f;

        int16_t x1 = cx + (radius - 10) * cosf(rad);
        int16_t y1 = cy + (radius - 10) * sinf(rad);
        int16_t x2 = cx + radius * cosf(rad);
        int16_t y2 = cy + radius * sinf(rad);
        ST7789_DrawLine(x1, y1, x2, y2, ST7789_WHITE);

        char buf[4];
				sprintf(buf, "%d", v);
				uint8_t len = strlen(buf);

				int16_t tx = cx + (radius - 25) * cosf(rad);
				int16_t ty = cy + (radius - 25) * sinf(rad);

// Can gi?a ch? s? theo chi?u ngang v� d?c, d?a v�o s? k� t? th?c t?
				ST7789_DrawString(tx - (len * 6) / 2, ty - 4, buf, ST7789_WHITE, ST7789_BLACK, 1);
    }

    // V? kim ch? t?c d? hi?n t?i
    float needleDeg = startAngle + (sweep * rpm / maxRPM);
    float needleRad = needleDeg * PI / 180.0f;
    int16_t nx = cx + (radius - 15) * cosf(needleRad);
    int16_t ny = cy + (radius - 15) * sinf(needleRad);
    ST7789_DrawLine(cx, cy, nx, ny, ST7789_RED);

    // T�m kim
    ST7789_FillCircle(cx, cy, 4, ST7789_WHITE);

    // S? t?c d? hi?n t?i, hi?n th? du?i t�m
    char speedBuf[8];
    sprintf(speedBuf, "%3d", rpm);
    ST7789_DrawString(cx - 10, cy + 24 , "RPM", ST7789_GREEN, ST7789_BLACK, 1);
		ST7789_DrawString(cx - 18, cy + 34 , "X1000", ST7789_GREEN, ST7789_BLACK, 1);
}

void ST7789_DrawCarIconTop(int16_t cx, int16_t cy, uint16_t color,uint16_t colorLevel)
{
    int16_t w = 60, h = 100;
    int16_t x0 = cx - w / 2, y0 = cy - h / 2;

    // Th�n xe � bo g�c b?ng rect + circle
    ST7789_FillRect(x0, y0 + 8, w, h - 16, color);
    ST7789_FillCircle(x0 + 8, y0 + 8, 8, color);
    ST7789_FillCircle(x0 + w - 8, y0 + 8, 8, color);
    ST7789_FillCircle(x0 + 8, y0 + h - 8, 8, color);
    ST7789_FillCircle(x0 + w - 8, y0 + h - 8, 8, color);
    ST7789_FillRect(x0 + 8, y0, w - 16, 8, color);
    ST7789_FillRect(x0 + 8, y0 + h - 8, w - 16, 8, color);

    // K�nh tru?c/sau
    ST7789_DrawLine(x0 + 6, y0 + 18, x0 + w - 6, y0 + 18, ST7789_BLACK);
    ST7789_DrawLine(x0 + 6, y0 + h - 18, x0 + w - 6, y0 + h - 18, ST7789_BLACK);

    // 4 b�nh xe
    ST7789_FillRect(x0 - 8, y0 + 10, 8, 24, colorLevel);
    ST7789_FillRect(x0 + w, y0 + 10, 8, 24, colorLevel);
    ST7789_FillRect(x0 - 8, y0 + h - 28, 8, 24, colorLevel);
    ST7789_FillRect(x0 + w, y0 + h - 28, 8, 24, colorLevel);
}

void ST7789_DrawSignalBars(int16_t x, int16_t y, uint8_t level, uint16_t color)
{
    uint8_t barWidth = 8, gap = 2;
    uint8_t heights[3] = {12, 20, 28};

    for (uint8_t i = 0; i < 3; i++)
    {
        uint16_t barColor = (i < level) ? color : ST7789_BLACK; // bar chua t?i m?c th� t?t (den)
        int16_t bx = x + i * (barWidth + gap);
        int16_t by = y + (14 - heights[i]); // can d�y b?ng nhau
        ST7789_FillRect(bx, by, barWidth, heights[i], barColor);
    }
}

void ST7789_DrawParkingScreen(uint16_t distanceCm)
{
    ST7789_FillScreen(ST7789_BLACK);

   

    // X�c d?nh m?c + m�u theo kho?ng c�ch (ch?nh ngu?ng theo th?c t? c?m bi?n c?a b?n)
    uint8_t level;
    uint16_t color;
    if (distanceCm > 50)      	 color = ST7789_GREEN; 
    else if (distanceCm > 20) 	 color = ST7789_YELLOW; 
    else                      	 color = ST7789_RED; 
		
		 // V? xe ? gi?a m�n
    ST7789_DrawCarIconTop(120, 160, ST7789_WHITE, color);

    // Ch? "P" g�c tr�n ph?i
    ST7789_DrawString(100, 10, "P", ST7789_WHITE, ST7789_BLACK, 6);

}

void ST7789_DrawTempHumiPage(int8_t temp, uint8_t humi)
{
    ST7789_FillScreen(ST7789_BLACK);

    ST7789_DrawString(60, 40, "TEMPERATURE", ST7789_WHITE, ST7789_BLACK, 2);
    char tempBuf[8];
    sprintf(tempBuf, "%d C", temp);
    ST7789_DrawString(70, 60, tempBuf, ST7789_GREEN, ST7789_BLACK, 3);

    ST7789_DrawString(70, 130, "HUMIDITY", ST7789_WHITE, ST7789_BLACK, 2);
    char humiBuf[8];
    sprintf(humiBuf, "%d %%", humi);
    ST7789_DrawString(70, 150, humiBuf, ST7789_GREEN, ST7789_BLACK, 3);
}

void ST7789_DrawGearIcon(int16_t cx, int16_t cy, int16_t radius, uint16_t color)
{
    uint8_t teeth = 8;              // số răng
    int16_t toothLen = radius * 0.35f;  // độ dài răng nhô ra
    int16_t toothWidth = 6;         // độ rộng mỗi răng (tính theo góc)

    // Thân bánh răng (vòng tròn đặc)
    ST7789_FillCircle(cx, cy, radius, color);

    // Vẽ các răng xung quanh viền
    for (uint8_t i = 0; i < teeth; i++)
    {
        float angle = i * (360.0f / teeth) * PI / 180.0f;
        int16_t tx = cx + (radius + toothLen / 2) * cosf(angle);
        int16_t ty = cy + (radius + toothLen / 2) * sinf(angle);
        // Mỗi răng là 1 hình tròn nhỏ nhô ra ngoài viền chính -> tạo hiệu ứng răng cưa
        ST7789_FillCircle(tx, ty, toothWidth / 2 + 6, color);
    }

    // Lỗ tròn ở giữa (nền đen, tạo cảm giác bánh răng có tâm rỗng)
    ST7789_FillCircle(cx, cy, radius / 2, ST7789_BLACK);
}

void ST7789_DrawSettingScreen(void)
{
    ST7789_FillScreen(ST7789_BLACK);
    ST7789_DrawString(60, 20, "SETTING", ST7789_WHITE, ST7789_BLACK, 3);
    ST7789_DrawGearIcon(120, 130, 50, ST7789_WHITE);

}

void ST7789_DrawDashboard()
{
    ST7789_FillScreen(ST7789_BLACK);

    char buf[20];

    sprintf(buf, "Speed: ");
    ST7789_DrawString(10, 30, buf, ST7789_WHITE, ST7789_BLACK, 2);

    sprintf(buf, "Distance: ");
    ST7789_DrawString(10, 70, buf, ST7789_WHITE, ST7789_BLACK, 2);

    sprintf(buf, "Temperature: ");
    ST7789_DrawString(10, 110, buf, ST7789_WHITE, ST7789_BLACK, 2);

    sprintf(buf, "Humidity: ");
    ST7789_DrawString(10, 150, buf, ST7789_WHITE, ST7789_BLACK, 2);
}

// Vẽ khung + nhãn — CHỈ gọi 1 lần lúc khởi động
void ST7789_DrawDashboardLabels(void)
{
    ST7789_FillScreen(ST7789_BLACK);
    ST7789_DrawString(10, 30,  "Speed:",    ST7789_WHITE, ST7789_BLACK, 2);
    ST7789_DrawString(10, 70,  "Distance:", ST7789_WHITE, ST7789_BLACK, 2);
    ST7789_DrawString(10, 110, "Temperature:",     ST7789_WHITE, ST7789_BLACK, 2);
    ST7789_DrawString(10, 150, "Humidity:",     ST7789_WHITE, ST7789_BLACK, 2);
}

// Chỉ cập nhật vùng SỐ — gọi liên tục, không xoá cả màn
void ST7789_UpdateDashboardValues(uint16_t speed, uint16_t distance, int8_t temp, uint8_t humi)
{
    char buf[10];
    // Toạ độ x=130 -> đủ chỗ cho nhãn dài nhất ("Distance:"), vùng số nằm bên phải nhãn
    uint16_t valX = 160;
    uint16_t valW = 100; // độ rộng vùng số, đủ chứa số lớn nhất có thể

    // Xoá đúng vùng số cũ (vẽ đè màu đen) trước khi ghi số mới -> tránh dính số cũ
    ST7789_FillRect(valX, 30,  valW, 16, ST7789_BLACK);
    sprintf(buf, "%d", speed);
    ST7789_DrawString(valX, 30, buf, ST7789_GREEN, ST7789_BLACK, 2);

    ST7789_FillRect(valX, 70,  valW, 16, ST7789_BLACK);
    sprintf(buf, "%d", distance);
    ST7789_DrawString(valX, 70, buf, ST7789_GREEN, ST7789_BLACK, 2);

    ST7789_FillRect(valX, 110, valW, 16, ST7789_BLACK);
    sprintf(buf, "%d", temp);
    ST7789_DrawString(valX, 110, buf, ST7789_GREEN, ST7789_BLACK, 2);

    ST7789_FillRect(valX, 150, valW, 16, ST7789_BLACK);
    sprintf(buf, "%d", humi);
    ST7789_DrawString(valX, 150, buf, ST7789_GREEN, ST7789_BLACK, 2);
}
