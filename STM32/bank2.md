## 如何在HAL库中生成正弦波、AM、FM、PM波、方波、三角波
1. 生成正弦波
```c
#include "math.h"

#define SINE_TABLE_SIZE  256
#define M_PI 3.14159265358979323846f
uint16_t sine_table[SINE_TABLE_SIZE];

// 生成 0~3.3V 映射到 0~4095 的正弦表（DAC 12位，右对齐）
void Generate_Sine_Table(void) {
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        float angle = (float)i / SINE_TABLE_SIZE * 2.0f * M_PI;
        float val = sinf(angle);                // -1 ~ +1
        val = (val + 1.0f) * 2047.5f;           // 0 ~ 4095
        sine_table[i] = (uint16_t)val;
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_DAC_Init();
    MX_TIM6_Init();

    Generate_Sine_Table();

    HAL_TIM_Base_Start(&htim6);   // 启动定时器触发
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t *)sine_table, 
                      SINE_TABLE_SIZE, DAC_ALIGN_12B_R);
    while (1) {
        // 你的其他代码
    }
}
```
2. 生成方波

使用定时器PWM输出。配置通道，频率和占空比。

3. 生成三角波

使用硬件三角波模式：DAC + 定时器触发。

4. 生成AM波

原理，中断法生成。定时器中断里计算AM公式输出DAC。

5.生成FM波

中断法，实时改变频率或相位增量。

6. 生成PM波

中断法，实时改变相位偏移