#include "NRF24L01.h"

#define CS1   GPIOA->BSRRL = GPIO_Pin_4
#define CS0   GPIOA->BSRRH = GPIO_Pin_4
#define CE1   GPIOC->BSRRL = GPIO_Pin_5
#define CE0   GPIOC->BSRRH = GPIO_Pin_5    

u8 TX_ADDRESS[5] = {0X12,0X34,0X56,0X78,0XA0};  // ����һ����̬���͵�ַ
u8 RX_ADDRESS[5] = {0X12,0X34,0X56,0X78,0XA0};

struct NRF24L01_Data_ NRF24L01_Data;
NRF_State NRF_Mode;
BOOL Is_ACKBUF_IDEL = True;
BOOL Is_TXBUF_IDEL = True;
BOOL Is_RXBUF_IDEL = True;

BOOL NRF24L01_Init(NRF_State Mode);
void NRF24L01_SendData(void);
BOOL NRF24L01_GetData(void);
BOOL NRF24L01_Request_ACKBUF(u8 IsImmediate);
void NRF24L01_Free_RXBUF(void);
//�ṹ���ʼ��
volatile struct NRF24L01_ NRF24L01 = 
{
	&NRF24L01_Data,
	NRF24L01_Init,
	NRF24L01_SendData,
	NRF24L01_GetData,
	NRF24L01_Request_ACKBUF,
	NRF24L01_Free_RXBUF
};

void NRF24L01_GPIO_Init(void)
{
	GPIO_InitTypeDef   GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA, ENABLE);
	
	//CE 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOC,&GPIO_InitStructure);
	//CS
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	CS1;
	//IRQ 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOC,&GPIO_InitStructure);

	//SPI	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;  //���ų�ʼ��
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_SPI1);  //��ӳ��
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource7,GPIO_AF_SPI1);
}

void NRF24L01_IRQ_Init(void)
{
	EXTI_InitTypeDef   EXTI_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  /* IRQ�����ж������� */
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource4); 
  EXTI_InitStructure.EXTI_Line = EXTI_Line4;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* IRQ�������ȼ����� */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 15;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void NRF24L01_SPI_Init(void)
{
	SPI_InitTypeDef    SPI_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1,ENABLE); 

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //ȫ˫��ģʽ
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;   //��Ϊ����ʹ��
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;   //���ݳ���8
	SPI_InitStructure.SPI_CPOL  = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;   //�������NSS����
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; //10M
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1,&SPI_InitStructure);
	SPI_Cmd(SPI1,ENABLE);
}



u8 NRF24L01_SPI_RW(u8 Data)
{
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET){}; 
  SPI_I2S_SendData(SPI1, Data);		
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET){};
  return SPI_I2S_ReceiveData(SPI1);
}

NRF_State NRF24L01_WriteReg(u8 Reg,u8 Data)
{
 	NRF_State Status;
	
  CS0;				
	Status = (NRF_State)NRF24L01_SPI_RW(Reg);	 
  NRF24L01_SPI_RW(Data);              
	CS1;			

  return(Status);
}

u8 NRF24L01_ReadReg(u8 Reg)
{
 	u8 Data;

 	CS0;				
	NRF24L01_SPI_RW(Reg); 
	Data = NRF24L01_SPI_RW(NOP);            
	CS1;		
 	
	return Data;

}

NRF_State NRF24L01_WriteBuf(u8 Reg,u8 Data[],u8 Length)
{
	NRF_State State;
	
	CS0;
	State = (NRF_State)NRF24L01_SPI_RW(Reg);
	for(u8 i = 0;i<Length;i++)
	{
		NRF24L01_SPI_RW(Data[i]);
	}
	CS1;

	return State;
}

NRF_State NRF24L01_ReadBuf(u8 Reg,u8 Data[],u8 Length)
{
	NRF_State Status;
	u8 i;

	CS0;
	Status = (NRF_State)NRF24L01_SPI_RW(Reg); 
	for(i=0;i<Length;i++)		 
	{	
		Data[i] = NRF24L01_SPI_RW(NOP); //��NRF24L01��ȡ���� 
	}
	CS1;	

	return Status;		
}

