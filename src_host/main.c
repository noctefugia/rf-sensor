#include "utils.h"
#include "taskman.h"
#include "uart.h"
#include "spi_soft.h"
#include "nrf24.h"

#define SYS_UPDATE_PERIOD 50
#define SYS_WWDG_MIN_VALUE 0x5A
#define SYS_WWDG_INIT_VALUE 0x6E
#define SYS_MAX_OFFLINE_TIME 100 //5sec

typedef enum {
	SL_RED = 0,
	SL_GREEN,
	SL_COUNT //add new leds above this line
} SysLED_TypeDef;

typedef enum {
	ST_UPDATE_SLOW,
	ST_COUNT //add new tasks above this line
} SysTask_TypeDef;


typedef enum {
	CMDI_PING,
	CMDI_REQUEST_SENSOR_DATA,
	CMDI_UNDEFINED = 0xFF //place before this flag
} CommandIn_TypeDef;


typedef enum {
	CMDO_PING,
	CMDO_RF_MESSAGE_LOST,
	CMDO_RF_RETRY_COUNT,
	CMDO_RF_SENSOR_DATA,
	CMDO_UNDEFINED = 0xFF //place before this flag
} CommandOut_TypeDef;


struct system_struct {
	struct io_struct io_led[2];
	uint32_t cpu_freq, counter;
	bool rf_tx_flag : 1;
	uint8_t cur_led, last_rf_tx_time;
};
struct system_struct sys = {0};
//extern struct system_struct sys;

void Clock_Init(void);
void Timer_Init(uint32_t period_ms);
void Timer_Interrupt(void);
void SleepMs(uint32_t time_ms);
void Event_TaskTick(uint8_t index);
void ResetWatchdog(void);
uint8_t SetResetFlag(uint8_t value);
void Reset_Hardware(void);
void Startup_Delay(void);
void Event_Command(uint8_t cmd_size, uint8_t *cmd_buff);
void RF_Update(void);
void ChangeStateLED(SysLED_TypeDef new_led);

#define RF_PAYLOAD_SIZE 24
#define RF_CHANNEL 22
const uint8_t rf_addr_tx[NRF24_CUR_ADDR_LEN] = {0xE7,0xE7,0xE7,0xE7,0xE7};
const uint8_t rf_addr_rx[NRF24_CUR_ADDR_LEN] = {0xD7,0xD7,0xD7,0xD7,0xD7};


main()
{	
	WWDG_Init(SYS_WWDG_INIT_VALUE, SYS_WWDG_MIN_VALUE);

	Reset_Hardware();
	Clock_Init();
	Sleep(5000);
	ResetWatchdog();
	
	AssignIO(&sys.io_led[SL_RED], PORT_LED_A, PIN_LED_A, TRUE, GPIO_MODE_OUT_PP_LOW_SLOW);
	AssignIO(&sys.io_led[SL_GREEN], PORT_LED_B, PIN_LED_B, TRUE, GPIO_MODE_OUT_PP_LOW_SLOW);
	
	SPIS_Init();
	NRF24_Init(RF_CHANNEL, RF_PAYLOAD_SIZE, rf_addr_tx, rf_addr_rx);
	Timer_Init(SYS_UPDATE_PERIOD);
	UART_Init(115200, SYS_UPDATE_PERIOD, 200, &Event_Command);
	Taskman_Init(SYS_UPDATE_PERIOD, &Event_TaskTick);
	Task_Create(ST_UPDATE_SLOW, 1000, TRUE, TRUE);
	sys.counter = 0;
	sys.rf_tx_flag = FALSE;
	sys.cur_led = SL_RED;
	sys.last_rf_tx_time = 0;
	
	Startup_Delay();
	
	enableInterrupts();
	Taskman_Update(0); //execute all tasks
	
	SleepMs(500);
	while (TRUE) {
		ResetWatchdog();
	}
}


void Startup_Delay(void)
{
	uint8_t i;
	
	WriteIO(&sys.io_led[SL_GREEN], IO_High);
	for (i = 0; i < 0xFF; ++i) {
		ResetWatchdog();
		Sleep(1000);
	}
	WriteIO(&sys.io_led[SL_GREEN], IO_Low);
}


void Reset_Hardware(void)
{
	disableInterrupts();
	ITC_DeInit(); EXTI_DeInit();
	GPIO_DeInit(GPIOA); GPIO_DeInit(GPIOB);
	GPIO_DeInit(GPIOC); GPIO_DeInit(GPIOD);
}


void ResetWatchdog(void)
{
	if ((WWDG_GetCounter() & 0x7F) <= SYS_WWDG_MIN_VALUE)
		WWDG_SetCounter(SYS_WWDG_INIT_VALUE);
}


void Event_TaskTick(uint8_t index)
{
	switch (index) {
			
		case ST_UPDATE_SLOW:
			WriteIO(&sys.io_led[sys.cur_led], IO_Reverse);
			break;
			
		default:
			assert_failed((uint8_t *)__FILE__, __LINE__);
	}
}


