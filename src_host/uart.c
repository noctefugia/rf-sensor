#include "uart.h"

struct uart_struct sys_uart = {0};

void UART_Init(uint32_t bdr, uint16_t upd_period, uint16_t max_time, void (*rcv_evt)(uint8_t, uint8_t*))
{
	GPIO_Init(PORT_UART_TX, PIN_UART_TX, GPIO_MODE_OUT_PP_HIGH_FAST);
	GPIO_Init(PORT_UART_RX, PIN_UART_RX, GPIO_MODE_IN_PU_NO_IT);
		 
	UART1_DeInit();
	UART1_Init(bdr, UART1_WORDLENGTH_8D,
		UART1_STOPBITS_1, UART1_PARITY_NO,
		UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE
	);
	UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);
	UART1_ITConfig(UART1_IT_TXE, ENABLE);
	
	UART_ResetBufferRX();
	UART_ResetBufferTX();
	sys_uart.update_period = upd_period;
	sys_uart.max_rxtx_time = (uint8_t)(max_time / upd_period);
	sys_uart.rx_event = rcv_evt;
	sys_uart.init = TRUE;
}


void UART_HandlerRX(void)
{
	uint8_t rx_data;
	
	rx_data = UART1_ReceiveData8();
	UART1_ClearITPendingBit(UART1_IT_RXNE); 
	
	//disable RX handler while TX
	if ((!sys_uart.init) || (!UART_TXidle()))
		return;
		
	if (sys_uart.rx_msg_size == 0) {
		if (rx_data > UART_RX_BUFF_SZ) {
			UART_ResetBufferRX();
		} else {
			sys_uart.rx_msg_size = (uint8_t)(rx_data + 2); //add checksum bytes
			sys_uart.rx_buffer_pos = 0;
		}
	} 
	
	else {
		sys_uart.rx_buffer[sys_uart.rx_buffer_pos] = rx_data;
		++sys_uart.rx_buffer_pos;
		if (sys_uart.rx_buffer_pos >= UART_RX_BUFF_SZ)
			UART_ResetBufferRX();
		else if (sys_uart.rx_buffer_pos >= sys_uart.rx_msg_size)
			UART_ProcessRX();
	}
}


void UART_HandlerTX(void)
{
	uint8_t tx_data;
	
	if ((!sys_uart.init) || (sys_uart.tx_msg_size == 0)) {
		UART1_ITConfig(UART1_IT_TXE, DISABLE);
		return;
	}
		
	if ((sys_uart.tx_buffer_pos >= sys_uart.tx_msg_size) || (sys_uart.tx_buffer_pos >= UART_TX_BUFF_SZ)) {
		UART_ResetBufferTX();
	} else {
		if (!sys_uart.tx_start) {
			tx_data = (uint8_t)(sys_uart.tx_msg_size - 2);
			sys_uart.tx_start = TRUE;
		} else {
			tx_data = sys_uart.tx_buffer[sys_uart.tx_buffer_pos];
			++sys_uart.tx_buffer_pos;
		}
		UART1_SendData8(tx_data);
	}
}


void UART_ResetBufferRX(void)
{
	sys_uart.rx_buffer_pos = 0;
	sys_uart.rx_msg_size = 0;
	sys_uart.rx_time = 0;
}


void UART_ResetBufferTX(void)
{
	sys_uart.tx_buffer_pos = 0;
	sys_uart.tx_msg_size = 0;
	sys_uart.tx_start = FALSE;
	sys_uart.tx_time = 0;
}


void UART_Send(uint8_t msg_sz, uint8_t *msg_buff)
{
	uint8_t i;
	uint16_t crc;
	
	assert_param(IS_UART_TX_BUFF_SZ_OK(msg_sz));

	if ((!sys_uart.init) || (sys_uart.tx_msg_size > 0))
		return;
		
	UART_ResetBufferTX();
	sys_uart.tx_msg_size = msg_sz; //first byte
	
	crc = 0;
	CRC16(&crc, msg_sz); //compute crc from all data
	for (i = 0; i < msg_sz; ++i) { //fill buffer with packet data
		sys_uart.tx_buffer[i] = msg_buff[i];
		CRC16(&crc, msg_buff[i]);
	}
	//add checksum to the end of buffer
	sys_uart.tx_buffer[msg_sz] = BYTE_H(crc);
	sys_uart.tx_buffer[msg_sz+1] = BYTE_L(crc);
	sys_uart.tx_msg_size += 2;
	
	UART1_ITConfig(UART1_IT_TXE, ENABLE);
}


void UART_ProcessRX(void)
{
	uint8_t i;
	uint16_t crc1, crc2;
	
	if (!sys_uart.init)
		return;
		
	if (sys_uart.rx_msg_size > 2) {
		sys_uart.rx_msg_size -= 2;
		crc1 = 0;
		CRC16(&crc1, sys_uart.rx_msg_size);
		for (i = 0; i < sys_uart.rx_msg_size; ++i)
			CRC16(&crc1, sys_uart.rx_buffer[i]);

		crc2 = WORD_HL(sys_uart.rx_buffer[sys_uart.rx_msg_size], sys_uart.rx_buffer[sys_uart.rx_msg_size+1]);
		if (crc1 == crc2)
			(*sys_uart.rx_event)(sys_uart.rx_msg_size, sys_uart.rx_buffer);
	}
	
	UART_ResetBufferRX();
}


UART_ERROR_TypeDef UART_Update(void)
{
	if (!sys_uart.init)
		return UART_ERR_NONE;
		
	if ( (sys_uart.rx_msg_size > 0) && ((sys_uart.rx_time++) > sys_uart.max_rxtx_time) ) {
		UART_ResetBufferRX();
		return UART_ERR_RX_RESET;
	} else if ( (sys_uart.tx_msg_size > 0) && ((sys_uart.tx_time++) > sys_uart.max_rxtx_time) ) {
		UART_ResetBufferTX();
		return UART_ERR_TX_RESET;
	} else {
		return UART_ERR_NONE;
	}
}


bool UART_RXidle(void)
{	
	return (bool)((sys_uart.rx_msg_size == 0) ? TRUE: FALSE);
}


bool UART_TXidle(void)
{
	return (bool)((sys_uart.tx_msg_size == 0) ? TRUE : FALSE);
}
