#include "utils.h"
#include "button.h"
#include "taskman.h"
#include "i2c_soft.h"
#include "aht20.h"
#include "ltr381.h"
#include "adxl345.h"
#include "spi_soft.h"
#include "nrf24.h"

#define SYS_UPDATE_PERIOD 50
#define SYS_BUTTON_RELOAD_TIME 100
#define SYS_WWDG_MIN_VALUE 0x5A
#define SYS_WWDG_INIT_VALUE 0x6E
#define SYS_MAX_OFFLINE_TIME 100 //5sec
#define SYS_MAX_IDLE_TIME 600 //30sec

typedef enum {
	SL_RED = 0,
	SL_GREEN,
	SL_COUNT //add new leds above this line
} SysLED_TypeDef;

typedef enum {
	ST_BLINK = 0,
	ST_UPDATE_SLOW,
	ST_SEND_DATA,
	ST_COUNT //add new tasks above this line
} SysTask_TypeDef;



struct system_struct {
	struct io_struct io_volt_mon, io_led[2];
	uint32_t cpu_freq, counter;
	uint8_t ltr_buff[LTR381_CS_RAW_BYTE_COUNT], aht_buff[AHT20_MEASUREMENT_BYTE_COUNT], adxl_buff[ADXL345_RAW_BYTE_COUNT];
	bool normal_power : 1, halt_flag : 1, ltr_flag : 1, aht_flag : 1, rf_tx_flag : 1;
	uint8_t cur_led, request_timer;
	uint16_t idle_timer;
	//uint32_t ltr_buff[4];
	//float temperature, humidity;
	//int16_t accel_buff[3];
};
struct system_struct sys = {0};
//extern struct system_struct sys;

void Clock_Init(void);
void Timer_Init(uint32_t period_ms);
void Timer_Interrupt(void);
void SleepMs(uint32_t time_ms);
void Event_ButtonClick(uint8_t index);
void Event_TaskTick(uint8_t index);
void ResetWatchdog(void);
void Pin_Interrupt(void);
uint8_t SetResetFlag(uint8_t value);
void Reset_Hardware(void);
void Shutdown_Hardware(void);
void Startup_Delay(void);
void RF_Update(void);
void RF_SendData(void);
void ChangeStateLED(SysLED_TypeDef new_led);

//6(aht)+12(ltr)+6(adxl)=24
#define RF_PAYLOAD_SIZE 24
#define RF_CHANNEL 22
const uint8_t rf_addr_tx[NRF24_CUR_ADDR_LEN] = {0xD7,0xD7,0xD7,0xD7,0xD7};
const uint8_t rf_addr_rx[NRF24_CUR_ADDR_LEN] = {0xE7,0xE7,0xE7,0xE7,0xE7};

