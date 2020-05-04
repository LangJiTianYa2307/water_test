/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:22:28
 * @LastEditors  : PanHan
 * @LastEditTime : 2020-01-14 14:04:32
 * @FilePath: \water_keil\Src\BusinessLogicLayer\BLL_Modbus.h
 */
#ifndef _BLL_MODBUS_H
#define _BLL_MODBUS_H

#include "TypeDef.h"

/*--------------------------------寄存器及线圈宏定义-------------------------------------*/
/*状态更新类型宏定义*/
#define MB_UPDATE_WATER_PRES   1
#define MB_UPDATE_ABS_PRES     2
#define MB_UPDATE_STATE        3
#define MB_UPDATE_DELAY_TIME   4
#define MB_UPDATE_LOW_1        5
#define MB_UPDATE_LOW_2        6
#define MB_UPDATE_LOW_3        7
#define MB_UPDATE_LOW_4        8
#define MB_UPDATE_HIHG_1       9
#define MB_UPDATE_HIGH_2       10
#define MB_UPDATE_HIGH_3       11
#define MB_UPDATE_HIGH_4       12
#define MB_UPDATE_VALVE        13
#define MB_UPDATE_SENS         14
#define MB_UPDATE_RES          15
#define MB_CLEAR_FLAG          16
#define MB_UPDATE_ERR          17
#define MB_UPDATE_DATA_FLAG    18
#define MB_UPDATE_SENS_FLAG    19
#define MB_TEST_OVER           20
#define MB_UPDATE_OPENING      21
#define MB_UPDATE_VB_STATUS    22
#define MB_UPDATE_ALL_RES      23
/*寄存器位置宏定义*/
//只读寄存器偏移量
#define MB_WATER_PRES_HI       0
#define MB_WATER_PRES_LO       1
#define MB_ABS_PRES_HI         2
#define MB_ABS_PRES_LO         3
#define MB_STATE               4
#define MB_DELAY_TIME_HI       5
#define MB_DELAY_TIME_LO       6
#define MB_L_LEAK_1_HI         7
#define MB_L_LEAK_1_LO         8
#define MB_L_LEAK_2_HI         9
#define MB_L_LEAK_2_LO         10
#define MB_L_LEAK_3_HI         11
#define MB_L_LEAK_3_LO         12
#define MB_L_LEAK_4_HI         13
#define MB_L_LEAK_4_LO         14
#define MB_H_LEAK_1_HI         15
#define MB_H_LEAK_1_LO         16
#define MB_H_LEAK_2_HI         17
#define MB_H_LEAK_2_LO         18
#define MB_H_LEAK_3_HI         19
#define MB_H_LEAK_3_LO         20
#define MB_H_LEAK_4_HI         21
#define MB_H_LEAK_4_LO         22

#define MB_ERR_2               27
#define MB_VB_STATUS           28
#define MB_RES_FLAG            29
#define MB_DATA_UPDATE_FLAG    30
#define MB_ERROR               31
#define MB_SENS_FLAG           32

#define MB_L_LEAK_1_PRE_HI     33
#define MB_L_LEAK_1_PRE_LO     34
#define MB_L_LEAK_2_PRE_HI     35
#define MB_L_LEAK_2_PRE_LO     36
#define MB_L_LEAK_3_PRE_HI     37
#define MB_L_LEAK_3_PRE_LO     38
#define MB_L_LEAK_4_PRE_HI     39
#define MB_L_LEAK_4_PRE_LO     40
#define MB_H_LEAK_1_PRE_HI     41
#define MB_H_LEAK_1_PRE_LO     42
#define MB_H_LEAK_2_PRE_HI     43
#define MB_H_LEAK_2_PRE_LO     44
#define MB_H_LEAK_3_PRE_HI     45
#define MB_H_LEAK_3_PRE_LO     46
#define MB_H_LEAK_4_PRE_HI     47
#define MB_H_LEAK_4_PRE_LO     48
#define MB_ALL_RES             49
//读写寄存器偏移量
#define MB_OPENING_HI          98
#define MB_OPENING_LO          99
//读写线圈偏移量
#define MB_WATER_IN_V            0    
#define	MB_ADD_PRES_V            1   
#define MB_PUMP_WATER_V          2
#define MB_AIR_IN_V              3
#define MB_WATER_OUT_V           4
#define	MB_DRY_V                 5
#define MB_H_VACUUM_V            6
#define MB_H_LEAK_1_V            7
#define MB_H_LEAK_2_V            8
#define MB_H_LEAK_3_V            9
#define MB_H_LEAK_4_V            10
#define MB_L_VACUUM_V            11
#define MB_L_LEAK_1_V            12
#define MB_L_LEAK_2_V            13
#define MB_L_LEAK_3_V            14
#define MB_L_LEAK_4_V            15
#define MB_AIR_OUT_V             16
#define MB_NEEDLE_V              17
#define MB_BALANCE_V             18
#define MB_WATER_PUMP            19
#define MB_CYL_H_V               20
#define MB_CYL_L_V               21

