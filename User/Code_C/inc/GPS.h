#ifndef __GPS_H__
#define __GPS_H__
#include "stm32f4xx.h"
#include "Time.h"
#include "Common.h"
struct GPS_Data_
{
	u8 Count;
	float POS_X;
	float POS_Y;
};
struct GXGGA
{
	float UTC;          //UTCʱ��
	float LAT;          //γ��
	char  NorS;         //�ϱ�
	float LON;          //����
	char WorE;          //����
	u8 state;           //��λ״̬ 0��δ��λ 1���޲�֣�SPSģʽ 2������֣�SPSģʽ 3��PPSģʽ
	u8 SatNum;          //��������
	float HDOP;         //ˮƽ����˥������
	float AltSea;       //��ƽ��߶�
	float AltEarth;     //����ں�ƽ��ĵ���߶�
	u16 GPSBaseTime;    //��һ���յ�GPS���վ���ݿ�ʼ�ļ�ʱ����λ��
	u16 GPSBase;        //���վ���
	
};
struct GXGLL
{
	float UTC;          //UTCʱ��
	float LAT;          //γ��
	char  NorS;         //�ϱ�
	float LON;          //����
	char WorE;          //����
	u8 state;           //��λ״̬ A����Ч V����Ч
};

struct GXRMC
{
	
};
struct GXVTG
{
	
};
extern struct GPS_
{
	struct GPS_Data_* Data;
	void (*Init)(void);
	void (*Updata)(void);
}GPS;

#endif

