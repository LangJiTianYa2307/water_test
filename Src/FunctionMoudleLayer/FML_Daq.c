/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-02-28 15:37:26
 * @LastEditors: PanHan
 * @LastEditTime: 2020-03-11 20:44:26
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\FML_Daq.c
 */


/*
2020-03-04优化分时采样的部分
2020-03-11修复数据采集计数器变量类型，防止计数器溢出；数据采集部分，水压数据部分存在异常，进行修复
*/
#include "FML_Daq.h"
#include "delay.h"
#include "led.h"
#include "ADS_1256.h"
#include "math.h"
#include "GeneralFunc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "rtos_timers.h"

#define USE_3_SIGMA
/*-----------------------------------私有变量---------------------------------------*/
#define PRES_BUFF_LEN 1000    //压力数据缓冲区长度
#define RES_ARR_LEN 120       //用于计算泄漏值所用的缓冲区长度
#define PUMP_AIR_LEN 1000     //用于计算10秒抽气时的压力平均值所用的缓冲区长度
#define WATER_PRES_LEN 200    //水压滤波缓冲区长度
#define PRES_DISP_LEN 50      //实施数据显示所用的到数据长度
#define FILTER_LEN  5
/*----------------------------------------------------------------------------------*/
static void AD_DaqDev_init(DataSever * const me);   //初始化
static float AD_DaqDev_getPresData(DataSever * const me, PresDataType dataType);

static float AD_DaqDev_getMeanPres(void);
static float AD_DaqDev_getResMeanPres(void);
static float daq2waterPres(int input);  //将数据采集卡中的转为水压
static float daq2absPres(int input);    //将数据采集卡中的数据转换为绝对压力
static float deleteGrossError(const float * const input, int len);
static float calcMeanF(const float * const input, int len);
static void daqTimerCb(TimerHandle_t xTimer);

static float vacuumPresBuff[PRES_BUFF_LEN];  //真空传感器压力缓冲数组
static float waterPresBuff[PRES_BUFF_LEN];   //水压缓冲数组
static TimerHandle_t daqTimerHandle;

static DataSever ad_daqDev; //数据采集设备对象

/*用来缓冲采集的数据，以便对其进行滤波*/
static float measurePres[FILTER_LEN];
static float waterPres[FILTER_LEN];

/*
*函数功能：创建传感器设备类（单例）
*输入参数：无
*返 回 值：指向对象的指针
*/
DataSever * DataSever_create(void)
{
	static DataSever * me = 0;
	if (me == 0)
	{
		me = & ad_daqDev;
		me->init = AD_DaqDev_init;
    me->getPresData = AD_DaqDev_getPresData;
		
		me->m_measurePres = 0.0f;
		me->m_waterPres = 0.0f;
		
		me->init(me);  //初始化
	}
	return me;
}

/*
*函数功能：初始化传感器设备
*输入参数：指向对象的指针
*返 回 值：无
*/
static void AD_DaqDev_init(DataSever * const me)
{
	ADS1256_Init();
  //用于数据采集的定时器，更新周期10ms，周期更新
	daqTimerHandle = xTimerCreate((const char*    )"daqCb",
                                (TickType_t     )5,
                                (UBaseType_t    )pdTRUE,
                                (void *         )7,
                                (TimerCallbackFunction_t)daqTimerCb);
	xTimerStart(daqTimerHandle, 10);//启动数据采集定时器	
}

/**
  * @brief	对象操作函数，用于获取压力数据
  * @param	me:指向对象的指针
  * @param  dataType:需要获取的数据类型
  * @retval	压力数据
  */
static float AD_DaqDev_getPresData(DataSever * const me, PresDataType dataType)
{
  switch (dataType)
  {
  case PUMP_PRES_DATA:
    //获取抽气阶段的平均压力
    return AD_DaqDev_getMeanPres();
    break;
  case TEST_PRES_DATA:
    //获取测试阶段的平均压力
    return AD_DaqDev_getResMeanPres();
    break;
  case CUR_VACUM_DATA:
    //获取实时气压
    return calcMeanF(&vacuumPresBuff[PRES_BUFF_LEN - PRES_DISP_LEN], PRES_DISP_LEN);
    break;
  case CUR_WATER_DATA:
    //获取实时水压
    return calcMeanF(&waterPresBuff[PRES_BUFF_LEN - PRES_DISP_LEN], PRES_DISP_LEN);
    break;
  case PRO_DEC_WATER:
    //产品检测所需要的水压
    return calcMeanF(&waterPresBuff[PRES_BUFF_LEN - WATER_PRES_LEN], WATER_PRES_LEN);
    break;
  default:
    break;
  }
}

