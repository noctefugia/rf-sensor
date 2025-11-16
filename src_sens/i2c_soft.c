#include "I2C_soft.h"

struct i2cs_struct sys_i2cs = {0};


void I2CS_Sleep(uint16_t t) {
    while(--t);
}


void I2CS_Init(void) {
	I2CS_PAIR_TypeDef i;
	
	AssignIO(&sys_i2cs.pair[I2CSP_MAIN].io_sda, PORT_MCU_SDA, PIN_MCU_SDA, TRUE, GPIO_MODE_OUT_OD_HIZ_SLOW);
	AssignIO(&sys_i2cs.pair[I2CSP_MAIN].io_scl, PORT_MCU_SCL, PIN_MCU_SCL, TRUE, GPIO_MODE_OUT_OD_HIZ_SLOW);

	sys_i2cs.init = TRUE;
	for(i = 0; i < I2CSP_COUNT; ++i)
		I2CS_Stop(i);
	
	I2CS_Sleep(I2CS_DELAY);	
	I2CS_ResetAll();
}


void I2CS_Start(I2CS_PAIR_TypeDef pair_no) {
	assert_param(IS_I2CS_PAIR_OK(pair_no));
	
	if (!sys_i2cs.init)
		return;
		
	WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_High);
	I2CS_Sleep(I2CS_DELAY);
	WriteIO(&sys_i2cs.pair[pair_no].io_sda, IO_Low);
	I2CS_Sleep(I2CS_DELAY);
}


void I2CS_Stop(I2CS_PAIR_TypeDef pair_no) {
	assert_param(IS_I2CS_PAIR_OK(pair_no));
	
	if (!sys_i2cs.init)
		return;
		
	WriteIO(&sys_i2cs.pair[pair_no].io_sda, IO_Low);
	I2CS_Sleep(I2CS_DELAY);
	WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_High);
	I2CS_Sleep(I2CS_DELAY);
	WriteIO(&sys_i2cs.pair[pair_no].io_sda, IO_High);
	I2CS_Sleep(I2CS_DELAY);
}


void I2CS_Reset(I2CS_PAIR_TypeDef pair_no)
{  
	uint8_t i;
	
	assert_param(IS_I2CS_PAIR_OK(pair_no));
	
	ModeIO(&sys_i2cs.pair[pair_no].io_sda, GPIO_MODE_OUT_OD_HIZ_SLOW);
	ModeIO(&sys_i2cs.pair[pair_no].io_scl, GPIO_MODE_OUT_OD_HIZ_SLOW);
	I2CS_Stop(pair_no);
	
	WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_Low);
	I2CS_Sleep(I2CS_DELAY);
	I2CS_Start(pair_no);
	WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_Low);
	I2CS_Sleep(I2CS_DELAY);
	
	for (i = 0; i<9; ++i) {
		WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_High);
		I2CS_Sleep(I2CS_DELAY);
		WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_Low);
		I2CS_Sleep(I2CS_DELAY);
	}
	
	I2CS_Start(pair_no);
	WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_Low);
	I2CS_Sleep(I2CS_DELAY);
	I2CS_Stop(pair_no);
} 


void I2CS_ResetAll(void)
{
	I2CS_PAIR_TypeDef i;
	
	for (i = 0; i < I2CSP_COUNT; ++i)
		I2CS_Reset(i);
}


I2CS_STATE_TypeDef I2CS_WriteByte(I2CS_PAIR_TypeDef pair_no, uint8_t data)
{
	uint8_t i;
	I2CS_STATE_TypeDef ack;
	
	assert_param(IS_I2CS_PAIR_OK(pair_no));
	
	if (!sys_i2cs.init)
		return I2CSS_ERROR;
		
	for (i = 0; i<8; ++i){
		WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_Low);
		I2CS_Sleep(I2CS_DELAY);
		if (((data << i) & 0x80) == 0)
			WriteIO(&sys_i2cs.pair[pair_no].io_sda, IO_Low);
		else
			WriteIO(&sys_i2cs.pair[pair_no].io_sda, IO_High);
		I2CS_Sleep(I2CS_DELAY);
		WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_High);
		I2CS_Sleep(I2CS_DELAY);
	}
	
	WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_Low);
	I2CS_Sleep(I2CS_DELAY);
		
	ModeIO(&sys_i2cs.pair[pair_no].io_sda, GPIO_MODE_IN_FL_NO_IT);
	I2CS_Sleep(I2CS_DELAY);
	WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_High);
	I2CS_Sleep(I2CS_DELAY);
	
	ReadIO(&sys_i2cs.pair[pair_no].io_sda, &ack);
	ModeIO(&sys_i2cs.pair[pair_no].io_sda, GPIO_MODE_OUT_OD_HIZ_SLOW);
	
	WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_Low);
	I2CS_Sleep(I2CS_DELAY);
	
	return ack;
}


