/*
 * @brief NXP LPC1769 LPCXpresso board file
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "board.h"
#include "string.h"

#include "retarget.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
#define SPEED_100KHZ         100000
#define SPEED_400KHZ         400000
#define VREFVAL         		 1218
#define VOLVAL         		 	 15397
#define CURVAL         		 	 94638
/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/* System oscillator rate and RTC oscillator rate */
const uint32_t OscRateIn = 12000000;
const uint32_t RTCOscRateIn = 32768;
#define MAXINPUTS  3  //10
//#define MAXOUTPUTS 12 //8

/* **************************************************** 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11, */
//static const uint8_t GPIOPortInput[2][MAXINPUTS]   = {{ 2, 2, 0, 1, 1, 2, 2, 2, 2, 2},\
//																											{13, 0,30,31,30, 4, 3, 1, 2,10}};
//static const uint8_t GPIOPortOutput[2][MAXOUTPUTS] = {{ 2, 3, 0, 0, 0, 0, 3, 0},\
//																											{ 7,26,23,24,25,26,25,29}};

static const uint8_t GPIOPortInput[2][MAXINPUTS]   = {{  1,  1, },\
																										                            	{31, 30, }};
static const uint8_t GPIOPortOutput[2][MAXOUTPUTS] = {{  3,  0,  0,  0,  0,  0,  3,  3,  0, 2,2,2}, \
																										                           	{ 25, 26, 25, 23, 24, 29, 25, 26, 30, 3,2,7}};
 
																																																						
//static const uint8_t GPIOPortInput[2][MAXINPUTS]   = {{ 1, 2, 2, 2, 0, 1, 1, 2, 2, 2, 2, 2},\
//																											{26, 7,13, 0,30,31,30, 4, 3, 1, 2,10}};

static int mode_poll;   /* Poll/Interrupt mode flag */
static I2C_ID_T i2cDev = DEFAULT_I2C; /* Currently active I2C device */
																											
volatile uint32_t GPIOsVal[2]={0,0};	/* 0=input, 1=output */			
volatile uint32_t timerFreq;
unsigned int timerCur=0,timerLst=0;
unsigned char powerCnt=0,timerCnt=0;
/*****************************************************************************
 * Private functions
 ****************************************************************************/
/* Initializes board LED(s) */
static void Board_GPIO_Init(void)
{
	uint8_t idx;
	for(idx=0;idx<MAXINPUTS;idx++)
	{
		Chip_GPIO_WriteDirBit(LPC_GPIO, GPIOPortInput[0][idx], GPIOPortInput[1][idx], false);
	}
	for(idx=0;idx<MAXOUTPUTS;idx++)
	{
		Chip_GPIO_WriteDirBit(LPC_GPIO, GPIOPortOutput[0][idx], GPIOPortOutput[1][idx], true);
		Board_GPIOOutput_Set(idx,false);
	}
}


/* Set I2C mode to polling/interrupt */
static void i2c_set_mode(I2C_ID_T id, int polling)
{
	if(!polling) {
		mode_poll &= ~(1 << id);
		Chip_I2C_SetMasterEventHandler(id, Chip_I2C_EventHandler);
		NVIC_EnableIRQ(id == I2C0 ? I2C0_IRQn : I2C1_IRQn);
	} else {
		mode_poll |= 1 << id;
		NVIC_DisableIRQ(id == I2C0 ? I2C0_IRQn : I2C1_IRQn);
		Chip_I2C_SetMasterEventHandler(id, Chip_I2C_EventHandlerPolling);
	}
}

/* Initialize the I2C bus */
static void i2c_app_init(I2C_ID_T id, int speed)
{
	Board_I2C_Init();

	/* Initialize I2C */
	Chip_I2C_Init(id);
	Chip_I2C_SetClockRate(id, speed);

	/* Set default mode to interrupt */
	i2c_set_mode(id, 0);
}

