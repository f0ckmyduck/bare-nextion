/** @file
 *  @brief Contains some uart hardware abstraction functions.
*/

#include <limits.h>

#include "stm32f1xx.h"

#include "queue.h"
#include "uart.h"

struct queue g_uart_input;

void USART1_IRQHandler()
{
	/* if some error occurred, discard the rx byte*/
	if (USART1->SR &
	    (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
		return;
	}

	/* check if the receive register is not empty */
	if (USART1->SR & USART_SR_RXNE) {
		queue_append_char(&g_uart_input, USART1->DR);
	}
}

void uart_init(const uint_fast32_t baud, const uint_fast32_t sysclock)
{
	/* reset queues */
	queue_reset(&g_uart_input);

	/* enable clock for usart1 */
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

	/* set PA10 input floating and PA9 pushpull */
	GPIOA->CRH = (GPIOA->CRH & ~0x00000ff0) | 0x000004b0;

	/* receive / transmit interrupt enable, receive / transmit enable */
	USART1->CR1 = USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE;

	/* reset control register 2 & 3 to make sure nothing set the registers */
	USART1->CR2 = 0x0000;
	USART1->CR3 = 0x0000;

	/* set baud like the example 8Mhz / 16 * 9600b/s = 52.08333 */
	USART1->BRR = (sysclock / (16 * baud)) << 4 | 0xc;

	/* Enable IRQ */
	NVIC_SetPriority(USART1_IRQn, 1);
	NVIC_ClearPendingIRQ(USART1_IRQn);
	NVIC_EnableIRQ(USART1_IRQn);

	/* enable uart1 unit */
	USART1->CR1 |= USART_CR1_UE;
}

inline void uart_send_char(const char out)
{
	// wait till data transmit register empty
	while (!(USART1->SR & USART_SR_TXE)) {
	}
	USART1->DR = out;

	// wait for transmission complete
	while (!(USART1->SR & USART_SR_TC)) {
	}
}

inline void uart_send_str(const char out[], size_t size)
{
	size_t i = 0;

	while (out[i] != '\0' && i < size) {
		uart_send_char(out[i]);
		i++;
	}
}
