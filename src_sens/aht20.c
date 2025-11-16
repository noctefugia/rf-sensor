#include "aht20.h"

struct aht20_struct sys_aht20 = {0};


void AHT20_Reset(void)
{   
	sys_aht20.data_ready = FALSE;
}


I2CS_STATE_TypeDef AHT20_GetStatus(uint8_t *status)
{         
	I2CS_STATE_TypeDef ack;
	
	*status = 0;
		
	I2CS_Start(AHT20_I2C_PAIR);
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_WRITE_FLAG));
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)AHT20C_STATUS_WORD);
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	
	I2CS_Start(AHT20_I2C_PAIR);
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_READ_FLAG));
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	
	*status = I2CS_ReadByte(AHT20_I2C_PAIR, I2CSS_NO_ACK);
	
	I2CS_Stop(AHT20_I2C_PAIR);

	return ack;
}	


I2CS_STATE_TypeDef AHT20_Calibrate(void)
{ 
	I2CS_STATE_TypeDef ack;
			
	I2CS_Start(AHT20_I2C_PAIR);
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_WRITE_FLAG));
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)AHT20C_INIT);
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)AHT20CP_INIT1);
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)AHT20CP_INIT2);
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	
	I2CS_Stop(AHT20_I2C_PAIR);
	
	return ack;
}


I2CS_STATE_TypeDef AHT20_TriggerMeasurement(void)
{ 
	I2CS_STATE_TypeDef ack;
		
	I2CS_Start(AHT20_I2C_PAIR);
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_WRITE_FLAG));
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)AHT20C_TRIG_MEASUREMENT);
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)AHT20CP_TRIG_MEASUREMENT1);
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)AHT20CP_TRIG_MEASUREMENT2);
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	
	I2CS_Stop(AHT20_I2C_PAIR);
	
	return ack;
}


I2CS_STATE_TypeDef AHT20_SoftReset(void)
{ 
	I2CS_STATE_TypeDef ack;
		
	I2CS_Start(AHT20_I2C_PAIR);
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_WRITE_FLAG));
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)AHT20C_TRIG_MEASUREMENT);
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	
	I2CS_Stop(AHT20_I2C_PAIR);
	
	return ack;
}


I2CS_STATE_TypeDef AHT20_ReadData(uint8_t *data_buff)
{
	uint8_t i, data, n;
	I2CS_STATE_TypeDef ack, ack_read;
	
	I2CS_Start(AHT20_I2C_PAIR);
	ack = I2CS_WriteByte(AHT20_I2C_PAIR, (uint8_t)(AHT20_I2C_ADDRESS | AHT20_I2C_READ_FLAG));
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
	
	for (i = 0; i < AHT20_MEASUREMENT_BYTE_COUNT; ++i) {
		ack_read = (I2CS_STATE_TypeDef)((i == ((uint8_t)(AHT20_MEASUREMENT_BYTE_COUNT - 1))) ? I2CSS_NO_ACK : I2CSS_ACK);
		data_buff[i] = I2CS_ReadByte(AHT20_I2C_PAIR, ack_read);
	}
	
	I2CS_Stop(AHT20_I2C_PAIR);

	return ack;
}


//first read after power-on can give wrong data
I2CS_STATE_TypeDef AHT20_GetData(bool *ready_flag, float *temp, float *hmdt)
{ 
	I2CS_STATE_TypeDef ack;
	uint8_t reg_val, data_buff[AHT20_MEASUREMENT_BYTE_COUNT];
	uint32_t temp_raw, hmdt_raw;
	
	*ready_flag = FALSE;
	/*ovewrite only if data ok
	*hmdt = 0x00000000;
	*temp = 0x00000000;
	*/
		
	ack = AHT20_GetStatus(&reg_val);
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);

	//if sensor is enabled & not busy - read data & start new measurement
	if ((reg_val & AHT20_BIT_CAL_EN) && !(reg_val & AHT20_BIT_BUSY)) {
		if (sys_aht20.data_ready) {
			ack = AHT20_ReadData(data_buff);
			I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
			if (ack == I2CSS_ACK) {
				hmdt_raw = (uint32_t)(DWORD_HML(data_buff[1], data_buff[2], data_buff[3]) >> 4);
				temp_raw = DWORD_HML((data_buff[3] & 0x0F), data_buff[4], data_buff[5]);
				*hmdt = (float)(((float)hmdt_raw) * 0.000095f);
				*temp = (float)(((float)temp_raw) * 0.000191f - 50.0f);
				*ready_flag = TRUE;
			}
		}
		ack = AHT20_TriggerMeasurement(); //start new measurement
		I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
		sys_aht20.data_ready = TRUE;
	}
	
	return ack;
}


//first read after power-on can give wrong data
I2CS_STATE_TypeDef AHT20_GetDataRaw(bool *ready_flag, uint8_t *data_buff)
{ 
	I2CS_STATE_TypeDef ack;
	uint8_t reg_val;
	
	*ready_flag = FALSE;
		
	ack = AHT20_GetStatus(&reg_val);
	I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);

	//if sensor is enabled & not busy - read data & start new measurement
	if ((reg_val & AHT20_BIT_CAL_EN) && !(reg_val & AHT20_BIT_BUSY)) {
		if (sys_aht20.data_ready) {
			ack = AHT20_ReadData(data_buff);
			I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
			if (ack == I2CSS_ACK)
				*ready_flag = TRUE;
		}
		ack = AHT20_TriggerMeasurement(); //start new measurement
		I2CS_CHECK_ACK(ack, AHT20_I2C_PAIR);
		sys_aht20.data_ready = TRUE;
	}
	
	return ack;
}
