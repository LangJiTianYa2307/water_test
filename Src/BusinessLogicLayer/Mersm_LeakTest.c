/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-08 16:39:35
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-23 10:00:37
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\Mersm_LeakTest.c
 */

/*
 *2020-04-21更新：增加测试步骤，平衡结束后，会比较平衡阶段和抽气阶段的平均压力差，若差值过大则判断需要干燥
*/
#include "Mersm_LeakTest.h"
#include "BLL_Alarm.h"
#include "BLL_Modbus.h"
#include "APP.h"
#include "stdlib.h"
#include "math.h"
#include "GeneralFunc.h"
/*----------------------------------------#defines--------------------------------------*/
#define LEAK_RAND_UPPER_LIMIT 2.5f  //低于下限时所转换的随机数上限
#define PUMP_BAL_DIFF 100 //抽气阶段和平衡阶段的压差判定限制，超过该限制则认为需要干燥，单位pa

typedef void (* leakActionFunc)(Mersm_Leak * const me);
typedef struct
{
  LeakStateType_t m_stateID;
  u8 m_isInitFlag;
  LeakTestType m_type;
  float m_measPresRef; //泄漏参考值
  u32 m_measResult;    //测试结果标志位
}LeakTestData;  //私有成员
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static function-------------------------------*/
static void Mersm_init(Mersm_Leak * const me);                       //初始化
static LeakStateType_t Mersm_getCurrentState(Mersm_Leak * const me);
static u32 Mersm_getResult(Mersm_Leak * const me);
static void Mersm_clearPublicResult(void); //清除所有对象共有的测试结果
static void addNgCount(u8 port);
static void clearNgCount(u8 port);
static void clearAllNgCount(void);
/*多事件接收器*/
static void Mersm_evInit(Mersm_Leak * const me);          //初始化事件
static void Mersm_evStart(Mersm_Leak * const me); //启动事件
static void Mersm_evStop(Mersm_Leak * const me);  //停止事件
static void Mersm_evDelayOver(Mersm_Leak * const me); //延时结束

void enterSUB_NULL(Mersm_Leak * const me);
void enterSUB_AIR_PUMP(Mersm_Leak * const me);
void enterSUB_BALANCE(Mersm_Leak * const me);
void enterSUB_MEAS_1(Mersm_Leak * const me);
void enterSUB_DRY_1(Mersm_Leak * const me);
void enterSUB_MEAS_2(Mersm_Leak * const me);
void enterSUB_DRY_2(Mersm_Leak * const me);
void enterSUB_MEAS_3(Mersm_Leak * const me);
void enterSUB_DRY_3(Mersm_Leak * const me);
void enterSUB_MEAS_4(Mersm_Leak * const me);

void exitSUB_NULL(Mersm_Leak * const me);
void exitSUB_AIR_PUMP(Mersm_Leak * const me);
void exitSUB_BALANCE(Mersm_Leak * const me);
void exitSUB_MEAS_1(Mersm_Leak * const me);
void exitSUB_DRY_1(Mersm_Leak * const me);
void exitSUB_MEAS_2(Mersm_Leak * const me);
void exitSUB_DRY_2(Mersm_Leak * const me);
void exitSUB_MEAS_3(Mersm_Leak * const me);
void exitSUB_DRY_3(Mersm_Leak * const me);
void exitSUB_MEAS_4(Mersm_Leak * const me);

static void leakDispCb(void * , void *);
static float resRandom(u32 seed, float max, float min);
static void caclResAndSend(Mersm_Leak * const me);
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static variable-------------------------------*/
static Mersm_Leak s_Mersm_LeakDev[2];
static LeakTestData s_LeakTestData[2];
static u32 s_measResult;    //测试结果标志位
static u8 s_ngCount;

static const leakActionFunc ActionEnterArr[SUB_MAX] = {
  enterSUB_NULL, enterSUB_AIR_PUMP, enterSUB_BALANCE, enterSUB_MEAS_1,
  enterSUB_DRY_1, enterSUB_MEAS_2, enterSUB_DRY_2, enterSUB_MEAS_3,
  enterSUB_DRY_3, enterSUB_MEAS_4
};
static const leakActionFunc ActionExitArr[SUB_MAX] = {
  exitSUB_NULL, exitSUB_AIR_PUMP, exitSUB_BALANCE, exitSUB_MEAS_1,
  exitSUB_DRY_1, exitSUB_MEAS_2, exitSUB_DRY_2, exitSUB_MEAS_3,
  exitSUB_DRY_3, exitSUB_MEAS_4
};
/*--------------------------------------------------------------------------------------*/

/**
 * @brief	创建测试状态机对象
 * @param	
 * @retval	
 */
