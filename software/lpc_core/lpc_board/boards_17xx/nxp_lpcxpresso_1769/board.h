/*
 * @brief NXP LPC1769 XPresso board file
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

#ifndef __BOARD_H_
#define __BOARD_H_

#include "chip.h"
#include "lwip/ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup BOARD_NXP_LPCXPRESSO_1769 NXP LPC1769 LPCXpresso board software API functions
 * @ingroup LPCOPEN_17XX_BOARD_LPCXPRESSO_1769
 * The board support software API functions provide some simple abstracted
 * functions used across multiple LPCOpen board examples. See @ref BOARD_COMMON_API
 * for the functions defined by this board support layer.<br>
 * @{
 */

/** @defgroup BOARD_NXP_LPCXPRESSO_1769_OPTIONS BOARD: NXP LPC1769 LPCXpresso board build options
 * This board has options that configure its operation at build-time.<br>
 * @{
 */

/** Define DEBUG_ENABLE to enable IO via the DEBUGSTR, DEBUGOUT, and
    DEBUGIN macros. If not defined, DEBUG* functions will be optimized
    out of the code at build time.
 */
#define DEBUG_ENABLE

/** Define DEBUG_SEMIHOSTING along with DEBUG_ENABLE to enable IO support
    via semihosting. You may need to use a C library that supports
    semihosting with this option.
 */
// #define DEBUG_SEMIHOSTING

/** Board UART used for debug output and input using the DEBUG* macros. This
    is also the port used for Board_UARTPutChar, Board_UARTGetChar, and
    Board_UARTPutSTR functions.
 */
#define DEBUG_UART LPC_UART0

/**
 * @}
 */

/* Board name */
#define BOARD_NXP_LPCXPRESSO_1769

#define USE_RMII

/**
 * I2C defines
 */
#define DEFAULT_I2C          I2C0
#define I2C_SLAVE_EEPROM_SIZE       64
#define I2C_SLAVE_EEPROM_ADDR       0x50
#define I2C_SLAVE_TMP75_ADDR       	0x48

#define ETH_BUF_RX_LEN       	2048
#define ETH_BUF_TX_LEN       	2048

#define MAXINPUTS  3
#define MAXOUTPUTS (9 + 3)

#define  GPIO_OUT_BUZZER_IX         0
#define  GPIO_OUT_FAN_IX            1
#define  GPIO_OUT_SWITCH_IX         2
#define  GPIO_OUT_CAMERA_IX         3
#define  GPIO_OUT_HEAT_IX           4
#define  GPIO_OUT_LIGHT_IX          5
#define  GPIO_OUT_FLIGHT_IX         7
#define  GPIO_OUT_LED_DOOR_OPEN_IX  9
#define  GPIO_OUT_LED_SYS_RUN_IX    10

#define  GPIO_IN_DOOR_OPEN_IX     0
#define  GPIO_IN_G_IN1_IX         1

extern volatile uint32_t GPIOsVal[2];
extern unsigned char ethbufRx[ETH_BUF_RX_LEN];
extern unsigned char ethbufTx[ETH_BUF_TX_LEN];

typedef struct {
	unsigned char data[4];
} sysPARAM;

typedef struct {
	unsigned char data[4];
} devPARAM;

typedef struct {
	unsigned char STATE;
	unsigned char ID;
	devPARAM IP;
	devPARAM LNG;
	devPARAM LAT;
	unsigned char COEF[2];	/* COEFFICIENT */
	unsigned char MAINTAIN[2];	/* MAINTAIN */
	unsigned char RESERVED[2];	/* RESERVED */
} devGrpPARAM;

typedef struct {	/* system real time */
	unsigned char SECOND;
	unsigned char MINUTE;
	unsigned char DAY;
	unsigned char MONTH;	
	unsigned char YEAR;			
} sysRTC;

typedef struct {
	union devID{	/* device ID */
		struct {
			unsigned char MSB;
			unsigned char LSB;
		}devIDu8;
		unsigned short devIDu16;
	};
	union sysSTAT{	/* system state */
		struct {
			unsigned char MSB;
			unsigned char LSB;
		}sysSTATu8;
		unsigned short sysSTATu16;
	} sysSTAT;
	unsigned char sysVOL[3];	/* system voltage = ([3]<<8+[2]<<8+[1]) mV */
	unsigned char sysCUR[3];	/* system current = ([3]<<8+[2]<<8+[1]) mA */
	unsigned char sysPOW[2];	/* system power */
	unsigned char ioINPUT[2];	/* io input */
	unsigned char ioOUTPUT[2];	/* io output */
	unsigned char sysTEMP1[3];	/* system temperature1 = ([3]<<8+[2]<<8+[1]) mDeg */
	unsigned char sysTEMP2[3];	/* system temperature2 = ([3]<<8+[2]<<8+[1]) mDeg */
	unsigned char reserved1[2];
	unsigned char statDEV[90];	/* device n state:bit7 1=online, 0=offline; bit6_5 11=fail, 10=warning, 00=ok */
	unsigned char sysERRCLR[2];	/* clear system error, 1=clear */
	unsigned char sysOUTSET[2];	/* set system output, 1=clear */
	unsigned char sysOUTCLR[2];	/* clear system output, 1=clear */
	unsigned char sysID;	/* system ID */
	sysPARAM sysIP;	/* system ip address 0 */
	sysPARAM sysLNG;	/* system longitude */
	sysPARAM sysLAT;	/* system latitude */
	unsigned char sysTEMP1LOW[3];	/* system temperature1 low threshold = ([3]<<8+[2]<<8+[1]) mDeg */
	unsigned char sysTEMP1HIGH[3];	/* system temperature1 high threshold = ([3]<<8+[2]<<8+[1]) mDeg */
	unsigned char reserved2[2];
	unsigned char sysTEMP2LOW[3];	/* system temperature2 low threshold = ([3]<<8+[2]<<8+[1]) mDeg */
	unsigned char sysTEMP2HIGH[3];	/* system temperature2 high threshold = ([3]<<8+[2]<<8+[1]) mDeg */
	unsigned char reserved3[2];
	sysRTC sysDATE;
	sysPARAM sysBLUR;	/* system vedio blur threshold value, default=100 */
	sysPARAM sysNOISY;	/* system vedio noisy threshold value, default=100 */
	unsigned char reserved4[11];
	devGrpPARAM devGrpREG[90];
	unsigned char reserved5[2];	
} SYS_PMAC_REG;

