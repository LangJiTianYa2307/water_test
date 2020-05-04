/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-05 22:42:15
 * @LastEditors: PanHan
 * @LastEditTime: 2020-03-15 16:36:51
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\FML_ExtProporV.c
 */
#include "FML_ExtProporV.h"
#include "my_timer.h"

static void Propor_init(ProporValDev * const me);  //初始化
static void Propor_setOutputPres(ProporValDev * const me, float pres);//设置输出压力
static void Propor_setMaxPres(float pres);  //设置气源压力
static float Propor_getMaxPres(void);
static void registerCallback(ProporValDev * me, void * pInstance, const pGeneralCb p, ProporValCbType type); //注册回调函数
/*--------私有成员-------*/
float m_maxPres = 0;  //气源最大气压
ProporValDev proporValDev;//静态分配

static NotifyHandle s_notifyHandle[CB_PRO_MAX];

ProporValDev * proporValCreate(void)
{
	static ProporValDev * me = 0;
  int i = 0;
	if (me == 0){
		me = & proporValDev;
		me->init = Propor_init;
		me->setOutputPres = Propor_setOutputPres;
    me->registerCallback = registerCallback;
		me->m_outPres = 0;
		me->m_voltage = 0;
		me->m_openPercent = 0;
		me->getMaxPres = Propor_getMaxPres;
		me->setMaxPres = Propor_setMaxPres;
    
    for (i = 0; i < CB_PRO_MAX; i ++){
      me->m_cbHandle[i] = &s_notifyHandle[i];
    }
		m_maxPres = 700;  //
		me->init(me);
	}
	return me;
}

static void Propor_init(ProporValDev * const me)
{
	/*计数频率108 / 108 = 1 MHz，自动重装值为 500，
	  所以PWM频率为2 kHz
	*/
	TIM3_PWM_Init(1000 - 1, 54 - 1);
}

/**
 * @brief	设置阀门开度
 * @param	opening:阀门开度（0~1）
 * @retval	
 */
static void Propor_setOutputPres(ProporValDev * const me, float opening)
{
	int out = 0;
	out = 1000 - opening * 1000.0;
	if (out > 1000) out = 1000;
	if (out < 0) out = 0;
	TIM_SetTIM3Compare4(out);
	me->m_outPres = opening;
	me->m_openPercent = opening * 100;

  //调用回调函数更新当前阀门开度
  if (me->m_cbHandle[CB_PRO_DISP]->callback){
    me->m_cbHandle[CB_PRO_DISP]->callback(me->m_cbHandle[CB_PRO_DISP]->pInstance, & me->m_openPercent);
  }
}

/*
*函数功能：设置气源压力
*输入参数：pres:气源压力
*返 回 值：无
*/
static void Propor_setMaxPres(float pres)
{
	m_maxPres = pres;
}

static float Propor_getMaxPres(void)
{
	return m_maxPres;
}

/**
 * @brief	注册回调函数
 * @param	pInstance:注册使用的对象
 * @param p:指向需要调用的函数
 * @param type:回调函数类型
 * @retval	
 */
static void registerCallback(ProporValDev * me, void * pInstance, const pGeneralCb p, ProporValCbType type)
{
  me->m_cbHandle[type]->callback = p;
  me->m_cbHandle[type]->pInstance = pInstance;
}