#define MB_START                 32
#define MB_STOP                  33
#define MB_DEBUG                 34
#define MB_DRY                   35
/*数据更新标志位*/
#define L_LEAK_1_DATA            0x01
#define L_LEAK_2_DATA            0x02
#define L_LEAK_3_DATA            0x04
#define L_LEAK_4_DATA            0x08
#define H_LEAK_1_DATA            0x10
#define H_LEAK_2_DATA            0x20
#define H_LEAK_3_DATA            0x40
#define H_LEAK_4_DATA            0x80
#define DETEC_FINISH             0x4000
#define DETEC_STOP               0x8000
/*发送给MODBUS寄存器的设备状态定义*/
#define MB_STATE_READY           1
#define MB_STATE_CYL_DOWN        2
#define MB_STATE_13_TOR          3
#define MB_STATE_EXIST           4
#define MB_STATE_PURGE           5
#define MB_STATE_PUMP            6
#define MB_STATE_IS_DRY          7
#define MB_STATE_WATER_IN        8
#define MB_STATE_L_PUMP          9
#define MB_STATE_L_BAL           10
#define MB_STATE_L_1_LEAK        11
#define MB_STATE_L_1_DRY         12
#define MB_STATE_L_2_LEAK        13
#define MB_STATE_L_2_DRY         14
#define MB_STATE_L_3_LEAK        15
#define MB_STATE_L_3_DRY         16
#define MB_STATE_L_4_LEAK        17
#define MB_STATE_ADD_PRES        18
#define MB_STATE_H_PUMP          19
#define MB_STATE_H_BAL           20
#define MB_STATE_H_1_LEAK        21
#define MB_STATE_H_1_DRY         22
#define MB_STATE_H_2_LEAK        23
#define MB_STATE_H_2_DRY         24
#define MB_STATE_H_3_LEAK        25
#define MB_STATE_H_3_DRY         26
#define MB_STATE_H_4_LEAK        27
#define MB_STATE_WATER_OUT       28
#define MB_STATE_WATER_PUMP      29
#define MB_STATE_CYL_UP          30
#define MB_STATE_DEBUG           31
#define MB_STATE_DRY_CYL_DOWN    32
#define MB_STATE_DRY_CYL_UP      33
#define MB_STATE_DRY_CYL_PURGE   34
#define MB_STATE_DRY_CYL_PUMP    35
/*传感器状态*/
#define MB_SENS_LIQ              0x0001
#define MB_SENS_GRAT             0x0002
#define MB_SENS_VACU             0x0004
#define MB_SENS_DRY              0x0008
#define MB_SENS_CYL_H            0x0010
#define MB_SENS_CYL_L            0x0020
/*---------------------------------------------------------------------------------------*/
typedef struct _ModbusDev ModbusDev;
struct _ModbusDev
{
	void (* init)(void);  //初始化
	void (* devRun_thread)(void);  //modbus消息处理
	u8 * m_pRecvBuff;      //保存接收的数据
	u8 * m_pSendBuff;      //发送数据缓冲区
};
ModbusDev * modbusCreate(void);


#endif