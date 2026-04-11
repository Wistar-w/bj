# 波形显示

## 1. 概述

本项目实现基于 STM32 的波形显示系统，支持生成并显示**正弦波、方波、三角波**，可通过**按键**或**串口命令**切换波形，并支持正弦波自动滚动。显示设备可选择**串口屏**或 **OLED 屏**。

---

## 2. 波形数据生成

> 屏幕分辨率：128×64 像素  
> Y 轴范围：0~63（0 为顶部，63 为底部）  
> 波形中心置于 Y = 32

### 2.1 波形数组定义

```c
#define POINTS 128                      // 波形点数（屏幕宽度）

uint8_t sine_wave[POINTS];              // 正弦波数据
uint8_t square_wave[POINTS];            // 方波数据
uint8_t triangle_wave[POINTS];          // 三角波数据
```
### 2.2 基础波形生成// 不推荐使用，仅作对比
```c
/* 正弦波（浮点版本，可能存在精度问题，建议使用下方查表法） */
void Generate_SineWave(void) {
    for(int i = 0; i < POINTS; i++) {
        float angle = 2 * 3.14159f * i / POINTS;
        float val = sinf(angle);                // -1..1
        uint8_t y = 32 + (uint8_t)(val * 31);   // 映射到 0~63，中心 32
        sine_wave[i] = y;
    }
}

/* 方波 */
void Generate_SquareWave(void) {
    int half = POINTS / 2;
    for(int i = 0; i < POINTS; i++) {
        square_wave[i] = (i < half) ? 20 : 44;  // 中心 32
    }
}

/* 三角波 */
void Generate_TriangleWave(void) {
    int period = POINTS / 2;
    for(int i = 0; i < POINTS; i++) {
        int phase = i % period;
        if(i < period)
            triangle_wave[i] = 20 + (phase * 24) / period;
        else
            triangle_wave[i] = 44 - (phase * 24) / period;
    }
}
```
### 2.3 方波动态滚动（查表法，支持滚动）

```c
/**
 * @brief 生成正弦波数据（查表法）
 * @param offset_idx  相位偏移索引（0 ~ POINTS-1），实现波形滚动
 * @param wave_out    输出数组，长度为 POINTS，每个元素为 Y 坐标（0~63）
 * @note 内部使用 128 点正弦表，值域 0~255，映射到屏幕坐标 0~63
 *       通过偏移索引实现波形向左/右移动，无需浮点运算
 */
void Generate_SineWave(int offset_idx, uint8_t *wave_out) {
    //预先生成一个周期的正弦波幅度值，这样不用实时计算 sin。
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
    /*遍历屏幕的 128 个点*/
    for (int i = 0; i < POINTS; i++) {
        int idx = (i + offset_idx) % POINTS;//每次增加 1，波形就会向左移动 1 个像素，防止超过最大索引 127
        uint16_t y = (uint16_t)sin_table[idx] * 63 / 255;   // 映射到 0~63
        wave_out[i] = (uint8_t)y;//存入输出数组
    }
}

/**
 * @brief 生成方波表（128点，值域 0~63，中心32）
 * @param wave_out  输出数组
 * @param offset_idx  相位偏移索引（0~127），实现滚动
 */
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

/**
 * @brief 生成三角波表（128点，值域 0~63，中心32）
 * @param wave_out  输出数组
 * @param offset_idx  相位偏移索引（0~127），实现滚动
 */
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
```
#### 2.4 OLED 屏幕显示
```c
// 显存缓冲区（8页 × 128列）
extern uint8_t oled_buffer[8][128];

void OLED_ClearBuffer(void);   // 清空缓冲区
void OLED_Refresh(void);       // 刷新缓冲区到屏幕
void OLED_DrawPoint(uint8_t x, uint8_t y);      // 画点
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2); // 画线
```
完整的 OLED 波形绘制函数示例：
```c
void OLED_Draw_Waveform(uint8_t *wave_data, char *title) {
    OLED_ClearBuffer();

    extern const unsigned char F8X16[];   // 字库在 oledfont.h 中定义

    // 在屏幕底部（y=48）显示姓名
    char *name = "Wistar";// 名字
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


    // 绘制坐标轴(可选)
    OLED_DrawLine(0, 32, 127, 32);   // X轴
    OLED_DrawLine(64, 0, 64, 63);    // Y轴

    // 绘制波形
    for (int i = 0; i < POINTS - 1; i++) {
        OLED_DrawLine(i, wave_data[i], i+1, wave_data[i+1]);
    }

    OLED_Refresh();
}
```
### 3. 交互控制
#### 3.1按键中断（EXTI）
```c
volatile uint8_t key_flag = 0;

/**
 * @brief 按键中断回调（需在 CubeMX 中配置 EXTI 下降沿触发）
 * @note  为避免阻塞，建议使用非阻塞消抖
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == KEY_UP_Pin) {          // 与 CubeMX 中标签一致
        static uint32_t last_time = 0;
        if (HAL_GetTick() - last_time > 20) {
            last_time = HAL_GetTick();
            if (HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == GPIO_PIN_RESET) {
                key_flag = 1;
            }
        }
    }
}
```
#### 3.2串口控制
```c
void process_cmd(uint8_t cmd) {
    switch(cmd) {
        case '1':  // 正弦波
            Generate_SineWave(phase_index, sine_wave);
            OLED_Draw_Waveform(sine_wave, "Sine");
            auto_scroll = 1;
            break;
        case '2':  // 方波
        /*静态
            OLED_Draw_Waveform(square_wave, "Square");
            auto_scroll = 0;
            */
            Generate_SquareWave(phase_index, square_wave);
            OLED_Draw_Waveform(square_wave, "Square");
            auto_scroll = 1;
            break;
        case '3':  // 三角波
            Generate_TriangleWave(phase_index, triangle_wave);
            OLED_Draw_Waveform(triangle_wave, "Triangle");
            auto_scroll = 1;
            break;
        case '4':  // 开关自动滚动
            auto_scroll = !auto_scroll;
            break;
        default: break;
    }
}
```
#### 3.3 正弦波自动滚动
```c 
volatile uint8_t auto_scroll = 1;      // 自动滚动开关
int phase_index = 0;                   // 当前相位索引
uint32_t last_scroll_time = 0;         // 上次滚动时间戳

// 在主循环中调用（见第 5 节）
if (auto_scroll && (HAL_GetTick() - last_scroll_time >= 30)) {
    last_scroll_time = HAL_GetTick();
    phase_index = (phase_index + 1) % POINTS;
    switch (current_waveform) {
        case 0: Generate_SineWave(phase_index, sine_wave);
                OLED_Draw_Waveform(sine_wave, "Sine"); break;
        case 1: Generate_SquareWave(phase_index, square_wave);
                OLED_Draw_Waveform(square_wave, "Square"); break;
        case 2: Generate_TriangleWave(phase_index, triangle_wave);
                OLED_Draw_Waveform(triangle_wave, "Triangle"); break;
    }
}
```
>int main(void) {
    // ... 初始化代码 ...
    while (1) {
        // 处理按键
        if (key_flag) { ... }
        // 处理串口命令（在中断中已处理，此处无需额外代码）
        // 正弦波滚动
        if (auto_scroll && (HAL_GetTick() - last_scroll_time >= 30)) {
            last_scroll_time = HAL_GetTick();
            phase_index = (phase_index + 1) % POINTS;
            Generate_SineWave(phase_index, sine_wave);
            OLED_Draw_Waveform(sine_wave, "Sine");
        }
    }
}