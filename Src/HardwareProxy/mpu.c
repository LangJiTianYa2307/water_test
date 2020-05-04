/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors: PanHan
 * @LastEditTime: 2020-01-14 13:58:59
 * @FilePath: \water_keil\Src\HardwareProxy\mpu.c
 */
#include "mpu.h"
#include "led.h"
#include "delay.h"


//设置某个区域的MPU保护
//baseaddr:MPU保护区域的基址(首地址)
//size:MPU保护区域的大小(必须是32的倍数,单位为字节),可设置的值参考:CORTEX_MPU_Region_Size
//rnum:MPU保护区编号,范围:0~7,最大支持8个保护区域,可设置的值参考：CORTEX_MPU_Region_Number
//ap:访问权限,访问关系如下:可设置的值参考：CORTEX_MPU_Region_Permission_Attributes
//0,无访问（特权&用户都不可访问）
//1,仅支持特权读写访问
//2,禁止用户写访问（特权可读写访问）
//3,全访问（特权&用户都可访问）
//4,无法预测(禁止设置为4!!!)
//5,仅支持特权读访问
//6,只读（特权&用户都不可以写）
//详见:STM32F7 Series Cortex-M7 processor programming manual.pdf,4.6节,Table 89.
//sen:是否允许共用;0,不允许;1,允许
//cen:是否允许catch;0,不允许;1,允许
//返回值;0,成功.
//    其他,错误.
u8 MPU_Set_Protection(u32 baseaddr,u32 size,u32 rnum,u32 ap,u8 sen,u8 cen,u8 ben)
{
	MPU_Region_InitTypeDef MPU_Initure;
	HAL_MPU_Disable();								        //配置MPU之前先关闭MPU,配置完成以后在使能MPU

	MPU_Initure.Enable=MPU_REGION_ENABLE;			        //使能该保护区域 
	MPU_Initure.Number=rnum;			                    //设置保护区域
	MPU_Initure.BaseAddress=baseaddr;	                    //设置基址
	MPU_Initure.Size=size;				                    //设置保护区域大小
	MPU_Initure.SubRegionDisable=0X00;                      //禁止子区域
	MPU_Initure.TypeExtField=MPU_TEX_LEVEL0;                //设置类型扩展域为level0
	MPU_Initure.AccessPermission=(u8)ap;		            //设置访问权限,
	MPU_Initure.DisableExec=MPU_INSTRUCTION_ACCESS_ENABLE;	//允许指令访问(允许读取指令)
	MPU_Initure.IsShareable=sen;                            //是否允许共用
	MPU_Initure.IsCacheable=cen;                            //是否允许cache
	MPU_Initure.IsBufferable=ben;                           //是否允许缓冲
	HAL_MPU_ConfigRegion(&MPU_Initure);                     //配置MPU
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);			        //开启MPU
    return 0;
}


//设置需要保护的存储块
//必须对部分存储区域进行MPU保护,否则可能导致程序运行异常
//比如MCU屏不显示,摄像头采集数据出错等等问题...
void MPU_Memory_Protection(void)
{
	MPU_Set_Protection(0x20000000,MPU_REGION_SIZE_512KB,MPU_REGION_NUMBER1,MPU_REGION_FULL_ACCESS,1,1,0);	//保护整个内部SRAM,包括SRAM1,SRAM2和DTCM,共512K字节
	MPU_Set_Protection(0XC0000000,MPU_REGION_SIZE_32MB,MPU_REGION_NUMBER2,MPU_REGION_FULL_ACCESS,1,1,1);	//保护SDRAM区域,共32M字节
}


//MemManage错误处理中断
//进入此中断以后,将无法恢复程序运行!!
void MemManage_Handler(void)
{ 
	LED1(0);							//点亮DS1
	printf("Mem Access Error!!\r\n"); 	//输出错误信息
	delay_ms(1000);	
	printf("Soft Reseting...\r\n");		//提示软件重启
	delay_ms(1000);	
	NVIC_SystemReset();					//软复位
}
