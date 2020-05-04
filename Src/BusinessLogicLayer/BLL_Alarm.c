/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:22:28
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-15 10:10:33
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\BLL_Alarm.c
 */
#include "BLL_Alarm.h"
#include "BLL_Modbus.h"
#include "APP.h"
/*-----------------------------------------------私有变量----------------------------------------------*/
FlagHand errHand; //错误处理对象
static u32 m_errCode;  //错误标识1
static u16 m_vbStatus;
static u16 m_vbRes;
/*-----------------------------------------------------------------------------------------------------*/
/*-----------------------------------------------成员函数----------------------------------------------*/
static void errHand_init(void);                 //初始化
static void errHand_setErrBit(ERROR_t type);    //设置错误位
static void errHand_clearErrBit(ERROR_t type);  //清除错误位
static u16 errHand_getAllErr(void);             //获取所有错误
static void errHand_clearAllErr(void);          //清除所有错误

static void setVbStatus(VB_STATUS_t type);    //设置错误位
static void clearVbStatus(VB_STATUS_t type);  //清除错误位
static u16 getAllVbStatus(void);             //获取所有错误
static void clearAllVbStatus(void);          //清除所有错误

static void setVbRes(VB_RES_t type);    //设置错误位
static void clearVbRes(VB_RES_t type);  //清除错误位
static u16 getAllVbRes(void);             //获取所有错误
static void clearAllVbRes(void);          //清除所有错误

static void errHand_threadRun(void);            //独立线程
/*-----------------------------------------------------------------------------------------------------*/
/*
*函数功能：创建错误处理对象，单例模式
*输入参数：无
*返 回 值：me:指向所创建对象的指针
*/
FlagHand * errHandCreate(void)
{
	static FlagHand * me = 0;
	if (me == 0)
	{
		me = & errHand;
		me->clearAllErr = errHand_clearAllErr;
		me->clearErrBit = errHand_clearErrBit;
		me->getAllErr = errHand_getAllErr;
		me->init = errHand_init;
		me->setErrBit = errHand_setErrBit;
		
		me->setVbRes = setVbRes;
		me->clearVbRes = clearVbRes;
		me->getAllVbRes = getAllVbRes;
		me->clearAllVbRes = clearAllVbRes;
		
		me->setVbStatus = setVbStatus;
		me->clearVbStatus = clearVbStatus;
		me->getAllVbStatus = getAllVbStatus;
		me->clearAllVbStatus = clearAllVbStatus;
		me->threadRun = errHand_threadRun;
		
		me->init();
	}
	return me;
}

/*
*函数功能：错误处理对象初始化，创建时自动调用
*输入参数：无
*返 回 值：无
*/
static void errHand_init(void)
{
	m_errCode = 0;
}

/*
*函数功能：对错误状态进行置位
*输入参数：无
*返 回 值：无
*/
static void errHand_setErrBit(ERROR_t type)
{
	AppMesage msg;
	//只有在发现需要置位的标志没有置1时才给MODBUS发消息，减少线程负担
	m_errCode |= type;
	msg.pVoid = &m_errCode;
	/*给MODBUS发消息更新标识位*/
	msg.dataType = MB_UPDATE_ERR;
	xQueueSend(g_modbusQ, &msg, 10);
	/*有错误发生*/
	setVbStatus(EX_ERR_S);
}

/*
*函数功能：清除错误状态
*输入参数：无
*返 回 值：无
*/
static void errHand_clearErrBit(ERROR_t type)
{
	AppMesage msg;
	//只有在指定位置1的时候才会清零，减少线程负担
	if (m_errCode & type){
		m_errCode &= ~type;
		msg.pVoid = &m_errCode;
		/*给MODBUS发消息更新标识位*/
		msg.dataType = MB_UPDATE_ERR;
		xQueueSend(g_modbusQ, &msg, 10);
		/*当没有错误时，清除掉VB_STATUS中的错误标识
			并给MODBUS发消息，更新VB_STATUS*/
		if ((m_errCode == 0) && (m_vbStatus & EX_ERR_S)) clearVbStatus(EX_ERR_S);
	}
}

/*
*函数功能：获取所有错误状态
*输入参数：无
*返 回 值：错误状态
*/
static u16 errHand_getAllErr(void)
{
	return m_errCode;
}

static void errHand_clearAllErr(void)
{
	AppMesage msg;
	m_errCode = 0;
	/*给MODBUS发消息更新标识位*/
	msg.dataType = MB_UPDATE_ERR;
	msg.pVoid = &m_errCode;
	xQueueSend(g_modbusQ, &msg, 10);
	/*当没有错误时，清除掉VB_STATUS中的错误标识
			并给MODBUS发消息，更新VB_STATUS*/
	if (m_vbStatus & EX_ERR_S)  clearVbStatus(EX_ERR_S);
}