BOOL NRF24L01_ConnectCheck(void)
{
	u8 BUF1[5] = {0XC2,0XC2,0XC2,0XC2,0XC2};
	u8 BUF2[5] = {0};
	u8 i = 0;
	
	NRF24L01_WriteBuf(NRF_WRITE_REG + TX_ADDR,BUF1,5);
	NRF24L01_ReadBuf (TX_ADDR,BUF2,5); 
	                
	for(i=0;i<5;i++)
	{
		if(BUF2[i]!=0xC2)
		break;
	} 
	       
	if(i==5)
		return True ;         //MCU��NRF�ɹ����� 
	else
		return False ;        //MCU��NRF����������
}

void NRF24L01_SetRXMode(void)
{
	CE0;	 
	NRF24L01_WriteReg(NRF_WRITE_REG+NRF_CONFIG, 0x0F);   //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ 
	NRF_Mode = RXMode;
	CE1;
}

void NRF24L01_SetTXMode(void)
{
	CE0;		  
	NRF24L01_WriteReg(NRF_WRITE_REG+NRF_CONFIG,0X0E);    //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ,���������ж�
	NRF_Mode = TXMode;
	CE1;
}

BOOL NRF24L01_Init(NRF_State Mode)
{
	BOOL Mark = False;
	NRF24L01_GPIO_Init();
	NRF24L01_IRQ_Init();
	NRF24L01_SPI_Init();
	CE0;
	u8 State =NRF24L01_ReadReg(STATUS);		                                    //��ȡstatus�Ĵ�����ֵ
	NRF24L01_WriteReg(NRF_WRITE_REG+STATUS,State);                            //����жϱ�־
	NRF24L01_WriteReg(FLUSH_TX,NOP);   												                //���TX FIFO�Ĵ��� 
	NRF24L01_WriteReg(FLUSH_RX,NOP);                                          //���RX FIFO�Ĵ���                        
	
	NRF24L01_WriteBuf(NRF_WRITE_REG+TX_ADDR,TX_ADDRESS,5);			//дTX�ڵ��ַ 
	NRF24L01_WriteBuf(NRF_WRITE_REG+RX_ADDR_P0,TX_ADDRESS,5);  //����TX�ڵ��ַ,��ҪΪ��ʹ��ACK	 
	NRF24L01_WriteReg(NRF_WRITE_REG+SETUP_AW,0X03);            //�������ݿ��   
	NRF24L01_WriteReg(NRF_WRITE_REG+EN_AA,0X01);               //ʹ��ͨ��0���Զ�Ӧ��    
	NRF24L01_WriteReg(NRF_WRITE_REG+EN_RXADDR,0X01);           //ʹ��ͨ��0�Ľ��յ�ַ 
	NRF24L01_WriteReg(NRF_WRITE_REG+NRF_FEATURE , 0X06);                    //ʹ�ܶ�̬���ɣ�ʹ��Ӧ��������
	
	if(NRF24L01_ReadReg(NRF_FEATURE) == 0x00 && (NRF24L01_ReadReg(NRF_DYNPD) == 0x00)) 
		NRF24L01_WriteReg(NRF_ACTIVATE, 0x73);                                //�����ظ���д
	
	NRF24L01_WriteReg(NRF_WRITE_REG+NRF_FEATURE , 0X06);                    //ʹ�ܶ�̬���ɣ�ʹ��Ӧ��������
	NRF24L01_WriteReg(NRF_WRITE_REG+NRF_DYNPD, NRF_DPL_P0);
	NRF24L01_WriteReg(NRF_WRITE_REG+SETUP_RETR,NRF_ARD(4000) | NRF_ARC(2));  //�����Զ��ط����ʱ��:4000us + 86us;����Զ��ط�����:2��
	NRF24L01_WriteReg(NRF_WRITE_REG+RF_CH,50);                               //����RFͨ��Ϊ50
	NRF24L01_WriteReg(NRF_WRITE_REG+RF_SETUP,NRF_PWR_0dBm | NRF_DR_2Mbps);  //����TX�������,0db����,2Mbps,���������濪��   
	if(Mode == TXMode)
		NRF24L01_WriteReg(NRF_WRITE_REG+NRF_CONFIG,0x0E);    //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ,���������ж�
	else
		NRF24L01_WriteReg(NRF_WRITE_REG+NRF_CONFIG,0x0F);    //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ,���������ж�
	CE1;

	NRF_Mode = Mode;

	return Mark;
}
BOOL NRF24L01_Request_ACKBUF(u8 IsImmediate)
{
	if(IsImmediate == True)                        //������ȡACK_BUF��ʹ��Ȩ
	{
		return True;
	}
	else
	{
		if(Is_ACKBUF_IDEL == True)                  //ACK_BUF�Ǳ������ͣ��Ҵ����жϣ������жϵ������ݰ���ϣ���ˣ����뱣֤�����ݷ��ͳ�ȥ֮ǰ���������ݲ���
		{
			return True;
		}
		else
			return False;
	}

}
void NRF24L01_SendData(void)
{
	if(NRF_Mode == TXMode)
	{
		if(Is_TXBUF_IDEL == True)           
		{
			CE0;	
			NRF24L01_WriteBuf(WR_TX_PLOAD,NRF24L01_Data.TX_BUF,32);
			CE1;
			Is_TXBUF_IDEL = False;                   //�ڳɹ����ͳ�ȥ�������֮ǰ�����ܻ�����Ĵ���������д��
		}
	}
	else
		Is_ACKBUF_IDEL = False;                    //ACK_BUF�Ǳ������ͣ��Ҵ����жϣ������жϵ������ݰ���ϣ���ˣ����뱣֤�����ݷ��ͳ�ȥ֮ǰ���������ݲ���

}
BOOL NRF24L01_GetData(void)
{
	if(Is_RXBUF_IDEL == False)
	{
		return True;
	}
	else
	{		
		return False;
	}
}

