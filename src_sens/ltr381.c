#include "ltr381.h"



I2CS_STATE_TypeDef LTR381_Init(void)
{
	I2CS_STATE_TypeDef ack;
	uint8_t data;
	
	ack = I2CS_ReadRegister(LTR381_I2C_PAIR, LTR381_I2C_ADDRESS, LTRR_MAIN_CTRL, &data);
	I2CS_CHECK_ACK(ack, LTR381_I2C_PAIR);
	
	data = (LTRRBMC_CS_MODE | LTRRBMC_ALS_CS_EN);
	ack = I2CS_WriteRegister(LTR381_I2C_PAIR, LTR381_I2C_ADDRESS, LTRR_MAIN_CTRL, data);
	I2CS_CHECK_ACK(ack, LTR381_I2C_PAIR);
	
	return ack;
}


I2CS_STATE_TypeDef LTR381_SoftReset(void)
{
	I2CS_STATE_TypeDef ack;
	
	I2CS_WriteRegister(LTR381_I2C_PAIR, LTR381_I2C_ADDRESS, LTRR_MAIN_CTRL, LTRRBMC_SW_RESET);
	ack = I2CSS_ACK; //no ack signal after reset?
	//I2CS_CHECK_ACK(ack, LTR381_I2C_PAIR);
	
	return ack;
}


I2CS_STATE_TypeDef LTR381_ReadData(bool *ready_flag, uint32_t *buff)
{
	I2CS_STATE_TypeDef ack;
	uint8_t i, j, n;
	
	*ready_flag = FALSE;
	ack = I2CS_ReadRegister(LTR381_I2C_PAIR, LTR381_I2C_ADDRESS, LTRR_MAIN_STATUS, &i);
	I2CS_CHECK_ACK(ack, LTR381_I2C_PAIR);

	if (i & LTRRBMS_ALS_CS_DATA_STATUS) {
		for (i = 0; i < LTR381_CS_BUFF_SZ; ++i) {
			buff[i] = 0;
			for (j = 0; j < LTR381_CS_BUFF_ELEM_SZ; ++j) {
				I2CS_Sleep(I2CS_DELAY);	
				ack = I2CS_ReadRegister(LTR381_I2C_PAIR, LTR381_I2C_ADDRESS, 
					LTR381_CS_BUFF_BEGIN + i*LTR381_CS_BUFF_ELEM_SZ + j, &n);
				I2CS_CHECK_ACK(ack, LTR381_I2C_PAIR);
				buff[i] |= (uint32_t)(((uint32_t)n) << (8*j));
			}
		}
		*ready_flag = TRUE;
	}
	
	return ack;
}


I2CS_STATE_TypeDef LTR381_ReadDataRaw(bool *ready_flag, uint8_t *buff)
{
	I2CS_STATE_TypeDef ack;
	uint8_t i;
	
	*ready_flag = FALSE;
	ack = I2CS_ReadRegister(LTR381_I2C_PAIR, LTR381_I2C_ADDRESS, LTRR_MAIN_STATUS, &i);
	I2CS_CHECK_ACK(ack, LTR381_I2C_PAIR);

	if (i & LTRRBMS_ALS_CS_DATA_STATUS) {
		for (i = 0; i < LTR381_CS_RAW_BYTE_COUNT; ++i) {
			ack = I2CS_ReadRegister(LTR381_I2C_PAIR, LTR381_I2C_ADDRESS, LTR381_CS_BUFF_BEGIN + i, &buff[i]);
			I2CS_CHECK_ACK(ack, LTR381_I2C_PAIR);
		}
		*ready_flag = TRUE;
	}
	
	return ack;
}