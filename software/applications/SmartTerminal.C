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
	
				// ��λϵͳ���б�־��				
    Board_GPIOOutput_Set(GPIO_OUT_LED_SYS_RUN_IX, flag);
				sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB |= (0x01<<6); // ϵͳ����				
							
				// ���Ÿ澯��Ϣ����
				bval = sysGrpREG.sysPMACREG.sysOUTSET[0] ;   //ÿ�η�ӳ���״̬ʱ������������λ����0��Ȼ�������������
    sysGrpREG.sysPMACREG.ioOUTPUT[0] = (bval & ~(0x01));
				sysGrpREG.sysPMACREG.ioOUTPUT[0] = (bval & ~(0x01));
				bval = Board_GPIOInput_State(GPIO_IN_DOOR_OPEN_IX);
				if (bval) { //���Ŵ�
					   Board_GPIOOutput_Set(GPIO_OUT_LED_DOOR_OPEN_IX, false);  // �����Ѵ�ָʾ��
					   sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<6);      // ��λ���Ѵ򿪱�־
					   sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB |= (0x01<<4);    // �ŴŸ澯
					   if (sysGrpREG.sysPMACREG.sysOUTSET[0] &(0x01<<6)) {  //��������򿪣��������ʱ�ȴ�������
									    wval = (sysGrpREG.sysPMACREG.sysOUTSET[1] >> 5) & 0x00000003;  //ȡ���趨�Ĳ�������ʱ��
									    if (wval == 0) {
													    wval = 5;   // Ĭ����ʱ5���Ӵ򿪲���
													} 
													wval = wval * 1000 * 60 / configTICK_RATE_HZ; //��������ӳ�ʱ��,ת����ms
				         if (dlyCnt++ > wval) {   //��������ʱ�䣬�򿪷�����
														    if (sysGrpREG.sysPMACREG.sysOUTSET[0] & (0x01<<0)) {
												 	        Board_GPIOOutput_Set(GPIO_OUT_BUZZER_IX, true); //�򿪷�����
														        sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<0);
																		} else {
												 	        Board_GPIOOutput_Set(GPIO_OUT_BUZZER_IX, false); //�رշ�����
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
				
				//���G_IN1��״̬
    bval = Board_GPIOInput_State(GPIO_IN_G_IN1_IX);   
				if (bval) {
				    sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<5);
				} else {
				    sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<5);				
				}
							
				// ��λ�¶ȳ��ޱ�־
				sval = sysGrpREG.sysPMACREG.sysTEMP1[0];  //��òɼ����¶�ֵ
				if (((sval&0x80)!=0) || sval <  sysGrpREG.sysPMACREG.sysTEMP1LOW[0]) { 
				    Board_GPIOOutput_Set(GPIO_OUT_HEAT_IX, false);// ��������
				    Board_GPIOOutput_Set(GPIO_OUT_FAN_IX, true);//�رշ���
								sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<4);   //����
								sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<1);		//����
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	|= (0x01<<5);							
				} else if( sval > 50 || sval > sysGrpREG.sysPMACREG.sysTEMP1HIGH[0]) { 
				    Board_GPIOOutput_Set(GPIO_OUT_FAN_IX, false);//��������
				    Board_GPIOOutput_Set(GPIO_OUT_HEAT_IX, true);// �رռ���
								sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<4);
								sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<1);	
					   sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	|= (0x01<<5);		
				} else {    //�������������ڼ�
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	&= ~(0x01<<5);		
					
        if (sysGrpREG.sysPMACREG.sysOUTSET[0]&(0x01<<4)) {
								    Board_GPIOOutput_Set(GPIO_OUT_HEAT_IX, false);// ��������
									   sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<4);   //����
								} else {
				        Board_GPIOOutput_Set(GPIO_OUT_HEAT_IX, true);// �رռ���		
												sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<4); 
								} 
								
        if (sysGrpREG.sysPMACREG.sysOUTSET[0]&(0x01<<1)) {
								    Board_GPIOOutput_Set(GPIO_OUT_FAN_IX, false);// ��������
								    sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<1);										
								} else {
				        Board_GPIOOutput_Set(GPIO_OUT_FAN_IX, true);// �رշ���			
												sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<1);	
								}								
				}
				
				// ����
					bval = Board_GPIOInput_State(GPIO_IN_DOOR_OPEN_IX); //����Ŵ�
				if (bval && sysGrpREG.sysPMACREG.sysOUTSET[0] & (0x01<<5)) {
				    Board_GPIOOutput_Set(GPIO_OUT_LIGHT_IX, true); //��
        sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<5);					
				} else {
				    Board_GPIOOutput_Set(GPIO_OUT_LIGHT_IX, false); //�ر�		
					   sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<5);
				}						
							
				//��λ�����
				if (sysGrpREG.sysPMACREG.sysOUTSET[0] & (0x01<<3)) {
				    Board_GPIOOutput_Set(GPIO_OUT_CAMERA_IX, false);
        sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<3);					
				} else {
				    Board_GPIOOutput_Set(GPIO_OUT_CAMERA_IX, true);					
					   sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<3);
				}				
				
				//��λ�����״̬
				if (sysGrpREG.sysPMACREG.sysOUTSET[0] & (0x01<<7)) {
				    Board_GPIOOutput_Set(GPIO_OUT_FLIGHT_IX, false);
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	|= (0x01<<3);
        sysGrpREG.sysPMACREG.ioOUTPUT[0] |= (0x01<<7);					
				} else {
				    Board_GPIOOutput_Set(GPIO_OUT_FLIGHT_IX, true);
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	&= ~(0x01<<3);					
					   sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<7);
				}
				
				//��λ������״̬
				if (sysGrpREG.sysPMACREG.sysOUTSET[0] & (0x01<<2)) {
				    Board_GPIOOutput_Set(GPIO_OUT_SWITCH_IX, false); //�򿪽�����
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	|= (0x01<<2);
        sysGrpREG.sysPMACREG.ioOUTPUT[0]	|= (0x01<<2);
					   SwitchDlyOpenCnt = 0;
				} else {
					   SwitchDlyOpenCnt++;
					   if (SwitchDlyOpenCnt > 1) {
								    sysGrpREG.sysPMACREG.sysOUTSET[0] |= (0x01<<2);
								}					
				    Board_GPIOOutput_Set(GPIO_OUT_SWITCH_IX, true);  //�رս�����
        sysGrpREG.sysPMACREG.sysSTAT.sysSTATu8.LSB	&= ~(0x01<<2);					
					   sysGrpREG.sysPMACREG.ioOUTPUT[0] &= ~(0x01<<2);
					

				}				
}