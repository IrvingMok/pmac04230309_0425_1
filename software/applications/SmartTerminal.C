#include <string.h>
#include "board.h"
#include "lpc_phy.h"
#include "arch\lpc17xx_40xx_emac.h"
#include "arch\lpc_arch.h"
#include "arch\sys_arch.h"
#include "lpc_phy.h"/* For the PHY monitor support */

uint32_t SwitchDlyOpenCnt = 0;

void UserystemInit(void)
{
	   unsigned char i;
	   unsigned char bval;
	   unsigned char u8i;
	   unsigned int  n;
	 
	
	   for (i = 0; i < MAXOUTPUTS; i++) {
				    Board_GPIOOutput_DIROut(i);
				}
				for (i = 0; i < MAXINPUTS; i++) {
				    Board_GPIOInput_DIROut(i);
				}
//				bval = sysGrpREG.sysPMACREG.sysOUTSET[0];
//				for(u8i=0;u8i<8;u8i++)
//				{
//					    if (u8i ==0 || u8i == 5) {
//					        //if (outupdate & (0x01<<0 | 0x01<<5))
//													//	   Board_GPIOOutput_Set(u8i,false);	
//									    //else Board_GPIOOutput_Set(u8i,true);	
//										   ;
//					    } else {
//					        if(bval&(0x01<<u8i))
//					        Board_GPIOOutput_Set(u8i,false);		
//					        else Board_GPIOOutput_Set(u8i,true);	
//				     }
//				}			
}