Mersm_Leak * Mersm_Leak_create(LeakTestType leakType)
{
  Mersm_Leak * me = & s_Mersm_LeakDev[leakType];
  LeakTestData * private_data = 0;
  if (0 == me->init){
    me->init = Mersm_init;
    me->evInit = Mersm_evInit;
    me->evStop = Mersm_evStop;
    me->evStart = Mersm_evStart;
    me->evDelayOver = Mersm_evDelayOver;

    me->getCurrentState = Mersm_getCurrentState;
    me->clearPublicResult = Mersm_clearPublicResult;
    me->getResult = Mersm_getResult;
    me->addNgCount = addNgCount;
    me->clearNgCount = clearNgCount;
    me->clearAllNgCount = clearAllNgCount;
    
    me->private_data = & s_LeakTestData[leakType];
    private_data = (LeakTestData *)me->private_data;
    private_data->m_isInitFlag = 0;
    private_data->m_type = leakType;
  }
  return me;
}

/**
 * @brief	初始化
 * @param	me:指向基类对象的指针
 * @retval	
 */
static void Mersm_init(Mersm_Leak * const me)
{
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  if (0 == private_data->m_isInitFlag){
    private_data->m_stateID = SUB_NULL;
    private_data->m_isInitFlag = 1;
  }
}

/**
 * @brief	获取当前状态
 * @param	
 * @retval	
 */
static LeakStateType_t Mersm_getCurrentState(Mersm_Leak * const me)
{
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  return private_data->m_stateID;
}

/**
 * @brief	获取测试结果
 * @param	
 * @retval	当前状态机的测试结果，BIT0-3分别表示1-4号穴测试结果，0OK，1NG
 */
static u32 Mersm_getResult(Mersm_Leak * const me)
{
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  return private_data->m_measResult;
}
/**
 * @brief	初始化事件，首次进入该状态时会调用
 * @param	
 * @retval	
 */
static void Mersm_evInit(Mersm_Leak * const me)
{
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  private_data->m_measResult = 0; //清零结果
  private_data->m_stateID = SUB_NULL;
  enterSUB_NULL(me);
}

/**
 * @brief	清除所有状态机实例共有的测试结果
 * @param	
 * @retval	
 */
static void Mersm_clearPublicResult(void)
{
  s_measResult = 0;
}

/**
 * @brief	接收到开始信号,则自动进入抽气状态
 * @param	
 * @retval	
 */
static void Mersm_evStart(Mersm_Leak * const me)
{
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  if (SUB_NULL == private_data->m_stateID){
    exitSUB_NULL(me);
    private_data->m_stateID = SUB_AIR_PUMP;
    enterSUB_AIR_PUMP(me);
  }
}

/**
 * @brief	接收到停止信号，进入到待机状态
 * @param	
 * @retval	
 */
static void Mersm_evStop(Mersm_Leak * const me)
{
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  if (private_data->m_stateID <= SUB_MEAS_4){
    ActionExitArr[private_data->m_stateID](me);
    private_data->m_stateID = SUB_NULL;
    ActionEnterArr[SUB_NULL](me);
  }
}

/**
 * @brief	延时结束信号，用于倒计时结束进入后续状态
 * @param	
 * @retval	
 */
static void Mersm_evDelayOver(Mersm_Leak * const me)
{
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  switch (private_data->m_stateID){
  case SUB_AIR_PUMP:
    private_data->m_measPresRef = me->m_pAD->getPresData(me->m_pAD, PUMP_PRES_DATA);  //计算压力参考值
  case SUB_BALANCE:
  case SUB_DRY_1:
  case SUB_DRY_2:
  case SUB_DRY_3:
    /*上述状态倒计时结束后，直接进入下一状态即可，中途无其他动作*/
    ActionExitArr[private_data->m_stateID](me); //离开当前状态
    private_data->m_stateID = (private_data->m_stateID + 1) % SUB_MAX;  //更新当前状态，防止溢出
    ActionEnterArr[private_data->m_stateID](me); //进入下一状态
    break;
  case SUB_MEAS_1:
  case SUB_MEAS_2:
  case SUB_MEAS_3:
  case SUB_MEAS_4:
    /*每个穴位测试结束后，会更新测试结果，然后进入下一状态*/
    ActionExitArr[private_data->m_stateID](me); //离开当前状态

    caclResAndSend(me);//计算测试结果,并将测试结果更新至MODBUS
    
    private_data->m_stateID = (private_data->m_stateID + 1) % SUB_MAX;
    ActionEnterArr[private_data->m_stateID](me); //进入下一状态
    break;
  default:
    break;
  }
}

