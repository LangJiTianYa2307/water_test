/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:22:28
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-11 14:30:04
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\BLL_Modbus.c
 */
#include "BLL_Modbus.h"
#include "APP.h"
#include "lwip_comm.h" 
#include "lwip_client_app.h"
#include "FML_ExtValve.h"
#include "FML_DelayTime.h"
#include "FML_ExtSignal.h"
#include "FML_ExtProporV.h"

#include "BLL_MainFsm.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
/* ----------------------- Defines ------------------------------------------*/
#define REG_DIS_START 1
#define REG_DIS_NREGS 100
#define REG_COIL_START 101
#define REG_COIL_NREGS 100
#define REG_INPUT_START 201
#define REG_INPUT_NREGS 100
#define REG_HOLD_START 301
#define REG_HOLD_NREGS 100
/* ----------------------- Static variables ---------------------------------*/
static USHORT   usRegInputStart = REG_INPUT_START;  //输入寄存器的起始地址
static USHORT   usRegInputBuf[REG_INPUT_NREGS];     //输入寄存器数组
static USHORT   usRegHoldStart = REG_HOLD_START;  //保持寄存器的起始地址
static USHORT   usRegHoldBuf[REG_HOLD_NREGS];     //保持寄存器数组
static USHORT   usRegDisStart = REG_DIS_START;  //离散寄存器的起始地址
static UCHAR   usRegDisBuf[REG_DIS_NREGS];     //离散寄存器数组
static USHORT   usRegCoilStart = REG_COIL_START;  //线圈寄存器的起始地址
static UCHAR   usRegCoilBuf[REG_COIL_NREGS];     //线圈寄存器数组
/* --------------------------------私有成员-------------------------------------*/
ModbusDev modbusDev;
static u8 sendBuff[1000];
static u16 m_dataLen;       //数据长度
/* -----------------------------------------------------------------------------*/
/* --------------------------------公有函数-------------------------------------*/
static void modbus_init(void);
static void modbus_devRun(void);
static void update_ext_v_cb(void *, void *);
static void update_ext_sig_cb(void *, void *);
static void update_pro_v_cb(void *, void *);
static void update_delay_time(void *, void *);
/* -----------------------------------------------------------------------------*/
ModbusDev * modbusCreate(void)
{
	static ModbusDev * me = 0;
	if (me == 0){
		me = & modbusDev;
		me->init = modbus_init;
		me->devRun_thread = modbus_devRun;
		
		me->m_pRecvBuff = 0;
		me->m_pSendBuff = sendBuff;
		me->init();
	}
	return me;
}

/**
 * @brief	modbus初始化，此处会初始化LWIP和MODBUS
 * @param	
 * @retval	
 */
static void modbus_init(void)
{
	eMBErrorCode    eStatus;

	eStatus = eMBTCPInit(502);

	/* Enable the Modbus Protocol Stack. */
	eStatus = eMBEnable();
	/*调试用*/
	usRegInputBuf[0] = 0x86;
  /*注册回调函数*/
  FML_DelayTime * pFML_DelayTime = FML_DelayTime_Create(D_TIM_1);
  pFML_DelayTime->registerCallback(pFML_DelayTime, & modbusDev, update_delay_time, TIME_DISP);
  
  ValveIODev * pValveIODev = valveIODevCreate();
  pValveIODev->registerCallback(pValveIODev, & modbusDev, update_ext_v_cb, CB_V_DISP);

  ExitSing * pExitSing = exitSingCreate();
  pExitSing->registerCallback(pExitSing, & modbusDev, update_ext_sig_cb, EXT_UPDATE);

  ProporValDev * pProporValDev = proporValCreate();
  pProporValDev->registerCallback(pProporValDev, & modbusDev, update_pro_v_cb, CB_PRO_DISP);
}

/**
 * @brief	modbus线程，用于处理传入的参数
 * @param	
 * @retval	
 */
