#include "TFT1_44.h"


#define RS_0() GPIO_ResetBits(GPIOB,GPIO_Pin_10)
#define RS_1() GPIO_SetBits(GPIOB,GPIO_Pin_10)

#define CS_0() GPIO_ResetBits(GPIOB,GPIO_Pin_11)
#define CS_1() GPIO_SetBits(GPIOB,GPIO_Pin_11)

#define RST_0() GPIO_ResetBits(GPIOA,GPIO_Pin_8)
#define RST_1() GPIO_SetBits(GPIOA,GPIO_Pin_8)

void TFT_Init(void);
void TFT_ShowString(Point POS,const char* P,u16 Color,u16 BackGroundColor);
void TFT_ShowNumber(Point POS,int Number,u16 Color,u16 BackGroundColor);
void Fill_Rectangle(Rectanle RECT,BOOL IsHollow,u16 Color);
void TFT_ShowFloat(Point POS,float Number,u16 Color,u16 BackGroundColor);
void TFT_Clear_Pixel(Rectanle RECT);
struct TFT_ TFT = 
{
	TFT_Init,
	TFT_ShowString,
	TFT_ShowNumber,
	Fill_Rectangle,
	TFT_ShowFloat,
	TFT_Clear_Pixel
};

void Init_TFTG_GPIO(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;

	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOB, ENABLE );//PORTBʱ��ʹ�� 
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA, ENABLE );//PORTAʱ��ʹ�� 
	RCC_APB1PeriphClockCmd(	RCC_APB1Periph_SPI2,  ENABLE );//SPI2ʱ��ʹ�� 	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15 ;	  //Pin_13 sck  pin15 MOSI
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_11);//PAB11	CS

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_10);//PB10	RS

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA, GPIO_Pin_8);//PA8	RST

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //����SPI�������˫�������ģʽ:SPI����Ϊ˫��˫��ȫ˫��  SPI_Direction_1Line_Tx
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//����SPI����ģʽ:����Ϊ��SPI
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;		//ѡ���˴���ʱ�ӵ���̬:ʱ�����ո�
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;	//���ݲ����ڵڶ���ʱ����
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSS�ź���Ӳ����NSS�ܽţ����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ����
	SPI_InitStructure.SPI_BaudRatePrescaler =SPI_BaudRatePrescaler_2;		//���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ16
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRCֵ����Ķ���ʽ
	SPI_Init(SPI2, &SPI_InitStructure);  //����SPI_InitStruct��ָ���Ĳ�����ʼ������SPIx�Ĵ���

	SPI_Cmd(SPI2, ENABLE); //ʹ��SPI����

}
u8 SPI_WriteByte(SPI_TypeDef* SPIx,u8 Byte)
{
	while((SPIx->SR&SPI_I2S_FLAG_TXE)==RESET);		//�ȴ���������	  
	SPIx->DR=Byte;	 	//����һ��byte   
	while((SPIx->SR&SPI_I2S_FLAG_RXNE)==RESET);//�ȴ�������һ��byte  
	return SPIx->DR;          	     //�����յ�������			
} 
void TFT_Write_Command(u8 Data)
{
	RS_0();
	SPI_WriteByte(SPI2,Data);
}

void  TFT_Write_Data(u8 Data)  
{
	RS_1();
	SPI_WriteByte(SPI2,Data);
}

void TFT_Single_Colour(u8 TFT_DataH,u8 TFT_DataL)
{
	u8 i,j;
	
	for (i=0;i<160;i++)
		for (j=0;j<128;j++)
		{
			TFT_Write_Data(TFT_DataH);
			TFT_Write_Data(TFT_DataL);
		}
}

