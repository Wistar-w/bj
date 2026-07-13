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
#include "OLED.h"
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
#define FFT_SIZE     2048
#define SAMPLE_RATE  20000    // Hz, adjust to your timer frequency

uint16_t ADC_Value[FFT_SIZE];
uint16_t ADC_Buffer_Copy[FFT_SIZE];//缓存，cpu处理不过来
float fft_input[FFT_SIZE];
float fft_output[FFT_SIZE * 2];
float magnitude[FFT_SIZE / 2];
float hanning_window[FFT_SIZE];
float time_data[FFT_SIZE];
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

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
void Process_FFT_Data(uint16_t *src)
{
    uint32_t sum = 0;


    // 求平均值，去DC
    for(int i=0;i<FFT_SIZE;i++)
    {
        sum += src[i];
    }


    float mean = (float)sum / FFT_SIZE;



    for(int i=0;i<FFT_SIZE;i++)
    {

        // ADC去直流
        float adc_value = (float)src[i] - mean;


        //========================
        // 时域数据(V)
        //========================
        time_data[i] =
        adc_value * 3.3f / 4096.0f;



        //========================
        // FFT输入
        //========================
        fft_input[i] =
        adc_value * hanning_window[i];

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
float Get_Freq(int i) {
    return (float)i * SAMPLE_RATE / FFT_SIZE;
}
// ------------------------------------------------------------------
// 4. Classify waveform based on harmonic ratios
// ------------------------------------------------------------------
WaveType Classify_Waveform(float *mag, int len, int peak_idx) {
    float fundamental = mag[peak_idx];

    // No signal or too weak
    if (fundamental < 1.0f) return WAVE_UNKNOWN;

    // Check FM: two sidebands close to carrier
// 改进后的FM检测
int peak_count=0;
int peaks[20];  // 存储峰位置
int peak_idx_arr = 0;

// 统计边带并存储位置
for(int i=peak_idx-20; i<peak_idx+20; i++) {
    if(i >= 0 && i < len && mag[i] > fundamental*0.1) {
        peaks[peak_idx_arr++] = i;
        peak_count++;
    }
}

if (peak_count > 5) {
    // 检查峰间距
    float avg_distance = 0;
    int distance_count = 0;
    
    for(int i=1; i<peak_idx_arr; i++) {
        float distance = (float)(peaks[i] - peaks[i-1]);
        avg_distance += distance;
        distance_count++;
    }
    
    if(distance_count > 0) {
        avg_distance /= distance_count;
        
        // 检查80%的间距是否在20%容差内
        int valid_distances = 0;
        for(int i=1; i<peak_idx_arr; i++) {
            float distance = (float)(peaks[i] - peaks[i-1]);
            if(fabs(distance - avg_distance) / avg_distance < 0.2f) {
                valid_distances++;
            }
        }
        
        if(valid_distances >= distance_count * 0.8f) {
            return WAVE_FM;
        }
    }
}

    // Check AM: multiple sidebands
// 改进后的AM检测
int left=0,right=0;
float left_freqs[3], right_freqs[3];
int left_count=0, right_count=0;

for(int i=1; i<FFT_SIZE/2; i++) {
    if(peak_idx-i >= 0 && mag[peak_idx-i] > mag[peak_idx]*0.05) {
        left++;
        if(left_count < 3) {
            left_freqs[left_count] = Get_Freq(peak_idx-i);
            left_count++;
        }
    }
    if(peak_idx+i < len && mag[peak_idx+i] > mag[peak_idx]*0.05) {
        right++;
        if(right_count < 3) {
            right_freqs[right_count] = Get_Freq(peak_idx+i);
            right_count++;
        }
    }
}

if(left == right && left <= 3) {
    // 检查边带间距
    float avg_left_distance = 0, avg_right_distance = 0;
    int left_dist_count = 0, right_dist_count = 0;
    
    for(int i=1; i<left_count; i++) {
        float distance = left_freqs[i] - left_freqs[i-1];
        avg_left_distance += distance;
        left_dist_count++;
    }
    if(left_dist_count > 0) avg_left_distance /= left_dist_count;
    
    for(int i=1; i<right_count; i++) {
        float distance = right_freqs[i] - right_freqs[i-1];
        avg_right_distance += distance;
        right_dist_count++;
    }
    if(right_dist_count > 0) avg_right_distance /= right_dist_count;
    
    // 检查左右间距是否大致相等
    if(fabs(avg_left_distance - avg_right_distance) / avg_left_distance < 0.2f) {
        return WAVE_AM;
    }
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

void vofa(int len)
{
		//float voltage = time_data[i] * 3.3f / 4096.0f;
	for(int i=0;i<len;i++)
	printf("%f,%f,%f\r\n",Get_Freq(i),magnitude[i],time_data[i]);
}
void OLED_DrawFFTWave(float *mag,int len)
{

float max=0;


for(int i=1;i<len;i++)
{
    if(mag[i]>max)
        max=mag[i];
}


for(int x=0;x<128;x++)
{

    int index=x*len/128;


    int h=(mag[index]/max)*30;


    OLED_DrawLine(x,63,x,63-h);

}

}
void OLED_DrawTimeWave(float *data,int len)
{

    float max=0;


    for(int i=0;i<len;i++)
    {
        if(fabs(data[i])>max)
            max=fabs(data[i]);
    }


    int last_y=32;


    for(int x=0;x<128;x++)
    {

        int index=x*len/128;


        int y=16-(data[index]/max)*25;


        if(y<0)y=0;
        if(y>63)y=63;


        if(x>0)
        {
            OLED_DrawLine(x-1,last_y,x,y);
        }


        last_y=y;
    }
}
void OLED_ShowTimeFFT(float *time,int time_len,float *fft,int fft_len,int type)
{

OLED_ClearBuffer();

OLED_DrawString(0,0,"TIME");

			 char * wave_name[] = {"Unknown", "Sine", "Square", "Triangle", "AM", "FM"};
			 printf("%-8s",wave_name[type]); 
			  OLED_DrawString(40,0,wave_name[type]);
	
OLED_DrawTimeWave(time,time_len);


OLED_DrawString(0,4,"FFT");


//OLED_DrawFFTWave(fft,fft_len);
OLED_DrawFFTWave(magnitude,FFT_SIZE/2);			 

OLED_Refresh();
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
	
	OLED_Init();
OLED_ClearBuffer();

//OLED_ShowString(0,0,"FFT ");

OLED_Refresh();
  /* USER CODE BEGIN 2 */
    Init_Window();
arm_rfft_fast_init_f32(&fft_instance, FFT_SIZE);
printf("Continuous Conversion Enabled.\r\n");

if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC_Value, FFT_SIZE) != HAL_OK) {
    printf("ADC DMA Start Failed!\r\n");
    while(1);
} else {
    printf("ADC DMA Started.\r\n");
}

HAL_ADC_Start(&hadc1);
printf("ADC Continuous Conversion Running.\r\n");

HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

printf("System Ready.\r\n");
//for(int i=0;i<FFT_SIZE/2;i++)
//{
//	printf("%f,%f\r\n",Get_Freq(i),magnitude[i]);
//}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		
		if (dma_flag) {
        dma_flag = 0;   // 清除标志
			
memcpy(ADC_Buffer_Copy,ADC_Value,sizeof(ADC_Value));//完整复制
			
        // ① 预处理：去直流 + 加窗
        Process_FFT_Data(ADC_Buffer_Copy);

        // ② 执行实数 FFT
        arm_rfft_fast_f32(&fft_instance, fft_input, fft_output, 0);

        // ③ 计算幅度谱（仅前 N/2 个点）
        arm_cmplx_mag_f32(fft_output, magnitude, FFT_SIZE / 2);
				for(int i=0;i<FFT_SIZE/2;i++)
					{
    magnitude[i] = magnitude[i] * 4.0f / FFT_SIZE;
					}
        // ④ 查找基频峰值
        int peak = Find_Fundamental(magnitude, FFT_SIZE / 2);

        // ⑤ 分类波形
        WaveType type = Classify_Waveform(magnitude, FFT_SIZE / 2, peak);

        // ⑥ 输出结果
			 char * wave_name[] = {"Unknown", "Sine", "Square", "Triangle", "AM", "FM"};
			 printf("%-8s",wave_name[type]); 
			  OLED_DrawString(40,0,wave_name[2]);
			 // OLED显示
			 //OLED_DrawString(40,0,wave_name[type]);放外面会被后面覆盖
			 OLED_ShowTimeFFT(time_data,FFT_SIZE,magnitude,FFT_SIZE/2,type);
			 //OLED_Refresh();
			 //vofa
			vofa(FFT_SIZE/2);
	
			
		}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
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

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
```