/*
*函数功能：错误处理线程
*输入参数：无
*返 回 值：无
*/
static void errHand_threadRun(void)
{
}

/**
  * @brief	设置设备状态（传给PLC）
  * @param	type:设备状态类型
  * @retval	
  */
static void setVbStatus(VB_STATUS_t type)
{
	AppMesage msg;
	switch (type){
		case EX_READY_S:
			/*处于需要干燥的状态时，不响应READY信号*/
			if (! (m_vbStatus & EX_DRY_S)){
				/*收到准备完毕信号，应该清除RUN,DEBUG,OUTDRY等信号*/
				m_vbStatus &= ~(EX_RUN_S | EX_DEBUG_S | EX_DRY_S | EX_END_S);
				m_vbStatus |= type;
			}
			break;
		case EX_RUN_S:
			/*收到RUN信号，表示设备开始动作，因此需要清除READY&END*/
			m_vbStatus &= ~(EX_READY_S | EX_END_S);
			m_vbStatus |= type;
			break;
		case EX_DEBUG_S:
			/*收到DEBUG信号，表示设备进入调试状态，因此需要清除READY&END*/
			m_vbStatus &= ~(EX_READY_S | EX_END_S);
			m_vbStatus |= type;
			break;
		case EX_DRY_S:
			/*收到干燥*/
			m_vbStatus |= type;
			break;
		case EX_END_S:
			m_vbStatus &= ~(EX_READY_S | EX_RUN_S);
			m_vbStatus |= type;
			break;
		case EX_ERR_S:
			m_vbStatus &= ~(EX_READY_S);
			m_vbStatus |= type;
			break;
		default:
			m_vbStatus |= type;
			break;
	}

	/*给MODBUS发消息更新标识位*/
	msg.pVoid = &m_vbStatus;
	msg.dataType = MB_UPDATE_VB_STATUS;
	xQueueSend(g_modbusQ, &msg, 10);
}

/**
  * @brief	清除设备状态（传递给PLC）
  * @param	type:设备状态类型
  * @retval	
  */
static void clearVbStatus(VB_STATUS_t type)
{
	AppMesage msg;
	switch (type){
		case EX_ERR_S:
			/*清除错误状态时，如果系统当前状态只有错误，则可以置为READY状态*/
			if (m_vbStatus == EX_ERR_S) m_vbStatus = EX_READY_S;
	    else  m_vbStatus &= ~type;
			break;
		default:
			m_vbStatus &= ~type;
			break;
	}
	/*给MODBUS发消息更新标识位*/
	msg.pVoid = &m_vbStatus;
	msg.dataType = MB_UPDATE_VB_STATUS;
	xQueueSend(g_modbusQ, &msg, 10);
}

/**
  * @brief	get device status
  * @param	
  * @retval	
  */
static u16 getAllVbStatus(void)
{
	return m_vbStatus;
}

/**
  * @brief	clear all device status
  * @param	
  * @retval	
  */
static void clearAllVbStatus(void)
{
	AppMesage msg;
	m_vbStatus = 0;
	msg.pVoid = &m_vbStatus;
	/*给MODBUS发消息更新标识位*/
	msg.dataType = MB_UPDATE_VB_STATUS;
	xQueueSend(g_modbusQ, &msg, 10);
}

/**
  * @brief	设置测试结果
  * @param	
  * @retval	
  */
static void setVbRes(VB_RES_t type)
{
	AppMesage msg;
	m_vbRes |= type;
	/*给MODBUS发消息更新标识位*/
	msg.dataType = MB_UPDATE_RES;
	msg.pVoid = &m_vbRes;
	xQueueSend(g_modbusQ, &msg, 10);
}

/**
  * @brief	清除测试结果
  * @param	
  * @retval	
  */
static void clearVbRes(VB_RES_t type)
{
	AppMesage msg;
	m_vbRes &= ~type;
	/*给MODBUS发消息更新标识位*/
	msg.dataType = MB_UPDATE_RES;
	msg.pVoid = &m_vbRes;
	xQueueSend(g_modbusQ, &msg, 10);
}

/**
  * @brief	获取测试结果
  * @param	
  * @retval	
  */
static u16 getAllVbRes(void)
{
	return m_vbRes;
}

/**
  * @brief	清楚所有测试结果
  * @param	
  * @retval	
  */
static void clearAllVbRes(void)
{
	AppMesage msg;
	m_vbRes = 0;
	/*给MODBUS发消息更新标识位*/
	msg.dataType = MB_UPDATE_RES;
	msg.pVoid = &m_vbRes;
	xQueueSend(g_modbusQ, &msg, 10);
}