/**
  * @brief   计算浮点数数组的平均值
  * @param   input:数组指针
  * @param   len:数组长度
  * @retval  平均值
  */
static float calcMeanF(const float * const input, int len)
{
  if (len == 0) return 0;
	float Sum = 0;
	u32 i = 0;
	for (i = 0; i < len; i ++){
		Sum = Sum + input[i];
	}
	return Sum / len;
}

/*
*函数功能：将采集数据转换为水压（信号1-5v，压力范围0-1MPa）
*输入参数：input:采样数据
*返 回 值：压力值，单位kPa
*/
static float daq2waterPres(int input)
{
	float out = 0.0f;
	out = (input * 5.0 / 8388607.0 - 1) * 1000.0 / 4.0;
	return out;
}

/*
*函数功能：将采集数据转换为水压（信号0-5v，压力范围-1-2Bar）
*输入参数：input:采样数据
*返 回 值：压力值，单位Pa
*/
static float daq2absPres(int input)
{
	float out = 0.0f;
	out = (input * 5.0 / 8388607.0 - 1) / 4.0 * 300.0 * 1000.0;
	return out;
}

/*
*函数功能：获取一段时间内的测试压力均值(剔除粗大误差)
*输入参数：无
*返 回 值：压力值，单位Pa
*/
static float AD_DaqDev_getMeanPres(void)
{
	return deleteGrossError(vacuumPresBuff, PUMP_AIR_LEN);
}

/*
*函数功能：获取一段时间内的测试压力均值
*输入参数：无
*返 回 值：压力值，单位Pa
*/
static float AD_DaqDev_getResMeanPres(void)
{
	return deleteGrossError(&vacuumPresBuff[PRES_BUFF_LEN - RES_ARR_LEN], RES_ARR_LEN);
}

/*
*函数功能：对一定长度的数据剔除其粗大误差并返回剔除之后的平均值
*输入参数：input数组首地址，len数组长度
*返 回 值：剔除粗大误差后的均值
*/
static float deleteGrossError(const float * const input, int len)
{
	u32 errBitArr[40] = {0};  //粗大误差错误位
	float sum = 0;
	int i = 0;
	float sigma_3 = 0;   //3_sigma
	float means = 0;     //记录当前剔除掉粗大误差后的均值
	int errCount = 0;
	int preErrCount = 0;
	if (len > (32 * 40)) return 0;
	/*-------------------------------------首次剔除------------------------------------------*/
	/*计算平均值*/
	for (i = 0; i < len; i ++){
		sum += input[i];
	}
	means = sum / len;
	/*计算样本标准差和3_sigma*/
	sum = 0;
	for (i = 0; i < len; i ++){
		sum += (input[i] - means) * (input[i] - means); //计算平方和
	}
	sigma_3 = 3 * sqrtf((sum / (len - 1)));    //计算3_sigma
	sum = 0;
	/*遍历所有数据，只记录小于3_sigma的数据和，记录粗大误差的下标*/
	for (i = 0; i < len; i ++)
	{
		if (fabsf(input[i] - means) < sigma_3)	sum += input[i];
		else{
			errBitArr[i/32] |= (1 << (i%32));   
			errCount ++;
		}
	}
	/*如果粗大误差计数为0，直接返回其均值*/
	means = sum / (len - errCount);
	if (errCount == 0){
		//printf("there is no gross error\r\n");
		return means;
	}		
	/*-------------------------------------二次剔除------------------------------------------*/
	preErrCount = errCount;
	/*计算样本标准差和3_sigma*/
	sum = 0;
	for (i = 0; i < len; i ++){
		if (!(errBitArr[i/32] & (1 << (i%32)))){
			sum += (input[i] - means) * (input[i] - means); //计算平方和
		}
	}
	sigma_3 = 3 * sqrtf((sum / (len - errCount - 1)));    //计算3_sigma
	sum = 0;
	/*遍历所有数据，只记录小于3_sigma的数据和，记录粗大误差的数量以及其下标*/
	for (i = 0; i < len; i ++){
		if (!(errBitArr[i/32] & (1 << (i%32)))){
			if (fabsf(input[i] - means) < sigma_3)	sum += input[i];
			else{
				errBitArr[i/32] |= (1 << (i%32));   
				errCount ++;
			}
		}
	}
	means = sum / (len - errCount);
	/*如果粗大误差计数没有增加，直接返回其均值*/
	if (preErrCount == errCount){
		//printf("the num of gross error is %d\r\n", errCount);
		return means;
	}		
	/*-------------------------------------最后剔除------------------------------------------*/
	/*计算样本标准差和3_sigma*/
	sum = 0;
	for (i = 0; i < len; i ++){
		if (!(errBitArr[i/32] & (1 << (i%32)))){
			sum += (input[i] - means) * (input[i] - means); //计算平方和
		}
	}
	sigma_3 = 3 * sqrtf((sum / (len - errCount - 1)));    //计算3_sigma
	sum = 0;
	/*遍历所有数据，只记录小于3_sigma的数据和，记录粗大误差的数量以及其下标*/
	for (i = 0; i < len; i ++){
		if (!(errBitArr[i/32] & (1 << (i%32)))){
			if (fabsf(input[i] - means) < sigma_3)	sum += input[i];
			else{
				errBitArr[i/32] |= (1 << (i%32));   
				errCount ++;
			}
		}
	}
	means = sum / (len - errCount);

	//printf("the num of gross error is %d\r\n", errCount);
	return means;
}

