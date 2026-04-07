#### 波形数据生成
>假设屏幕宽度 128 像素，高度 64 像素，Y 轴范围 0~63（0 为顶部，63 为底部）。为了方便，我们将波形中心置于 Y=32。
```c
#define POINTS 128                      // 波形点数（屏幕宽度）
uint8_t sine_wave[POINTS];              // 正弦波数据
uint8_t square_wave[POINTS];            // 方波数据
uint8_t triangle_wave[POINTS];          // 三角波数据

/*正弦波*/
void Generate_SineWave(void) {
    for(int i = 0; i < POINTS; i++) {
        float angle = 2 * 3.14159 * i / POINTS;
        float val = sin(angle);  // -1..1
        // 映射到 0..63，中心 32
        uint8_t y = 32 + (uint8_t)(val * 31);
        sine_wave[i] = y;
    }
}
/*方波*/
void Generate_SquareWave(void) {
    int half = POINTS / 2;
    for(int i = 0; i < POINTS; i++) {
        if(i < half) square_wave[i] = 20;   // 低电平
        else square_wave[i] = 44;           // 高电平
    }
}
/*三角波*/
void Generate_TriangleWave(void) {
    int period = POINTS / 2;
    for(int i = 0; i < POINTS; i++) {
        int phase = i % period;
        if(i < period) triangle_wave[i] = 20 + (phase * 24) / period;
        else triangle_wave[i] = 44 - (phase * 24) / period;
    }
}

```

```c
/**
 * @brief  将波形数据发送到串口屏绘制
 * @param  wave_data  波形Y坐标数组（0~63）
 * @param  name       波形名称字符串
 */
void Draw_Waveform_On_Screen(uint8_t *wave_data, char *name) {
    // 1. 清屏
    printf("CLS(0);\r\n");   // 黑色清屏，根据屏幕实际命令修改

    // 2. 显示姓名（坐标(0,0)）
    printf("DS32(0,0,4,1,0,\"Your Name\");\r\n");

    // 3. 显示波形名称（坐标(100,0)）
    printf("DS32(100,0,4,1,0,\"%s\");\r\n", name);

    // 4. 绘制坐标轴（可选）
    // X轴：从(0,32)到(127,32)
    printf("DRAW(0,32,127,32,1);\r\n");
    // Y轴：从(64,0)到(64,63)
    printf("DRAW(64,0,64,63,1);\r\n");

    // 5. 绘制波形（连续画线）
    for (int i = 0; i < POINTS - 1; i++) {
        uint8_t x1 = i;
        uint8_t y1 = wave_data[i];
        uint8_t x2 = i + 1;
        uint8_t y2 = wave_data[i + 1];
        printf("DRAW(%d,%d,%d,%d,1);\r\n", x1, y1, x2, y2);
    }
}
```
#### 按键中断
```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if(GPIO_Pin == KEY_UP_Pin) {
        // 消抖延时（简单处理）
        HAL_Delay(20);
        if(HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == GPIO_PIN_RESET) {
            printf("KEY pressed\r\n");
            // 设置标志位，主循环处理
            key_flag = 1;//记得 volatile uint8_t key_flag = 0;
        }
    }
}
```
#### 生成波形数据并绘制
```c

#define WAVE_POINTS 128      // 波形点数
int sine_wave[WAVE_POINTS];  // 存储波形Y坐标


/**
 * @brief 生成正弦波数据并发送到串口屏
 */
void draw_sine_waveform(void) {
    // 生成波形数据并实时发送
    for(int x = 0; x < WAVE_POINTS; x++) {
        // 计算Y值：sin波形范围0-100，中心在50[citation:10]
        float angle = 2 * 3.14159f * x / WAVE_POINTS;
        float val = sinf(angle);                    // -1 ~ 1
        int y = 50 + (int)(val * 45);               // 范围5~95
        
        // 发送数据到串口屏（通道0）
        send_waveform_data(y, 0);
    }
}

/**
 * @brief 生成方波数据并发送到串口屏
 */
void draw_square_waveform(void) {
    int half = WAVE_POINTS / 2;
    
    for(int x = 0; x < WAVE_POINTS; x++) {
        int y = (x < half) ? 25 : 75;   // 低电平25，高电平75
        send_waveform_data(y, 0);
    }
}

/**
 * @brief 生成三角波数据并发送到串口屏
 */
void draw_triangle_waveform(void) {
    int period = WAVE_POINTS / 2;
    
    for(int x = 0; x < WAVE_POINTS; x++) {
        int phase = x % period;
        int y;
        if(x < period) {
            y = 25 + (phase * 50) / period;    // 上升沿 25→75
        } else {
            y = 75 - (phase * 50) / period;    // 下降沿 75→25
        }
        send_waveform_data(y, 0);
    }
}
```
#### 用OLED屏幕显示
```c

```