/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#include "tcpserver.h"
#include "board.h"
#include "lwip/opt.h"
#include <string.h>

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"
/*-----------------------------------------------------------------------------------*/
enum ENUMCMD
{
	CMDNONE = 0x00,	//reserved
	CMDRALL = 0x01,	//read all
	CMDRAFB = 0x81,	//read all feedback data
	CMDRREG = 0x05,	//read register
	CMDRFB  = 0x85,	//read feedback data
	CMDWREG = 0x0A,	//write register
	CMDWFB  = 0x8A,	//write feedback
	CMDBDCT = 0x0F,	//broadcast registers 0 to 114 states
	CMDBDFB = 0x8F,	//broadcast registers 0 to 114 states
}enumcmd;
/* CMD + LENGTH MSB + LENGTH LSB + REGISTER MSB + REGISTER LSB + WR_LENGTH MSB + WR_LENGTH LSB + DATA 0,1,2,3...N + CRC LSB + CRC MSB */
/* Server address configuration. */

//#define configSVR_ADDR0 118
//#define configSVR_ADDR1 25
//#define configSVR_ADDR2 36
//#define configSVR_ADDR3 169		
//#define configSVR_PORT0 9985

//#define configSVR_ADDR0 192
//#define configSVR_ADDR1 168
//#define configSVR_ADDR2 0
//#define configSVR_ADDR3 101		
//#define configSVR_PORT0 9985

#define configSVR_ADDR0 165
#define configSVR_ADDR1 114
#define configSVR_ADDR2 50
#define configSVR_ADDR3 177		
#define configSVR_PORT0 9985

#define BROADCAST_LENGTH 500

unsigned short itxlen=0;
bool rxtxflag=false;
unsigned int u8sever_port=configSVR_PORT0;
extern uint32_t  DataSaveCnt;
/*-----------------------------------------------------------------------------------*/
static void ethbuf_cpy(void *srcptr, unsigned char *desptr, unsigned short datalen) 
{
	unsigned short u16cnt = datalen;
	unsigned short u16i=0;
	unsigned char * u8ptr=srcptr;
	*desptr = datalen;
	desptr++;
	for(u16i=0;u16i<u16cnt;u16i++)
	{
		*desptr = *u8ptr;
		desptr++;
		u8ptr++;
	}
}

