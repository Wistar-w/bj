# 波形显示系统（STM32 + OLED）

本项目基于 STM32 单片机，使用 0.96 英寸 I2C 接口 OLED 显示屏，实现**正弦波、方波、三角波**的实时显示与切换，支持**按键**和**串口命令**控制，所有波形均可自动滚动。

## 功能特点

- 生成三种波形：正弦波、方波、三角波  
- 波形自动向左滚动（可开关）  
- 按键循环切换波形  
- 串口命令切换波形（`'1'` `'2'` `'3'`）及开关滚动（`'4'`）  
- 屏幕底部显示姓名和当前波形名称  
- 波形垂直居中，坐标轴辅助观察  

## 硬件连接

| 组件      | STM32 引脚          |
|-----------|---------------------|
| OLED SCL  | 任意 I2C SCK（如 PB6） |
| OLED SDA  | 任意 I2C SDA（如 PB7） |
| 按键      | 配置为 EXTI 下降沿触发（如 PA0） |
| USART1 TX | PA9                 |
| USART1 RX | PA10                |

- OLED 使用 **I2C1**，地址为 `0x78`（7 位地址 `0x3C` 左移一位）。  
- 按键需外部上拉或配置内部上拉，按下时引脚为低电平。  
- 串口波特率：115200，8N1。

## 使用的 OLED 驱动文件

本项目的显示功能依赖于窝的 oled.c / oled.h / oledfont.h。你需要确保驱动中已实现以下**关键扩展**（若未实现，请参考下文添加）：

### 必须实现的函数和变量

```c
// 显存缓冲区（8页 × 128列）
extern uint8_t oled_buffer[8][128];

void OLED_ClearBuffer(void);      // 清空显存缓冲区（全部置0）
void OLED_Refresh(void);          // 将缓冲区数据一次性发送到屏幕
void OLED_DrawPoint(uint8_t x, uint8_t y);   // 在缓冲区中画点
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2); // 画线
```
### 定义显存缓冲区（在文件顶部）：
```c
static uint8_t oled_buffer[8][128];
//实现 OLED_ClearBuffer：
```
```c
void OLED_ClearBuffer(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}
//实现 OLED_Refresh：
```
```c
void OLED_Refresh(void) {
    for (uint8_t page = 0; page < 8; page++) {
        OLED_SetPos(0, page);
        for (uint8_t col = 0; col < 128; col++) {
            OLED_WriteData(oled_buffer[page][col]);
        }
    }
}
//实现 OLED_DrawPoint：
```
```c
void OLED_DrawPoint(uint8_t x, uint8_t y) {
    if (x >= 128 || y >= 64) return;
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    oled_buffer[page][x] |= (1 << bit);
}
//实现 OLED_DrawLine（Bresenham 算法）：
```
```c
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    // 参考网上标准 Bresenham 实现，内部调用 OLED_DrawPoint
}
```
>记得在 oled.h 中添加以上函数的声明。
## 主程序使用说明
#### 1. 包含头文件
```c
#include "oled.h"
#include "usart.h"
#include "gpio.h"
#include "i2c.h"
```
#### 2. 初始化
```c
OLED_Init();
OLED_Clear();
OLED_ClearBuffer();

// 生成初始波形数据（查表法，相位 0）
Generate_SineWave(0, sine_wave);
Generate_SquareWave(0, square_wave);
Generate_TriangleWave(0, triangle_wave);

// 默认显示正弦波并开启滚动
current_waveform = 0;
auto_scroll = 1;
OLED_Draw_Waveform(sine_wave, "Sine");

// 启动串口接收
HAL_UART_Receive_IT(&huart1, &rx_buffer, 1);
```
#### 3. 主循环
主循环中需要处理按键标志和自动滚动。详细代码参见项目中的waveform_test.md(就是main.c啊这)

# 祝您调试顺利！如有问题，欢迎交流。
![alt text](819d3e18e145f75185c0d55b0a4f5fad.jpg)