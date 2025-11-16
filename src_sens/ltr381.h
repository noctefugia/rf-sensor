#ifndef __LTR381_H
#define __LTR381_H

#include "utils.h" 
#include "i2c_soft.h" 

#define LTR381_I2C_PAIR I2CSP_MAIN
#define LTR381_I2C_ADDRESS ((uint8_t)0b10100110)
#define LTR381_I2C_WRITE_FLAG ((uint8_t)0x00)
#define LTR381_I2C_READ_FLAG ((uint8_t)0x01)

typedef enum {
	LTRR_MAIN_CTRL = 0x00,
	LTRR_ALS_CS_MEAS_RATE = 0x04,
	LTRR_ALS_CS_GAIN = 0x05,
	LTRR_PART_ID = 0x06,
	LTRR_MAIN_STATUS = 0x07,
	LTRR_CS_DATA_IR0 = 0x0A,
	LTRR_CS_DATA_IR1 = 0x0B,
	LTRR_CS_DATA_IR2 = 0x0C,
	LTRR_CS_DATA_GREEN_0 = 0x0D,
	LTRR_CS_DATA_GREEN_1 = 0x0E,
	LTRR_CS_DATA_GREEN_2 = 0x0F,
	LTRR_CS_DATA_RED_0 = 0x10,
	LTRR_CS_DATA_RED_1 = 0x11,
	LTRR_CS_DATA_RED_2 = 0x12,
	LTRR_CS_DATA_BLUE_0 = 0x13,
	LTRR_CS_DATA_BLUE_1 = 0x14,
	LTRR_CS_DATA_BLUE_2 = 0x15,
	LTRR_INT_CFG = 0x19,
	LTRR_INT_PST = 0x1A,
	LTRR_THRES_UP_0 = 0x21,
	LTRR_THRES_UP_1 = 0x22,
	LTRR_THRES_UP_2 = 0x23,
	LTRR_THRES_LOW_0 = 0x24,
	LTRR_THRES_LOW_1 = 0x25,
	LTRR_THRES_LOW_2 = 0x26,
	LTRR_INVALID
} LTR381_REG_TypeDef;

#define LTR381_CS_BUFF_SZ ((uint8_t)4)
#define LTR381_CS_BUFF_ELEM_SZ ((uint8_t)3)
#define LTR381_CS_BUFF_BEGIN ((uint8_t)LTRR_CS_DATA_IR0)
#define LTR381_CS_RAW_BYTE_COUNT ((uint8_t)LTR381_CS_BUFF_SZ*LTR381_CS_BUFF_ELEM_SZ)

//main control reg bits
#define LTRRBMC_SW_RESET ((uint8_t)0b00010000)
#define LTRRBMC_CS_MODE ((uint8_t)0b00000100)
#define LTRRBMC_ALS_CS_EN ((uint8_t)0b00000010)

//main status reg bits
#define LTRRBMS_ALS_CS_DATA_STATUS ((uint8_t)0b00001000)
#define LTRRBMS_ALS_CS_INT_STATUS ((uint8_t)0b00010000)
#define LTRRBMS_POWER_ON_STATUS ((uint8_t)0b00100000)

I2CS_STATE_TypeDef LTR381_Init(void);
I2CS_STATE_TypeDef LTR381_SoftReset(void);
I2CS_STATE_TypeDef LTR381_ReadData(bool *ready_flag, uint32_t *buff);
I2CS_STATE_TypeDef LTR381_ReadDataRaw(bool *ready_flag, uint8_t *buff);

#endif /* __LTR381_H */