void SystemRunChk(void)	
{
	   static unsigned char flag;
	   static unsigned int  dlyCnt;
	   static unsigned char doflag;
	   unsigned char i;
	   unsigned char bval;
	   unsigned char bflag;
	   unsigned int  wval;
	   char  sval;
		
	
	   if (!flag) {
				    flag = 1;
				} else {
				    flag = 0;
				}
	
				// 置位系统运行标志，				
    Board_GPIOOutput_Set(GPIO_OUT_LED_SYS_RUN_IX, flag);
				sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB |= (0x01<<6); // 系统运行				
							
				// 开门告警信息处理
				bval = sysGrpREG.sysPMACREG.sysOUTSET[0] ;   //每次反映输出状态时，将蜂鸣器的位先清0，然后根据条件设置
    sysGrpREG.sysPMACREG.ioOUTPUT[0] = (bval & ~(0x01));
				sysGrpREG.sysPMACREG.ioOUTPUT[0] = (bval & ~(0x01));
				bval = Board_GPIOInput_State(GPIO_IN_DOOR_OPEN_IX);
				if (bval) { //柜门打开
					   Board_GPIOOutput_Set(GPIO_OUT_LED_DOOR_OPEN_IX, false);  // 打开门已打开指示灯
					   sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<6);      // 置位门已打开标志
					   sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB |= (0x01<<4);    // 门磁告警
					   if (sysGrpREG.sysPMACREG.sysOUTSET[0] &(0x01<<6)) {  //如果布防打开，则进入延时等待蜂鸣器
									    wval = (sysGrpREG.sysPMACREG.sysOUTSET[1] >> 5) & 0x00000003;  //取出设定的布防启动时间
									    if (wval == 0) {
													    wval = 5;   // 默认延时5分钟打开布防
													} 
													wval = wval * 1000 * 60 / configTICK_RATE_HZ; //这里计算延迟时间,转换成ms
				         if (dlyCnt++ > wval) {   //超过布放时间，打开蜂鸣器
														    if (sysGrpREG.sysPMACREG.sysOUTSET[0] & (0x01<<0)) {
												 	        Board_GPIOOutput_Set(GPIO_OUT_BUZZER_IX, true); //打开蜂鸣器
														        sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<0);
																		} else {
												 	        Board_GPIOOutput_Set(GPIO_OUT_BUZZER_IX, false); //关闭蜂鸣器
														        sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<0);																		
																		}														    
							      }
								}
				} else {
				    dlyCnt = 0;
					   Board_GPIOOutput_Set(GPIO_OUT_LED_DOOR_OPEN_IX, false);
								Board_GPIOOutput_Set(GPIO_OUT_BUZZER_IX, false); 
					   sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB &= ~(0x01<<4); 
				}
				
				//获得G_IN1的状态
    bval = Board_GPIOInput_State(GPIO_IN_G_IN1_IX);   
				if (bval) {
				    sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<5);
				} else {
				    sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<5);				
				}
							
				// 置位温度超限标志
				sval = sysGrpREG.sysPMACREG.sysTEMP1[0];  //获得采集的温度值
				if (((sval&0x80)!=0) || sval <  sysGrpREG.sysPMACREG.sysTEMP1LOW[0]) { 
				    Board_GPIOOutput_Set(GPIO_OUT_HEAT_IX, false);// 启动加热
				    Board_GPIOOutput_Set(GPIO_OUT_FAN_IX, true);//关闭风扇
								sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<4);   //加热
								sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<1);		//风扇
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	|= (0x01<<5);							
				} else if( sval > 50 || sval > sysGrpREG.sysPMACREG.sysTEMP1HIGH[0]) { 
				    Board_GPIOOutput_Set(GPIO_OUT_FAN_IX, false);//启动风扇
				    Board_GPIOOutput_Set(GPIO_OUT_HEAT_IX, true);// 关闭加热
								sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<4);
								sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<1);	
					   sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	|= (0x01<<5);		
				} else {    //处于正常运行期间
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	&= ~(0x01<<5);		
					
        if (sysGrpREG.sysPMACREG.sysOUTSET[0]&(0x01<<4)) {
								    Board_GPIOOutput_Set(GPIO_OUT_HEAT_IX, false);// 启动加热
									   sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<4);   //加热
								} else {
				        Board_GPIOOutput_Set(GPIO_OUT_HEAT_IX, true);// 关闭加热		
												sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<4); 
								} 
								
        if (sysGrpREG.sysPMACREG.sysOUTSET[0]&(0x01<<1)) {
								    Board_GPIOOutput_Set(GPIO_OUT_FAN_IX, false);// 启动风扇
								    sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<1);										
								} else {
				        Board_GPIOOutput_Set(GPIO_OUT_FAN_IX, true);// 关闭风扇			
												sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<1);	
								}								
				}
				
				// 照明
					bval = Board_GPIOInput_State(GPIO_IN_DOOR_OPEN_IX); //如果门打开
				if (bval && sysGrpREG.sysPMACREG.sysOUTSET[0] & (0x01<<5)) {
				    Board_GPIOOutput_Set(GPIO_OUT_LIGHT_IX, true); //打开
        sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<5);					
				} else {
				    Board_GPIOOutput_Set(GPIO_OUT_LIGHT_IX, false); //关闭		
					   sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<5);
				}						
							
				//置位摄像机
				if (sysGrpREG.sysPMACREG.sysOUTSET[0] & (0x01<<3)) {
				    Board_GPIOOutput_Set(GPIO_OUT_CAMERA_IX, false);
        sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<3);					
				} else {
				    Board_GPIOOutput_Set(GPIO_OUT_CAMERA_IX, true);					
					   sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<3);
				}				
				
				//置位补光灯状态
				if (sysGrpREG.sysPMACREG.sysOUTSET[0] & (0x01<<7)) {
				    Board_GPIOOutput_Set(GPIO_OUT_FLIGHT_IX, false);
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	|= (0x01<<3);
        sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<7);					
				} else {
				    Board_GPIOOutput_Set(GPIO_OUT_FLIGHT_IX, true);
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	&= ~(0x01<<3);					
					   sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<7);
				}
				
				//置位交换机状态
				if (sysGrpREG.sysPMACREG.sysOUTSET[0] & (0x01<<2)) {
				    Board_GPIOOutput_Set(GPIO_OUT_SWITCH_IX, false); //打开交换机
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	|= (0x01<<2);
        sysGrpREG.sysPMACREG.ioOUTPUT[0]	|= (0x01<<2);
					   SwitchDlyOpenCnt = 0;
				} else {
					   SwitchDlyOpenCnt++;
					   if (SwitchDlyOpenCnt > 1) {
								    sysGrpREG.sysPMACREG.sysOUTSET[0] |= (0x01<<2);
								}					
				    Board_GPIOOutput_Set(GPIO_OUT_SWITCH_IX, true);  //关闭交换机
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	&= ~(0x01<<2);					
					   sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<2);
					

				}				
}
