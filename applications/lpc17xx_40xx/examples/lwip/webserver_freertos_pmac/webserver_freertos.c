/*
 * @brief LWIP FreeRTOS HTTP Webserver example
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
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

#include "lwip/init.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/memp.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/timers.h"
#include "netif/etharp.h"
#include "tcpserver.h"
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif

#include <string.h>
#include "board.h"
#include "lpc_phy.h"
#include "arch\lpc17xx_40xx_emac.h"
#include "arch\lpc_arch.h"
#include "arch\sys_arch.h"
#include "lpc_phy.h"/* For the PHY monitor support */
#include "SmartTerminal.H"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
static I2C_ID_T i2cDev = DEFAULT_I2C; /* Currently active I2C device */
static I2C_XFER_T eep_xfer;
volatile uint8_t eepTxData[260]={0,};
volatile uint8_t eepRxData[256]={0,};
static I2C_XFER_T temp_xfer;
volatile uint8_t tempTxData[4]={0,};
volatile uint8_t tempRxData[4]={0,};
/* NETIF data */
struct netif lpc_netif;
ip_addr_t lpc_sever_ipaddr;
ip_addr_t lpc_client_ipaddr;
bool ip_rdy = false;
bool broadcastflag = false;
uint32_t  DataSaveCnt = 0;
/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/
union SYS_UNION_REG sysGrpREG;
unsigned char ethbufRx[ETH_BUF_RX_LEN]={0,};
unsigned char ethbufTx[ETH_BUF_TX_LEN]={0,};
/*****************************************************************************
 * Private functions
 ****************************************************************************/

extern void http_server_netconn_init(void);
void DataSave(void);
void DataLoad(void);

/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	/* LED0 is used for the link status, on = PHY cable detected */
	/* Initial LED state is off to show an unconnected cable state */
//	Board_LED_Set(0, false);
}

/* Callback for TCPIP thread to indicate TCPIP init is done */
static void tcpip_init_done_signal(void *arg)
{
	/* Tell main thread TCP/IP init is done */
	*(s32_t *) arg = 1;
}