static void modbus_devRun(void)
{
	BaseType_t res;         //freeRTOS基本类型
	AppMesage appMsg;
	u32 * pU32Temp = 0;
  int i = 0;
	while (1){
		res = xQueueReceive(g_modbusQ, &appMsg, portMAX_DELAY);//获取消息队列中的数据
		if (res == pdTRUE){
			switch(appMsg.dataType){
				case MB_UPDATE_DELAY_TIME:
          /*更新每个阶段的延时倒计时*/
					usRegInputBuf[MB_DELAY_TIME_HI] = (*(u32*)appMsg.pVoid >> 16) & 0xffff;
				  usRegInputBuf[MB_DELAY_TIME_LO] = (*(u32*)appMsg.pVoid) & 0xffff;
					break;
				case MB_UPDATE_STATE:
					/*更新状态，直接传递指针*/
					usRegInputBuf[MB_STATE] = (u32)appMsg.pVoid & 0xffff;
					break;
				case MB_UPDATE_WATER_PRES:
				case MB_UPDATE_ABS_PRES:
          /*更新水压和负压数据*/
					pU32Temp = (u32*)appMsg.pVoid;
					usRegInputBuf[MB_WATER_PRES_HI] = (*pU32Temp >> 16) & 0xffff;
					usRegInputBuf[MB_WATER_PRES_LO] = (*pU32Temp) & 0xffff;
					pU32Temp ++;
					usRegInputBuf[MB_ABS_PRES_HI] = (*pU32Temp >> 16) & 0xffff;
					usRegInputBuf[MB_ABS_PRES_LO] = (*pU32Temp) & 0xffff;
					break;
        case MB_UPDATE_LOW_1:
          /*更新#1低压泄露*/
          usRegInputBuf[MB_L_LEAK_1_HI] = ((u32)appMsg.pVoid >> 16) & 0xffff;
				  usRegInputBuf[MB_L_LEAK_1_LO] = ((u32)appMsg.pVoid) & 0xffff;
          break;
        case MB_UPDATE_LOW_2:
          /*更新#2低压泄露*/
          usRegInputBuf[MB_L_LEAK_2_HI] = ((u32)appMsg.pVoid >> 16) & 0xffff;
				  usRegInputBuf[MB_L_LEAK_2_LO] = ((u32)appMsg.pVoid) & 0xffff;
          break;
        case MB_UPDATE_LOW_3:
          /*更新#3低压泄露*/
          usRegInputBuf[MB_L_LEAK_3_HI] = ((u32)appMsg.pVoid >> 16) & 0xffff;
				  usRegInputBuf[MB_L_LEAK_3_LO] = ((u32)appMsg.pVoid) & 0xffff;
          break;
        case MB_UPDATE_LOW_4:
          /*更新#4低压泄露*/
          usRegInputBuf[MB_L_LEAK_4_HI] = ((u32)appMsg.pVoid >> 16) & 0xffff;
				  usRegInputBuf[MB_L_LEAK_4_LO] = ((u32)appMsg.pVoid) & 0xffff;
          break;
        case MB_UPDATE_HIHG_1:
          /*更新#1高压泄露*/
          usRegInputBuf[MB_H_LEAK_1_HI] = ((u32)appMsg.pVoid >> 16) & 0xffff;
				  usRegInputBuf[MB_H_LEAK_1_LO] = ((u32)appMsg.pVoid) & 0xffff;
          break;
        case MB_UPDATE_HIGH_2:
          /*更新#2高压泄露*/
          usRegInputBuf[MB_H_LEAK_2_HI] = ((u32)appMsg.pVoid >> 16) & 0xffff;
				  usRegInputBuf[MB_H_LEAK_2_LO] = ((u32)appMsg.pVoid) & 0xffff;
          break;
        case MB_UPDATE_HIGH_3:
          /*更新#3高压泄露*/
          usRegInputBuf[MB_H_LEAK_3_HI] = ((u32)appMsg.pVoid >> 16) & 0xffff;
				  usRegInputBuf[MB_H_LEAK_3_LO] = ((u32)appMsg.pVoid) & 0xffff;
          break;
        case MB_UPDATE_HIGH_4:
          /*更新#4高压泄露*/
          usRegInputBuf[MB_H_LEAK_4_HI] = ((u32)appMsg.pVoid >> 16) & 0xffff;
				  usRegInputBuf[MB_H_LEAK_4_LO] = ((u32)appMsg.pVoid) & 0xffff;
          break;
        case MB_UPDATE_RES:
          /*更新结果,接收到的消息为U32，最高位表示结果0-OK，1-NG，1-8位表示不同类别的测试结果*/
					usRegInputBuf[MB_RES_FLAG] = *(u16 *)appMsg.pVoid;
          break;
				case MB_UPDATE_ALL_RES:
          /*更新结果,接收到的消息为U32，最高位表示结果0-OK，1-NG，1-8位表示不同类别的测试结果(包含微漏和大漏)*/
					usRegInputBuf[MB_ALL_RES] = *(u16 *)appMsg.pVoid;
          break;
				case MB_CLEAR_FLAG:
					/*清除掉标志位*/
					usRegInputBuf[MB_DATA_UPDATE_FLAG] = 0;
					break;
				case MB_UPDATE_ERR:
					/*更新错误状态寄存器*/
					usRegInputBuf[MB_ERROR] = *(u32 *)appMsg.pVoid & 0xffff;
					usRegInputBuf[MB_ERR_2] = *(u32 *)appMsg.pVoid >> 16;
					break;
				case MB_UPDATE_DATA_FLAG:
					/*指示哪个数据正在更新，直接传递指针*/
					usRegInputBuf[MB_DATA_UPDATE_FLAG] = (u16)appMsg.pVoid;
					break;
				case MB_TEST_OVER:
					/*每次收到开始信号会发出此消息，此时会将当前测试数据复制给PRE数据进行保存;
					  同时会把当前数据清0*/
					for (i = 0; i < 8; i ++){
						usRegInputBuf[33 + 2*i] = usRegInputBuf[7 + 2*i];
						usRegInputBuf[33 + 2*i + 1] = usRegInputBuf[7 + 2*i + 1];
						usRegInputBuf[7 + 2*i] = 0;
						usRegInputBuf[7 + 2*i + 1] = 0;
					}
					break;
				case MB_UPDATE_VB_STATUS:
					/*更新VB_STATUS,直接发送16位数据*/
					usRegInputBuf[MB_VB_STATUS] = *(u16 *)appMsg.pVoid;
					break;
				default:
					break;
			}				
		}
	}
}

