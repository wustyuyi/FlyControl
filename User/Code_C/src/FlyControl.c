#include "FlyControl.h"


#define ANGLE_SPEED            (45.0f * DEG_TO_RAD)    //���ת���ٶ�
#define ANGLE_MAX              (20.0f * DEG_TO_RAD)    //������
#define POS_SPEED              0.5f                    //����˶��ٶ� M/S
#define ALT_MAX                2.0f                    //���߶�
#define POS_MAX                10.0f                   //����˶���Χ
#define THR_MAX                300                     //PID �߶ȱջ���������������ֵ    
#define FILTER_ANGLE   	       0.1f                    //�ǶȻ�ǰ���˲���ϵ��
#define FILTER_ANGLE_SPEED     0.1f                    //���ٶȻ�ǰ���˲�ϵ��
#define FILTER_POS_SPEED       0.1f                    //�߶��⻷���ǰ���˲�


void ATT_Inner_Loop(u32 Time);
void ATT_Outer_Loop(u32 Time);

void POS_Inner_Loop(u32 Time);
void POS_Outer_Loop(u32 Time);

/*
	PID ATT_Inner_PID_x;
	PID ATT_Inner_PID_y;
	PID ATT_Inner_PID_z;
	
	PID ATT_Outer_PID_x;
	PID ATT_Outer_PID_y;	
	PID ATT_Outer_PID_z;	
	
	PID POS_Inner_PID_x;
	PID POS_Inner_PID_y;
	PID POS_Inner_PID_z;
	
	PID POS_Outer_PID_x;
	PID POS_Outer_PID_y;
	PID POS_Outer_PID_z;
	int Throttle;

	float ALT_Onland;
	BOOL IsLock;
	BOOL IsLost;
	BOOL IsError;
	Fly_Mode Mode;  
*/
struct Control_Para_ Control_Para = 
{
	PID(80 ,8,2,100),
	PID(80 ,8,2,100),
	PID(800,8,0,100),
	
	PID(2,0,0,0.2),
	PID(2,0,0,0.2),
	PID(0,2,0,0.2), 
	
	PID(00,0,0,100),
	PID(00,0,0,100),
	PID(1800,1000,100,200),
	
	PID(0.0,0,0,0),
	PID(0.0,0,0,0),
	PID(1,0,0,0),
	
	0,0,0,
	True,
	False,
	ALT
};
	
struct FlyControl_ FlyControl = 
{	
	&Control_Para,
	
	ATT_Inner_Loop,
	ATT_Outer_Loop,

	POS_Inner_Loop,
	POS_Outer_Loop,	
	
};

/*
	�����Ƶ�λ 
*/
int Throttle;
void ATT_Inner_Loop(u32 Time)
{
	Vector Inner_Output;
	
		
	if(Control_Para.IsLost == True)  Control_Para.IsLock = True;
	if(Control_Para.IsLock == True)            //ͣ����λ
	{
		Control_Para.ATT_Inner_PID_x.Rst();
		Control_Para.ATT_Inner_PID_y.Rst();
		Control_Para.ATT_Inner_PID_z.Rst();
		
		Motor.Stop();
		return;
	}
	
	//�ο�����ǰ���˲�
	FILER(Control_Para.ATT_Inner_PID_x.Setpoint,Control_Para.ATT_Outer_PID_x.Output,FILTER_ANGLE_SPEED);
	FILER(Control_Para.ATT_Inner_PID_y.Setpoint,Control_Para.ATT_Outer_PID_y.Output,FILTER_ANGLE_SPEED);
	FILER(Control_Para.ATT_Inner_PID_z.Setpoint,Control_Para.ATT_Outer_PID_z.Output,FILTER_ANGLE_SPEED);
	
	//��̬ģʽ�� ������Ϊң����ֱ�ӷ��͵�����
	if(Control_Para.Mode == ATT) Throttle = RC_THROTTLE;
	else	Throttle = Control_Para.POS_Inner_PID_z.Output;
	
	//����
	Control_Para.ATT_Inner_PID_x.Feedback = Attitude.Rate->x;
	Control_Para.ATT_Inner_PID_y.Feedback = Attitude.Rate->y;
	Control_Para.ATT_Inner_PID_z.Feedback = Attitude.Rate->z;
	
	//PID����
	Inner_Output.x = Control_Para.ATT_Inner_PID_x.Cal_PID_POS(Time);
	Inner_Output.y = Control_Para.ATT_Inner_PID_y.Cal_PID_POS(Time);
 	Inner_Output.z = Control_Para.ATT_Inner_PID_z.Cal_PID_POS(Time);
#ifdef FOUR_ROTOR		
	//�������
	Motor.PWM->PWM5 = 0;
	Motor.PWM->PWM3 = + Inner_Output.x -  Inner_Output.y + Inner_Output.z + Throttle; 
	Motor.PWM->PWM2 = - Inner_Output.x -  Inner_Output.y - Inner_Output.z + Throttle;

	Motor.PWM->PWM6 = 0;
	Motor.PWM->PWM4 = + Inner_Output.x +  Inner_Output.y - Inner_Output.z + Throttle;
	Motor.PWM->PWM1 = - Inner_Output.x +  Inner_Output.y + Inner_Output.z + Throttle;	
#else

#endif
	Motor.Output(True);
}
/*
	�����Ƶ�λ
*/

