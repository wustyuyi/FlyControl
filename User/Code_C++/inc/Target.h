#ifndef __TARGET_H__
#define __TARGET_H__
#include "Time.h"

typedef void (*pFun)(u16 Time);

class Target_
{
	public:
		Target_(pFun Aim, int Cycle_Time)
		{
			Cycle = Cycle_Time - 1;
			CycleMS = Cycle_Time / 1000;
			Time_Store = 0;
			Target = Aim;
		};
		void Run(void);		
		u32 Frequent;     //ʵ��ִ������
		u32 Time_Need;    //ִ�л���ʱ��

	private:
		uint64_t Time_Store;  //������һ��ִ�е�ʱ��
		u32 Cycle;        //ִ������
		u32 CycleMS;
		pFun Target;
};

#endif
