#ifndef __OLED_H
#define __OLED_H

#include "stm32f4xx_hal.h"
#include "main.h"

// -------------------- 用户配置区 --------------------
// OLED I2C 地址 (通常是 0x78 或 0x7A)
#define OLED_ADDR 0x78

// 命令/数据标识
#define OLED_CMD  0x00
#define OLED_DATA 0x40
// ---------------------------------------------------

// 显存缓冲区大小
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_PAGES  8   // 64/8

// 函数声明
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr);
void OLED_ShowString(uint8_t x, uint8_t y, char* str);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len);
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t BMP[]);

// 新增的绘图函数（使用显存缓冲区）
void OLED_Refresh(void);                           // 刷新显存到屏幕
void OLED_DrawPoint(uint8_t x, uint8_t y);         // 画点
void OLED_ClearPoint(uint8_t x, uint8_t y);        // 清除点
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);  // 画线
void OLED_ClearBuffer(void);                       // 清空显存缓冲区（不刷新屏幕）

#endif
