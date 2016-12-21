#include "HMC5883.h"

#define HMC5883ADDRESS 0x3C

void HMC5883_Init(void);
void HMC5883_Updata(void);

I2C_Soft_ I2C_HMC5883('E',GPIO_Pin_6,GPIO_Pin_5,1);
Filter_MidValue MAG_Filter;
struct HMC5883_Data_ HMC5883_Data = 
{
	Vector(0,0,0),
	Vector(0,0,0),
	Vector(4.750219159169664e+02,4.750219159169664e+02,4.804578169915654e+02),
	Vector(-32.661910849854834,-2.094765596671001e+02,-5.119679040951526e+02),
	0
};



struct HMC5883_ HMC5883 = 
{
	False,
	&HMC5883_Data,
	HMC5883_Init,
	HMC5883_Updata
};

void HMC5883_Init(void)
{
	I2C_HMC5883.Init();
	I2C_HMC5883.Single_Write(HMC5883ADDRESS,0X00,0X7C);// 8��ƽ�� 220HZ
	I2C_HMC5883.Single_Write(HMC5883ADDRESS,0X01,0X20);// +-1.3GA 0.92MG/LSB
	I2C_HMC5883.Single_Write(HMC5883ADDRESS,0X02,0X80);// ���� ����
	SystemTime.WaitMS(10);
}

void HMC5883_Updata(void)
{
	u8 BUF[6];
	static u16 ErrorCnt = 0; // 100���У�������ٴ�
	static u16 AllCnt = 0;   // ��¼��ȡ���ٴΣ��������100��
	I2C_HMC5883.Start();
	I2C_HMC5883.SendByte(HMC5883ADDRESS);
	I2C_HMC5883.WaitAck();
	I2C_HMC5883.SendByte(0X03);
	I2C_HMC5883.WaitAck();
	I2C_HMC5883.Start();
	I2C_HMC5883.SendByte(HMC5883ADDRESS + 1);
	I2C_HMC5883.WaitAck();
	for(u8 i = 0;i < 5;i++)
	{
		BUF[i] = I2C_HMC5883.ReadByte();
		I2C_HMC5883.Ack();
	}
	
	BUF[5] = I2C_HMC5883.ReadByte();
	I2C_HMC5883.NoAck();
	I2C_HMC5883.Stop();
	
	HMC5883.Data->MAG_Original.x = (int16_t)(BUF[0] << 8 | BUF[1]);
	HMC5883.Data->MAG_Original.z = (int16_t)(BUF[2] << 8 | BUF[3]);
	HMC5883.Data->MAG_Original.y = (int16_t)(BUF[4] << 8 | BUF[5]);

	HMC5883.Data->MAG_Original = MAG_Filter.MidValue(HMC5883.Data->MAG_Original);	
	
	Vector MAG_TMP;
	//����У���㷨��У�����Ϊ��λ��
	MAG_TMP.x = (HMC5883.Data->MAG_Original.x - HMC5883.Data->Offset.x) / HMC5883.Data->Kp.x;
	MAG_TMP.y = (HMC5883.Data->MAG_Original.y - HMC5883.Data->Offset.y) / HMC5883.Data->Kp.y;
	MAG_TMP.z = (HMC5883.Data->MAG_Original.z - HMC5883.Data->Offset.z) / HMC5883.Data->Kp.z;		
	HMC5883.Data->Length = MAG_TMP.x * MAG_TMP.x + MAG_TMP.y * MAG_TMP.y + MAG_TMP.z * MAG_TMP.z;
		
	//��֤����ƽ�����Ƿ�ӽ�1 �����ӽ���1������,������������Ч
	if(abs(HMC5883.Data->Length - 1) < 0.2f)	HMC5883.Data->MAG_ADC = MAG_TMP;
	else	
	{ 
		ErrorCnt++;
		HMC5883.Data->MAG_ADC = Vector(0,0,0);
	}
	//����ٷְٴ��ڰٷ�֮10����Ϊ����������ֹͣʹ��,����������
	//EEROR_LQ
	if(AllCnt++ == 100)
	{		
		if(ErrorCnt > 10)
		{
			HMC5883.IsSensorError = True;
		}
		HMC5883.Data->ErrorCnt = ErrorCnt;//��¼����ٷֱ�
		AllCnt = 0;
		ErrorCnt = 0;
	}
		
}
