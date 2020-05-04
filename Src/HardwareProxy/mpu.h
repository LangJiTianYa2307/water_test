/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors  : PanHan
 * @LastEditTime : 2020-01-14 13:58:54
 * @FilePath: \water_keil\Src\HardwareProxy\mpu.h
 */
#ifndef __MPU_H
#define __MPU_H
#include "sys.h"


#ifdef __cplusplus
       extern "C" {
#endif

u8 MPU_Set_Protection(u32 baseaddr,u32 size,u32 rnum,u32 ap,u8 sen,u8 cen,u8 ben);
void MPU_Memory_Protection(void);
         
#ifdef __cplusplus
        }
#endif
        
#endif