static void ethdata_decode(void) 
{
	unsigned short u16i=0,idx,crcflag,cmdrxlen,cmdrxreg,cmdrxwrlen;
	unsigned char ethcmd=0,outupdate,u8i;
	unsigned char idxcrc[2]={0,0};
	idx = crc_16(&ethbufRx[1],idxcrc,ethbufRx[0]);
	if(1)//((idxcrc[0]=ethbufRx[ethbufRx[0]-2])&&(idxcrc[1]=ethbufRx[ethbufRx[0]-1]))  //CRC LSB AT THE FIRST AND MSB AT THE END
	{
		crcflag = 1;
		ethcmd = ethbufRx[1];
		cmdrxlen 		= (ethbufRx[2]<<8) + ethbufRx[3];
		cmdrxreg 		= (ethbufRx[4]<<8) + ethbufRx[5];
		cmdrxwrlen 	= (ethbufRx[6]<<8) + ethbufRx[7];
//		printf("Recved CRC 1\n");
	}
	else if((idxcrc[1]=ethbufRx[ethbufRx[0]-2])&&(idxcrc[0]=ethbufRx[ethbufRx[0]-1]))
	{
		crcflag =2;
		ethcmd 			= ethbufRx[1];
		cmdrxlen 		= (ethbufRx[2]<<8) + ethbufRx[3];
		cmdrxreg 		= (ethbufRx[4]<<8) + ethbufRx[5];
		cmdrxwrlen 	= (ethbufRx[6]<<8) + ethbufRx[7];
	}
	else
	{
		crcflag = 0;
		printf("Recved CRC error\n");
		printf("Recved CRC 2\n");
	}
	if(crcflag)
	{
		rxtxflag = true;
		switch(ethcmd){
			case CMDRALL:
				cmdrxwrlen = 2000;
				ethbufTx[0] = CMDRAFB;
				ethbufTx[1] = ((2000+9)&0xFF00)>>8;
				ethbufTx[2] = (2000+9)&0xFF;
				ethbufTx[3] = ethbufRx[4];
				ethbufTx[4] = ethbufRx[5];
				ethbufTx[5] = (2000&0xFF00)>>8;
				ethbufTx[6] = 2000&0xFF;
			  memcpy(&ethbufTx[7],&sysGrpREG.sys8uREG[0],cmdrxwrlen);
				crc_16(&ethbufTx[0],&idxcrc[0],cmdrxwrlen+7);
				if(crcflag==1)
				{
					ethbufTx[7+cmdrxwrlen] = idxcrc[0];
					ethbufTx[8+cmdrxwrlen] = idxcrc[1];
				}
				else
				{
					ethbufTx[7+cmdrxwrlen] = idxcrc[1];
					ethbufTx[8+cmdrxwrlen] = idxcrc[0];
				}
				itxlen = cmdrxwrlen + 9;
				break;
			case CMDRREG:
				ethbufTx[0] = CMDRFB;
				ethbufTx[1] = ((cmdrxwrlen+9)&0xFF00)>>8;
				ethbufTx[2] = (cmdrxwrlen+9)&0xFF;
				ethbufTx[3] = ethbufRx[4];
				ethbufTx[4] = ethbufRx[5];
				ethbufTx[5] = ethbufRx[6];
				ethbufTx[6] = ethbufRx[7];
				memcpy(&ethbufTx[7],&sysGrpREG.sys8uREG[cmdrxreg],cmdrxwrlen);
				crc_16(&ethbufTx[0],&idxcrc[0],cmdrxwrlen+7);
				if(crcflag==1)
				{
					ethbufTx[7+cmdrxwrlen] = idxcrc[0];
					ethbufTx[8+cmdrxwrlen] = idxcrc[1];
				}
				else
				{
					ethbufTx[7+cmdrxwrlen] = idxcrc[1];
					ethbufTx[8+cmdrxwrlen] = idxcrc[0];
				}
				itxlen = cmdrxwrlen + 9;
				break;
			case CMDWREG:
				ethbufTx[0] = CMDWFB;
				ethbufTx[1] = ((cmdrxwrlen+9)&0xFF00)>>8;
				ethbufTx[2] = (cmdrxwrlen+9)&0xFF;
				ethbufTx[3] = ethbufRx[4];
				ethbufTx[4] = ethbufRx[5];
				ethbufTx[5] = ethbufRx[6];
				ethbufTx[6] = ethbufRx[7];
				memcpy(&sysGrpREG.sys8uREG[cmdrxreg],&ethbufRx[8],cmdrxwrlen);
			DataSaveCnt++;
				if(sysGrpREG.sysPMACREG.sysERRCLR[0]||sysGrpREG.sysPMACREG.sysERRCLR[1])
				{
					//reserved
				}
				/*
				outupdate = sysGrpREG.sysPMACREG.sysOUTSET[0];
				for(u8i=0;u8i<8;u8i++)
				{
					if(outupdate&(0x01<<u8i))
					Board_GPIOOutput_Set(u8i,false);		
					else Board_GPIOOutput_Set(u8i,true);	
				}
				outupdate = sysGrpREG.sysPMACREG.sysOUTSET[1];
				*/
//				for(u8i=8;u8i<9;u8i++)
//				{
//					if(outupdate&(0x01<<u8i))
//					Board_GPIOOutput_Set(u8i,true);		
//					else Board_GPIOOutput_Set(u8i,false);	
//				}
				
//				if(sysGrpREG.sysPMACREG.sysOUTSET[0])
//				{
//					outupdate = sysGrpREG.sysPMACREG.sysOUTSET[0];
//					for(u8i=0;u8i<8;u8i++)
//					{
//						outupdate = (outupdate&0xFF)>>u8i;
//						if(outupdate&0x01)
//						Board_GPIOOutput_Set(u8i,true);						
//					}
//				}
//				if(sysGrpREG.sysPMACREG.sysOUTSET[1])
//				{
//					outupdate = sysGrpREG.sysPMACREG.sysOUTSET[1];
//					for(u8i=0;u8i<8;u8i++)
//					{
//						outupdate = (outupdate&0xFF)>>u8i;
//						if(outupdate&0x01)
//						Board_GPIOOutput_Set(u8i,true);						
//					}
//				}
//				if(sysGrpREG.sysPMACREG.sysOUTCLR[0])
//				{
//					outupdate = sysGrpREG.sysPMACREG.sysOUTCLR[0];
//					for(u8i=0;u8i<8;u8i++)
//					{
//						outupdate = (outupdate&0xFF)>>u8i;
//						if(outupdate&0x01)
//						Board_GPIOOutput_Set(u8i,false);						
//					}
//				}
//				if(sysGrpREG.sysPMACREG.sysOUTCLR[1])
//				{
//					outupdate = sysGrpREG.sysPMACREG.sysOUTCLR[1];
//					for(u8i=0;u8i<8;u8i++)
//					{
//						outupdate = (outupdate&0xFF)>>u8i;
//						if(outupdate&0x01)
//						Board_GPIOOutput_Set(u8i,false);						
//					}
//				}
				memcpy(&ethbufTx[7],&sysGrpREG.sys8uREG[cmdrxreg],cmdrxwrlen);
				crc_16(&ethbufTx[0],&idxcrc[0],cmdrxwrlen+7);
				if(crcflag==1)
				{
					ethbufTx[7+cmdrxwrlen] = idxcrc[0];
					ethbufTx[8+cmdrxwrlen] = idxcrc[1];
				}
				else
				{
					ethbufTx[7+cmdrxwrlen] = idxcrc[1];
					ethbufTx[8+cmdrxwrlen] = idxcrc[0];
				}
				itxlen = cmdrxwrlen + 9;
				break;
			case CMDBDCT:
				ethbufTx[0] = CMDBDFB;
				ethbufTx[1] = ((cmdrxwrlen+9)&0xFF00)>>8;
				ethbufTx[2] = (cmdrxwrlen+9)&0xFF;
				ethbufTx[3] = ethbufRx[4];
				ethbufTx[4] = ethbufRx[5];
				ethbufTx[5] = ethbufRx[6];
				ethbufTx[6] = ethbufRx[7];
				memcpy(&ethbufTx[7],&sysGrpREG.sys8uREG[cmdrxreg],cmdrxwrlen);
				crc_16(&ethbufTx[0],&idxcrc[0],cmdrxwrlen+7);
				if(crcflag==1)
				{
					ethbufTx[7+cmdrxwrlen] = idxcrc[0];
					ethbufTx[8+cmdrxwrlen] = idxcrc[1];
				}
				else
				{
					ethbufTx[7+cmdrxwrlen] = idxcrc[1];
					ethbufTx[8+cmdrxwrlen] = idxcrc[0];
				}
				itxlen = cmdrxwrlen + 9;
				break;
			default:
				rxtxflag = false;
				printf("Recved CMD error\n");
				break;

		}
	}
	else;

}