/* State machine handler for I2C0 and I2C1 */
static void i2c_state_handling(I2C_ID_T id)
{
	if (Chip_I2C_IsMasterActive(id)) {
		Chip_I2C_MasterStateHandler(id);
	} else {
		Chip_I2C_SlaveStateHandler(id);
	}
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/
/**
 * @brief	I2C0 Interrupt handler
 * @return	None
 */
void I2C0_IRQHandler(void)
{
	i2c_state_handling(I2C0);
}

/* Initialize UART pins */
void Board_UART_Init(LPC_USART_T *pUART)
{
	/* Pin Muxing has already been done during SystemInit */
}

/* Initialize debug output via UART for board */
void Board_Debug_Init(void)
{
#if defined(DEBUG_ENABLE)
	Board_UART_Init(DEBUG_UART);

	Chip_UART_Init(DEBUG_UART);
	Chip_UART_SetBaud(DEBUG_UART, 115200);
	Chip_UART_ConfigData(DEBUG_UART, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);

	/* Enable UART Transmit */
	Chip_UART_TXEnable(DEBUG_UART);
#endif
}

/* Sends a character on the UART */
void Board_UARTPutChar(char ch)
{
#if defined(DEBUG_ENABLE)
	while ((Chip_UART_ReadLineStatus(DEBUG_UART) & UART_LSR_THRE) == 0) {}
	Chip_UART_SendByte(DEBUG_UART, (uint8_t) ch);
#endif
}

/* Gets a character from the UART, returns EOF if no character is ready */
int Board_UARTGetChar(void)
{
#if defined(DEBUG_ENABLE)
	if (Chip_UART_ReadLineStatus(DEBUG_UART) & UART_LSR_RDR) {
		return (int) Chip_UART_ReadByte(DEBUG_UART);
	}
#endif
	return EOF;
}

/* Outputs a string on the debug UART */
void Board_UARTPutSTR(char *str)
{
#if defined(DEBUG_ENABLE)
	while (*str != '\0') {
		Board_UARTPutChar(*str++);
	}
#endif
}

/* get the current state of a board INPUT and OUTPUT */
void Board_GPIOs_State(void)
{
	uint8_t idx;
	GPIOsVal[0]=0;
	GPIOsVal[1]=0;
	for(idx=0;idx<MAXINPUTS;idx++)
	{
		GPIOsVal[0] |= (Chip_GPIO_ReadPortBit(LPC_GPIO, GPIOPortInput[0][idx], GPIOPortInput[1][idx])<<idx);
	}
	for(idx=0;idx<MAXOUTPUTS;idx++)
	{
		GPIOsVal[1] |= (Chip_GPIO_ReadPortBit(LPC_GPIO, GPIOPortOutput[0][idx], GPIOPortOutput[1][idx])<<idx);
	}
}

/* Sets the state of a board GPIO to on or off */
void Board_GPIOOutput_Set(uint8_t GPIONum, bool OnOff)
{
		Chip_GPIO_WritePortBit(LPC_GPIO, GPIOPortOutput[0][GPIONum], GPIOPortOutput[1][GPIONum], OnOff);
}

/* Returns the current state of a board INPUT */
bool Board_GPIOInput_State(uint8_t GPIONum)
{
	bool state = false;
	state = Chip_GPIO_ReadPortBit(LPC_GPIO, GPIOPortInput[0][GPIONum], GPIOPortInput[1][GPIONum]);
	return state;
}

/* Returns the current state of a board OUTPUT */
bool Board_GPIOOutput_State(uint8_t GPIONum)
{
	bool state = false;
	state = Chip_GPIO_ReadPortBit(LPC_GPIO, GPIOPortOutput[0][GPIONum], GPIOPortOutput[1][GPIONum]);
	return state;
}

void Board_GPIOOutput_Toggle(uint8_t GPIONum)
{
		Board_GPIOOutput_Set(GPIONum, !Board_GPIOOutput_State(GPIONum));
}

void Board_GPIOOutput_DIROut(uint8_t GIOPNum)
{	
 Chip_GPIO_SetPinDIROutput(LPC_GPIO, GPIOPortOutput[0][GIOPNum], GPIOPortOutput[1][GIOPNum]);
}

void Board_GPIOInput_DIROut(uint8_t GIOPNum)
{	
 Chip_GPIO_SetPinDIRInput(LPC_GPIO, GPIOPortInput[0][GIOPNum], GPIOPortInput[1][GIOPNum]);
}

/* Set up and initialize all required blocks and functions related to the
   board hardware */
void Board_Init(void)
{
	/* Sets up DEBUG UART */
	DEBUGINIT();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);
	Chip_IOCON_Init(LPC_IOCON);
	/* Initialize GPIOs */
	Board_GPIO_Init();
	/* Initialize SPI true=master, false=slave */
	Board_SPI_Init(true);
	/* Initialize I2C */
	Board_I2C_Init();
	i2c_app_init(i2cDev,SPEED_100KHZ);
	Board_GPIOOutput_Set(0,true);
		/* Enable timer 1 clock */
	Chip_TIMER_Init(LPC_TIMER0);
	Chip_Clock_SetPCLKDiv(SYSCTL_PCLK_TIMER0,2);
	/* Timer rate is system clock rate */
	timerFreq = Chip_Clock_GetSystemClockRate();
	/* Timer setup for match and interrupt at TICKRATE_HZ */
	Chip_TIMER_Reset(LPC_TIMER0);
	Chip_TIMER_CaptureRisingEdgeEnable(LPC_TIMER0,0);
	Chip_TIMER_CaptureEnableInt(LPC_TIMER0,0);
	Chip_TIMER_Enable(LPC_TIMER0);

	/* Enable timer interrupt */
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER0_IRQn);

}