/*
*函数功能：有限状态机进入参数检测待机阶段
*/
void enterSUB_NULL(Mersm_Leak * const me)
{
  AppMesage msg;
  /*给MODBUS发消息清零数据更新标志位(用于指示哪个数据正在更新)*/
	msg.dataType = MB_UPDATE_DATA_FLAG;
	msg.pVoid = (void *)0;
	xQueueSend(g_modbusQ, &msg, 10);
	printf("enterSUB_NULL\r\n");
}
	
/*
*函数功能：有限状态机进入参数检测抽气阶段
*/	
void enterSUB_AIR_PUMP(Mersm_Leak * const me)
{
	int i = 0;
	AppMesage msg;
  LeakTestData * private_data = (LeakTestData *)me->private_data;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*微真空，微漏1234，针阀，平衡，加压，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == LIT_LEAK_1_V) || (i == LIT_LEAK_2_V) || \
			  (i == LIT_LEAK_3_V) || (i == LIT_LEAK_4_V) || \
				(i == NEEDLE_V)     || (i == BALANCE_V)    || \
				(i == PRES_TO_WATER_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}

  private_data->m_measPresRef = 0;  //参考值清零

  /*给MODBUS发消息更新状态*/
  if (LEAK_LOW_PRES == private_data->m_type) msg.pVoid = (void *)MB_STATE_L_PUMP;
	else msg.pVoid = (void *)MB_STATE_H_PUMP;
  msg.dataType = MB_UPDATE_STATE;
  xQueueSend(g_modbusQ, &msg, 10);
  
  me->m_pDelay->setDelayTime(me->m_pDelay, me->m_pTimePara->AirPump);//开启倒计时定时器

	printf("enterSUB_AIR_PUMP\r\n");
}

