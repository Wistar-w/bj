#### printf的重定向
```c
/**
 * @brief  printf 重定向：将字符通过 USART1 发送
 * @param  ch  要发送的字符
 * @return     返回发送的字符（符合 fputc 规范）
 */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
int fputc(int ch, FILE *f) {
    return __io_putchar(ch);
}


/**
 *printf的重定向
 * @brief   fputc 函数重定向
 */
int fputc(int ch, FILE *f)//重定向
{
HAL_UART_Transmit(&huart1,(uint8_t *)&ch, 1, HAL_MAX_DELAY);
return ch;
}//记得加stdio.h

```
#### 串口中断接收定长数据
```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
 if(huart -> Instance==USART1)//指向存放基址的变量（基址），判断发生接收中断的串口
 {
	 printf("\r\n开启接收中断\r\n");
	 HAL_UART_Transmit_IT(&huart1,RXbuff,LENGTH);//回显发送数据
	 	HAL_UART_Receive_IT(&huart1,RXbuff,LENGTH);//使能接收中断,继续
}
}
```
#### 串口中断接收不定长数据
```c
/*重写回调函数（有size参数）*/
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  printf("\r\n已经接收到DMA不定长数据，长度为：%d字节，数据如下：",Size);
	 HAL_UART_Transmit_IT(&huart1,RXbuff,Size);//回显发送数据
	 	HAL_UARTEx_ReceiveToIdle_IT(&huart1,RXbuff,LENGTH);//使能接收中断,继续(normal模式下)
}
/*DMA的话就用
HAL_UART_Transmit_DMA
HAL_UART_Receive_DMA
*/
```
#### 