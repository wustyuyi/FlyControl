#include "Attitude.h"
#define Kp 1.0f
#define Ki 0.0f

Filter_2nd ATT_Filter(0.1883633f,0,0,-1.023694f,0.2120577f);
Quaternion Q;//,Qz;
u8 IsCalulate = True;
struct Vector Angle;
struct Vector Rate;

/*
	����ϵ��ϵ˵����
	��������ϵ����MPU6050оƬ������ϵһ��  -> PWM����ӿڷ����ǻ�ͷ����
	            ˳�������᷽��ȥ��˳ʱ�뷽��Ϊ������
	            Pitch �� ������ϵY���˶�		Vector Angle.y
	            Roll  �� ������ϵX���˶�		Vector Angle.x
							Yaw   �� ������ϵZ���˶�   Vector Angle.z
	�ο�����ϵ������������ϵ  

*/


void Updata_Eular(void);

struct Attitude_ Attitude = 
{
	&Rate,
  &Angle,
	Updata_Eular,
};

void Updata_Quaternion(Vector GYR,Vector ACC,Vector MAG,double DltaT)
{
	float Norm;
	double HalfT = DltaT / 2.0f;
	double vx,vy,vz;
	double ex,ey,ez;
	double gx,gy,gz;
	double ax,ay,az;
	static double exInt = 0, eyInt = 0, ezInt = 0;//������̬�������Ļ���


	/***************************************************
	����gx��gy��gz�ֱ��Ӧ������Ľ��ٶȣ���λ�ǻ���/��
������ax��ay��az�ֱ��Ӧ������ļ��ٶ�ԭʼ����
	***************************************************/
	
	ax = ACC.x;
	ay = ACC.y;
	az = ACC.z;

	gx = Radians(GYR.x * MPU6050_GYR_2000DPS);
	gy = Radians(GYR.y * MPU6050_GYR_2000DPS);
	gz = Radians(GYR.z * MPU6050_GYR_2000DPS);	
	
	Attitude.Rate->x = gx;
	Attitude.Rate->y = gy;
	Attitude.Rate->z = gz;
	
	//�����ٶȵ�ԭʼ���ݣ���һ�����õ���λ���ٶ�
	arm_sqrt_f32(ax * ax + ay * ay + az * az,&Norm);
	if(Norm == 0) return;
	ax = ax / Norm;
	ay = ay / Norm;
	az = az / Norm;
	
	/**************************************************
	����Ԫ������ɡ��������Ҿ����еĵ����е�����Ԫ�ء�
	�������Ҿ����ŷ���ǵĶ��壬��������ϵ������������
	ת����������ϵ��������������Ԫ�ء����������vx��vy��vz��
	��ʵ���ǵ�ǰ�Ļ����������ϵ�ϣ����������������λ������
	(�ñ�ʾ������̬����Ԫ�����л���)
	***************************************************/
	vx = 2.0f * (Q.q2 * Q.q4 - Q.q1 * Q.q3);
	vy = 2.0f * (Q.q1 * Q.q2 + Q.q3 * Q.q4);
	vz = 1.0f - 2.0f * ( Q.q2 * Q.q2 + Q.q3 * Q.q3);//Q.w * Q.w + Q.z * Q.z;
	
	/***************************************************
	���������������������(Ҳ����������)����ʾ��	ex��
	ey��ez�����������������Ĳ���������������Ծ���λ�ڻ���
	����ϵ�ϵģ������ݻ������Ҳ���ڻ�������ϵ�����Ҳ���Ĵ�
	С�����ݻ����������ȣ����������������ݡ����������ǶԻ�
	��ֱ�ӻ��֣����Զ����ݵľ�������ֱ�������ڶԻ�������ϵ��
	������
	***************************************************/
	ex = (ay * vz - az * vy);
	ey = (az * vx - ax * vz);
	ez = (ax * vy - ay * vx);
	
	/***************************************************
	�ò���������PI����������ƫ��ͨ������Kp��Ki������������
	�Կ��Ƽ��ٶȼ����������ǻ�����̬���ٶ�
	***************************************************/
	if(Ki > 0)
	{
		exInt = exInt + ex * Ki;
		eyInt = eyInt + ey * Ki;
		ezInt = ezInt + ez * Ki;
		gx = gx + Kp * ex + exInt;
		gy = gy + Kp * ey + eyInt;
		gz = gz + Kp * ez + ezInt;
	}
	else
	{
		gx = gx + Kp * ex;
		gy = gy + Kp * ey;
		gz = gz + Kp * ez;   
	}
	
	//��Ԫ��΢�ַ��� 
	Q.q1 += (-Q.q2 * gx - Q.q3 * gy - Q.q4 * gz) * HalfT;
	Q.q2 += ( Q.q1 * gx + Q.q3 * gz - Q.q4 * gy) * HalfT;
	Q.q3 += ( Q.q1 * gy - Q.q2 * gz + Q.q4 * gx) * HalfT;
	Q.q4 += ( Q.q1 * gz + Q.q2 * gy - Q.q3 * gx) * HalfT;

	//��Ԫ����λ��
	arm_sqrt_f32(Q.q1 * Q.q1 + Q.q2 * Q.q2 + Q.q3 * Q.q3 + Q.q4 * Q.q4,&Norm);
	
	if(Norm == 0) return;
	
	Q.q1 = Q.q1 / Norm;
	Q.q2 = Q.q2 / Norm;
	Q.q3 = Q.q3 / Norm;
	Q.q4 = Q.q4 / Norm;		
	
	Angle.x = Degrees(atan2f(2.0f*(Q.q1*Q.q2 + Q.q3*Q.q4),1 - 2.0f*(Q.q2*Q.q2 + Q.q3*Q.q3)));
	Angle.y = Degrees(Safe_Asin(2.0f*(Q.q1*Q.q3 - Q.q2*Q.q4)));
	Angle.z = Degrees(atan2f(2.0f*(Q.q2*Q.q3 - Q.q1*Q.q4),2.0f*(Q.q1*Q.q1 + Q.q2*Q.q2) - 1));
	//�����᲻�غϣ��Ҵ��ڷ���
	//���õشż���ƫ�����ٶ�
	//Ϊ����Z��������Ը����Ǽ������ǵ�Ӱ�죬��������Z��
	//�شż������ڷɻ���бʱ��Ҫ���ݽǶȽ��в���
	//��y���˶�->����
	
//	if((MAG.x != 0) || (MAG.y != 0)) 
//	{
//		//���صش�
//		float MagX,MagY,AngleZ;
//		float COS_P = arm_cos_f32(Radians(-Angle.y));
//		float COS_R = arm_cos_f32(Radians(Angle.x));
//		float SIN_P = arm_sin_f32(Radians(-Angle.y));
//		float SIN_R = arm_sin_f32(Radians(Angle.x));
//		
//		MagX = HMC5883.Data->MAG_ADC.x * COS_P + HMC5883.Data->MAG_ADC.y * SIN_R * SIN_P - HMC5883.Data->MAG_ADC.z * COS_R * SIN_P;
//		MagY = HMC5883.Data->MAG_ADC.y * COS_R - HMC5883.Data->MAG_ADC.z * SIN_R;
//		
//		AngleZ = Degrees(atan2f(MagX,MagY));
//		
//		if(abs(To_180_degrees(AngleZ - Angle.z)) < 1.0f)
//			gz -= 0.1f * Kp * Radians(To_180_degrees(AngleZ - Angle.z));
//		else
//			gz -= Kp * Radians(To_180_degrees(AngleZ - Angle.z));	
//	}
//	//��Ԫ��΢�ַ��� 
//	Qz.q1 += (-Qz.q2 * gx - Qz.q3 * gy - Qz.q4 * gz) * HalfT;
//	Qz.q2 += ( Qz.q1 * gx + Qz.q3 * gz - Qz.q4 * gy) * HalfT;
//	Qz.q3 += ( Qz.q1 * gy - Qz.q2 * gz + Qz.q4 * gx) * HalfT;
//	Qz.q4 += ( Qz.q1 * gz + Qz.q2 * gy - Qz.q3 * gx) * HalfT;

//	//��Ԫ����λ��
//	arm_sqrt_f32(Qz.q1 * Qz.q1 + Qz.q2 * Qz.q2 + Qz.q3 * Qz.q3 + Qz.q4 * Qz.q4,&Norm);
//	
//	if(Norm == 0) return;
//	
//	Qz.q1 = Qz.q1 / Norm;
//	Qz.q2 = Qz.q2 / Norm;
//	Qz.q3 = Qz.q3 / Norm;
//	Qz.q4 = Qz.q4 / Norm;

}

/***************************************************
* ����ŷ����
* ÿ��2MS����һ����̬��
***************************************************/

void Updata_Eular(void)
{
	double DltaT;
	uint64_t Time_Now = 0;
	static uint64_t Time_Pre = 0;
	
	Time_Now = SystemTime.Now_US();
	DltaT = (Time_Now - Time_Pre) * (double)1e-6;
	Time_Pre = Time_Now;
	
	Updata_Quaternion(MPU6050.Data->GYR_ADC,MPU6050.Data->ACC_ADC,HMC5883.Data->MAG_ADC,DltaT);	

}





