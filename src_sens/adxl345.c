#include "adxl345.h"


I2CS_STATE_TypeDef ADXL345_Init(void)
{
	I2CS_STATE_TypeDef ack;
	uint8_t data;
	
	ack = I2CS_WriteRegister(ADXL345_I2C_PAIR, ADXL345_I2C_ADDRESS, ADXLR_POWER_CTL,
		ADXLRBPC_MEASURE);
	I2CS_CHECK_ACK(ack, ADXL345_I2C_PAIR);
	
	ack = I2CS_WriteRegister(ADXL345_I2C_PAIR, ADXL345_I2C_ADDRESS, ADXLR_DATA_FORMAT,
		ADXLRBDF_RANGE_4G);
	I2CS_CHECK_ACK(ack, ADXL345_I2C_PAIR);
	
	ack = I2CS_WriteRegister(ADXL345_I2C_PAIR, ADXL345_I2C_ADDRESS, ADXLR_BW_RATE,
		ADXLRBBWR_LOW_RATE_25HZ);
	I2CS_CHECK_ACK(ack, ADXL345_I2C_PAIR);
	
	return ack;
}


I2CS_STATE_TypeDef ADXL345_SoftReset(void)
{
	I2CS_STATE_TypeDef ack;
	
	ack = I2CS_WriteRegister(ADXL345_I2C_PAIR, ADXL345_I2C_ADDRESS, ADXLR_POWER_CTL, 0);
	I2CS_CHECK_ACK(ack, ADXL345_I2C_PAIR);
	
	return ack;
}


I2CS_STATE_TypeDef ADXL345_ReadData(int16_t *buff)
{
	I2CS_STATE_TypeDef ack;
	uint8_t i, j, temp_buff[ADXL345_COMPONENT_SZ];
	
	for (i = 0; i < ADXL345_COMPONENT_COUNT; ++i) {
		for (j = 0; j < ADXL345_COMPONENT_SZ; ++j) {
			ack = I2CS_ReadRegister(ADXL345_I2C_PAIR, ADXL345_I2C_ADDRESS, ADXLR_DATAX0 + i*ADXL345_COMPONENT_SZ + j, &temp_buff[j]);
			I2CS_CHECK_ACK(ack, ADXL345_I2C_PAIR);
		}
		buff[i] = (int16_t)((((uint16_t)temp_buff[1]) << 8) | temp_buff[0]);
	}
	
	return ack;
}


I2CS_STATE_TypeDef ADXL345_ReadDataRaw(uint8_t *buff)
{
	I2CS_STATE_TypeDef ack;
	uint8_t i;
	
	for (i = 0; i < ADXL345_RAW_BYTE_COUNT; ++i) {
		ack = I2CS_ReadRegister(ADXL345_I2C_PAIR, ADXL345_I2C_ADDRESS, ADXLR_DATAX0 + i, &buff[i]);
		I2CS_CHECK_ACK(ack, ADXL345_I2C_PAIR);
	}
	
	return ack;
}