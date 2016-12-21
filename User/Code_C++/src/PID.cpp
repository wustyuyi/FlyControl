#include "PID.h"
/*
		�㷨˵����
		PID����
*/
PID::PID(void)
{
	Kp = 0;
	Ki = 0;
	Kd = 0;
	
	LastError = 0;
	SumError = 0;
	Setpoint = 0;
	Feedback = 0;
	
	I_Limit = 0;
}

PID::PID(float Init_Kp,float Init_Ki,float Init_Kd,float Init_I_Limit)
{
	
	Kp = Init_Kp;
	Ki = Init_Ki;
	Kd = Init_Kd;
	
	LastError = 0;
	SumError = 0;
	Setpoint = 0;
	Feedback = 0;
	Output = 0;
	I_Limit = Init_I_Limit;
}

void PID::Rst(void)
{	
	LastError = 0;
	SumError = 0;
	Setpoint = 0;
	Feedback = 0;
	Output = 0;
}

/*
		����PID, ����ʱ�����Լ100US
*/
double PID::Cal_PID_POS(int dt)
{
	double pError = 0;
	double dError = 0;
	double Error = Setpoint - Feedback;

	if((Error != 0) & (Ki != 0))
	{
		SumError += Error * Ki * dt / 1000.f;
		if(SumError > I_Limit) 
			SumError = I_Limit;
		if(SumError < -I_Limit) 
			SumError = -I_Limit;
	}
	
	if((Kd != 0))
	{
		dError = (Error - LastError) * Kd / dt * 1000.f;
		LastError = Error;
	}
	
	if((Error != 0) & (Kp != 0))
		pError = Error * Kp;
	
	Output = pError + SumError + dError;
	ValueTmp = dError;
	return Output;
}


