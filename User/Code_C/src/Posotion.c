#include "Position.h"


#define ERROR_HOLD_TIME 2500

void Position_Init(void);
void Position_Updata(u16 Time);

Pos_Data_ Pos_Data = 
{
	0,0,0,
	0,0,0
};

struct Position_ Position = 
{
	&Pos_Data,
	False,
	Position_Init,
	Position_Updata
};



Filter_Balance FUSE_ALT_ULTRA_SPE(10,0,0);
Filter_Balance FUSE_ALT_ULTRA_POS(1,0,0);
Filter_Balance FUSE_ALT_MS5611_SPE(1,0,0);
Filter_Balance FUSE_ALT_MS5611_POS(10,0,0);
Filter_Balance FUSE_ALT_ACC_SPE(1,0,0);

float MS5611_Fused_Pos;
float Ultra_Fused_Spe;
float Ultra_Fused_Pos;
float ACC_Fused_Spe;


void Position_Init(void)
{
	MS5611_SPI.Init();
	GPS.Init();
}

void MS5611_Fuse_Updata(u16 Time,float ACC_Earth)
{
	
	float Speed = 0;;	
	float Alt = 0;
	
	Alt = MS5611_SPI.Data->Altitude;
	Speed = MS5611_SPI.Data->Speed;
		
	Speed = FUSE_ALT_MS5611_SPE.BalanceFilter(ACC_Earth,0,(float)Time / 1000);
	Alt   = FUSE_ALT_MS5611_POS.BalanceFilter(ACC_Fused_Spe,Alt,(float)Time / 1000);	
	
	MS5611_Fused_Pos = Alt;

}

BOOL Ultra_Fuse_Updata(u16 Time,float ACC_Earth)
{
	
	float Speed = 0;;	
	float Alt = 0;
	
	Alt = Ultra.Altitude;
	Speed = Ultra.Speed;

	
	Ultra_Fused_Spe = FUSE_ALT_ULTRA_SPE.BalanceFilter(ACC_Earth,Speed,(float)Time / 1000);
	User_Data.Data2 = Ultra_Fused_Spe * 100.0f;
	User_Data.Data3 = ACC_Fused_Spe * 100.0f;
	// ���������Ƶ��ٶ�����ٶȼƹ��Ƶ��ٶ�������ʱ����Ϊ��������������
	if(fabs(ACC_Fused_Spe - Ultra_Fused_Spe) > Math.Constrain(fabs(ACC_Fused_Spe) + 0.2f,1,0)) return False; 
	
	Ultra_Fused_Pos = FUSE_ALT_ULTRA_POS.BalanceFilter(ACC_Fused_Spe,Alt,(float)Time / 1000);	

	return True;
}

void ACC_Fuse_Updata(u16 Time,float ACC_Earth)
{
	static float ACC_Fused_Spe_Bias = 0;
	float Tmp = FUSE_ALT_ACC_SPE.BalanceFilter(ACC_Earth,0,(float)Time / 1000);	
	float TimeNow = SystemTime.Now_MS();
	if(TimeNow > 5000)
	{
		if(FUSE_ALT_ACC_SPE.Kp == 1)
		{
			//������������
			FUSE_ALT_ACC_SPE.Output *= 10;
			FUSE_ALT_ACC_SPE.Kp /= 10;
			ACC_Fused_Spe_Bias = FUSE_ALT_ACC_SPE.Output;
		}
	}
	else
	{
		Position.IsReady = True;
	}
	ACC_Fused_Spe = Tmp - ACC_Fused_Spe_Bias;
}

void Position_Updata(u16 Time)
{
	static float Store_POS_MS5611 = 0;
	BOOL Ret;
	Vector ACC_Earth;
	
	ACC_Earth = Math.Body_To_Earth(MPU6050.Data->ACC_ADC,Attitude.Angle->y,Attitude.Angle->x);
	ACC_Earth.z -= 4095;
	ACC_Earth.z /= 4095;
	ACC_Earth.z *= 9.8f;
	
	ACC_Fuse_Updata(Time,ACC_Earth.z);
	MS5611_Fuse_Updata(Time,ACC_Earth.z);
	Ret = Ultra_Fuse_Updata(Time,ACC_Earth.z);	

	if(Ret == False)	
		Pos_Data.POS_Z += MS5611_Fused_Pos - Store_POS_MS5611;
	else
		Pos_Data.POS_Z = Ultra_Fused_Pos;	
	
	Pos_Data.SPE_Z = ACC_Fused_Spe;
	Store_POS_MS5611 = MS5611_Fused_Pos;
	
	User_Data.Data1 = Pos_Data.POS_Z * 100.0f;
	User_Data.Data2 = Pos_Data.SPE_Z * 100.0f;
	User_Data.Data3 = MS5611_Fused_Pos * 100.0f;	
}