main()
{	
	WWDG_Init(SYS_WWDG_INIT_VALUE, SYS_WWDG_MIN_VALUE);

	Reset_Hardware();
	Clock_Init();
	Sleep(5000);
	ResetWatchdog();
	
	AssignIO(&sys.io_volt_mon, PORT_VOLT_MON, PIN_VOLT_MON, TRUE, GPIO_MODE_IN_FL_NO_IT);
	AssignIO(&sys.io_led[SL_RED], PORT_LED_A, PIN_LED_A, TRUE, GPIO_MODE_OUT_PP_LOW_SLOW);
	AssignIO(&sys.io_led[SL_GREEN], PORT_LED_B, PIN_LED_B, TRUE, GPIO_MODE_OUT_PP_LOW_SLOW);
	
	Timer_Init(SYS_UPDATE_PERIOD);
	Button_Init(SYS_UPDATE_PERIOD, SYS_BUTTON_RELOAD_TIME, &Event_ButtonClick);
	I2CS_Init();
	SPIS_Init();
	Taskman_Init(SYS_UPDATE_PERIOD, &Event_TaskTick);
	Task_Create(ST_BLINK, 2300, TRUE, TRUE);
	Task_Create(ST_UPDATE_SLOW, 1000, TRUE, TRUE);
	assert_param((AHT20_Calibrate() == I2CSS_ACK));
	assert_param((LTR381_SoftReset() == I2CSS_ACK));
	 
	sys.counter = 0;
	sys.normal_power = TRUE;
	sys.ltr_flag = FALSE;
	sys.aht_flag = FALSE;
	sys.rf_tx_flag = FALSE;
	sys.cur_led = SL_RED;
	sys.request_timer = 0;
	sys.idle_timer = 0;
	
	Startup_Delay();
	assert_param((LTR381_Init() == I2CSS_ACK));
	assert_param((ADXL345_Init() == I2CSS_ACK));
	NRF24_Init(RF_CHANNEL, RF_PAYLOAD_SIZE, rf_addr_tx, rf_addr_rx);
	
	enableInterrupts();
	Taskman_Update(0); //execute all tasks
	
	SleepMs(500);
	sys.halt_flag = FALSE;
	while (TRUE) {
		ResetWatchdog();
		if (sys.halt_flag)
			Shutdown_Hardware();
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


void Shutdown_Hardware(void)
{
	WriteIO(&sys.io_led[SL_GREEN], IO_High);
	SleepMs(1000);
	while ( Button_IsDown(SYS_UPDATE_PERIOD, 0) )
		ResetWatchdog();
	
	NRF24_PowerDown();
	assert_param((AHT20_SoftReset() == I2CSS_ACK));
	assert_param((LTR381_SoftReset() == I2CSS_ACK));
	assert_param((ADXL345_SoftReset() == I2CSS_ACK));
	Reset_Hardware();
	GPIO_Init(PORT_BTN_EXTRA_INT, PIN_BTN_EXTRA_INT, GPIO_MODE_IN_PU_IT);
	TIM2_Cmd(DISABLE);
	ITC_SetSoftwarePriority(ITC_IRQ_PORT_BTN_A, ITC_PRIORITYLEVEL_0);
	EXTI_SetExtIntSensitivity(EXTI_PORT_BTN_A, EXTI_SENSITIVITY_FALL_ONLY);
	EXTI_SetTLISensitivity(EXTI_TLISENSITIVITY_FALL_ONLY);
	ResetWatchdog();
	
	enableInterrupts();
	halt();
	
	WWDG_SWReset();
}


void ResetWatchdog(void)
{
	if ((WWDG_GetCounter() & 0x7F) <= SYS_WWDG_MIN_VALUE)
		WWDG_SetCounter(SYS_WWDG_INIT_VALUE);
}


void Event_ButtonClick(uint8_t index)
{
	sys.halt_flag = TRUE;
}


void Pin_Interrupt(void)
{

}

void Event_TaskTick(uint8_t index)
{
	bool state;
	
	switch (index) {
		
		case ST_BLINK:
			if (!sys.normal_power)
				Button_ExtraLed(0);
			break;
			
		case ST_UPDATE_SLOW:
			ReadIO(&sys.io_volt_mon, &state);
			sys.normal_power = state ? TRUE : FALSE;
			WriteIO(&sys.io_led[sys.cur_led], IO_Reverse);
			assert_param((AHT20_GetDataRaw(&state, sys.aht_buff) == I2CSS_ACK));
			sys.aht_flag = (state) ? TRUE : FALSE;
			
			break;
			
		case ST_SEND_DATA:
			RF_SendData();
			break;
			
		default:
			assert_failed((uint8_t *)__FILE__, __LINE__);
	}
}



void Clock_Init(void)
{
	CLK_DeInit();
								 
	//CLK_HSECmd(DISABLE);
	//CLK_LSICmd(ENABLE);
	//while(CLK_GetFlagStatus(CLK_FLAG_LSIRDY) == FALSE)
		//ResetWatchdog();
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
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, DISABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, DISABLE);
	//CLK->PCKENR1 |= (uint8_t)((uint8_t)1 << ((uint8_t)CLK_PERIPHERAL_TIMER2 & (uint8_t)0x0F));
	
	sys.cpu_freq = GetCurClockFreq();
}


void Timer_Init(uint32_t period_ms)
{
	uint16_t tim_period;
	
	//TIM2_DeInit();
	tim_period = (uint16_t)(sys.cpu_freq / (1024 * (1000 / period_ms)));
	TIM2_TimeBaseInit(TIM2_PRESCALER_1024, tim_period); 
	TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
	TIM2_Cmd(ENABLE);
}


void Timer_Interrupt(void)
{
	sys.counter = (sys.counter == 0) ? 1 : sys.counter + 1;
		
	/* update code begin */
	Button_Update();
	Taskman_Update(sys.counter);
	RF_Update();
	/* update code end */
	
	TIM2_ClearFlag(TIM2_FLAG_UPDATE);
}


void ChangeStateLED(SysLED_TypeDef new_led)
{
	if (sys.cur_led == new_led)
		return;
		
	WriteIO(&sys.io_led[sys.cur_led], IO_Low);
	sys.cur_led = new_led;
}


void RF_Update(void)
{
	uint8_t rf_buff[RF_PAYLOAD_SIZE];
	
	if ( (sys.request_timer) && ((--sys.request_timer) == 0) )
		ChangeStateLED(SL_RED);
	
	//automatic shutdown if device is offline
	if (sys.cur_led == SL_RED) {
		if ((++sys.idle_timer) > SYS_MAX_IDLE_TIME)
			sys.halt_flag = TRUE;
	} else {
		sys.idle_timer = 0;
	}
		
	if ( (sys.rf_tx_flag) && (!NRF24_IsSending()) ) {
		NRF24_PowerUpRx();
		sys.rf_tx_flag = FALSE;
	} else if ( /*(!sys.rf_tx_flag) &&*/ (NRF24_DataReady()) ) {
		NRF24_GetData(rf_buff);
		ResetWatchdog();
		if ( (rf_buff[0] == 0xC0) && (rf_buff[1] == 0xDE) && (rf_buff[2] == 0x01) )
			Task_Create(ST_SEND_DATA, 100, TRUE, FALSE);
	}
}


void RF_SendData(void)
{
	uint8_t rf_buff[RF_PAYLOAD_SIZE], i, buff_pos;
	bool state;
	
	if (sys.rf_tx_flag) {
		Task_Create(ST_SEND_DATA, 0, TRUE, FALSE); //retry in next iteration if busy
		return;
	}
		
	ResetWatchdog();	
	assert_param((LTR381_ReadDataRaw(&state, sys.ltr_buff) == I2CSS_ACK));
	sys.ltr_flag = (state) ? TRUE : FALSE;
	assert_param((ADXL345_ReadDataRaw(sys.adxl_buff) == I2CSS_ACK));
	ResetWatchdog();	
	
	//pack adxl data
	for (i = 0; i < RF_PAYLOAD_SIZE; ++i)
		rf_buff[i] = (i < ADXL345_RAW_BYTE_COUNT) ? sys.adxl_buff[i] : 0;
	buff_pos = ADXL345_RAW_BYTE_COUNT;
	//pack aht data
	if (sys.aht_flag) {
		for (i = 0; i < AHT20_MEASUREMENT_BYTE_COUNT; ++i)
			rf_buff[buff_pos+i] = sys.aht_buff[i];
	}
	buff_pos += AHT20_MEASUREMENT_BYTE_COUNT;
	//pack ltr data
	if (sys.ltr_flag) {
		for (i = 0; i < LTR381_CS_RAW_BYTE_COUNT; ++i)
			rf_buff[buff_pos+i] = sys.ltr_buff[i];
	}
	
	//send packet
	NRF24_Send(rf_buff);
	ResetWatchdog();
	sys.rf_tx_flag = TRUE;
	sys.request_timer = SYS_MAX_OFFLINE_TIME;
	ChangeStateLED(SL_GREEN);
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
	uint16_t n;
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
	disableInterrupts();
	n = 0;
  while (1)
  {
		for (i = 0; i < 4; ++i) {
			ResetWatchdog();
			Sleep(5000);
		}
		WriteIO(&sys.io_led[SL_RED], IO_Reverse);
		if ((++n) > 500)
			WWDG_SWReset();
  }
}
#endif

/*
> .text, .const and .vector are ROM
> .bsct, .ubsct, .data and .bss are all RAM
> .info. and .debug are symbol tables that do not use target resources/memory.
*/
