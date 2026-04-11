```c
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 波形显示系统，默认三角波自动滚动
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "oled.h"
#include "i2c.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
void process_cmd(uint8_t cmd);
void Generate_SineWave(int offset_idx);
void Generate_SquareWave(int offset_idx, uint8_t *wave_out);
void Generate_TriangleWave(int offset_idx, uint8_t *wave_out);
void OLED_Draw_Waveform(uint8_t *wave_data, char *title);
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define POINTS 128
#define SCROLL_STEP  1
#define SCROLL_INTERVAL_MS 30
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
volatile uint8_t key_flag = 0;
uint8_t rx_buffer = 0;
uint8_t auto_scroll = 1;               // 默认开启滚动
int phase_index = 0;
uint8_t current_waveform = 1;          // 0:正弦, 1:方波, 2:三角波（默认波自己可以改）

uint8_t sine_wave[POINTS];
uint8_t square_wave[POINTS];
uint8_t triangle_wave[POINTS];

uint32_t last_scroll_time = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  OLED_Init();
  OLED_Clear();
  OLED_ClearBuffer();

  // 生成初始波形（相位0）
  Generate_SineWave(0);
  Generate_SquareWave(0, square_wave);
  Generate_TriangleWave(0, triangle_wave);

  // 默认显示三角波并开启滚动
	
	OLED_Draw_Waveform(square_wave, "Square");
  //OLED_Draw_Waveform(triangle_wave, "Triangle");
	//OLED_Draw_Waveform(sine_wave, "Sine");
  auto_scroll = 1;
  current_waveform = 1;//也要改

  // 启动串口接收中断
  HAL_UART_Receive_IT(&huart1, &rx_buffer, 1);

  printf("System Ready, Triangle wave auto-scrolling ON\r\n");
  /* USER CODE END 2 */

  while (1)
  {
    // 1. 处理按键切换波形（循环顺序：三角波→正弦波→方波→三角波）
    if (key_flag) {
      key_flag = 0;
			static uint8_t wave_idx = 1;
      //static uint8_t wave_idx = 2;   // 从三角波开始
			//static uint8_t wave_idx = 0;   // 从正弦波开始
      wave_idx = (wave_idx + 1) % 3;
      switch (wave_idx) {
        case 0:
          current_waveform = 0;
          OLED_Draw_Waveform(sine_wave, "Sine");
          auto_scroll = 1;
          printf("Sine wave, auto-scroll ON\r\n");
          break;
        case 1:
          current_waveform = 1;
          OLED_Draw_Waveform(square_wave, "Square");
          auto_scroll = 1;
          printf("Square wave, auto-scroll ON\r\n");
          break;
        case 2:
          current_waveform = 2;
          OLED_Draw_Waveform(triangle_wave, "Triangle");
          auto_scroll = 1;
          printf("Triangle wave, auto-scroll ON\r\n");
          break;
      }
    }

    // 2. 自动滚动（根据当前波形）
    if (auto_scroll) {
      uint32_t now = HAL_GetTick();
      if (now - last_scroll_time >= SCROLL_INTERVAL_MS) {
        last_scroll_time = now;
        phase_index = (phase_index + SCROLL_STEP) % POINTS;
        switch (current_waveform) {
          case 0:
            Generate_SineWave(phase_index);
            OLED_Draw_Waveform(sine_wave, "Sine");
            break;
          case 1:
            Generate_SquareWave(phase_index, square_wave);
            OLED_Draw_Waveform(square_wave, "Square");
            break;
          case 2:
            Generate_TriangleWave(phase_index, triangle_wave);
            OLED_Draw_Waveform(triangle_wave, "Triangle");
            break;
        }
      }
    }
  }
}

/**
  * @brief System Clock Configuration (CubeMX 自动生成)
  */
void SystemClock_Config(void)
{
  // CubeMX 会生成实际配置代码，这里省略
}

/* USER CODE BEGIN 4 */

/***************************** 串口重定向 ************************************/
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
int fputc(int ch, FILE *f) {
    return __io_putchar(ch);
}

/***************************** 按键中断回调 **********************************/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == key_up_Pin) {
        static uint32_t last_time = 0;
        if (HAL_GetTick() - last_time > 20) {
            last_time = HAL_GetTick();
            if (HAL_GPIO_ReadPin(key_up_GPIO_Port,key_up_Pin) == GPIO_PIN_RESET) {
                printf("KEY pressed\r\n");
                key_flag = 1;
            }
        }
    }
}

/***************************** 串口中断回调 **********************************/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        process_cmd(rx_buffer);
        HAL_UART_Receive_IT(&huart1, &rx_buffer, 1);
    }
}

/***************************** 串口命令处理 **********************************/
void process_cmd(uint8_t cmd) {
    switch(cmd) {
        case '1':
            current_waveform = 0;
            Generate_SineWave(phase_index);
            OLED_Draw_Waveform(sine_wave, "Sine");
            auto_scroll = 1;
            printf("Sine wave, auto-scroll ON\r\n");
            break;
        case '2':
            current_waveform = 1;
            Generate_SquareWave(phase_index, square_wave);
            OLED_Draw_Waveform(square_wave, "Square");
            auto_scroll = 1;
            printf("Square wave, auto-scroll ON\r\n");
            break;
        case '3':
            current_waveform = 2;
            Generate_TriangleWave(phase_index, triangle_wave);
            OLED_Draw_Waveform(triangle_wave, "Triangle");
            auto_scroll = 1;
            printf("Triangle wave, auto-scroll ON\r\n");
            break;
        case '4':
            auto_scroll = !auto_scroll;
            printf("Auto-scroll %s\r\n", auto_scroll ? "ON" : "OFF");
            break;
        default:
            break;
    }
}

/***************************** 正弦波生成（查表法） ************************/
void Generate_SineWave(int offset_idx) {
    static const uint8_t sin_table[128] = {
        128,134,140,147,153,159,165,171,177,182,188,193,198,203,208,213,
        217,221,225,229,233,236,239,242,245,247,250,252,253,255,255,255,
        255,255,255,253,252,250,247,245,242,239,236,233,229,225,221,217,
        213,208,203,198,193,188,182,177,171,165,159,153,147,140,134,128,
        122,116,109,103,97,91,85,79,74,68,63,58,53,48,43,39,
        35,31,27,23,20,17,14,11,9,6,4,3,1,1,0,0,
        0,0,1,1,3,4,6,9,11,14,17,20,23,27,31,35,
        39,43,48,53,58,63,68,74,79,85,91,97,103,109,116,122
    };
    
    for (int i = 0; i < POINTS; i++) {
        int idx = (i + offset_idx) % POINTS;
        uint16_t y = (uint16_t)sin_table[idx] * 63 / 255;
        sine_wave[i] = (uint8_t)y;
    }
}

/***************************** 方波生成（查表法） **************************/
void Generate_SquareWave(int offset_idx, uint8_t *wave_out) {
    static const uint8_t square_table[128] = {
        20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
        20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
        20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
        20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
        44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,
        44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,
        44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,
        44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44
    };
    
    for (int i = 0; i < POINTS; i++) {
        int idx = (i + offset_idx) % POINTS;
        wave_out[i] = square_table[idx];
    }
}

/***************************** 三角波生成（查表法） ************************/
void Generate_TriangleWave(int offset_idx, uint8_t *wave_out) {
    static const uint8_t triangle_table[128] = {
        20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
        21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,
        37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,
        53,54,55,56,57,58,59,60,61,62,63,63,63,63,63,63,
        63,63,63,63,63,63,63,62,61,60,59,58,57,56,55,54,
        53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,
        37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,
        21,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20
    };
    
    for (int i = 0; i < POINTS; i++) {
        int idx = (i + offset_idx) % POINTS;
        wave_out[i] = triangle_table[idx];
    }
}

/***************************** OLED 波形绘制 ********************************/
void OLED_Draw_Waveform(uint8_t *wave_data, char *title) {
    OLED_ClearBuffer();

    extern const unsigned char F8X16[];   // 字库在 oledfont.h 中定义

    // 在屏幕底部（y=48）显示姓名
    char *name = "Wistar";                // 请修改为你的名字
    uint8_t x = 0, y_text = 48;
    for (uint8_t i = 0; name[i] != '\0'; i++) {
        uint8_t ch = name[i] - ' ';
        for (uint8_t col = 0; col < 8; col++) {
            uint8_t data = F8X16[ch * 16 + col];
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (data & (1 << bit))
                    OLED_DrawPoint(x + col, y_text + bit);
            }
        }
        for (uint8_t col = 0; col < 8; col++) {
            uint8_t data = F8X16[ch * 16 + 8 + col];
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (data & (1 << bit))
                    OLED_DrawPoint(x + col, y_text + 8 + bit);
            }
        }
        x += 8;
        if (x > 120) break;
    }

    // 显示波形名称（坐标 80, 48）
    x = 80;
    for (uint8_t i = 0; title[i] != '\0'; i++) {
        uint8_t ch = title[i] - ' ';
        for (uint8_t col = 0; col < 8; col++) {
            uint8_t data = F8X16[ch * 16 + col];
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (data & (1 << bit))
                    OLED_DrawPoint(x + col, y_text + bit);
            }
        }
        for (uint8_t col = 0; col < 8; col++) {
            uint8_t data = F8X16[ch * 16 + 8 + col];
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (data & (1 << bit))
                    OLED_DrawPoint(x + col, y_text + 8 + bit);
            }
        }
        x += 8;
        if (x > 120) break;
    }

    // 绘制坐标轴
    uint8_t axis_y = 32;
    OLED_DrawLine(0, axis_y, 127, axis_y);
    OLED_DrawLine(64, 0, 64, 63);

    // 绘制波形
    for (int i = 0; i < POINTS - 1; i++) {
        uint8_t y1 = wave_data[i];
        uint8_t y2 = wave_data[i+1];
        if (y1 > 63) y1 = 63;
        if (y2 > 63) y2 = 63;
        OLED_DrawLine(i, y1, i+1, y2);
    }

    OLED_Refresh();
}

/* USER CODE END 4 */

/**
  * @brief  Error handler
  */
void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
    /* 可添加调试输出 */
}
#endif
```