void RF_Update(void)
{
	uint8_t state, uart_buff[RF_PAYLOAD_SIZE+1], uart_buff_sz, state_led;
	
	ResetWatchdog();
	if ( (sys.last_rf_tx_time) && ((--sys.last_rf_tx_time) == 0) )
		ChangeStateLED(SL_RED);
		
	if ( (sys.rf_tx_flag) && (!NRF24_IsSending()) ) {
		state = NRF24_LastMessageStatus();
		if (state == NRF24_LMS_MESSAGE_LOST) {
			uart_buff[0] = CMDO_RF_MESSAGE_LOST;
			uart_buff_sz = 1;
			sys.last_rf_tx_time = 0;
			state_led = SL_RED;
		} else {
			uart_buff[0] = CMDO_RF_RETRY_COUNT;
			uart_buff[1] = NRF24_RetransmissionCount();
			uart_buff_sz = 2;
			sys.last_rf_tx_time = SYS_MAX_OFFLINE_TIME;
			state_led = SL_GREEN;
		}
		ChangeStateLED(state_led);
		UART_Send(uart_buff_sz, uart_buff);
		
		NRF24_PowerUpRx();
		sys.rf_tx_flag = FALSE;
	}
	
	else if ( (UART_TXidle()) && (NRF24_DataReady()) ) {
		ResetWatchdog();
		NRF24_GetData(uart_buff + 1);
		ResetWatchdog();
		uart_buff[0] = CMDO_RF_SENSOR_DATA;
		uart_buff_sz = RF_PAYLOAD_SIZE + 1;
		UART_Send(uart_buff_sz, uart_buff);
		ResetWatchdog();
	}
}
				
	
void ChangeStateLED(SysLED_TypeDef new_led)
{
	if (sys.cur_led == new_led)
		return;
		
	WriteIO(&sys.io_led[sys.cur_led], IO_Low);
	sys.cur_led = new_led;
}


void Event_Command(uint8_t cmd_size, uint8_t *cmd_buff)
{
	CommandIn_TypeDef cmd_id;
	bool cmd_ok;
	uint8_t i, param_count, *param;
	uint8_t out_buff[UART_TX_PACKET_MAX_SIZE-1], rf_buff[RF_PAYLOAD_SIZE];
	
	cmd_id = cmd_buff[0];
	if (cmd_size > 1)
		param = &cmd_buff[1];
	
	param_count = 0;
	cmd_ok = TRUE;
	switch (cmd_id) {
		case CMDI_PING:
			//param_count = 0;
			break;
		case CMDI_REQUEST_SENSOR_DATA:
			//param_count = 0;
			break;
		default:
			cmd_ok = FALSE;
	}
	if (param_count != (cmd_size - 1))
		cmd_ok = FALSE;
		
	if (cmd_ok) {
		switch (cmd_id) {
			case CMDI_PING:
				out_buff[0] = CMDO_PING;
				for (i = 0; i < 4; ++i)
					out_buff[4-i] = (uint8_t)((sys.counter >> (8*i)) & 0xFF);
				UART_Send(5, out_buff);
				break;
				
			case CMDI_REQUEST_SENSOR_DATA:
				ResetWatchdog();
				if ( (!sys.rf_tx_flag) /*&& (!NRF24_IsSending())*/ ) {
					rf_buff[0] = 0xC0;
					rf_buff[1] = 0xDE;
					rf_buff[2] = 0x01;
					NRF24_Send(rf_buff);
					sys.rf_tx_flag = TRUE;
				}
				ResetWatchdog();
				break;
		}
	} else {
		out_buff[0] = CMDO_UNDEFINED;
		UART_Send(1, out_buff);
	}
}


void Clock_Init(void)
{
	CLK_DeInit();
								 
	CLK_HSECmd(DISABLE);
	CLK_LSICmd(ENABLE);
	while(CLK_GetFlagStatus(CLK_FLAG_LSIRDY) == FALSE)
		ResetWatchdog();
	CLK_HSICmd(ENABLE);
	while(CLK_GetFlagStatus(CLK_FLAG_HSIRDY) == FALSE)
		ResetWatchdog();
	
	CLK_ClockSwitchCmd(ENABLE);
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
	CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);
	
	CLK_ClockSwitchConfig(CLK_SWITCHMODE_AUTO, CLK_SOURCE_HSI, 
	DISABLE, CLK_CURRENTCLOCKSTATE_ENABLE);
	
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_AWU, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, DISABLE);
	
	sys.cpu_freq = GetCurClockFreq();
}


void Timer_Init(uint32_t period_ms)
{
	uint16_t tim_period;
	
	TIM2_DeInit();
	tim_period = (uint16_t)(sys.cpu_freq / (1024 * (1000 / period_ms)));
	TIM2_TimeBaseInit(TIM2_PRESCALER_1024, tim_period); 
	TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
	TIM2_Cmd(ENABLE);
}


void Timer_Interrupt(void)
{
	
	sys.counter = (sys.counter == 0) ? 1 : sys.counter + 1;
		
	/* update code begin */
	RF_Update();
	ResetWatchdog();
	UART_Update();
	ResetWatchdog();
	Taskman_Update(sys.counter);
	ResetWatchdog();
	/* update code end */
	
	TIM2_ClearFlag(TIM2_FLAG_UPDATE);
}


void SleepMs(uint32_t time_ms)
{
	uint32_t last_time;
	
	last_time = sys.counter * SYS_UPDATE_PERIOD;
	while ((sys.counter*SYS_UPDATE_PERIOD - last_time) < time_ms)
		ResetWatchdog();
}


#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{ 
	uint8_t i;
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
	disableInterrupts();
  while (1)
  {
		for (i = 0; i < 4; ++i) {
			ResetWatchdog();
			Sleep(5000);
		}
		WriteIO(&sys.io_led[SL_RED], IO_Reverse);
  }
}
#endif

/*
> .text, .const and .vector are ROM
> .bsct, .ubsct, .data and .bss are all RAM
> .info. and .debug are symbol tables that do not use target resources/memory.
*/