void NRF24L01_Free_RXBUF(void)
{
	Is_RXBUF_IDEL = True;
}

extern "C"{
void EXTI4_IRQHandler(void)
{

	if(EXTI_GetITStatus(EXTI_Line4) != RESET)
  {
		if(NRF_Mode == RXMode)
		{
			CE0;  	
			
			u8 State = NRF24L01_ReadReg(STATUS);		                            //��ȡstatus�Ĵ�����ֵ
			NRF24L01_WriteReg(NRF_WRITE_REG+STATUS,State);

			if(State & RX_DR)                                                   //���յ�����
			{
				u8 BUF_TMP[32];
				NRF24L01_ReadBuf(RD_RX_PLOAD,BUF_TMP,32);                         //��ȡ����   
				NRF24L01_WriteReg(FLUSH_RX,NOP);                                  //���RX FIFO�Ĵ���
			
				if(Is_RXBUF_IDEL == True)
				{
					for(u8 i = 0;i < 32;i++)
						NRF24L01.Data->RX_BUF[i] = BUF_TMP[i];
					Is_RXBUF_IDEL = False;
				}
				
				if(Is_ACKBUF_IDEL == False)
				{
					NRF24L01_WriteBuf(WR_ACK_PAYLOAD,NRF24L01_Data.ACK_BUF,32);
					Is_ACKBUF_IDEL = True;
				}			
			}                               
		
			CE1;
		}
	 else
		{
			CE0;  
			/*��ȡ��̬ACK����ֵ*/ 
			u8 BUF_TMP[32];
			NRF24L01_ReadBuf(RD_RX_PLOAD,BUF_TMP,32);                         //��ȡ����   
			NRF24L01_WriteReg(FLUSH_RX,NOP);                                  //���RX FIFO�Ĵ���	

			if(Is_RXBUF_IDEL == True)
			{
				for(u8 i = 0;i < 32;i++)
					NRF24L01.Data->RX_BUF[i] = BUF_TMP[i];
				Is_RXBUF_IDEL = False;
			}
			
			u8 State = NRF24L01_ReadReg(STATUS);       
			NRF24L01_WriteReg(NRF_WRITE_REG+STATUS,State); 	                  //��ȡstatus�Ĵ�����ֵ
			NRF24L01_WriteReg(FLUSH_TX,NOP);   												        //���TX FIFO�Ĵ��� 
			Is_TXBUF_IDEL = True;
			CE1;
		}
	}

	EXTI_ClearITPendingBit(EXTI_Line4);

}

}