/* LWIP kickoff and PHY link monitor thread */
static void vSetupIFTask(void *pvParameters) {
	ip_addr_t ipaddr, netmask, gw;
	volatile s32_t tcpipdone = 0;
	uint32_t physts;
	static int prt_ip = 0;
	
	DEBUGSTR("LWIP HTTP Web Server FreeRTOS Demo...\r\n");
	
	/* Wait until the TCP/IP thread is finished before
	   continuing or wierd things may happen */
	DEBUGSTR("Waiting for TCPIP thread to initialize...\r\n");
	tcpip_init(tcpip_init_done_signal, (void *) &tcpipdone);
	while (!tcpipdone) {
		msDelay(1);
	}

	DEBUGSTR("Starting LWIP HTTP server...\r\n");

	/* Static IP assignment */
#if LWIP_DHCP
	IP4_ADDR(&gw, 0, 0, 0, 0);
	IP4_ADDR(&ipaddr, 0, 0, 0, 0);
	IP4_ADDR(&netmask, 0, 0, 0, 0);
#else
//	IP4_ADDR(&gw, 192, 168, 1, 2);
//	IP4_ADDR(&ipaddr, 192, 168, 1, 116);
//	IP4_ADDR(&netmask, 255, 255, 255, 0);
	
	IP4_ADDR(&gw, 165, 114, 50, 1);
	IP4_ADDR(&ipaddr, 165, 114, 50, 133);
	IP4_ADDR(&netmask, 255, 255, 255, 0);
	
//	IP4_ADDR(&gw, 192, 168, 0, 1);
//	IP4_ADDR(&ipaddr, 192, 168, 0, 133);
//	IP4_ADDR(&netmask, 255, 255, 255, 0);
#endif

	/* Add netif interface for lpc17xx_8x */
	memset(&lpc_netif, 0, sizeof(lpc_netif));
	if (!netif_add(&lpc_netif, &ipaddr, &netmask, &gw, NULL, lpc_enetif_init,
				   tcpip_input)) {
		DEBUGSTR("Net interface failed to initialize\r\n");
		while(1);			   
	}
	netif_set_default(&lpc_netif);
	netif_set_up(&lpc_netif);

	/* Enable MAC interrupts only after LWIP is ready */
	NVIC_SetPriority(ETHERNET_IRQn, config_ETHERNET_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(ETHERNET_IRQn);

#if LWIP_DHCP
	dhcp_start(&lpc_netif);
#endif
	
	/* Initialize and start application */
	http_server_netconn_init();
	tcpserver_init();
	/* This loop monitors the PHY link and will handle cable events
	   via the PHY driver. */
	while (1) {
		/* Call the PHY status update state machine once in a while
		   to keep the link status up-to-date */
		physts = lpcPHYStsPoll();

		/* Only check for connection state when the PHY status has changed */
		if (physts & PHY_LINK_CHANGED) {
			if (physts & PHY_LINK_CONNECTED) {
//				Board_LED_Set(0, true);
				prt_ip = 0;
				ip_rdy = false;

				/* Set interface speed and duplex */
				if (physts & PHY_LINK_SPEED100) {
					Chip_ENET_Set100Mbps(LPC_ETHERNET);
					NETIF_INIT_SNMP(&lpc_netif, snmp_ifType_ethernet_csmacd, 100000000);
				}
				else {
					Chip_ENET_Set10Mbps(LPC_ETHERNET);
					NETIF_INIT_SNMP(&lpc_netif, snmp_ifType_ethernet_csmacd, 10000000);
				}
				if (physts & PHY_LINK_FULLDUPLX) {
					Chip_ENET_SetFullDuplex(LPC_ETHERNET);
				}
				else {
					Chip_ENET_SetHalfDuplex(LPC_ETHERNET);
				}

				tcpip_callback_with_block((tcpip_callback_fn) netif_set_link_up,
										  (void *) &lpc_netif, 1);
			}
			else {
//				Board_LED_Set(0, false);
				tcpip_callback_with_block((tcpip_callback_fn) netif_set_link_down,
										  (void *) &lpc_netif, 1);
			}

			/* Delay for link detection (250mS) */
			vTaskDelay(configTICK_RATE_HZ / 4);
		}

		/* Print IP address info */
		if (!prt_ip) {
			if (lpc_netif.ip_addr.addr) {
				static char tmp_buff[16];
				DEBUGOUT("IP_ADDR    : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &lpc_netif.ip_addr, tmp_buff, 16));
				DEBUGOUT("NET_MASK   : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &lpc_netif.netmask, tmp_buff, 16));
				DEBUGOUT("GATEWAY_IP : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &lpc_netif.gw, tmp_buff, 16));
				lpc_client_ipaddr = lpc_netif.ip_addr;
				prt_ip = 1;
				ip_rdy = true;
			}
		}
	}
}

static void vAppFuncTask(void *pvParameters) {
	uint8_t tmp,idx;
	temp_xfer.slaveAddr = I2C_SLAVE_TMP75_ADDR;
	temp_xfer.txBuff = (uint8_t *) &tempTxData;
	temp_xfer.rxBuff = (uint8_t *) &tempRxData;
	temp_xfer.txSz = 1;
	temp_xfer.rxSz = 2;
	
 DataLoad();
	UserystemInit();
	
	while(1)
	{
		tmp = Chip_I2C_MasterSend(i2cDev, temp_xfer.slaveAddr, temp_xfer.txBuff, temp_xfer.txSz);
		tmp = Chip_I2C_MasterRead(i2cDev, temp_xfer.slaveAddr, temp_xfer.rxBuff, temp_xfer.rxSz);

		sysGrpREG.sysPMACREG.sysTEMP1[0] = tempRxData[0];
		
		if (DataSaveCnt) {
		    DataSave();
			   DataSaveCnt--;
		}
		
		SystemRunChk();
		
		vTaskDelay(configTICK_RATE_HZ*2);
	}
}

/* Do not change vPowerFuncTask function */
static void vPowerFuncTask(void *pvParameters) {
	uint8_t tmp,idx;
	while(1)
	{
		Chip_TIMER_Reset(LPC_TIMER0);
		timerLst = 0;
		timerCur = 0;
		Board_GPIOOutput_Toggle(0);
		vTaskDelay(configTICK_RATE_HZ/100);
		NVIC_ClearPendingIRQ(TIMER0_IRQn);
		NVIC_EnableIRQ(TIMER0_IRQn);
		vTaskDelay(configTICK_RATE_HZ*3);
//		idx++;
//		if(idx>1) 
//		{
//			broadcastflag = true;
//			idx = 0;
//		}
	}
}

#define  EEPROM_PAGE_SIZE  32
#define  EEPROM_START_PAGE  2
#define  DB_BUF_START_REG  115
#define  DB_BUF_SIZE_MAX   2000

//void DataSave(void)
//{
//	uint8_t tmp;
//	unsigned short bufidx;	
// unsigned char addrbuf[2];
//	unsigned short  regaddr;
//	unsigned short  pageidx;
//	unsigned int  idx;	
//	unsigned char *pbuf;
//	
//	
//	pbuf = &sysGrpREG.sys8uREG[DB_BUF_START_REG];
//	
// eep_xfer.slaveAddr = I2C_SLAVE_EEPROM_ADDR;
//	eep_xfer.txBuff = (uint8_t *) &eepTxData[0] ;
//	eep_xfer.rxBuff = (uint8_t *) &eepRxData[0] ;
//	eep_xfer.txSz = 252;
//	eep_xfer.rxSz = 250;


//		// write DB_BUF_SIZE_MAX/EEPROM_PAGE_SIZE
//		for (bufidx=0,pageidx = EEPROM_START_PAGE; pageidx < DB_BUF_SIZE_MAX/EEPROM_PAGE_SIZE; pageidx++,bufidx+=EEPROM_PAGE_SIZE) {
//			   regaddr = pageidx * EEPROM_PAGE_SIZE;
//			   eepTxData[0] = (uint8_t)(regaddr);
//	     eepTxData[1] = (uint8_t)(regaddr>>8);
//			   pbuf = &sysGrpREG.sys8uREG[DB_BUF_START_REG+bufidx];						
//			   memcpy((uint8_t*)&eepTxData[2], pbuf, EEPROM_PAGE_SIZE);
//			   tmp = Chip_I2C_MasterSend(i2cDev, eep_xfer.slaveAddr, &eep_xfer.txBuff[0], EEPROM_PAGE_SIZE + 2);	
//		    //for(idx=0;idx<99999;idx++);	
//		}

//		for (idx = 0; idx < 2000; idx++) {
//		    sysGrpREG.sys8uREG[idx] = 0x00;
//		}
//		
//		// read		
//		for (bufidx=0,pageidx = EEPROM_START_PAGE; pageidx < DB_BUF_SIZE_MAX/EEPROM_PAGE_SIZE; pageidx++,bufidx+=EEPROM_PAGE_SIZE) {
//			   regaddr = pageidx * EEPROM_PAGE_SIZE;
//			   addrbuf[0] = (uint8_t)(regaddr);
//	     addrbuf[1] = (uint8_t)(regaddr>>8);
//			   tmp = Chip_I2C_MasterSend(i2cDev, eep_xfer.slaveAddr, &addrbuf[0], 2);
//					 //for(idx=0;idx<99999;idx++);	
//			   tmp = Chip_I2C_MasterRead(i2cDev, eep_xfer.slaveAddr, &eep_xfer.rxBuff[0], EEPROM_PAGE_SIZE);	
//			   pbuf = &sysGrpREG.sys8uREG[DB_BUF_START_REG+bufidx];				
//			   memcpy(pbuf, (uint8_t*)&eepRxData[0], EEPROM_PAGE_SIZE);
//		}		
//}

void DataSave(void)
{
	uint8_t tmp;
	unsigned short bufidx;	
 unsigned char addrbuf[2];
	unsigned short  regaddr;
	unsigned short  pageidx;
	unsigned int  idx;	
	unsigned char *pbuf;
	
	
 eep_xfer.slaveAddr = I2C_SLAVE_EEPROM_ADDR;

		// write DB_BUF_SIZE_MAX/EEPROM_PAGE_SIZE
		for (bufidx=0,pageidx = EEPROM_START_PAGE; pageidx < DB_BUF_SIZE_MAX/EEPROM_PAGE_SIZE; pageidx++,bufidx+=EEPROM_PAGE_SIZE) {
			   regaddr = pageidx * EEPROM_PAGE_SIZE;
			   eepTxData[0] = (uint8_t)(regaddr);
	     eepTxData[1] = (uint8_t)(regaddr>>8);
			   pbuf = &sysGrpREG.sys8uREG[DB_BUF_START_REG+bufidx];						
			   memcpy((uint8_t*)&eepTxData[2], pbuf, EEPROM_PAGE_SIZE);
			   tmp = Chip_I2C_MasterSend(i2cDev, eep_xfer.slaveAddr,  (uint8_t*)&eepTxData[0], EEPROM_PAGE_SIZE + 2);	
		    //for(idx=0;idx<99999;idx++);	
		}	
}


void DataLoad(void)
{
	uint8_t tmp;
	unsigned short bufidx;	
 unsigned char addrbuf[2];
	unsigned short  regaddr;
	unsigned short  pageidx;
	unsigned int  idx;	
	unsigned char *pbuf;
	

 eep_xfer.slaveAddr = I2C_SLAVE_EEPROM_ADDR;
	
		// read		
		for (bufidx=0,pageidx = EEPROM_START_PAGE; pageidx < DB_BUF_SIZE_MAX/EEPROM_PAGE_SIZE; pageidx++,bufidx+=EEPROM_PAGE_SIZE) {
			   regaddr = pageidx * EEPROM_PAGE_SIZE;
			   addrbuf[0] = (uint8_t)(regaddr);
	     addrbuf[1] = (uint8_t)(regaddr>>8);
			   tmp = Chip_I2C_MasterSend(i2cDev, eep_xfer.slaveAddr, &addrbuf[0], 2);
					 //for(idx=0;idx<99999;idx++);	
			   tmp = Chip_I2C_MasterRead(i2cDev, eep_xfer.slaveAddr, (uint8_t*)&eepRxData[0], EEPROM_PAGE_SIZE);	
			   pbuf = &sysGrpREG.sys8uREG[DB_BUF_START_REG+bufidx];				
			   memcpy(pbuf, (uint8_t*)&eepRxData[0], EEPROM_PAGE_SIZE);
		}		
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	MilliSecond delay function based on FreeRTOS
 * @param	ms	: Number of milliSeconds to delay
 * @return	Nothing
 * Needed for some functions, do not use prior to FreeRTOS running
 */
void msDelay(uint32_t ms)
{
	vTaskDelay((configTICK_RATE_HZ * ms) / 1000);
}

/**
 * @brief	main routine for example_lwip_tcpecho_freertos_17xx40xx
 * @return	Function should not exit
 */
int main(void)
{
	uint8_t tmp;
	unsigned int idx;
	prvSetupHardware();

//register testing
	sysGrpREG.sysPMACREG.devIDu8.LSB=0xa5;
	
	eep_xfer.slaveAddr = I2C_SLAVE_EEPROM_ADDR;
	eep_xfer.txBuff = (uint8_t *) &eepTxData[0];
	eep_xfer.rxBuff = (uint8_t *) &eepRxData[0];
	eep_xfer.txSz = 12;
	eep_xfer.rxSz = 32;
//i2c testing
	eepTxData[0]=0;
	eepTxData[1]=0;
	eep_xfer.txSz = 2;
	tmp = Chip_I2C_MasterSend(i2cDev, eep_xfer.slaveAddr, eep_xfer.txBuff, eep_xfer.txSz);
	tmp = Chip_I2C_MasterRead(i2cDev, eep_xfer.slaveAddr, eep_xfer.rxBuff, eep_xfer.rxSz);
	for(idx=0;idx<256;idx++)
	{
		eepTxData[idx] = idx;
	}
	eepTxData[0]=0;
	eepTxData[1]=0;
	eep_xfer.txSz = 34;
	tmp = Chip_I2C_MasterSend(i2cDev, eep_xfer.slaveAddr, eep_xfer.txBuff, eep_xfer.txSz);
	for(idx=0;idx<99999;idx++);
	eepTxData[0]=0;
	eepTxData[1]=0;
	eep_xfer.txSz = 2;
	tmp = Chip_I2C_MasterSend(i2cDev, eep_xfer.slaveAddr, eep_xfer.txBuff, eep_xfer.txSz);
	tmp = Chip_I2C_MasterRead(i2cDev, eep_xfer.slaveAddr, eep_xfer.rxBuff, eep_xfer.rxSz);
//spi testing	
	
	
	/* Add another thread for initializing physical interface. This
	   is delayed from the main LWIP initialization. */
	xTaskCreate(vSetupIFTask, (signed char *) "SetupIFx",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vAppFuncTask, (signed char *) "AppFunc",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);	
	
	xTaskCreate(vPowerFuncTask, (signed char *) "PowerFunc",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);	
	
	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

/**
 * @}
 */
