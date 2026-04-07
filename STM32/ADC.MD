### ADC
#### ADC是什么？
* ADC 是Analog to Digital Converter，模拟转数字。
* 模拟信号：电压是连续变化的，例如0V～3.3V。
* 数字信号：MCU只能识别0和1，我们要把模拟信号转换成数字值。
* 举例
> 电位器电压为1.65V，STM32ADC可以把它转成一个介于0-4095的值。如果分辨率是12位，电压=(ADC值/4095)X3.3V。

#### ADC单通道采集电位器应用（轮询）
>ADC轮询读取HAL库函数
>1. HAL_ADC_Start(&hadc1);
作用：启动ADC的转换过程
>2. HAL_ADCEx_Calibration_Start(&hadc1);
作用：启动ADC校准，提升采样精度
>3. HAL_ADC_PollForConversion(&hadc1, 50);
作用：等待ADC转换完成
>4. HAL_ADC_GetValue(&hadc1);
作用：读取ADC转换结果
```c
// 1. 校准（只需一次，通常在初始化后）
HAL_ADCEx_Calibration_Start(&hadc1);

// 2. 启动 ADC 转换
HAL_ADC_Start(&hadc1);

// 3. 等待转换完成（超时 50ms）
HAL_ADC_PollForConversion(&hadc1, 50);

// 4. 读取结果
uint32_t adc_value = HAL_ADC_GetValue(&hadc1);
```
##### start