/**
  @brief	更新阀门状态回调函数
  @param	pInstabce:注册回调函数的对象
  @param  pVoid:指向保存阀门状态数组的头指针
  @retval	
  */
static void update_ext_v_cb(void * pInstabce, void * pVoid)
{
  int i;
  /*更新阀门状态*/
  for (i = 0; i < 22; i ++){
    if (((u8 *)pVoid)[i]) usRegCoilBuf[i/8] |= (1 << (i%8));
    else usRegCoilBuf[i/8] &= ~(1 << (i%8));
  }
}

/**
 * @brief	更新传感器状态的回调函数
 * @param	pVoid:传感器状态
 * @retval	
 */
static void update_ext_sig_cb(void * pInstabce, void * pVoid)
{
  u8 sensorState = 0;
  u16 mbSensorState = 0;
  sensorState = *(u8 *)pVoid;
  if (sensorState & (1 << WATER_DECT_SING)) mbSensorState |= MB_SENS_LIQ;
  if (sensorState & (1 << VACUUM_SING)) mbSensorState |= MB_SENS_VACU;
  if (sensorState & (1 << H_CLY_POS_SING))  mbSensorState |= MB_SENS_CYL_H;
  if (sensorState & (1 << L_CLY_POS_SING))  mbSensorState |= MB_SENS_CYL_L;
  if (sensorState & (1 << GRAT_SING))  mbSensorState |= MB_SENS_GRAT;
  usRegInputBuf[MB_SENS_FLAG] = mbSensorState;
}

