#ifndef __UTILS_H
#define __UTILS_H

#include "stm8s.h"

/* KEYWORD DEFINITIONS -------------------------------------------------------*/
#define LOW RESET
#define HIGH SET
#define NULL (0)

typedef enum {
	IO_High,
	IO_Low,
	IO_Reverse,
	IO_Invalid
} IO_MODE_TypeDef;


/* MACRO DEFINITIONS ---------------------------------------------------------*/
#define BYTE_L(val) ((uint8_t)((val) & (uint8_t)0xFF))
#define BYTE_H(val) ((uint8_t)(BYTE_L((val) >> (uint8_t)8)))
#define WORD_HL(value_h, value_l) ((uint16_t)((((uint16_t)value_h) << 8) | value_l))
#define DWORD_HML(value_h, value_m, value_l) ((uint32_t)((((uint32_t)value_h) << 16) | (((uint16_t)value_m) << 8) | value_l))

/* PARAMETERS CHECKING --------------------------------------------------*/
#define IS_UTIL_IO_MODE_OK(io_mode) \
	((io_mode) < (IO_Invalid)) 
	
/* PIN DEFINITIONS -----------------------------------------------------------*/
#define PORT_BTN_A GPIOA
#define PIN_BTN_A GPIO_PIN_1
#define ITC_IRQ_PORT_BTN_A ITC_IRQ_PORTD
#define EXTI_PORT_BTN_A EXTI_PORT_GPIOD
#define PORT_BTN_EXTRA_INT GPIOD
#define PIN_BTN_EXTRA_INT GPIO_PIN_5

#define PORT_LED_A GPIOA
#define PIN_LED_A GPIO_PIN_2
#define PORT_LED_B GPIOA
#define PIN_LED_B GPIO_PIN_3

#define PORT_VOLT_MON GPIOC
#define PIN_VOLT_MON GPIO_PIN_3

#define PORT_MCU_SDA GPIOB
#define PIN_MCU_SDA GPIO_PIN_4
#define PORT_MCU_SCL GPIOB
#define PIN_MCU_SCL GPIO_PIN_5
	
#define PORT_SPI_MISO GPIOC
#define PIN_SPI_MISO GPIO_PIN_7
#define PORT_SPI_MOSI GPIOC
#define PIN_SPI_MOSI GPIO_PIN_6
#define PORT_SPI_SCK GPIOC
#define PIN_SPI_SCK GPIO_PIN_5
#define PORT_SPI_CS GPIOD
#define PIN_SPI_CS GPIO_PIN_2

#define PORT_NRF24_CE_RXTX GPIOD
#define PIN_NRF24_CE_RXTX GPIO_PIN_3
#define PORT_NRF24_IRQ GPIOD
#define PIN_NRF24_IRQ GPIO_PIN_6

/* STRUCTURE PROTOTYPES ------------------------------------------------------*/
struct buffer_struct {
	uint8_t *array;
	uint8_t size;
};

struct io_struct {
	GPIO_TypeDef *port;
	GPIO_Pin_TypeDef pin;
};

struct pos2d_struct {
	uint8_t x, y;
};

struct pos3d_struct {
	uint8_t x, y, z;
};

struct size2d_struct {
	uint8_t width, height;
};

struct size3d_struct {
	uint8_t width, length, height;
};


/* FUNCTION PROTOTYPES -------------------------------------------------------*/
uint32_t GetCurClockFreq(void);
void Sleep(uint32_t t);
void AssignIO(struct io_struct *cur_io, GPIO_TypeDef *cur_port, GPIO_Pin_TypeDef cur_pin, bool init_io, GPIO_Mode_TypeDef cur_mode);
void WriteIO(struct io_struct *cur_io, IO_MODE_TypeDef io_mode);
void ReadIO(struct io_struct *cur_io, bool *result);
void ModeIO(struct io_struct *cur_io, GPIO_Mode_TypeDef cur_mode);
uint8_t DecFromBCD(uint8_t bcd_val);
uint8_t DecToBCD(uint8_t dec_val);
uint16_t Crc16(uint8_t *pcBlock, uint8_t len);
int8_t Round8S(float x);
uint8_t Round8U(float x);

#endif /* __UTILS_H */


//.map file contents
//> .text, .const and .vector are ROM
//> .bsct, .ubsct, .data and .bss are all RAM
//> .info. and .debug are symbol tables that do not use target resources/memory.
