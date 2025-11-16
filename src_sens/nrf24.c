#include "nrf24.h"

extern struct spiss_struct sys_spis;
struct nrf24_struct sys_nrf24 = {0};


void NRF24_Init(uint8_t channel, uint8_t pay_len, uint8_t *tx_addr, uint8_t *rx_addr)
{
	uint8_t tttt;
	
	AssignIO(&sys_nrf24.io_ce_rxtx, PORT_NRF24_CE_RXTX, PIN_NRF24_CE_RXTX, TRUE, GPIO_MODE_OUT_PP_LOW_FAST);
	WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_High);
	
	NRF24_Config(channel, pay_len);
	NRF24_SetTxAddress(tx_addr);
	NRF24_SetRxAddress(rx_addr);
}


void	NRF24_Config(uint8_t channel, uint8_t pay_len)
{
	sys_nrf24.payload_len = pay_len;
	
	// Set RF channel
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_RF_CH, channel);
	
	// Set length of incoming payload 
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_RX_PW_P0, 0x00); // Auto-ACK pipe ...
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_RX_PW_P1, pay_len); // Data payload pipe
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_RX_PW_P2, 0x00); // Pipe not used 
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_RX_PW_P3, 0x00); // Pipe not used 
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_RX_PW_P4, 0x00); // Pipe not used 
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_RX_PW_P5, 0x00); // Pipe not used 
	
	// 1 Mbps, TX gain: 0dbm
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_RF_SETUP, (0<<NRF24_RF_DR)|((0x03)<<NRF24_RF_PWR));

	// CRC enable, 2 byte CRC length
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_CONFIG, NRF24_CUR_CONFIG);

	// Auto Acknowledgment
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_EN_AA, (1<<NRF24_ENAA_P0)|(1<<NRF24_ENAA_P1)|(0<<NRF24_ENAA_P2)|(0<<NRF24_ENAA_P3)|(0<<NRF24_ENAA_P4)|(0<<NRF24_ENAA_P5));

	// Enable RX addresses
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_EN_RXADDR,(1<<NRF24_ERX_P0)|(1<<NRF24_ERX_P1)|(0<<NRF24_ERX_P2)|(0<<NRF24_ERX_P3)|(0<<NRF24_ERX_P4)|(0<<NRF24_ERX_P5));

	// Auto retransmit delay: 1000 us and Up to 15 retransmit trials
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_SETUP_RETR, (0x04<<NRF24_ARD)|(0x0F<<NRF24_ARC));

	// Dynamic length configurations: No dynamic length
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_DYNPD, (0<<NRF24_DPL_P0)|(0<<NRF24_DPL_P1)|(0<<NRF24_DPL_P2)|(0<<NRF24_DPL_P3)|(0<<NRF24_DPL_P4)|(0<<NRF24_DPL_P5));

	// Start listening
	NRF24_PowerUpRx();
}


/* Set the RX address */
void NRF24_SetRxAddress(uint8_t * addr_buff) 
{
	WriteIO(&sys_nrf24.io_ce_rxtx, IO_Low);
	SPIS_WriteRegister(NRF24_SPISS_ID, NRF24_RX_ADDR_P1, addr_buff, NRF24_CUR_ADDR_LEN);
	WriteIO(&sys_nrf24.io_ce_rxtx, IO_High);
}

/* Returns the payload length */
uint8_t NRF24_GetPayloadLength()
{
	return sys_nrf24.payload_len;
}

/* Set the TX address */
void NRF24_SetTxAddress(uint8_t* addr_buff)
{
	/* RX_ADDR_P0 must be set to the sending addr for auto ack to work. */
	SPIS_WriteRegister(NRF24_SPISS_ID, NRF24_RX_ADDR_P0, addr_buff, NRF24_CUR_ADDR_LEN);
	SPIS_WriteRegister(NRF24_SPISS_ID, NRF24_TX_ADDR, addr_buff, NRF24_CUR_ADDR_LEN);
}


void NRF24_PowerUpRx(void)
{     
	WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_Low);
	SPIS_Transfer(NRF24_FLUSH_RX);
	WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_High);
	
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_STATUS, (1<<NRF24_RX_DR)|(1<<NRF24_TX_DS)|(1<<NRF24_MAX_RT)); 
	
	WriteIO(&sys_nrf24.io_ce_rxtx, IO_Low);
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_CONFIG, NRF24_CUR_CONFIG|((1<<NRF24_PWR_UP)|(1<<NRF24_PRIM_RX)));    
	WriteIO(&sys_nrf24.io_ce_rxtx, IO_High);
}


void NRF24_PowerUpTx(void)
{
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_STATUS, (1<<NRF24_RX_DR)|(1<<NRF24_TX_DS)|(1<<NRF24_MAX_RT)); 

	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_CONFIG, NRF24_CUR_CONFIG|((1<<NRF24_PWR_UP)|(0<<NRF24_PRIM_RX)));
}