/**
 * @brief	更新比例阀开度回调函数
 * @param	pVoid:指向比例阀开度的指针
 * @retval	
 */
static void update_pro_v_cb(void * pInstabce, void * pVoid)
{
  usRegHoldBuf[MB_OPENING_HI] = (*(u32*)pVoid >> 16) & 0xffff;
  usRegHoldBuf[MB_OPENING_LO] = (*(u32*)pVoid) & 0xffff;
}

/**
 * @brief	倒计时剩余时间显示更新
 * @param	pVoid:指向剩余时间
 * @retval	
 */
static void update_delay_time(void * pInstabce, void * pVoid)
{
  /*更新每个阶段的延时倒计时*/
  usRegInputBuf[MB_DELAY_TIME_HI] = (*(u32*)pVoid >> 16) & 0xffff;
  usRegInputBuf[MB_DELAY_TIME_LO] = (*(u32*)pVoid) & 0xffff;
}
/* ------------------------------------Modbus TCP相关的数据读取与发送函数声明-----------------------------------------*/
BOOL            xMBTCPPortGetRequest( UCHAR **ppucMBTCPFrame, USHORT * usTCPLength )
{
	* ppucMBTCPFrame = modbusDev.m_pRecvBuff;
	* usTCPLength = m_dataLen;
	return TRUE;
}

/*
*函数功能：通过网口发送应答帧
*输入参数：pucMBTCPFrame:帧数据;usTCPLength:帧长度
*返 回 值：TRUE:成功；FALSE:失败
*/
BOOL            xMBTCPPortSendResponse( const UCHAR *pucMBTCPFrame, USHORT usTCPLength )
{
	AppMesage appMsg;
	appMsg.dataType = usTCPLength;          //长度
	appMsg.pVoid = (void *)pucMBTCPFrame;   //数据
	xQueueSend(g_netQ, &appMsg, 10);
	return TRUE;
}

/*
*函数功能：TCP端口初始化，在MODBUS初始化的时候会调用以初始化网口
*输入参数：usTCPPort:端口号
*返 回 值：TRUE:成功；FALSE:失败
*/
BOOL            xMBTCPPortInit( USHORT usTCPPort )
{
	lwip_comm_init();
	tcp_client_init();
	return TRUE;
}

void            vMBTCPPortClose( void )
{

}

void            vMBTCPPortDisable( void )
{
	
}

/*
*函数功能：读取输入寄存器里面的数据
*输入参数：pucRegBuffer:指向保存所读到的数据缓冲区
           usAddress:寄存器起始地址
           usNRegs:需要读取的寄存器数量
*返 回 值：TRUE:成功；FALSE:失败
*/
eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
	eMBErrorCode    eStatus = MB_ENOERR;
	int             iRegIndex;
	if( ( usAddress >= REG_INPUT_START ) && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
	{
			iRegIndex = ( int )( usAddress - usRegInputStart );
			while( usNRegs > 0 )
			{
					*pucRegBuffer++ = ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
					*pucRegBuffer++ = ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
					iRegIndex++;
					usNRegs--;
			}
	}else
	{
			eStatus = MB_ENOREG;
	}
	return eStatus;
}