/**
  * @brief	数据采集定时器回调函数，用于采集传感器数据
  * @param	
  * @retval	
  */
static void daqTimerCb(TimerHandle_t xTimer)
{
	int i = 0;
	uint8_t id;
	int tempInt = 0;
  static u32 s_count = 0;
  static u32 s_waterBuffNum = 0;
  static u32 s_vacumBuffNum = 0;

  /*两个传感器分别采集数据，奇数真空传感器，偶数水压传感器*/
  if ((s_count % 2) == 0){
    /*原始数据滤波，长度为5的递推平滑滤波*/
    if (s_vacumBuffNum < FILTER_LEN){
      measurePres[s_vacumBuffNum] = daq2absPres(ADS1256_ReadAdc(0));
      ad_daqDev.m_measurePres = measurePres[s_vacumBuffNum];
    }
    else{
      for (i = 0; i < (FILTER_LEN - 1); i ++){
        measurePres[i] = measurePres[i + 1];
      }
      measurePres[FILTER_LEN - 1] = daq2absPres(ADS1256_ReadAdc(0));
    }
    fMeanFilter(measurePres, FILTER_LEN, &ad_daqDev.m_measurePres); //平滑滤波

    /*将滤波后的数据保存至缓冲数组*/
    if (s_vacumBuffNum < PRES_BUFF_LEN) vacuumPresBuff[s_vacumBuffNum] = ad_daqDev.m_measurePres;
    else{
      for (i = 0; i < (PRES_BUFF_LEN - 1); i ++){
        vacuumPresBuff[i] = vacuumPresBuff[i + 1];
      }
      vacuumPresBuff[(PRES_BUFF_LEN - 1)] = ad_daqDev.m_measurePres;
    }//end if (m_presBuffNum < PRES_BUFF_LEN)

    s_vacumBuffNum ++;
  }
  else{
    if (s_waterBuffNum < FILTER_LEN){
      waterPres[s_waterBuffNum] = daq2waterPres(ADS1256_ReadAdc(1));
      ad_daqDev.m_waterPres = waterPres[s_waterBuffNum];
    }
    else{
      for (i = 0; i < (FILTER_LEN - 1); i ++){
        waterPres[i] = waterPres[i + 1];
      }
      waterPres[FILTER_LEN - 1] = daq2waterPres(ADS1256_ReadAdc(1));
    }
    fMeanFilter(waterPres, FILTER_LEN, &ad_daqDev.m_waterPres);     //平滑滤波

    /*将滤波后的数据保存至缓冲数组*/
    if (s_waterBuffNum < PRES_BUFF_LEN)  waterPresBuff[s_waterBuffNum] = ad_daqDev.m_waterPres;
    else{
      for (i = 0; i < (PRES_BUFF_LEN - 1); i ++){
        waterPresBuff[i] = waterPresBuff[i + 1];
      }
      waterPresBuff[(PRES_BUFF_LEN - 1)] = ad_daqDev.m_waterPres;
    }
    
    s_waterBuffNum ++;
  }
  s_count ++;
}