void ATT_Outer_Loop(u32 Time)
{
	if(Control_Para.IsLock == True) 
	{
		Control_Para.ATT_Outer_PID_x.Rst();
		Control_Para.ATT_Outer_PID_y.Rst();
		Control_Para.ATT_Outer_PID_z.Rst();
		return;
	}
	
	//λ��ģʽ��  �ο�����Ϊλ�ñջ������ֵ  ǰ���˲�
	if(Control_Para.Mode == POS)
	{
		FILER(Control_Para.ATT_Outer_PID_x.Setpoint,Control_Para.POS_Inner_PID_x.Output,FILTER_ANGLE);
		FILER(Control_Para.ATT_Outer_PID_y.Setpoint,Control_Para.POS_Inner_PID_y.Output,FILTER_ANGLE);
		FILER(Control_Para.ATT_Outer_PID_z.Setpoint,0,FILTER_ANGLE);	//���������		
	}
	else
	{
		float Roll  = Math.Dead_Zone(RC_ROLL - RC_MID,2) / RC_RANGE * ANGLE_MAX;
		float Pitch = Math.Dead_Zone(RC_MID  - RC_PITCH,2) / RC_RANGE * ANGLE_MAX;
		
		FILER(Control_Para.ATT_Outer_PID_x.Setpoint,Roll ,FILTER_ANGLE);
		FILER(Control_Para.ATT_Outer_PID_y.Setpoint,Pitch,FILTER_ANGLE);
		FILER(Control_Para.ATT_Outer_PID_z.Setpoint,0,FILTER_ANGLE);	//���������			
	}
	
	// ����
	Control_Para.ATT_Outer_PID_x.Feedback = Radians(Attitude.Angle->x);
	Control_Para.ATT_Outer_PID_y.Feedback = Radians(Attitude.Angle->y);
	Control_Para.ATT_Outer_PID_z.Feedback = Attitude.Rate->z;
	// PID����
	Control_Para.ATT_Outer_PID_x.Cal_PID_POS(Time);
	Control_Para.ATT_Outer_PID_y.Cal_PID_POS(Time);
	Control_Para.ATT_Outer_PID_z.Cal_PID_POS(Time);
	// �޷�
	CONSTRAIN(Control_Para.ATT_Outer_PID_x.Output,ANGLE_SPEED,-ANGLE_SPEED);
	CONSTRAIN(Control_Para.ATT_Outer_PID_y.Output,ANGLE_SPEED,-ANGLE_SPEED);
	CONSTRAIN(Control_Para.ATT_Outer_PID_z.Output,ANGLE_SPEED,-ANGLE_SPEED);

}

