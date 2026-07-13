#include "oled.h"
#include "oledfont.h"
#include <string.h>   // ���� memset
#include <stdlib.h>

extern I2C_HandleTypeDef hi2c1;

// �Դ滺����: [ҳ][��]
static uint8_t oled_buffer[OLED_PAGES][OLED_WIDTH];

// ---------- ԭ�к��������ֲ��䣩----------
void OLED_WriteCmd(uint8_t cmd) {
    HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, OLED_CMD, I2C_MEMADD_SIZE_8BIT, &cmd, 1, 100);
}

void OLED_WriteData(uint8_t data) {
    HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, OLED_DATA, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

void OLED_Init(void) {
    HAL_Delay(200);
    OLED_WriteCmd(0xAE);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x10);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0xB0);
    OLED_WriteCmd(0x81);
    OLED_WriteCmd(0xFF);
    OLED_WriteCmd(0xA1);
    OLED_WriteCmd(0xA6);
    OLED_WriteCmd(0xA8);
    OLED_WriteCmd(0x3F);
    OLED_WriteCmd(0xC8);
    OLED_WriteCmd(0xD3);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0xD5);
    OLED_WriteCmd(0x80);
    OLED_WriteCmd(0xD9);
    OLED_WriteCmd(0xF1);
    OLED_WriteCmd(0xDA);
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0xDB);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x8D);
    OLED_WriteCmd(0x14);
    OLED_WriteCmd(0xAF);
    OLED_Clear();
    OLED_ClearBuffer();
    OLED_Refresh();
}

void OLED_Clear(void) {
    uint8_t i, n;
    for (i = 0; i < 8; i++) {
        OLED_WriteCmd(0xB0 + i);
        OLED_WriteCmd(0x00);
        OLED_WriteCmd(0x10);
        for (n = 0; n < 128; n++) {
            OLED_WriteData(0x00);
        }
    }
}

void OLED_SetPos(uint8_t x, uint8_t y) {
    OLED_WriteCmd(0xB0 + y);
    OLED_WriteCmd(((x & 0xF0) >> 4) | 0x10);
    OLED_WriteCmd((x & 0x0F) | 0x01);
}

void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr) {
    uint8_t c = chr - ' ';
    OLED_SetPos(x, y);
    for (uint8_t i = 0; i < 8; i++) {
        OLED_WriteData(F8X16[c * 16 + i]);
    }
    OLED_SetPos(x, y + 1);
    for (uint8_t i = 0; i < 8; i++) {
        OLED_WriteData(F8X16[c * 16 + i + 8]);
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, char* str) {
    while (*str) {
        if (x > 120) { x = 0; y += 2; }
        if (y > 6) return;
        OLED_ShowChar(x, y, *str);
        x += 8;
        str++;
    }
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len) {
    char str[12];
    for (uint8_t i = 0; i < len; i++) {
        str[len - 1 - i] = (num % 10) + '0';
        num /= 10;
    }
    str[len] = '\0';
    OLED_ShowString(x, y, str);
}

void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t BMP[]) {
    uint32_t j = 0;
    for (uint8_t y = y0; y < y1; y++) {
        OLED_SetPos(x0, y);
        for (uint8_t x = x0; x < x1; x++) {
            OLED_WriteData(BMP[j++]);
        }
    }
}

// ---------- �����Ļ�ͼ������ʹ���Դ滺������----------
void OLED_ClearBuffer(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void OLED_Refresh(void) {
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        OLED_SetPos(0, page);
        for (uint8_t col = 0; col < OLED_WIDTH; col++) {
            OLED_WriteData(oled_buffer[page][col]);
        }
    }
}

void OLED_DrawPoint(uint8_t x, uint8_t y) {
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    oled_buffer[page][x] |= (1 << bit);
}

void OLED_ClearPoint(uint8_t x, uint8_t y) {
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    oled_buffer[page][x] &= ~(1 << bit);
}

void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    int16_t dx = abs(x2 - x1), dy = abs(y2 - y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;
    int16_t x = x1, y = y1;
    while (1) {
        OLED_DrawPoint(x, y);
        if (x == x2 && y == y2) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx)  { err += dx; y += sy; }
    }
}
void OLED_DrawChar(uint8_t x,uint8_t y,uint8_t chr)
{
    uint8_t c=chr-' ';


    for(uint8_t i=0;i<8;i++)
    {
        oled_buffer[y][x+i]=F8X16[c*16+i];
    }


    for(uint8_t i=0;i<8;i++)
    {
        oled_buffer[y+1][x+i]=F8X16[c*16+i+8];
    }
}
void OLED_DrawString(uint8_t x,uint8_t y,char *str)
{

while(*str)
{

OLED_DrawChar(x,y,*str);

x+=8;

str++;

}

}