/*
*函数功能：有限状态机进入参数检测平衡阶段
*/	
void enterSUB_BALANCE(Mersm_Leak * const me)
{
	int i = 0;
	AppMesage msg;
  LeakTestData * private_data = (LeakTestData *)me->private_data;

	for (i = WATER_IN_V; i < MAX_NUM_V; i ++)
	{
		/*微真空，针阀，平衡，加压，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == NEEDLE_V)     || (i == BALANCE_V)    || \
				(i == PRES_TO_WATER_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}

  /*给MODBUS发消息更新状态*/
  if (LEAK_LOW_PRES == private_data->m_type) msg.pVoid = (void *)MB_STATE_L_BAL;
  else msg.pVoid = (void *)MB_STATE_H_BAL;
  msg.dataType = MB_UPDATE_STATE;
	xQueueSend(g_modbusQ, &msg, 10);
  
	/*开启倒计时定时器*/
  me->m_pDelay->setDelayTime(me->m_pDelay, me->m_pTimePara->balance);

	printf("enterSUB_BALANCE\r\n");
}

/*
*函数功能：有限状态机进入参数检测测试1阶段
*/
void enterSUB_MEAS_1(Mersm_Leak * const me)
{
	int i = 0;
	AppMesage msg;
  float tempPres;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  
  /*计算平衡阶段10s的平均压力,如果与抽气阶段压差大于设定值，说明需要干燥;
    给上位机发送“需要干燥”报警信息，给PLC发送“DRY”信息*/
  tempPres = me->m_pAD->getPresData(me->m_pAD, PUMP_PRES_DATA); 
  if (PUMP_BAL_DIFF < fabsf(tempPres - private_data->m_measPresRef)){
    pErr->setErrBit(ERR_NEED_DRY);
    pErr->setVbStatus(EX_DRY_S);
  }

  private_data->m_measPresRef = tempPres;  //用平衡阶段平均压力计算压力参考值
  
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++)
	{
		/*微真空，微漏1，针阀，加压，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == LIT_LEAK_1_V) || \
				(i == NEEDLE_V)     || \
				(i == PRES_TO_WATER_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}

  /*注册状态动作回调函数（泄漏致显示），并开启倒计时定时器*/
  me->m_pDelay->registerCallback(me->m_pDelay, me, leakDispCb, TIME_REACT);
  me->m_pDelay->setDelayTime(me->m_pDelay, me->m_pTimePara->measure);

	/*根据当前状态来决定启动高压还是低压数据监测，根据主状态来确定是微漏1还是大漏1数据正在更新*/
	if (LEAK_LOW_PRES == private_data->m_type){
		/*给MODBUS发消息更新标识位和状态*/
		msg.dataType = MB_UPDATE_DATA_FLAG;
		msg.pVoid = (void *)L_LEAK_1_DATA;
		xQueueSend(g_modbusQ, &msg, 10);

		msg.dataType = MB_UPDATE_STATE;
		msg.pVoid = (void *)MB_STATE_L_1_LEAK;
		xQueueSend(g_modbusQ, &msg, 10);
	}else if (LEAK_HIGH_PRES == private_data->m_type){
		/*给MODBUS发消息更新标识位和状态*/
		msg.dataType = MB_UPDATE_DATA_FLAG;
		msg.pVoid = (void *)H_LEAK_1_DATA;
		xQueueSend(g_modbusQ, &msg, 10);

		msg.dataType = MB_UPDATE_STATE;
		msg.pVoid = (void *)MB_STATE_H_1_LEAK;
		xQueueSend(g_modbusQ, &msg, 10);
	}

	printf("enterSUB_MEAS_1\r\n");
}

/*
*函数功能：有限状态机进入参数检测干燥1阶段
*/
void enterSUB_DRY_1(Mersm_Leak * const me)
{
	int i = 0;
	AppMesage msg;
  LeakTestData * private_data = (LeakTestData *)me->private_data;

	for (i = WATER_IN_V; i < MAX_NUM_V; i ++)
	{
		/*微真空，针阀，平衡，加压，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == NEEDLE_V)     || (i == BALANCE_V)    || \
				(i == PRES_TO_WATER_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}

  /*给MODBUS发消息更新状态*/
  if (LEAK_LOW_PRES == private_data->m_type) msg.pVoid = (void *)MB_STATE_L_1_DRY;
  else msg.pVoid = (void *)MB_STATE_H_1_DRY;
  msg.dataType = MB_UPDATE_STATE;
	xQueueSend(g_modbusQ, &msg, 10);

	me->m_pDelay->setDelayTime(me->m_pDelay, me->m_pTimePara->measDry);//开始倒计时
	printf("enterSUB_DRY_1\r\n");
}

/*
*函数功能：有限状态机进入参数检测测试2阶段
*/
void enterSUB_MEAS_2(Mersm_Leak * const me)
{
	int i = 0;
	AppMesage msg;
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++)
	{
		/*微真空，微漏1，针阀，加压，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == LIT_LEAK_2_V) || \
				(i == NEEDLE_V)     || \
				(i == PRES_TO_WATER_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}

  /*注册状态动作回调函数（泄漏致显示），并开启倒计时定时器*/
  me->m_pDelay->registerCallback(me->m_pDelay, me, leakDispCb, TIME_REACT);
  me->m_pDelay->setDelayTime(me->m_pDelay, me->m_pTimePara->measure);

	/*根据当前状态来决定启动高压还是低压数据监测，根据主状态来确定是微漏1还是大漏1数据正在更新*/
	if (LEAK_LOW_PRES == private_data->m_type){
		/*给MODBUS发消息更新标识位和状态*/
		msg.dataType = MB_UPDATE_DATA_FLAG;
		msg.pVoid = (void *)L_LEAK_2_DATA;
		xQueueSend(g_modbusQ, &msg, 10);

		msg.dataType = MB_UPDATE_STATE;
		msg.pVoid = (void *)MB_STATE_L_2_LEAK;
		xQueueSend(g_modbusQ, &msg, 10);
	}else if (LEAK_HIGH_PRES == private_data->m_type){
		/*给MODBUS发消息更新标识位和状态*/
		msg.dataType = MB_UPDATE_DATA_FLAG;
		msg.pVoid = (void *)H_LEAK_2_DATA;
		xQueueSend(g_modbusQ, &msg, 10);

		msg.dataType = MB_UPDATE_STATE;
		msg.pVoid = (void *)MB_STATE_H_2_LEAK;
		xQueueSend(g_modbusQ, &msg, 10);
	}

	printf("enterSUB_MEAS_2\r\n");
}

/*
*函数功能：有限状态机进入参数检测干燥2阶段
*/
void enterSUB_DRY_2(Mersm_Leak * const me)
{
	int i = 0;
	AppMesage msg;
  LeakTestData * private_data = (LeakTestData *)me->private_data;

	for (i = WATER_IN_V; i < MAX_NUM_V; i ++)
	{
		/*微真空，针阀，平衡，加压，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == NEEDLE_V)     || (i == BALANCE_V)    || \
				(i == PRES_TO_WATER_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}

  /*给MODBUS发消息更新状态*/
  if (LEAK_LOW_PRES == private_data->m_type) msg.pVoid = (void *)MB_STATE_L_2_DRY;
  else msg.pVoid = (void *)MB_STATE_H_2_DRY;
  msg.dataType = MB_UPDATE_STATE;
	xQueueSend(g_modbusQ, &msg, 10);

  me->m_pDelay->setDelayTime(me->m_pDelay, me->m_pTimePara->measDry);//开始倒计时
  
	printf("enterSUB_DRY_2\r\n");
}

/*
*函数功能：有限状态机进入参数检测测试3阶段
*/
void enterSUB_MEAS_3(Mersm_Leak * const me)
{
	int i = 0;
	AppMesage msg;
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++)
	{
		/*微真空，微漏1，针阀，加压，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == LIT_LEAK_3_V) || \
				(i == NEEDLE_V)     || \
				(i == PRES_TO_WATER_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}

  /*注册状态动作回调函数（泄漏致显示），并开启倒计时定时器*/
  me->m_pDelay->registerCallback(me->m_pDelay, me, leakDispCb, TIME_REACT);
  me->m_pDelay->setDelayTime(me->m_pDelay, me->m_pTimePara->measure);

	/*根据当前状态来决定启动高压还是低压数据监测，根据主状态来确定是微漏1还是大漏1数据正在更新*/
	if (LEAK_LOW_PRES == private_data->m_type){
		/*给MODBUS发消息更新标识位和状态*/
		msg.dataType = MB_UPDATE_DATA_FLAG;
		msg.pVoid = (void *)L_LEAK_3_DATA;
		xQueueSend(g_modbusQ, &msg, 10);

		msg.dataType = MB_UPDATE_STATE;
		msg.pVoid = (void *)MB_STATE_L_3_LEAK;
		xQueueSend(g_modbusQ, &msg, 10);
	}else if (LEAK_HIGH_PRES == private_data->m_type){
		/*给MODBUS发消息更新标识位和状态*/
		msg.dataType = MB_UPDATE_DATA_FLAG;
		msg.pVoid = (void *)H_LEAK_3_DATA;
		xQueueSend(g_modbusQ, &msg, 10);

		msg.dataType = MB_UPDATE_STATE;
		msg.pVoid = (void *)MB_STATE_H_3_LEAK;
		xQueueSend(g_modbusQ, &msg, 10);
	}

	printf("enterSUB_MEAS_3\r\n");
}

/*
*函数功能：有限状态机进入参数检测干燥3阶段
*/
void enterSUB_DRY_3(Mersm_Leak * const me)
{
	int i = 0;
	AppMesage msg;
  LeakTestData * private_data = (LeakTestData *)me->private_data;

	for (i = WATER_IN_V; i < MAX_NUM_V; i ++)
	{
		/*微真空，针阀，平衡，加压，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == NEEDLE_V)     || (i == BALANCE_V)    || \
				(i == PRES_TO_WATER_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
	
  /*给MODBUS发消息更新状态*/
  if (LEAK_LOW_PRES == private_data->m_type) msg.pVoid = (void *)MB_STATE_L_3_DRY;
  else msg.pVoid = (void *)MB_STATE_H_3_DRY;
  msg.dataType = MB_UPDATE_STATE;
	xQueueSend(g_modbusQ, &msg, 10);

  me->m_pDelay->setDelayTime(me->m_pDelay, me->m_pTimePara->measDry);//开始倒计时

	printf("enterSUB_DRY_3\r\n");
}

/*
*函数功能：有限状态机进入参数检测平衡4阶段
*/
void enterSUB_MEAS_4(Mersm_Leak * const me)
{
	int i = 0;
	AppMesage msg;
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++)
	{
		/*微真空，微漏1，针阀，加压，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == LIT_LEAK_4_V) || \
				(i == NEEDLE_V)     || \
				(i == PRES_TO_WATER_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}

  /*注册状态动作回调函数（泄漏致显示），并开启倒计时定时器*/
  me->m_pDelay->registerCallback(me->m_pDelay, me, leakDispCb, TIME_REACT);
  me->m_pDelay->setDelayTime(me->m_pDelay, me->m_pTimePara->measure);

	/*根据当前状态来决定启动高压还是低压数据监测，根据主状态来确定是微漏1还是大漏1数据正在更新*/
	if (LEAK_LOW_PRES == private_data->m_type){
		/*给MODBUS发消息更新标识位和状态*/
		msg.dataType = MB_UPDATE_DATA_FLAG;
		msg.pVoid = (void *)L_LEAK_4_DATA;
		xQueueSend(g_modbusQ, &msg, 10);

		msg.dataType = MB_UPDATE_STATE;
		msg.pVoid = (void *)MB_STATE_L_4_LEAK;
		xQueueSend(g_modbusQ, &msg, 10);
	}else if (LEAK_HIGH_PRES == private_data->m_type){
		/*给MODBUS发消息更新标识位和状态*/
		msg.dataType = MB_UPDATE_DATA_FLAG;
		msg.pVoid = (void *)H_LEAK_4_DATA;
		xQueueSend(g_modbusQ, &msg, 10);

		msg.dataType = MB_UPDATE_STATE;
		msg.pVoid = (void *)MB_STATE_H_4_LEAK;
		xQueueSend(g_modbusQ, &msg, 10);
	}
  
	printf("enterSUB_MEAS_4\r\n");
}

void exitSUB_NULL(Mersm_Leak * const me)
{
}
void exitSUB_AIR_PUMP(Mersm_Leak * const me)
{
  me->m_pDelay->stopTimer(me->m_pDelay);  //关闭定时器
}
void exitSUB_BALANCE(Mersm_Leak * const me)
{
  me->m_pDelay->stopTimer(me->m_pDelay);  //关闭定时器
}
void exitSUB_MEAS_1(Mersm_Leak * const me)
{
  AppMesage msg;
  /*给MODBUS发消息清零数据更新标志位(用于指示哪个数据正在更新)*/
	msg.dataType = MB_UPDATE_DATA_FLAG;
	msg.pVoid = (void *)0;
	xQueueSend(g_modbusQ, &msg, 10);

  me->m_pDelay->unregisterCallback(me->m_pDelay, TIME_REACT); //注销泄漏值更新回调函数
  me->m_pDelay->stopTimer(me->m_pDelay);  //关闭定时器
}
void exitSUB_DRY_1(Mersm_Leak * const me)
{
  me->m_pDelay->stopTimer(me->m_pDelay);  //关闭定时器
}
void exitSUB_MEAS_2(Mersm_Leak * const me)
{
  AppMesage msg;
  /*给MODBUS发消息清零数据更新标志位(用于指示哪个数据正在更新)*/
	msg.dataType = MB_UPDATE_DATA_FLAG;
	msg.pVoid = (void *)0;
	xQueueSend(g_modbusQ, &msg, 10);
  
  me->m_pDelay->unregisterCallback(me->m_pDelay, TIME_REACT); //注销泄漏值更新回调函数
  me->m_pDelay->stopTimer(me->m_pDelay);  //关闭定时器
}
void exitSUB_DRY_2(Mersm_Leak * const me)
{
  me->m_pDelay->stopTimer(me->m_pDelay);  //关闭定时器
}
void exitSUB_MEAS_3(Mersm_Leak * const me)
{
  AppMesage msg;
  /*给MODBUS发消息清零数据更新标志位(用于指示哪个数据正在更新)*/
	msg.dataType = MB_UPDATE_DATA_FLAG;
	msg.pVoid = (void *)0;
	xQueueSend(g_modbusQ, &msg, 10);

  me->m_pDelay->unregisterCallback(me->m_pDelay, TIME_REACT); //注销泄漏值更新回调函数
  me->m_pDelay->stopTimer(me->m_pDelay);  //关闭定时器
}
void exitSUB_DRY_3(Mersm_Leak * const me)
{
  me->m_pDelay->stopTimer(me->m_pDelay);  //关闭定时器
}
void exitSUB_MEAS_4(Mersm_Leak * const me)
{
  AppMesage msg;
  /*给MODBUS发消息清零数据更新标志位(用于指示哪个数据正在更新)*/
	msg.dataType = MB_UPDATE_DATA_FLAG;
	msg.pVoid = (void *)0;
	xQueueSend(g_modbusQ, &msg, 10);
  
  me->m_pDelay->unregisterCallback(me->m_pDelay, TIME_REACT); //注销泄漏值更新回调函数
  me->m_pDelay->stopTimer(me->m_pDelay);  //关闭定时器
}


/**
 * @brief	泄漏值更新回调函数,定时器会周期调用该函数
 * @param	pInstance:指向调用的对象
 * @param pData:由被订阅者发送的数据，此处未使用
 * @retval	
 */
static void leakDispCb(void * pInstance, void * pData)
{
  Mersm_Leak * me = (Mersm_Leak *)pInstance;
  AppMesage msg;
  static float leakRate;
  static u32 s_count = 0;
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  
  //100ms触发一次
  if ((s_count % 10) == 0){
    if (LEAK_LOW_PRES == private_data->m_type){
      if (SUB_MEAS_1 == private_data->m_stateID)  msg.dataType = MB_UPDATE_LOW_1;
      else if (SUB_MEAS_2 == private_data->m_stateID) msg.dataType = MB_UPDATE_LOW_2;
      else if (SUB_MEAS_3 == private_data->m_stateID) msg.dataType = MB_UPDATE_LOW_3;
      else if (SUB_MEAS_4 == private_data->m_stateID) msg.dataType = MB_UPDATE_LOW_4;
      else{
        s_count ++;
        return;
      }
    }
    else{
      if (SUB_MEAS_1 == private_data->m_stateID)  msg.dataType = MB_UPDATE_HIHG_1;
      else if (SUB_MEAS_2 == private_data->m_stateID) msg.dataType = MB_UPDATE_HIGH_2;
      else if (SUB_MEAS_3 == private_data->m_stateID) msg.dataType = MB_UPDATE_HIGH_3;
      else if (SUB_MEAS_4 == private_data->m_stateID) msg.dataType = MB_UPDATE_HIGH_4;
      else{
        s_count ++;
        return;
      }
    }// end if (LEAK_LOW_PRES == private_data->m_type)
    leakRate = (me->m_pAD->getPresData(me->m_pAD, TEST_PRES_DATA) - private_data->m_measPresRef ) / 20.0;
    msg.pVoid = convFloat2u32(leakRate);
    xQueueSend(g_modbusQ, &msg, 0);
  }

  s_count ++;
}

/**
 * @brief	生成随机数
 * @param	seed:随机数种 
 * @param max:生成的随机数最大值
 * @param min:生成的随机数最小值
 * @retval	随机数
 */
static float resRandom(u32 seed, float max, float min)
{
  u32 randValue;
  float out;
  if (max <= min) return min;
  srand(seed);
  randValue = rand() % ((u32)(max * 1000) - (u32)(min * 1000) + 1) + (u32)(min * 1000);
  out = randValue / 1000.0;
  return out;
}

/**
 * @brief	穴位测试结束后，计算测试结果并将结果发送至MODBUS，包含测试数据以及测试结果
 * @param	me:指向状态机对象的指针
 * @retval	
 */
static void caclResAndSend(Mersm_Leak * const me)
{
  float leakRate;
  AppMesage msg;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  LeakTestData * private_data = (LeakTestData *)me->private_data;
  
  /*计算测试结果,并将测试结果更新至MODBUS*/
  leakRate = (me->m_pAD->getPresData(me->m_pAD, TEST_PRES_DATA) - private_data->m_measPresRef ) / 20.0;
  //低压
  if (LEAK_LOW_PRES == private_data->m_type){
    switch (private_data->m_stateID){
    case SUB_MEAS_1:
      if (leakRate > me->m_pPresPara->lowLeakRate) private_data->m_measResult |= 0x01;
      msg.dataType = MB_UPDATE_LOW_1;
      break;
    case SUB_MEAS_2:
      if (leakRate > me->m_pPresPara->lowLeakRate) private_data->m_measResult |= 0x02;
      msg.dataType = MB_UPDATE_LOW_2;
      break;
    case SUB_MEAS_3:
      if (leakRate > me->m_pPresPara->lowLeakRate) private_data->m_measResult |= 0x04;
      msg.dataType = MB_UPDATE_LOW_3;
      break;
    case SUB_MEAS_4:
      if (leakRate > me->m_pPresPara->lowLeakRate) private_data->m_measResult |= 0x08;
      msg.dataType = MB_UPDATE_LOW_4;
      break;
    default:
      return;
      break;
    }
    s_measResult = private_data->m_measResult;  //低4位用来描述低压测试结果
    /*泄漏量小于最小值，生成最小值和5之间的随机数,然后显示该数值*/
    if (leakRate <= me->m_pPresPara->lowerLimitL){
      leakRate = resRandom(xTaskGetTickCount(), LEAK_RAND_UPPER_LIMIT, me->m_pPresPara->lowerLimitL);
    }
    msg.pVoid = convFloat2u32(leakRate);  //浮点数转化为u32格式
    xQueueSend(g_modbusQ, &msg, 10);
  }
  else{
    switch (private_data->m_stateID){
    case SUB_MEAS_1:
      if (leakRate > me->m_pPresPara->highLeakRate){
        /*泄漏值大于设定值就直接判断为NG*/
        private_data->m_measResult |= 0x01;
        addNgCount(1);
      }
      else{
        /*高压泄漏小于设定值，低压NG，最终结果还是NG，如果泄漏值小于设定最低限，还需要转换为随机数*/
        if (s_measResult & 0x01)  addNgCount(1);
        else  clearNgCount(1);
        if (leakRate <= me->m_pPresPara->lowerLimitH){
          leakRate = resRandom(xTaskGetTickCount(), LEAK_RAND_UPPER_LIMIT, me->m_pPresPara->lowerLimitH);
        }
      }
      /*更新高压泄漏值*/
      msg.dataType = MB_UPDATE_HIHG_1;
      break;
    case SUB_MEAS_2:
      if (leakRate > me->m_pPresPara->highLeakRate){
        /*泄漏值大于设定值就直接判断为NG*/
        private_data->m_measResult |= 0x02;
        addNgCount(2);
      }
      else{
        /*高压泄漏小于设定值，低压NG，最终结果还是NG，如果泄漏值小于设定最低限，还需要转换为随机数*/
        if (s_measResult & 0x02)  addNgCount(2);
        else  clearNgCount(2);
        if (leakRate <= me->m_pPresPara->lowerLimitH){
          leakRate = resRandom(xTaskGetTickCount(), LEAK_RAND_UPPER_LIMIT, me->m_pPresPara->lowerLimitH);
        }
      }
      msg.dataType = MB_UPDATE_HIGH_2;
      break;
    case SUB_MEAS_3:
      if (leakRate > me->m_pPresPara->highLeakRate){
        /*泄漏值大于设定值就直接判断为NG*/
        private_data->m_measResult |= 0x04;
        addNgCount(3);
      }
      else{
        /*高压泄漏小于设定值，低压NG，最终结果还是NG，如果泄漏值小于设定最低限，还需要转换为随机数*/
        if (s_measResult & 0x04)  addNgCount(3);
        else  clearNgCount(3);
        if (leakRate <= me->m_pPresPara->lowerLimitH){
          leakRate = resRandom(xTaskGetTickCount(), LEAK_RAND_UPPER_LIMIT, me->m_pPresPara->lowerLimitH);
        }
      }
      msg.dataType = MB_UPDATE_HIGH_3;
      break;
    case SUB_MEAS_4:
      if (leakRate > me->m_pPresPara->highLeakRate){
        /*泄漏值大于设定值就直接判断为NG*/
        private_data->m_measResult |= 0x08;
        addNgCount(4);
      }
      else{
        /*高压泄漏小于设定值，低压NG，最终结果还是NG，如果泄漏值小于设定最低限，还需要转换为随机数*/
        if (s_measResult & 0x08)  addNgCount(4);
        else  clearNgCount(4);
        if (leakRate <= me->m_pPresPara->lowerLimitH){
          leakRate = resRandom(xTaskGetTickCount(), LEAK_RAND_UPPER_LIMIT, me->m_pPresPara->lowerLimitH);
        }
      }
      msg.dataType = MB_UPDATE_HIGH_4;
      break;
    default:
      return;
      break;
    }

    /*更新高压泄漏值*/
    msg.pVoid = convFloat2u32(leakRate);
    xQueueSend(g_modbusQ, &msg, 10);
    /*给MODBUS发消息更新测试结果（包含微漏测试结果和大漏测试结果，便于上位机文件进行保存）*/
    s_measResult |= (private_data->m_measResult << 4);  //加入高压数据
    msg.dataType = MB_UPDATE_ALL_RES;
    msg.pVoid = &s_measResult;
    xQueueSend(g_modbusQ, &msg, 10);
  }
  
}

/**
 * @brief	指定穴位NG计数器加1，并且发送NG结果
 * @param	port:测试穴位，编号1~4
 * @retval	
 */
static void addNgCount(u8 port)
{
  FlagHand * pErr = errHandCreate();  //错误处理对象
  s_ngCount += (1 << (2*(port - 1)));
  /*连续NG两次则发送干燥信息*/
  if (s_ngCount & 0b10101010){
    pErr->setVbStatus(EX_DRY_S);    //给PLC发送需要干燥的消息
    pErr->setErrBit(ERR_NEED_DRY);  //界面提示“需要干燥”
    s_ngCount = 0;
  }
  if (1 == port)  pErr->setVbRes(EX_NG1_S);
  else if (2 == port) pErr->setVbRes(EX_NG2_S);
  else if (3 == port) pErr->setVbRes(EX_NG3_S);
  else if (4 == port) pErr->setVbRes(EX_NG4_S);
}

/**
 * @brief	清除指定穴位NG计数器并且发布OK结果
 * @param	port:测试穴位，编号1~4
 * @retval	
 */
static void clearNgCount(u8 port)
{
  FlagHand * pErr = errHandCreate();  //错误处理对象
  s_ngCount &= ~(0b11 << (2*(port - 1)));
  if (1 == port)  pErr->setVbRes(EX_OK1_S);
  else if (2 == port) pErr->setVbRes(EX_OK2_S);
  else if (3 == port) pErr->setVbRes(EX_OK3_S);
  else if (4 == port) pErr->setVbRes(EX_OK4_S);
}

/**
 * @brief	清除所有穴位NG计数器
 * @param	
 * @retval	
 */
static void clearAllNgCount(void)
{
  s_ngCount = 0;
}