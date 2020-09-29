#include <string.h>
#include "stm32f1xx.h"

UART_HandleTypeDef uart;
uint8_t valueBuff[5];

void send_string(char* s)
{
	HAL_UART_Transmit(&uart, (uint8_t*)s, strlen(s), 1000);
}

int main(void)
{
	SystemCoreClock = 8000000;	// taktowanie 8Mhz
	HAL_Init();

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_USART2_CLK_ENABLE();

	GPIO_InitTypeDef gpio;
	gpio.Mode = GPIO_MODE_AF_PP;
	gpio.Pin = GPIO_PIN_2;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &gpio);

	gpio.Mode = GPIO_MODE_AF_INPUT;
	gpio.Pin = GPIO_PIN_3;
	HAL_GPIO_Init(GPIOA, &gpio);

	uart.Instance = USART2;
	uart.Init.BaudRate = 115200;
	uart.Init.WordLength = UART_WORDLENGTH_8B;
	uart.Init.Parity = UART_PARITY_NONE;
	uart.Init.StopBits = UART_STOPBITS_1;
	uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	uart.Init.OverSampling = UART_OVERSAMPLING_16;
	uart.Init.Mode = UART_MODE_TX_RX;
	HAL_UART_Init(&uart);

	while(1)
			{
				int i = 0;
				while (i < 5 )
				{
					if (__HAL_UART_GET_FLAG(&uart, UART_FLAG_RXNE) == SET)
					{
						uint8_t value;
						HAL_UART_Receive(&uart, &value, 1, 100);
						valueBuff[i++] = value;
					}
				}

				HAL_UART_Transmit(&uart, (uint8_t*)valueBuff, 5, 1000);
				send_string("\r\n");
			}
}


