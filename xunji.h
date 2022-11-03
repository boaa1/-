#ifndef _xunji_H
#define _xunji_H
#include "stdio.h"

#define Height MT9V03X_H
#define Width  MT9V03X_W
#define Forward 40


void binodicization(void);   //大津法图像二值化
void imageProcessing(void);  //图像处理
void FindMidLine(void);      //查找中线
void FindDeviation(void);    //查找偏差
void getPID(void);           //找pid
void findRoad(void);         //寻迹



#endif // !_XUNJI_H