static void ethdata_broadcast(void)
{
	unsigned char idxcrc[2]={0,0};
	ethbufTx[0] = CMDRFB; //CMDBDFB;
	ethbufTx[1] = ((BROADCAST_LENGTH+9)&0xFF00)>>8;
	ethbufTx[2] = (BROADCAST_LENGTH+9)&0xFF;
	ethbufTx[3] = ethbufRx[4];
	ethbufTx[4] = ethbufRx[5];
	ethbufTx[5] = ethbufRx[6];
	ethbufTx[6] = ethbufRx[7];
	memcpy(&ethbufTx[7],&sysGrpREG.sys8uREG[0],BROADCAST_LENGTH);
	crc_16(&ethbufTx[0],&idxcrc[0],BROADCAST_LENGTH+7);
	ethbufTx[7+BROADCAST_LENGTH] = idxcrc[0];
	ethbufTx[8+BROADCAST_LENGTH] = idxcrc[1];
	itxlen = BROADCAST_LENGTH + 9;
}	

static void 
tcpclient_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err;
	unsigned int u32cnt=0;
	unsigned int client_idx=0;
	bool clientloop = false;
  LWIP_UNUSED_ARG(arg);
	
	while (1) {
		printf("tcpclient_thread starting\r\n");
		while(!ip_rdy)
		{
			printf("waiting DHCP bound\r\n");
			vTaskDelay(configTICK_RATE_HZ);
		}
		printf("Creating netconn_new\r\n");
  /* Create a new connection identifier. */
  /* Bind connection to well known port number 7. */
#if LWIP_IPV6
  conn = netconn_new(NETCONN_TCP_IPV6);
  netconn_bind(conn, IP6_ADDR_ANY, 7);
#else /* LWIP_IPV6 */
  conn = netconn_new(NETCONN_TCP);
#endif /* LWIP_IPV6 */
  LWIP_ERROR("tcpclient: invalid conn", (conn != NULL), return;);
	IP4_ADDR(&lpc_sever_ipaddr, configSVR_ADDR0, configSVR_ADDR1, configSVR_ADDR2, configSVR_ADDR3);

	if(conn!=NULL)
	{
			err = netconn_bind(conn, &lpc_client_ipaddr, u8sever_port);
			if(err != ERR_OK) printf("netconn_bind failed\n");
			err = netconn_connect (conn, &lpc_sever_ipaddr, u8sever_port ); 

			/* Process the new connection. */
			if (err == ERR_OK) {
				struct netbuf *buf;
				void *data;
				u16_t len;
				clientloop = true;
				printf("netconn_connect successful\n");
				while (clientloop) {
					printf("clientloop\n");
					netconn_set_recvtimeout(conn,1000*3);
				while ((err = netconn_recv(conn, &buf)) == ERR_OK) {
//					if ((err = netconn_recv(conn, &buf)) == ERR_OK) {
						printf("netconn_recv: \"%d\"\n",u32cnt++);
						do {
								 netbuf_data(buf, &data, &len);
								 ethbuf_cpy(data,ethbufRx,len);
//								 printf("ethbuf_cpy done\n");
								 ethdata_decode();
//								 printf("decode done\n");
								if(rxtxflag)
								{
//									printf("Sending %d\n",itxlen);
									err = netconn_write(conn, ethbufTx, itxlen, NETCONN_COPY);
									rxtxflag = false;
									printf("Sent\n");
								}
								else
								{
									printf("Recved incorrect data\n");
								}
	#if 0
								if (err != ERR_OK) {
									printf("tcpclient: netconn_write: error \"%s\"\n", lwip_strerr(err));
								}
	#endif
						} while (netbuf_next(buf) >= 0);
						netbuf_delete(buf);
					}

//					if(broadcastflag&&(err==ERR_TIMEOUT))
					if(err==ERR_TIMEOUT)
					{
						printf("broadcastflag\n");
						ethdata_broadcast();
						err = netconn_write(conn, ethbufTx, itxlen, NETCONN_NOCOPY);
						broadcastflag = false;
//						printf("netconn_write\n");
					}
					else if(err!=ERR_TIMEOUT)clientloop = false;
					else;
				}
				clientloop = false;
				/*printf("Got EOF, looping\n");*/ 
				/* Close connection and discard connection identifier. */
				printf("netconn_recv error\r\n");
				netconn_close(conn);
				printf("netconn_close\r\n");
				netconn_delete(conn);
				printf("netconn_delete\r\n");
				vTaskDelay(1000);
			}
			else
			{
				clientloop = false;
				netconn_close(conn);
				printf("netconn_close\r\n");
				netconn_delete(conn);
				printf("netconn_delete\r\n");
				printf("create netconn_connect failed.\r\n");
				vTaskDelay(1000);
			}
		}
		else
		{
			clientloop = false;
			printf("create netconn_new failed.\r\n");
			vTaskDelay(1000);
		}
  }
}