/* Returns the MAC address assigned to this board */
void Board_ENET_GetMacADDR(uint8_t *mcaddr)
{
	const uint8_t boardmac[] = {0x00, 0x60, 0x37, 0x12, 0x34, 0x56};

	memcpy(mcaddr, boardmac, 6);
}

/* Initialize pin muxing for SSP interface */
void Board_SSP1_Init(void)
{
		/* Set up clock and muxing for SSP1 interface */
		/*
		 * Initialize SSP0 pins connect
		 * P0.7: SCK
		 * P0.6: SSEL
		 * P0.8: MISO
		 * P0.9: MOSI
		 */
		Chip_IOCON_PinMux(LPC_IOCON, 0, 7, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, 0, 6, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, 0, 8, IOCON_MODE_INACT, IOCON_FUNC2);
		Chip_IOCON_PinMux(LPC_IOCON, 0, 9, IOCON_MODE_INACT, IOCON_FUNC2);
}

/* Initialize pin muxing for SPI1 interface */
void Board_SPI_Init(bool isMaster)
{
	/* Set up clock and muxing for SSP0 interface */
	/*
	 * Initialize SSP0 pins connect
	 * P0.15: SCK
	 * P0.16: SSEL
	 * P0.17: MISO
	 * P0.18: MOSI
	 */
	Board_SSP1_Init();
	if (isMaster) {
		Chip_IOCON_PinMux(LPC_IOCON, 0, 6, IOCON_MODE_PULLUP, IOCON_FUNC0);
		Chip_GPIO_WriteDirBit(LPC_GPIO, 0, 6, true);
		Board_SPI_DeassertSSEL();

	}
	else {
		Chip_IOCON_PinMux(LPC_IOCON, 0, 6, IOCON_MODE_PULLUP, IOCON_FUNC2);
	}
}

/* Assert SSEL pin */
void Board_SPI_AssertSSEL(void)
{
	Chip_GPIO_WritePortBit(LPC_GPIO, 0, 6, false);
}

/* De-Assert SSEL pin */
void Board_SPI_DeassertSSEL(void)
{
	Chip_GPIO_WritePortBit(LPC_GPIO, 0, 6, true);
}

