#ifndef __I2C_SOFT_H
#define __I2C_SOFT_H

#include "utils.h" 

typedef enum {
	I2CSS_NO_ACK = HIGH,
	I2CSS_ACK = LOW,
	I2CSS_ERROR,
	I2CSS_DISABLED
} I2CS_STATE_TypeDef;

typedef enum {
	I2CSP_MAIN = 0,
	I2CSP_COUNT //add new pairs before this line
} I2CS_PAIR_TypeDef;

#define I2CS_DELAY ((uint8_t)10)
#define I2CS_READ_FLAG ((uint8_t)1)
#define I2CS_WRITE_FLAG ((uint8_t)0)

#define IS_I2CS_PAIR_OK(pair_no) \
	((pair_no) < ((I2CS_PAIR_TypeDef)(I2CSP_COUNT))) 
	
#define I2CS_CHECK_ACK(ack, pair_no) \
	if ((ack) != (I2CSS_ACK)) {/*I2CS_Reset(pair_no);*/ return ack;}
	
	
struct i2cs_io_pair_struct {
	struct io_struct io_sda, io_scl;
};

struct i2cs_struct {
	bool init : 1;
	struct i2cs_io_pair_struct pair[I2CSP_COUNT];
};

void I2CS_Sleep(uint16_t t);
void I2CS_Init(void);
void I2CS_Start(I2CS_PAIR_TypeDef pair_no);
void I2CS_Stop(I2CS_PAIR_TypeDef pair_no);
void I2CS_Reset(I2CS_PAIR_TypeDef pair_no);
void I2CS_ResetAll(void);
I2CS_STATE_TypeDef I2CS_WriteByte(I2CS_PAIR_TypeDef pair_no, uint8_t data);
uint8_t I2CS_ReadByte(I2CS_PAIR_TypeDef pair_no, I2CS_STATE_TypeDef ack);
I2CS_STATE_TypeDef I2CS_WriteRegister(I2CS_PAIR_TypeDef pair_no, uint8_t addr, uint8_t reg_no, uint8_t value);
I2CS_STATE_TypeDef I2CS_ReadRegister(I2CS_PAIR_TypeDef pair_no,  uint8_t addr, uint8_t reg_no, uint8_t *value);

#endif /* __I2C_SOFT_H */