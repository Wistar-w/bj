/* 不要玩卡拉比丘喵  */
## 1.FFT的基础参数喵
```c
/*假如有喵*/
#define FFT_SIZE     256//决定精细度喵
#define SAMPLE_RATE  1028//ADC 每秒采集多少个数据点,这里ADC每秒采集1028个数据点喵  
```
#### Δf=Fs/N，所以喵Δf=1028/256=4HZ。fk​=kΔf
```c
float Get_Freq(int index)
{
    return index * SAMPLE_RATE / FFT_SIZE;//如果index=32,所以f=32*1028/256=4HZ，信号里一定有4Hz喵
}
```
## 2.FFT的幅值计算喵
![alt text](image.png)
>这就是镜像，偶实奇虚喵	​
```c
for(int i=0;i<FFT_SIZE/2;i++)
{
    float real=fft_output[2*i];
    float imag=fft_output[2*i+1];
    mag[i]=sqrt(real*real+imag*imag);
}
```
## 3.找最大峰喵（载波），基波（除去DC影响）喵
>除去0Hz（除去DC），DC是平均电压喵
```c
#define start 3//丢弃有区别的，建议问一下喵
int FindPeak(float *mag)
{
    int index=start;
    float max=mag[start];
    for(int i=start+1;i<FFT_SIZE/2;i++)
    {
        if(mag[i]>max)//到最大峰暂停喵
        {
            max=mag[i];
            index=i;
        }
    }
    return index;
}
```
## 正弦/方波/三角波/AM/PM喵
### 看谐波喵
比如基波1000，三次谐波为3000；基波为f0喵
ratio=mag[3*f0]/mag[f0]；//对应频域图的值，然后计算ratio喵
#### 正弦喵
![alt text](image-1.png)
A3​/A1​≈0
正弦的判断为ratio<0.05 喵
```c
if(ratio<0.05)
{
    return WAVE_SINE;
}

```
#### 方波喵
![alt text](image-2.png)
只有奇次谐波喵：1f0​,3f0​,5f0​
An​=1/n;A3​/A1​=1/3
```c
if(ratio>0.2 &&
   ratio<0.45)
{
    return WAVE_SQUARE;
}
```
#### 三角波喵
![alt text](image-3.png)
An​=1/n*n;A3​/A1​=1/9
```c
if(ratio>0.05 &&
   ratio<0.18)
{
    return WAVE_TRIANGLE;
}
```
#### AM喵
特点：有中心峰，两侧对称，只有少量边带喵
//if(mag[carrier_bin-i] > mag[carrier_bin]*0.05)
**里面的 0.05 不是 FFT 的公式，而是一个人为设定的阈值喵。
如果旁边的频率峰值大于载波峰值的 5%，认为它是一个有效边带喵。**
```c
int left=0，right=0;
for(int i=1;i<FFT_SIZE/2;i++)
{
    if(mag[carrier_bin-i] > mag[carrier_bin]*0.05)
    {
        left++;
    }
    if(mag[carrier_bin+i] > mag[carrier_bin]*0.05)  
    {
        right++;
    }
}
if(left==right&&left<=3)//3对应超过阈值的bin数量喵
{
    return AM;
}
```
#### FM喵
特点：多个边带，等间距喵
```c
/* 寻找附近峰喵 */
int peak_count=0;
//对于弱FM信号（β较小），可能需要降低阈值（如0.05）喵
//对于强FM信号（β较大），可以适当提高阈值（如0.2）喵
for(int i=carrier_bin-20;
    i<carrier_bin+20;
    i++)
{

if(mag[i]>
carrier_mag*0.1)

peak_count++;

}
/* 判断喵 */
if(peak_count>5)//可变喵
{
   return FM;
}
```
### 用vofa发送FFT喵
```c
//错误的发送喵
printf("%f\r\n",mag[i]);


//需要的是频率,幅值喵
/* 
0,10
46,12
93,15
140,20
...

VOFA才能画喵：

 ^
 |
 |             *
 |             |
 |       *     |
 |_______|_____|_________
 0       46    93    140Hz

*/
printf("%f,%f\r\n",Get_Freq(i),magnitude[i],time_data[i]);
//float adc_value = (float)src[i] - mean;
//float time_data[FFT_SIZE];
// time_data[i] =adc_value * 3.3f / 4096.0f;
```