/* Sets up board specific I2C0 interface */
void Board_I2C_Init(void)
{
		Chip_IOCON_PinMux(LPC_IOCON, 0, 27, IOCON_MODE_INACT, IOCON_FUNC1);
		Chip_IOCON_PinMux(LPC_IOCON, 0, 28, IOCON_MODE_INACT, IOCON_FUNC1);
		Chip_IOCON_SetI2CPad(LPC_IOCON, I2CPADCFG_STD_MODE);
}

void Serial_CreateStream(void *Stream)
{}

unsigned short crc_16(unsigned char *Array, unsigned char *Rcvbuf,unsigned int Len)
{
	unsigned int  IX,IY,CRC;
	CRC=0xFFFF;//set all 1

	if (Len<=0)
		CRC = 0;
	else
	{
		Len--;
		for (IX=0;IX<=Len;IX++)
		{
			CRC=CRC^(unsigned int)(Array[IX]);
			for(IY=0;IY<=7;IY++)
			{
				if ((CRC&1)!=0 )
					CRC=(CRC>>1)^0xA001;
				else
					CRC=CRC>>1;    //
			}
		}
	}
	Rcvbuf[1] = (CRC & 0xff00)>>8;//MSB
	Rcvbuf[0] = (CRC & 0x00ff);  //LSB

	CRC= Rcvbuf[1]<<8;
	CRC+= Rcvbuf[0];
	return CRC;
}

/**
 * @brief	Handle interrupt from 32-bit timer
 * @return	Nothing
 */
void TIMER0_IRQHandler(void)
{
	unsigned int itemp=0,idx=0;	
//	if (Chip_TIMER_CapturePending(LPC_TIMER0, 0)) {
		if(timerCnt==0) 
		{
			timerLst = Chip_TIMER_ReadCapture(LPC_TIMER0, 0);
			timerCnt++;
		}
		else if(timerCnt > 9)
		{
			NVIC_DisableIRQ(TIMER0_IRQn);
			timerCnt = 0;
			timerCur = Chip_TIMER_ReadCapture(LPC_TIMER0, 0);
			if(timerCur>timerLst)
			{
				idx = timerCur - timerLst;
//				printf("Cur:%x,Lst:%x,idx:%x\n",timerCur,timerLst,idx);
				if(Board_GPIOOutput_State(0))
				{
					itemp = 75942067 / idx * 1000;	//mV
//				itemp = timerFreq / 2 / idx * VREFVAL / VOLVAL;	// clk=96000000 / pdiv=2 * VREFVAL=1218 * 2(110mV=220V) * 10=cnt / VOLVAL=15397 / idx
					sysGrpREG.sysPMACREG.sysVOL[0] = (itemp & 0x00ff0000)>>16;
					sysGrpREG.sysPMACREG.sysVOL[1] = (itemp & 0x0000ff00)>>8;
					sysGrpREG.sysPMACREG.sysVOL[2] = (itemp & 0x000000ff);
					printf("itemp vol:%d\n",itemp);
				}
				else
				{
					itemp = 6177645 / idx;	//mA
//				itemp = timerFreq / 2 / idx * VREFVAL / CURVAL;	// clk=96000000 / pdiv=2 * VREFVAL=1218 (mV/1mohm) * 10=cnt / VOLVAL=15397 / idx
					sysGrpREG.sysPMACREG.sysCUR[0] = (itemp & 0x00ff0000)>>16;
					sysGrpREG.sysPMACREG.sysCUR[1] = (itemp & 0x0000ff00)>>8;
					sysGrpREG.sysPMACREG.sysCUR[2] = (itemp & 0x000000ff);
					printf("itemp cur:%d\n",itemp);
				}
			}
			else idx = 0xFFFFFFFF - timerLst + timerCur;
		}
		else
		{
			timerCnt++;
		}
		Chip_TIMER_ClearCapture(LPC_TIMER0,0);
//	}
}

