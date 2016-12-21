#include "SysState.h"

void State_Updata(u16 Time);

struct SystemState_ SystemState = 
{
	State_Updata
};



void RC_State(u16 Time)
{
	static u16 LostCnt = 0;
	//失控判定
	if((SBUS.SW_POS_Judge(RC_AUX5) == Down) & (SBUS.SW_POS_Judge(RC_AUX6) == Down))
	{
		LostCnt += Time;
		if(LostCnt > 1000)
		{
			FlyControl.Para->IsLost = True;
			LostCnt = 1000;
		}
	}
	else
	{
		FlyControl.Para->IsLost = False;
		LostCnt = 0;	
	}
}

void Pow_State(u16 Time)
{
	BOOL IsAnyError = False;

#ifdef FOUR_ROTOR
	if(Power.Data->BAT_2S < 7.0f)
	{
		Buzzer.On(100);
		SYS_State.State1 |= POW_LOW;
	}
	else
	{
		Buzzer.Off();
		SYS_State.State1 &= ~POW_LOW;	
	}
#else
	if((Power.Data->BAT_12S < 42.0f) || (Power.Data->BAT_2S < 7.0f))
	{
		Buzzer.On(100);
		SYS_State.State1 |= POW_LOW;
	}
	else
	{
		Buzzer.Off();
		SYS_State.State1 &= ~POW_LOW;	
	}
#endif
	if(abs(Power.Data->BAT_12S - 50) > 20)        	IsAnyError = True;
	if(abs(Power.Data->POW_4V5 - 4.5f) > 0.5f)      IsAnyError = True;
	if(abs(Power.Data->POW_5V - 5.0f) > 0.5f)       IsAnyError = True;
	if(abs(Power.Data->BAT_2S - 8.4f) > 3)          IsAnyError = True;
	if(abs(Power.Data->Main_3V3_MCU - 3.3f) > 0.5f) IsAnyError = True;
	if(abs(Power.Data->Main_3V3_SEN - 3.3f) > 0.5f) IsAnyError = True;
	if(abs(Power.Data->Main_5V - 5.0f) > 0.5f)      IsAnyError = True;	
	
	if(IsAnyError == True) SYS_State.State1 |= SYS_ERROR;
	else SYS_State.State1 &= ~SYS_ERROR;
	
	if(Power.Data->IsUPS_On == True) SYS_State.State1 |= UPS_ON;
	else SYS_State.State1 &= ~UPS_ON;
}


void Sensor_State(void)
{

}


void Controller_State(u16 Time)
{

	static u16 LockCnt = 0;
	static u16 UnlockCnt = 0;
	//解锁判定	 上锁判定
	if(FlyControl.Para->IsLock == True)
	{
		LockCnt = 0;
		
		if((RC_THROTTLE < THROTTLE_MIN + 10) & (RC_YAW > SBUS_MAX - 10) & (FlyControl.Para->IsLost == False) & (Position.IsReady == True))
		{
			UnlockCnt += Time;
			if(UnlockCnt > 2000)	
			{
				FlyControl.Para->IsLock = False;
				Buzzer.On(0);
			}
		}
		else	UnlockCnt = 0;	
	}
	else
	{
		Buzzer.Off();
		UnlockCnt = 0;
		if(RC_THROTTLE < THROTTLE_MIN + 10)
			LockCnt += Time;
		else
			LockCnt = 0;
		if(LockCnt > 10000)
			FlyControl.Para->IsLock = True;
	}
	
		//系统状态更新
	if(FlyControl.Para->IsLock == False) 
	{
		SYS_State.State1 |= UNLOCK;
		Led.Reverse(LED1,200);
	}
	else
	{
		SYS_State.State1 &= ~UNLOCK;
		Led.Off(LED1);
	}	

	if(FlyControl.Para->IsLost != False) 
	{
		SYS_State.State1 |= LOST;
	}
	else
	{
		SYS_State.State1 &= ~LOST;
	}
}


void State_Updata(u16 Time)
{
	RC_State(Time);
	Pow_State(Time);
	Controller_State(Time);
	Sensor_State();
}


