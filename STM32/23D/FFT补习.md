### 对 ADC 采样数据做 FFT 频谱分析，然后提取 AM/ASK/FM/PSK/FSK 等调制信号特征的程序中
#### FFT的点
```C
static float32_t deta_f_256 = 1000;
```
>256 点 FFT 的频率分辨率，即每个 FFT bin 对应的频率间隔，单位通常是 Hz。
![alt text](image.png)

这里就不得不提到ADC的采样率。
![ADC](image-1.png)
如图
>![Cube-ADC](image-2.png)
>...by 2是2分频

![alt text](image-3.png)
>+ 又，ADC 不能直接跑 72 MHz。在 STM32F4 中，ADC 时钟（ADCCLK）最高通常限制在 36 MHz。
>+ 所以会有一个ADC 预分频器（Prescaler）
>+ 所以如图没有配置TIM的时候ADC的时钟频率（ADCCLK）就是36MHz。
>+ 不加TIM就没有限制它了，此时ADC容易跑到上限（采样率1.2MSPS~2MSPS）(F4)

**那么要是配置了呢？**
ADC的实际采样率 = TIM 的触发频率（也就是定时器的更新频率）
![CY](image-4.png)
同时
![TIM](image-5.png)
还有TIM_CLK是
![TIM_CLK](image-6.png)
我的PSC=0；ARR=179。
所以ADC的采样率=72MHz/180=400KHz
>能正确识别出的最高信号频率是 200 kHz

为什么呢？
最高频率fs/2（奈奎斯特-香农采样定理）
这个有一个专有名词**频谱混叠（Aliasing）**
因为超过200KHZ，周期走不完整，会测出|输入频率−N*400KHz（采样率）|,N是正整数。
**那么接下来回到FFT**
（对了FFT的点得是2的幂数）
那么要接近100KHz的频率分辩率，应该是4096点
400KHZ/4096=97.6HZ
假设ADC,TIM,DMA已经配好了。
那么代码
```C
/* 宏定义与全局变量准备 */
#include "arm_math.h"   // 包含DSP库

#define FFT_point 4096
#define SAMPLE_RATE 400000.0f // 400 kHz

// 频率分辨率 = 400k / 4096 = 97.65625 Hz
#define FFT_deta (SAMPLE_RATE / FFT_point)

// ADC DMA 接收缓存（存放原始时域数据）
static uint16_t adc_dma_buff[FFT_point]; 
// 标志位，DMA采满一帧后置1
volatile uint8_t adc_done_flag = 0;
 //uint8_t是0或者1，当bool看
//volatile每次用的时候，会去内存里重新读它的值

// FFT 输入输出缓冲区 (Cortex-M4的硬件浮点要求 float32_t)
static float32_t fft_input[FFT_point];
static float32_t fft_output[FFT_point * 2]; // FFT输出是复数，长度是点数的2倍
static float32_t mag[FFT_point / 2];        // 存幅值，FFT结果是对称的，取前一半即可

// DSP 库的 FFT 句柄（用于 4096 点 FFT）
static arm_rfft_fast_instance_f32 S;
```
