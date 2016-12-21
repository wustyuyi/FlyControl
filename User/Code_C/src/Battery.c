#include "Battery.h"

void ADC_Init(void);
void ADC_Updata(void);
u16 ADC_Value[5];

struct Power_VOL_Data_ Power_VOL_Data;
struct Power_ Power = 
{
	&Power_VOL_Data,
	ADC_Init,
	ADC_Updata
};

void ADC_GOIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOC,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOE,&GPIO_InitStructure);
	
}
void ADC_DMA_Init(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
	DMA_InitStructure.DMA_BufferSize = 5;
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADC_Value;   //Ŀ������λ
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_BASE+0x4C;  //ADC->DR��ַ
	DMA_InitStructure.DMA_PeripheralBurst =DMA_PeripheralBurst_Single;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_Init(DMA2_Stream0,&DMA_InitStructure);
	DMA_Cmd(DMA2_Stream0,ENABLE);
}
void ADC_Init(void)
{

	ADC_InitTypeDef ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;

	ADC_GOIO_Init();
	ADC_DMA_Init();
	
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; 
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;     //��������
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;  //�ٶȾ����ܵ�
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_10Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	//ADC1����
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_NbrOfConversion = 5;  //ͨ����
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_Init(ADC1,&ADC_InitStructure);
		
	ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_112Cycles);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_10,2,ADC_SampleTime_112Cycles);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_11,3,ADC_SampleTime_112Cycles);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_12,4,ADC_SampleTime_112Cycles);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_13,5,ADC_SampleTime_112Cycles);
	ADC_DMARequestAfterLastTransferCmd(ADC1,ENABLE); //Դ���ݱ仯ʱ����DMA����
	ADC_DMACmd(ADC1,ENABLE);//ʹ��ADC��DMA����
	ADC_Cmd(ADC1,ENABLE);
	ADC_SoftwareStartConv(ADC1);
	ADS1118.Init();
}

void ADC_Updata(void)
{
	static u8 Mark = 0;
	float BatValue = 0;
	Power_VOL_Data.BAT_2S       = ADC_Value[0] / 4095.0f * 2.5f * 4.0f;
	Power_VOL_Data.Main_3V3_MCU = ADC_Value[1] / 4095.0f * 2.5f * 3.0f;
	Power_VOL_Data.Main_5V      = ADC_Value[2] / 4095.0f * 2.5f * 3.0f;
	Power_VOL_Data.BAT_1S       = ADC_Value[3] / 4095.0f * 2.5f * 3.0f;
	Power_VOL_Data.Main_3V3_SEN = ADC_Value[4] / 4095.0f * 2.5f * 3.0f;

	switch (Mark % 3)
	{
		case 0:
			BatValue = ADS1118.Updata(0);
			if(BatValue != -1)
			{
				Power_VOL_Data.POW_5V = BatValue;
				Mark++;
			}
			break;
		case 1:
			BatValue = ADS1118.Updata(1);
			if(BatValue != -1)
			{
				Power_VOL_Data.POW_4V5 = BatValue;
				Mark++;
			}				
			break;
		case 2:
			BatValue = ADS1118.Updata(2);
			if(BatValue != -1)
			{
				Power_VOL_Data.BAT_12S = BatValue;
				Mark++;
			}	
			break;
		default:
			break;
	}
		
	if((GPIOE->IDR & GPIO_Pin_15) == GPIO_Pin_15) Power_VOL_Data.IsUPS_On = True;
	else Power_VOL_Data.IsUPS_On = False;
}