void NRF24_PowerDown(void)
{
	WriteIO(&sys_nrf24.io_ce_rxtx, IO_Low);
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_CONFIG, NRF24_CUR_CONFIG);
}


/* Checks if data is available for reading */
/* Returns 1 if data is ready ... */
uint8_t NRF24_DataReady(void) 
{
	// See note in getData() function - just checking RX_DR isn't good enough
	uint8_t status = NRF24_GetStatus();
	
	// We can short circuit on RX_DR, but if it's not set, we still need
	// to check the FIFO for any pending packets
	if ( status & (1 << NRF24_RX_DR) ) 
		return 1;
	
	return ( !NRF24_RxFifoEmpty() );
}


/* Checks if receive FIFO is empty or not */
uint8_t NRF24_RxFifoEmpty(void)
{
	uint8_t fifoStatus;
	
	SPIS_ReadRegister(NRF24_SPISS_ID, NRF24_FIFO_STATUS, &fifoStatus, 1);
	
	return (fifoStatus & (1 << NRF24_RX_EMPTY));
}


/* Returns the length of data waiting in the RX fifo */
uint8_t NRF24_PayloadLengthDynamic(void)
{
	uint8_t status;
	
	WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_Low);
	SPIS_Transfer(NRF24_R_RX_PL_WID);
	status = SPIS_Transfer(0x00);
	WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_High);
	
	return status;
}


/* Reads payload bytes into data array */
void NRF24_GetData(uint8_t* data) 
{
	/* Pull down chip select */
	WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_Low);                              
	
	/* Send cmd to read rx payload */
	SPIS_Transfer(NRF24_R_RX_PAYLOAD);
	
	/* Read payload */
	SPIS_TransferSync(data, data, sys_nrf24.payload_len);
	
	/* Pull up chip select */
	WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_High);
	
	/* Reset status register */
	SPIS_ConfigRegister(NRF24_SPISS_ID, NRF24_STATUS, (1<<NRF24_RX_DR));   
}


/* Returns the number of retransmissions occured for the last message */
uint8_t NRF24_RetransmissionCount(void)
{
	uint8_t rv;
	
	SPIS_ReadRegister(NRF24_SPISS_ID, NRF24_OBSERVE_TX, &rv, 1);
	rv = rv & 0x0F;
	
	return rv;
}


// Sends a data package to the default address. Be sure to send the correct
// amount of bytes as configured as payload on the receiver.
void NRF24_Send(uint8_t* value) 
{    
	/* Go to Standby-I first */
	WriteIO(&sys_nrf24.io_ce_rxtx, IO_Low);
	 
	/* Set to transmitter mode , Power up if needed */
	NRF24_PowerUpTx();
	
	/* Do we really need to flush TX fifo each time ? */
	#if 1
		/* Pull down chip select */
		WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_Low);         
		/* Write cmd to flush transmit FIFO */
		SPIS_Transfer(NRF24_FLUSH_TX);     
		/* Pull up chip select */
		WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_High);                    
	#endif 
	
	/* Pull down chip select */
	WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_Low);
	
	/* Write cmd to write payload */
	SPIS_Transfer(NRF24_W_TX_PAYLOAD);
	
	/* Write payload */
	SPIS_TransmitSync(value, sys_nrf24.payload_len);   
	
	/* Pull up chip select */
	WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_High);
	
	/* Start the transmission */
	WriteIO(&sys_nrf24.io_ce_rxtx, IO_High);   
}


uint8_t NRF24_IsSending(void)
{
	uint8_t status;
	
	/* read the current status */
	status = NRF24_GetStatus();
							
	/* if sending successful (TX_DS) or max retries exceded (MAX_RT). */
	if ( status & ((1 << NRF24_TX_DS) | (1 << NRF24_MAX_RT)) )    
		return 0; /* false */
	
	return 1; /* true */
}

uint8_t NRF24_GetStatus(void)
{
	uint8_t rv;
	
	WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_Low);
	rv = SPIS_Transfer(NRF24_NOP);
	WriteIO(&sys_spis.io_cs[NRF24_SPISS_ID], IO_High);
	
	return rv;
}


NRF24_LMS_TypeDef NRF24_LastMessageStatus(void)
{
	uint8_t rv;
	
	rv = NRF24_GetStatus();
	
	/* Transmission went OK */
	if((rv & ((1 << NRF24_TX_DS))))
		return NRF24_LMS_TRANSMISSON_OK;
		
	/* Maximum retransmission count is reached */
	/* Last message probably went missing ... */
	else if((rv & ((1 << NRF24_MAX_RT))))
		return NRF24_LMS_MESSAGE_LOST; 
		
	/* Probably still sending ... */
	else
		return NRF24_LMS_UNDEFINED;
}