union SYS_UNION_REG{
	SYS_PMAC_REG sysPMACREG;
	unsigned char sys8uREG[2000];
};

extern union SYS_UNION_REG sysGrpREG;
/* eth */
extern struct netif lpc_netif;
extern ip_addr_t lpc_sever_ipaddr;
extern ip_addr_t lpc_client_ipaddr;
extern bool ip_rdy;
extern bool broadcastflag;

extern unsigned int timerCur,timerLst;
/**
 * @brief	Initialize pin muxing for a UART
 * @param	pUART	: Pointer to UART register block for UART pins to init
 * @return	Nothing
 */
void Board_UART_Init(LPC_USART_T *pUART);

/**
 * @brief	set pin output
 * @param	GPIONum	: output pin number
 * @param	OnOff	: true=high, false=low
 * @return	Nothing
 */
void Board_GPIOOutput_Set(uint8_t GPIONum, bool OnOff);

/**
 * @brief	get pin input state
 * @param	GPIONum	: input pin number
 * @return	true=high, false=low
 */
bool Board_GPIOInput_State(uint8_t GPIONum);

/**
 * @brief	toggle pin output
 * @param	GPIONum	: output pin number
 * @return	Nothing
 */
void Board_GPIOOutput_Toggle(uint8_t GPIONum);

/**
 * @brief	Returns the MAC address assigned to this board
 * @param	mcaddr : Pointer to 6-byte character array to populate with MAC address
 * @return	Nothing
 * @note    Returns the MAC address used by Ethernet
 */
void Board_ENET_GetMacADDR(uint8_t *mcaddr);

/**
 * @brief	Initialize pin muxing for SSP interface
 * @param	pSSP	: Pointer to SSP interface to initialize
 * @return	Nothing
 */
void Board_SSP1_Init(void);

/**
 * @brief	Initialize pin muxing for SPI interface
 * @param	isMaster	: true for master mode, false for slave mode
 * @return	Nothing
 */
void Board_SPI_Init(bool isMaster);

/**
 * @brief	Assert SSEL pin
 * @return	Nothing
 */
void Board_SPI_AssertSSEL(void);

/**
 * @brief	De-assert SSEL pin
 * @return	Nothing
 */
void Board_SPI_DeassertSSEL(void);

/**
 * @brief	Sets up board specific I2C interface
 * @param	id	: ID of I2C peripheral
 * @return	Nothing
 */
void Board_I2C_Init(void);

/**
 * @brief	Sets up I2C Fast Plus mode
 * @param	id	: Must always be I2C0
 * @return	Nothing
 * @note	This function must be called before calling
 *          Chip_I2C_SetClockRate() to set clock rates above
 *          normal range 100KHz to 400KHz. Only I2C0 supports
 *          this mode.
 */
STATIC INLINE void Board_I2C_EnableFastPlus(I2C_ID_T id)
{
	Chip_IOCON_SetI2CPad(LPC_IOCON, I2CPADCFG_FAST_MODE_PLUS);
}

/**
 * @brief	Disables I2C Fast plus mode and enable normal mode
 * @param	id	: Must always be I2C0
 * @return	Nothing
 */
STATIC INLINE void Board_I2C_DisableFastPlus(I2C_ID_T id)
{
	Chip_IOCON_SetI2CPad(LPC_IOCON, I2CPADCFG_STD_MODE);
}

/**
 * @brief	Create Serial Stream
 * @param	Stream	: Pointer to stream
 * @return	Nothing
 */
void Serial_CreateStream(void *Stream);

/**
 * @brief	calc crc16
 * @param	Array	: point to source array
 * @param	Rcvbuf	: point to return crc result 
 * @param	Len	: crc calc length
 * @return	return crc result 
 */
unsigned short crc_16(unsigned char *Array, unsigned char *Rcvbuf,unsigned int Len);


void Board_GPIOInput_DIROut(uint8_t GIOPNum);
void Board_GPIOOutput_DIROut(uint8_t GIOPNum);

/**
 * @}
 */

#include "board_api.h"
#include "lpc_phy.h"

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H_ */