void TFT_Init(void)
{
	Init_TFTG_GPIO();
	
	RST_0();
	SystemTime.WaitMS(1);
	RST_1();
	SystemTime.WaitMS(1);
	
	
	TFT_Write_Command(0x11); //Exit Sleep
	SystemTime.WaitMS(200);	
	TFT_Write_Command(0x3a); //Set Color Format
	TFT_Write_Data(0x05);
	TFT_Write_Command(0x36); //RGB
	TFT_Write_Data(0xa8);
	TFT_Write_Command(0x29); // Display On
	
	TFT_Write_Command(0x2A); //Set Column Address
	TFT_Write_Data(0x00);
	TFT_Write_Data(0x00);
	TFT_Write_Data(0x00);
	TFT_Write_Data(0x9F);
	TFT_Write_Command(0x2B); //Set Page Address
	TFT_Write_Data(0x00);
	TFT_Write_Data(0x00);
	TFT_Write_Data(0x00);
	TFT_Write_Data(0x7F);
	
	TFT_Write_Command(0x2C);
	TFT_Single_Colour(0,0);//��ɫ����
	
}


void Address_Set_Word(Point POS)
{
	TFT_Write_Command(0x2a);
	TFT_Write_Data(0);
	TFT_Write_Data((u8)(POS.X * 8) + 32);
	TFT_Write_Data(0);
	TFT_Write_Data((u8)(POS.X * 8) + 32 + 7);

	TFT_Write_Command(0x2b);
	TFT_Write_Data(0);
	TFT_Write_Data((u8)(POS.Y * 16));
	TFT_Write_Data(0);
	TFT_Write_Data((u8)(POS.Y * 16) + 15);

	TFT_Write_Command(0x2C);
}	

void Address_Set_Pixel(Rectanle RECT)
{
	TFT_Write_Command(0x2a);
	TFT_Write_Data(0);
	TFT_Write_Data(RECT.X_LU + 32);
	TFT_Write_Data(0);
	TFT_Write_Data(RECT.X_RD + 32);

	TFT_Write_Command(0x2b);
	TFT_Write_Data(0);
	TFT_Write_Data(RECT.Y_LU);
	TFT_Write_Data(0);
	TFT_Write_Data(RECT.Y_RD);

	TFT_Write_Command(0x2C);
}	

void TFT_ShowChar(Point Pos,u8 Aim,u16 Color,u16 BackGroundColor)
{  							  
	u8 Temp1,Temp2,Temp3;

	Aim = Aim - ' ';                        //�õ�ƫ�ƺ��ֵ
	Address_Set_Word(Pos);                      //������ʾ��������

	for(Temp3 = 0;Temp3 < 16;Temp3 ++)
	{   
		Temp1 = Zk_ASCII8X16[Aim][Temp3];		  //����1608���� 	                          
		for(Temp2 = 0;Temp2 < 8;Temp2 ++)
		{			    
			if(Temp1 & 0x01)
			{
				TFT_Write_Data((u8)Color);                  //��ɫ����
				TFT_Write_Data((u8)(Color >> 8));           //��ɫ����
			}
			else
			{			
				TFT_Write_Data((u8)BackGroundColor);	
				TFT_Write_Data((u8)(BackGroundColor >> 8));
			}
			Temp1 >>= 1;
		}  	 
	}    		
}  



void TFT_ShowString(Point POS,const char* P,u16 Color,u16 BackGroundColor)
{
	while((*P <= '~')&&(*P >= ' ')&&(*P != '\0'))        //�ж��ǲ��ǷǷ��ַ�!
	{           
		TFT_ShowChar(POS,*P,Color,BackGroundColor);
		POS.X++;
		P++;
	}
}

void TFT_ShowNumber(Point POS,int Number,u16 Color,u16 BackGroundColor)
{
	char BUF[5] = {' ',' ',' ',' ',' '};
	u8 i = 4;
	if(Number == 0) BUF[4] = 0x30;
	else
	{
		if(Number > 0) 
		{
			
			while(Number != 0)
			{
				BUF[i--] = Number % 10 + 0X30;
				Number /= 10;
			}
		}
		else
		{
			u8 i = 4;
			Number = -Number;
			while(Number != 0)
			{
				BUF[i--] = Number % 10 + 0X30;
				Number /= 10;
			}
			BUF[i] = '-';		
		}
	}
	
	TFT_ShowString(POS,BUF,Color,BackGroundColor);
}

