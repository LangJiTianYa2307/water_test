/*
 * @Descripttion: 通用功能函数
 * @version: 
 * @Author: PanHan
 * @Date: 2020-02-28 08:57:23
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-11 14:24:32
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\GeneralFunc.h
 */
#ifndef _GENERAL_FUNC_H
#define _GENERAL_FUNC_H


#include "TypeDef.h"

#ifdef __cplusplus
       extern "C" {
#endif

#define GF_FLOAT_DATA  0
#define GF_U32_DATA    1
#define GF_INT_DATA    2

#define fMeanFilter(arr, len, res)  meanFilter((void *)arr, len, GF_FLOAT_DATA, (void *)res)
#define u32MeanFilter(arr, len, res)  meanFilter((void *)arr, len, GF_U32_DATA, (void *)res)
#define intMeanFilter(arr, len, res)  meanFilter((void *)arr, len, GF_INT_DATA, (void *)res)


typedef union
{
  float f_data;
  u32 u32_data;
  int int_data;
}DataUnion;

void meanFilter(void * const arr, u32 len, u32 dataType, void * res);
void leastSquareLinearFit(const float * xArr, const float * yArr, const int len, float * a, float * b, float * mse);
u32 convFloat2u32(float data);
#ifdef __cplusplus
        }
#endif

#endif