void POS_Inner_Loop(u32 Time)
{
	
	if(Control_Para.IsLock == True)  
	{
		Control_Para.POS_Inner_PID_x.Rst();
		Control_Para.POS_Inner_PID_y.Rst();
		Control_Para.POS_Inner_PID_z.Rst();
		return;
	}
	//�ο�����ǰ���˲�
	FILER(Control_Para.POS_Inner_PID_x.Setpoint,Control_Para.POS_Outer_PID_x.Output,FILTER_POS_SPEED);
	FILER(Control_Para.POS_Inner_PID_y.Setpoint,Control_Para.POS_Outer_PID_y.Output,FILTER_POS_SPEED);
	FILER(Control_Para.POS_Inner_PID_z.Setpoint,Control_Para.POS_Outer_PID_z.Output,FILTER_POS_SPEED);
	//����
	Control_Para.POS_Inner_PID_x.Feedback = Position.Data->SPE_X;
	Control_Para.POS_Inner_PID_y.Feedback = Position.Data->SPE_Y;
	Control_Para.POS_Inner_PID_z.Feedback = Position.Data->SPE_Z;
	//PID����
	Control_Para.POS_Inner_PID_x.Cal_PID_POS(Time);
	Control_Para.POS_Inner_PID_y.Cal_PID_POS(Time);
	Control_Para.POS_Inner_PID_z.Cal_PID_POS(Time);
	// �޷�
	CONSTRAIN(Control_Para.POS_Inner_PID_x.Output,ANGLE_MAX,-ANGLE_MAX);
	CONSTRAIN(Control_Para.POS_Inner_PID_y.Output,ANGLE_MAX,-ANGLE_MAX);
	CONSTRAIN(Control_Para.POS_Inner_PID_z.Output,THR_MAX  ,  -THR_MAX);		
	// �߶�PID������������ֵ�������ٷ�֮30������ͨ���ֶ��������ţ�����Ӧ������һЩΣ��״����
	Control_Para.POS_Inner_PID_z.Output += Math.Constrain(RC_THROTTLE,THROTTLE_MID,THROTTLE_MIN);  
	
}

void POS_Outer_Loop(u32 Time)
{
	if(Control_Para.IsLock == True) 
	{
		Control_Para.POS_Outer_PID_x.Rst();
		Control_Para.POS_Outer_PID_y.Rst();
		Control_Para.POS_Outer_PID_z.Rst();
		
		Control_Para.POS_ORI_X = Position.Data->POS_X;
		Control_Para.POS_ORI_Y = Position.Data->POS_Y;
		Control_Para.POS_ORI_Z = Position.Data->POS_Z;
		return;
	}
	//�ο�����
	if(Control_Para.IsLost == False)
	{	
		Control_Para.POS_Outer_PID_x.Setpoint += Math.Dead_Zone(RC_ROLL     - RC_MID,2) / RC_RANGE * POS_SPEED * Time / 1000.0f;
		Control_Para.POS_Outer_PID_y.Setpoint += Math.Dead_Zone(RC_PITCH    - RC_MID,2) / RC_RANGE * POS_SPEED * Time / 1000.0f;
		Control_Para.POS_Outer_PID_z.Setpoint += Math.Dead_Zone(RC_THROTTLE - RC_MID,2) / RC_RANGE * POS_SPEED * Time / 1000.0f;
		//λ���޷�
		CONSTRAIN(Control_Para.POS_Outer_PID_x.Setpoint,POS_MAX,-POS_MAX);
		CONSTRAIN(Control_Para.POS_Outer_PID_y.Setpoint,POS_MAX,-POS_MAX);
		CONSTRAIN(Control_Para.POS_Outer_PID_z.Setpoint,ALT_MAX,0);
	}
	else
	{
		//�Զ�����
	}

	//����
	Control_Para.POS_Outer_PID_x.Feedback = Position.Data->POS_X - Control_Para.POS_ORI_X;
	Control_Para.POS_Outer_PID_y.Feedback = Position.Data->POS_Y - Control_Para.POS_ORI_Y;
	Control_Para.POS_Outer_PID_z.Feedback = Position.Data->POS_Z - Control_Para.POS_ORI_Z;
	//PID����
	Control_Para.POS_Outer_PID_x.Cal_PID_POS(Time);
	Control_Para.POS_Outer_PID_y.Cal_PID_POS(Time);
	Control_Para.POS_Outer_PID_z.Cal_PID_POS(Time);
	//�޷�
	CONSTRAIN(Control_Para.POS_Outer_PID_x.Output,POS_SPEED,-POS_SPEED);
	CONSTRAIN(Control_Para.POS_Outer_PID_y.Output,POS_SPEED,-POS_SPEED);
	CONSTRAIN(Control_Para.POS_Outer_PID_z.Output,POS_SPEED,-POS_SPEED);	
	
}