void TFT_ShowFloat(Point POS,float Number,u16 Color,u16 BackGroundColor)
{
	char BUF[7] = {' ',' ',' ','0','.','0',' '};
	u8 i = 6;
	int TMP = Number * 100.0f;

	if(TMP > 0) 
	{
		
		while(TMP != 0)
		{
			if(i == 4) i = 3;
			BUF[i--] = TMP % 10 + 0X30;
			TMP /= 10;
		}
	}
	else
	{
		if(TMP < 0)
		{
			u8 i = 6;
			TMP = -TMP;
			while(TMP != 0)
			{
				if(i == 4) i = 3;
				BUF[i--] = TMP % 10 + 0X30;
				TMP /= 10;
			}
			if(i == 4) i = 2;
			BUF[i] = '-';	
		}
	}

	
	TFT_ShowString(POS,BUF,Color,BackGroundColor);
}

//������
//Point:����ʾ���ֲ�ͬ���˴����õ������أ���������
void Fill_Rectangle(Rectanle RECT,BOOL IsHollow,u16 Color)
{
	Rectanle TMP;
	u8 X_Length = RECT.X_RD - RECT.X_LU;
	u8 Y_Length = RECT.Y_RD - RECT.Y_LU;
	if(IsHollow == False)
	{
		TMP.X_LU = RECT.X_LU;
		TMP.Y_LU = RECT.Y_LU;
		TMP.X_RD = RECT.X_RD;	
		TMP.Y_RD = RECT.Y_LU;		
		Address_Set_Pixel(TMP); 		
		
		for(u8 j = X_Length;j > 0;j--)
		{
			TFT_Write_Data((u8)Color);	
			TFT_Write_Data((u8)(Color >> 8));			
		}

		TMP.X_LU = RECT.X_RD;
		TMP.Y_LU = RECT.Y_LU;
		TMP.X_RD = RECT.X_RD;	
		TMP.Y_RD = RECT.Y_RD;		
		Address_Set_Pixel(TMP); 		
		
		for(u8 j = Y_Length;j > 0;j--)
		{
			TFT_Write_Data((u8)Color);	
			TFT_Write_Data((u8)(Color >> 8));			
		}

		TMP.X_LU = RECT.X_LU;
		TMP.Y_LU = RECT.Y_RD;
		TMP.X_RD = RECT.X_RD;	
		TMP.Y_RD = RECT.Y_RD;		
		Address_Set_Pixel(TMP); 		
		
		for(u8 j = Y_Length;j > 0;j--)
		{
			TFT_Write_Data((u8)Color);	
			TFT_Write_Data((u8)(Color >> 8));			
		}	
		
		TMP.X_LU = RECT.X_LU;
		TMP.Y_LU = RECT.Y_LU;
		TMP.X_RD = RECT.X_LU;	
		TMP.Y_RD = RECT.Y_RD;		
		Address_Set_Pixel(TMP); 		
		
		for(u8 j = Y_Length;j > 0;j--)
		{
			TFT_Write_Data((u8)Color);	
			TFT_Write_Data((u8)(Color >> 8));			
		}
	}
	else
	{
		X_Length++;
		Y_Length++;
		Address_Set_Pixel(RECT); 
		for(u8 i = X_Length;i > 0;i--)
			for(u8 j = Y_Length;j > 0;j--)
			{
				TFT_Write_Data((u8)Color);	
				TFT_Write_Data((u8)(Color >> 8));		
			}
	}
}

void TFT_Clear_Pixel(Rectanle RECT)
{
	u8 X_Length = RECT.X_RD - RECT.X_LU;
	u8 Y_Length = RECT.Y_RD - RECT.Y_LU;
	Address_Set_Pixel(RECT);
	for (u8 i=0;i<X_Length;i++)
		for (u8 j=0;j<Y_Length;j++)
		{
			TFT_Write_Data(0X00);
			TFT_Write_Data(0x00);
		}
	
}