uint8_t I2CS_ReadByte(I2CS_PAIR_TypeDef pair_no, I2CS_STATE_TypeDef ack)
{
	uint8_t i, data;
	bool state;
	GPIO_Mode_TypeDef io_mode;
	
	assert_param(IS_I2CS_PAIR_OK(pair_no));
	
	if (!sys_i2cs.init)
		return I2CSS_ERROR;
		
	data = 0;
	ModeIO(&sys_i2cs.pair[pair_no].io_sda, GPIO_MODE_IN_FL_NO_IT);
	I2CS_Sleep(I2CS_DELAY);
	for (i = 0; i<8; ++i){
		WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_Low);
		I2CS_Sleep(I2CS_DELAY);
		WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_High);
		I2CS_Sleep(I2CS_DELAY);
		data = (uint8_t)(data << 1);
		
		ReadIO(&sys_i2cs.pair[pair_no].io_sda, &state);
		if (state)
			data = (uint8_t)(data + 1);
	}
	
	WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_Low);
	I2CS_Sleep(I2CS_DELAY);
	
	if (ack == I2CSS_ACK)
		io_mode = GPIO_MODE_OUT_OD_LOW_SLOW;
	else
		io_mode = GPIO_MODE_OUT_OD_HIZ_SLOW;
	ModeIO(&sys_i2cs.pair[pair_no].io_sda, io_mode);
	
	I2CS_Sleep(I2CS_DELAY);
	WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_High);
	I2CS_Sleep(I2CS_DELAY);
	WriteIO(&sys_i2cs.pair[pair_no].io_scl, IO_Low);
	I2CS_Sleep(I2CS_DELAY);
	
	return data;
}


I2CS_STATE_TypeDef I2CS_ReadRegister(I2CS_PAIR_TypeDef pair_no, uint8_t addr, uint8_t reg_no, uint8_t *value)
{         
	I2CS_STATE_TypeDef ack;
	
	*value = 0;
		
	I2CS_Start(pair_no);
	ack = I2CS_WriteByte(pair_no, (uint8_t)(addr | I2CS_WRITE_FLAG));
	I2CS_CHECK_ACK(ack, pair_no);
	ack = I2CS_WriteByte(pair_no, (uint8_t)reg_no);
	I2CS_CHECK_ACK(ack, pair_no);
	
	I2CS_Start(pair_no);
	ack = I2CS_WriteByte(pair_no, (uint8_t)(addr | I2CS_READ_FLAG));
	I2CS_CHECK_ACK(ack, pair_no);
	*value = I2CS_ReadByte(pair_no, I2CSS_NO_ACK);
	
	I2CS_Stop(pair_no);

	return ack;
}	


I2CS_STATE_TypeDef I2CS_WriteRegister(I2CS_PAIR_TypeDef pair_no, uint8_t addr, uint8_t reg_no, uint8_t value)
{         
	I2CS_STATE_TypeDef ack;
		
	I2CS_Start(pair_no);
	ack = I2CS_WriteByte(pair_no, (uint8_t)(addr | I2CS_WRITE_FLAG));
	I2CS_CHECK_ACK(ack, pair_no);
	ack = I2CS_WriteByte(pair_no, (uint8_t)reg_no);
	I2CS_CHECK_ACK(ack, pair_no);
	
	ack = I2CS_WriteByte(pair_no, (uint8_t)value);
	//I2CS_CHECK_ACK(ack, pair_no); no need due to external checking 
	
	I2CS_Stop(pair_no);

	return ack;
}	