/*-----------------------------------------------------------------------------------*/
void
tcpserver_init(void)
{
//  sys_thread_new("tcpserver_thread", tcpserver_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
  sys_thread_new("tcpclient_thread", tcpclient_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}
/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */


//static void 
//tcpserver_thread(void *arg)
//{
//  struct netconn *conn, *newconn;
//  err_t err;
//  LWIP_UNUSED_ARG(arg);

//  /* Create a new connection identifier. */
//  conn = netconn_new(NETCONN_TCP);

//  /* Bind connection to well known port number 7. */
//  netconn_bind(conn, NULL, 7);

//  /* Tell connection to go into listening mode. */
//  netconn_listen(conn);

//  while (1) {

//    /* Grab new connection. */
//    err = netconn_accept(conn, &newconn);
//    /*printf("accepted new connection %p\n", newconn);*/
//    /* Process the new connection. */
//    if (err == ERR_OK) {
//      struct netbuf *buf;
//      void *data;
//      u16_t len;
//      
//      while ((err = netconn_recv(newconn, &buf)) == ERR_OK) {
//        /*printf("Recved\n");*/
//				printf("Recved\n");
//        do {
//             netbuf_data(buf, &data, &len);
//						 ethbuf_cpy(data,ethbufRx,len);
//						 ethdata_decode();
//						if(rxtxflag)
//						{
//							err = netconn_write(newconn, ethbufTx, itxlen, NETCONN_COPY);
//							printf("Sent\n");
//						}
//						else
//						{
//							printf("Recved incorrect data\n");
//						}
//#if 0
//            if (err != ERR_OK) {
//              printf("tcpecho: netconn_write: error \"%s\"\n", lwip_strerr(err));
//            }
//#endif
//        } while (netbuf_next(buf) >= 0);
//        netbuf_delete(buf);
//      }
//      /*printf("Got EOF, looping\n");*/ 
//      /* Close connection and discard connection identifier. */
//      netconn_close(newconn);
//      netconn_delete(newconn);
//    }
//  }
//}