/*
*函数功能：读写保持寄存器里面的数据
*输入参数：pucRegBuffer:指向保存所读到的数据缓冲区
           usAddress:寄存器起始地址
           usNRegs:需要读取的寄存器数量
           eMode:模式，读或者写
*返 回 值：MB_ENOERR:成功；MB_ENOREG:失败
*/
eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                 eMBRegisterMode eMode )
{
  eMBErrorCode    eStatus = MB_ENOERR;
  int             iRegIndex;
	/*-------------自定义变量区------------------*/
	AppMesage appMsg;  //APP消息
  MainFsmDev * me = MainFsmDev_create();  //主状态机
  me->init(me);
	u16 iNRegs = usNRegs;
	/*-------------------------------------------*/
	if( ( usAddress >= REG_HOLD_START )
			&& ( usAddress + usNRegs <= REG_HOLD_START + REG_HOLD_NREGS ) )
	{
		/*首先判断地址是否在支持的范围内，然后在根据参数对寄存器进行读写*/
		iRegIndex = ( int )( usAddress - usRegHoldStart );//得到偏移量
		if (eMode == MB_REG_WRITE)
		{
			while( usNRegs > 0 )
			{
				usRegHoldBuf[iRegIndex] = *pucRegBuffer++ << 8;
				usRegHoldBuf[iRegIndex] |= *pucRegBuffer++;
				iRegIndex++;
				usNRegs--;
			}
			/*更新参数*/
			if (((usAddress - usRegHoldStart) == 0) && (iNRegs == 84))
			{
				/*收到更新参数的数据，将参数数组首地址的指针发给设备控制线程*/
        appMsg.dataType = EV_SET_PARA;
        appMsg.pVoid = usRegHoldBuf;
        me->sendEvent(& appMsg);

			}else if (((usAddress - usRegHoldStart) == 98) && (iNRegs == 2))
			{
				/*此时抽到的数据为电子比例阀开度，仅在DEBUG模式下有效*/
        appMsg.dataType = EV_SET_PROPOR;
        appMsg.pVoid = &usRegHoldBuf[98];
        me->sendEvent(& appMsg);
			}
		}else if (eMode == MB_REG_READ)
		{
			/*读取保持寄存器里面的数据*/
			while( usNRegs > 0 )
			{
				*pucRegBuffer++ = ( unsigned char )( usRegHoldBuf[iRegIndex] >> 8 );
				*pucRegBuffer++ = ( unsigned char )( usRegHoldBuf[iRegIndex] & 0xFF );
				iRegIndex++;
				usNRegs--;
			}
		}else eStatus = MB_ENOREG;
	}else
	{
		eStatus = MB_ENOREG;
	}
	return eStatus;
}

