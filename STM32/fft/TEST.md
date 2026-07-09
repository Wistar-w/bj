hadc1.Instance->CR2 |= ADC_CR2_CONT;//强制开启ADC。
```c
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : FFT-based waveform classifier with ADC-DMA
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "arm_math.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    WAVE_UNKNOWN = 0,
    WAVE_SINE,
    WAVE_SQUARE,
    WAVE_TRIANGLE,
    WAVE_AM,
    WAVE_FM
} WaveType;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PI           3.1415926f
#define FFT_SIZE     256
#define SAMPLE_RATE  1280    // Hz, adjust to your timer frequency

uint16_t ADC_Value[FFT_SIZE];
float fft_input[FFT_SIZE];
float fft_output[FFT_SIZE * 2];
float magnitude[FFT_SIZE / 2];
float hanning_window[FFT_SIZE];
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
arm_rfft_fast_instance_f32 fft_instance;
volatile uint8_t dma_flag = 0;
uint8_t first_run = 1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// ------------------------------------------------------------------
// 1. Initialize Hanning window
// ------------------------------------------------------------------
void Init_Window(void) {
    for (int i = 0; i < FFT_SIZE; i++) {
        hanning_window[i] = 0.5f * (1.0f - cosf(2.0f * PI * i / (FFT_SIZE - 1)));
    }
}

// ------------------------------------------------------------------
// 2. Preprocess: remove DC offset + apply window
// ------------------------------------------------------------------
void Process_FFT_Data(uint16_t *src) {
    uint32_t sum = 0;
    for (int i = 0; i < FFT_SIZE; i++) {
        sum += src[i];
    }
    float mean = (float)sum / FFT_SIZE;

    for (int i = 0; i < FFT_SIZE; i++) {
        fft_input[i] = ((float)src[i] - mean) * hanning_window[i];
    }
}

// ------------------------------------------------------------------
// 3. Find fundamental frequency (peak index)
// ------------------------------------------------------------------
int Find_Fundamental(float *mag, int len) {
    int peak_index = 1;           // Start from index 1 to skip DC
    float max_val = mag[1];
    for (int i = 2; i < len; i++) {
        if (mag[i] > max_val) {
            max_val = mag[i];
            peak_index = i;
        }
    }
    return peak_index;
}

// ------------------------------------------------------------------
// 4. Classify waveform based on harmonic ratios
// ------------------------------------------------------------------
WaveType Classify_Waveform(float *mag, int len, int peak_idx) {
    float fundamental = mag[peak_idx];

    // No signal or too weak
    if (fundamental < 1.0f) return WAVE_UNKNOWN;

    // Check FM: two sidebands close to carrier
    if (peak_idx > 2 && peak_idx < len - 3) {
        float left  = mag[peak_idx - 2];
        float right = mag[peak_idx + 2];
        if (left > fundamental * 0.1f && right > fundamental * 0.1f) {
            return WAVE_FM;
        }
    }

    // Check AM: multiple sidebands
    int side_count = 0;
    for (int i = peak_idx - 5; i <= peak_idx + 5; i++) {
        if (i > 0 && i < len && i != peak_idx && mag[i] > fundamental * 0.15f) {
            side_count++;
        }
    }
    if (side_count > 3) {
        return WAVE_AM;
    }

    // Check 3rd harmonic ratio to distinguish Sine / Square / Triangle
    int idx_3rd = peak_idx * 3;
    if (idx_3rd < len) {
        float ratio = mag[idx_3rd] / fundamental;
        if (ratio < 0.05f) {
            return WAVE_SINE;
        } else if (ratio > 0.25f) {
            return WAVE_SQUARE;
        } else if (ratio > 0.08f) {
            return WAVE_TRIANGLE;
        }
    }
    return WAVE_UNKNOWN;
}

/* USER CODE END 0 */

// ------------------------------------------------------------------
// MAIN ENTRY
// ------------------------------------------------------------------
int main(void) {
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_I2C1_Init();
    MX_ADC1_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    Init_Window();
arm_rfft_fast_init_f32(&fft_instance, FFT_SIZE);

// --------------------------------------------------------------
// 1. 开启 ADC 连续转换模式（强制！解决中断不触发的核心）
// --------------------------------------------------------------
// 如果 CubeMX 没配置连续模式，这里强行打开
hadc1.Instance->CR2 |= ADC_CR2_CONT;  // CONT = 1，连续转换
printf("Continuous Conversion Enabled.\r\n");

// --------------------------------------------------------------
// 2. 启动 ADC DMA
// --------------------------------------------------------------
if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC_Value, FFT_SIZE) != HAL_OK) {
    printf("ADC DMA Start Failed!\r\n");
    while(1);
} else {
    printf("ADC DMA Started.\r\n");
}

// --------------------------------------------------------------
// 3. 启动 ADC 转换（软件触发）
// --------------------------------------------------------------
// 在连续模式下，调用一次 Start，ADC 就会一直转下去
HAL_ADC_Start(&hadc1);
printf("ADC Continuous Conversion Running.\r\n");

// 使能 DMA 中断（冗余保护）
HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

printf("System Ready.\r\n");
    /* USER CODE END 2 */

    /* Infinite loop */
    while (1) {
        if (dma_flag == 1) {
            dma_flag = 0;

            // Stop DMA to avoid overwriting data during processing
            HAL_ADC_Stop_DMA(&hadc1);

            // 1. Preprocess
            Process_FFT_Data(ADC_Value);

            // 2. FFT
            arm_rfft_fast_f32(&fft_instance, fft_input, fft_output, 0);

            // 3. Compute magnitude (skip DC, take first half)
            for (int i = 1; i < FFT_SIZE / 2; i++) {
                float real = fft_output[2 * i];
                float imag = fft_output[2 * i + 1];
                magnitude[i - 1] = sqrtf(real * real + imag * imag);
            }

            // 4. Find fundamental and classify
            int peak = Find_Fundamental(magnitude, FFT_SIZE / 2);
            WaveType type = Classify_Waveform(magnitude, FFT_SIZE / 2, peak);

            // 5. Output result
            const char *wave_name[] = {"Unknown", "Sine", "Square", "Triangle", "AM", "FM"};
            printf("Wave: %-8s  PeakIdx:%2d  Mag:%.1f\r\n",
                   wave_name[type], peak, magnitude[peak]);

            // 6. Restart DMA for next frame
            HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC_Value, FFT_SIZE);
        }

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

// ------------------------------------------------------------------
// System Clock Configuration (generated by CubeMX, kept as is)
// ------------------------------------------------------------------
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

// ------------------------------------------------------------------
// DMA Transfer Complete Callback (called by HAL when buffer is full)
// ------------------------------------------------------------------
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    // Optionally toggle a pin to verify interrupt entry
    // HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    dma_flag = 1;
}

// ------------------------------------------------------------------
// Retarget printf to USART2
// ------------------------------------------------------------------
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

int fputc(int ch, FILE *f) {
    return __io_putchar(ch);
}

/* USER CODE END 4 */

// ------------------------------------------------------------------
// Error Handler
// ------------------------------------------------------------------
void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

```