/*
*函数功能：线圈读写回调函数
*输入参数：pucRegBuffer:指向保存所读到的数据缓冲区
           usAddress:寄存器起始地址
           usNRegs:需要读取的寄存器数量
           eMode:模式，读或者写
*返 回 值：MB_ENOERR:成功；MB_ENOREG:失败
*/
eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,
               eMBRegisterMode eMode )
{
  eMBErrorCode    eStatus = MB_ENOERR;
	short           iNCoils = ( short )usNCoils;
	unsigned short  usBitOffset;
	/*-------------自定义变量区------------------*/
	MainFsmDev * me = MainFsmDev_create();  //主状态机
  me->init(me);
	AppMesage appMsg;  //APP消息
	/*-------------------------------------------*/
	/* Check if we have registers mapped at this block. */
	if( ( usAddress >= REG_COIL_START ) &&
			( usAddress + usNCoils <= REG_COIL_START + REG_COIL_NREGS ) )
	{
		usBitOffset = ( unsigned short )( usAddress - REG_COIL_START );
		switch ( eMode )
		{
			/* Read current values and pass to protocol stack. */
			case MB_REG_READ:
				while( iNCoils > 0 )
				{
					*pucRegBuffer++ = xMBUtilGetBits( usRegCoilBuf, usBitOffset,( unsigned char )( iNCoils >8 ? 8 : iNCoils ) );
					iNCoils -= 8;
					usBitOffset += 8;
				}
				break;

			/* Update current register values. */
			case MB_REG_WRITE:
				while( iNCoils > 0 )
				{
					xMBUtilSetBits( usRegCoilBuf, usBitOffset,( unsigned char )( iNCoils > 8 ? 8 : iNCoils ),*pucRegBuffer++ );
					iNCoils -= 8;
				}
				/*----------------------------根据线圈不同的状态来执行不同的指令----------------------------------------*/
				if ((usBitOffset == MB_START) && (usNCoils == 1))
				{
					/*只有当单独给START线圈置位的时候，测试系统开始运行，同时将STOP线圈置0*/
					if (usRegCoilBuf[MB_START / 8] & (1 << (MB_START % 8)))
					{
						usRegCoilBuf[MB_STOP / 8] &=  ~(1 << (MB_STOP % 8));
						appMsg.dataType = EV_START;
            me->sendEvent(& appMsg);
					}
				}else if ((usBitOffset == MB_STOP) && (usNCoils == 1))
				{
					/*只有当单独给STOP线圈置位的时候，测试系统停止，同时将START线圈置0*/
					if (usRegCoilBuf[MB_STOP / 8] & (1 << (MB_STOP % 8)))
					{
						usRegCoilBuf[MB_START / 8] &= ~(1 << (MB_START % 8));
						appMsg.dataType = EV_STOP;
            me->sendEvent(& appMsg);
					}
				}else if ((usBitOffset >= MB_WATER_IN_V) && (usBitOffset <= MB_CYL_L_V) && (usNCoils == 1))
        {
          /*单独控制阀门的时候，根据阀门的编号来给设备控制线程发送阀门控制信息*/
          if (usRegCoilBuf[usBitOffset/8] & (1 << (usBitOffset%8))) appMsg.pVoid = (void *)(0x80000000 | (1<<usBitOffset));
          else appMsg.pVoid = (void *)(0x7FFFFFFF & (1<<usBitOffset));

          appMsg.dataType = EV_SET_SIG_V;
          me->sendEvent(& appMsg);
        }else if ((usBitOffset == MB_DEBUG) && (usNCoils == 1))
        {
          /*1进入调试状态,0退出调试状态;然后给状态机发送DEBUG消息
          */
          // if (usRegCoilBuf[MB_DEBUG / 8] & (1 << (MB_DEBUG%8))) tempType = 1;
          // else tempType = 0;

          appMsg.dataType = EV_DEBUG_MODE;
          me->sendEvent(& appMsg);
        }else if ((usBitOffset == MB_DRY) && (usNCoils == 1))
        {
          /*1进入干燥状态,0退出干燥状态;然后给状态机发送DRY消息
          */
          // if (usRegCoilBuf[MB_DRY / 8] & (1 << (MB_DRY%8))) tempType = 1;
          // else tempType = 0;

          appMsg.dataType = EV_DRY_MODE;
          me->sendEvent(& appMsg);
        }
				/*------------------------------------------------------------------------------------------------------*/
				break;
		}

	}
	else
	{
			eStatus = MB_ENOREG;
	}
	return eStatus;
}

/*
*函数功能：离散寄存器回调函数
*输入参数：pucRegBuffer:指向保存所读到的数据缓冲区
           usAddress:寄存器起始地址
           usNRegs:需要读取的寄存器数量
*返 回 值：TRUE:成功；FALSE:失败
*/
eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
  eMBErrorCode    eStatus = MB_ENOERR;
	short           iNCoils = ( short )usNDiscrete;
	unsigned short  usBitOffset;

	/* Check if we have registers mapped at this block. */
	if( ( usAddress >= REG_DIS_START ) &&
			( usAddress + usNDiscrete <= REG_DIS_START + REG_DIS_NREGS ) )
	{
		usBitOffset = ( unsigned short )( usAddress - REG_COIL_START );
		while( iNCoils > 0 )
		{
			*pucRegBuffer++ = xMBUtilGetBits( usRegCoilBuf, usBitOffset,( unsigned char )( iNCoils >8 ? 8 : iNCoils ) );
			iNCoils -= 8;
			usBitOffset += 8;
		}
	}
	else
	{
			eStatus = MB_ENOREG;
	}
	return eStatus;
}

