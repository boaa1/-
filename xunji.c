#include "xunji.h"
#include "zf_common_headfile.h"
#include "zf_device_mt9v03x.h"
#include "stdio.h"
#include "string.h"
/**
  ******************************************************************************
  * @file    xunji.c
  * @author  
  * @version V1.0
  * @date    2022-10-30
  * @brief   总钻风摄像头寻迹代码
	* @note    调整图像的数组大小后要改求偏差的函数
  ******************************************************************************
  */ 

extern uint8 mt9v03x_image[MT9V03X_H][MT9V03X_W];

uint8 midLine[Height];            //存储中线的数组 

uint8 image[Height][Width];       //原始灰度图像
uint8 Pixle[Height][Width];       //二值化后的图像
uint8 heiBaiImage[Height][Width]; //二值化后的黑白图像 在main函数里显示的也是这个图像

uint32 Histogram[256];            //直方图
uint8 threshold;                  //阈值

int err,errSum,preErr;           //此次的偏差  偏差的积分  上一次的偏差


void findRoad(void){
	 binodicization();   //大津法图像二值化
	 imageProcessing();  //图像处理
	 FindMidLine();      //查找中线
	 FindDeviation();    //查找偏差
	 getPID();           //找pid
}


/**
  * @brief  大津法二值化图像
  * @param  void
  * @note   无
  * @retval None
  */
	void binodicization(void ){
		memcpy(image,mt9v03x_image,Height*Width);
		//Histogram={0};
		for(uint16 i=0;i<256;i++){
			Histogram[i]=0;
		}
		//将60*80个点的灰度值存入Histogram数组
		for(int i=0;i<Height;i++){
			for(int j=0;j<Width;j++){
				Histogram[image[i][j]]++;
			}
		}
		uint8_t D1,D2,D3;     //D1 最高点位置 D2 次高点位置 D3最低点位置
		uint32 H1=0,H2,H3=20000;
		H2=0;		                   //H1 最高点的值 
		//寻找直方图最高点
		for(int i=0;i<256;i++){
			if(Histogram[i]>H1){
					H1=Histogram[i];
					D1=i;
			}
		}
		//寻找次高点
		uint8_t OK=0;//查找成功标志位
		for(int i=H1-5;i>0;i-=2){
			for(int j=0;j<=255;j++){
				if(Histogram[j]>=i&&abs(j-D1)>30){
					H2=Histogram[j];
					D2=j;
					OK=1;
					break;
				}
			}
			if(OK) break;
		}
		//查找最低点
		if(D1>D2){
			//D1>D2从右往左查找最低点
			for(int i=D1;i>=D2;i--){
				if(Histogram[i]<H3){
					H3=Histogram[i];
					D3=i;
				}
			}
		}else{
			//D1<D2从左往右查找最低点
			for(int i=D2;i>=D1;i--){
				if(Histogram[i]<H3){
					H3=Histogram[i];
					D3=i;
				}
			}
		}
		//设定阈值
		threshold=D3;
		//二值化
		for(int i=0;i<Height;i++){
			for(int j=0;j<Width;j++){
				Pixle[i][j]=(image[i][j]>threshold) ? 1 : 0;
			}
		}
		//将 Pixle二值化后的0 1图像转换为 0 255的图像
//		for(int i=0;i<Height;i++){
//			for(int j=0;j<Width;j++){
//				heiBaiImage[i][j]=(Pixle[i][j]==1) ? 255 : 0;
//			}
//		}
		
	}
	/**
  * @brief  图像处理
  * @param  void
  * @note   查找A B C D 点，，判断十字路口 ，补线
	*          A ---  Width-1 行左侧黑点      B --- Width-1 行右侧黑点
	*          C --- 十字路口左下角黑点       C --- 十字路口右下角黑点
  * @retval None
  */
	
	void imageProcessing(void){
		uint8_t Ax,Ay,Bx,By,Cx,Cy,Dx,Dy;
		/**  查找Ax Ay   **/
		uint8 midPoint=Width/2,rightPx=Width-1;
		int leftPx=0;
		//寻找左边界黑点 ---> A
		for(int i=midPoint;i>0;i--){
			if(Pixle[i]-Pixle[i-1]==1){
				leftPx=i-1;
				break;
			}
		} 
		//寻找右边界黑点 ---> B
		for(int i=midPoint;i<Width-1;i++){
			if(Pixle[i]-Pixle[i+1]==1){
				rightPx=i+1;
				break;
			}
		}
		
		Ax=leftPx;
		Ay=Height-1;
		Bx=rightPx;
		By=Height-1;
		/****     查找 C D 点           ****/
		//uint8_t leftPx=Ax,rightPx=Bx;
		for(int i=Height-2;i>0;i--){
			for(int j=leftPx;j<Width-1;j++){
					if(Pixle[i][j+1]==1){
						leftPx=j-1;
						if(leftPx<0) leftPx=0;
						break;
					}
			}
			if(Pixle[i-1][leftPx]==1){
				Cx=leftPx;
				Cy=i;
				break;
			}
		}
		//D 点
		for(int i=Height-2;i>0;i--){
			for(int j=rightPx;j>0;j--){
					if(Pixle[i][j-1]==1){
						rightPx=j+1;
						if(rightPx>Width-1) rightPx=Width-1;
						break;
					}
			}
			if(Pixle[i-1][rightPx]==1){
				Dx=rightPx;
				Dy=i;
				break;
			}
		}
		
	//判断十字路口
		/***
		*    确定 C D 两点最高的点的纵坐标
		*    计算纵坐标往上10行黑点的个数
		*    取第 0 2 4 6 8 行
		*    每行取 0 5 10 ... Width 行
		*		 80/5=16 *10=160
		*/
		if(abs(Cy-Dy)<20&&abs(Cx-Dx)>=40){
			uint8 minY;             //C D 两点最高的一点
			minY=(Cy<Dy) ? Cy:Dy; 
			uint8 blackCount=0;     //计算最高的一行~10行黑点的个数   
			for(uint8 i=minY;i>minY-20;i-=2){
				for(uint8 j=0;j<Width;j+=5){
					if(Pixle[i][j]==0)
						blackCount++;
				}
			}
		//黑点的个数小于 10% 认为前方是十字路口
			/*** 补线
			* 取A、B两点的横纵坐标计算左侧斜率
			* 取C、D两点的横纵坐标计算右侧斜率
			* leftK=(Cx-Ax)/(Cy-Ay)
			* rightK=(Dx-Bx)/(Dy-By)
			*/ 
			/*
			60 20
			40 40
			(60-40)/(20-40)=-1
			*/
			float leftK=-1,rightK=1;     //定义左右两侧斜率
			if(blackCount<=(20*Width/5/10)){
				leftK=(float)(Cx-Ax)/(float)(Cy-Ay);
				rightK=(float)(Dx-Bx)/(float)(Dy-By);			
				//从 C D 两个 i 较大的点开始补线，补到第 minY-20行 并且 i-=1后>=0
				for(uint8 i=Cx+Dx-minY;i>=minY-40&&i>0;i--){
					static int leftX,rightX;
					if(i<=Cx){
						leftX=(uint8)(leftK*(i-Cy)+0.5)+Cx;
						if(leftX>Width-1) leftX=leftX-1;
						Pixle[i][leftX]=0;
					}
					if(i<=Dx){
						rightX=(uint8)(rightK*(i-Dy)+0.5)+Dx;
						if(rightX<0) rightX=0;
						Pixle[i][rightX]=0;
					}
				}
			}
		}
		//将 Pixle二值化后的0 1图像转换为 0 255的图像
		for(int i=0;i<Height;i++){
			for(int j=0;j<Width;j++){
				heiBaiImage[i][j]=(Pixle[i][j]==1) ? 255 : 0;
			}
		}
	}
	

	
	
/**
  * @brief  求中心线
  * @param  void
  * @note   无
  * @retval None
  */
	void FindMidLine(void){
		uint8_t midPoint=Width/2;//初始化最后一行中点
		uint8_t leftPoint=0,rightPoint=Width-1;
		for(int i=Height-1;i>=0;i--){
			leftPoint=0;
			rightPoint=Width-1;
			//中点往左查找leftPoint			
			for(int j=midPoint;j>0;j--){
				if(Pixle[i][j]-Pixle[i][j-1]==1){
					leftPoint=j-1;
					break;
				}
			}
			//中点往右查找rightPoint
			for(int j=midPoint;j<Width-1;j++){
				if(Pixle[i][j]-Pixle[i][j+1]==1){
					rightPoint=j+1;
					break;
				}
			}
			midPoint=(leftPoint+rightPoint)/2;
			midLine[i]=midPoint;
			//考虑左边界和右边界极小的情况  即前方是弯道  这样 一行白色赛道区域占比变小  小于阈值
			//从这一行往上的所有行的中线就按照这一行的赋值
			//最后 break 跳出循环
			//白色的个数是leftPoint-rightPoint  阈值是Width/8 
			if(abs(leftPoint-rightPoint)<=Width/8){
				for(uint8 j=i-1;j>=0;j--){
					midLine[j]=midPoint;
				}
				break;
			}
		}
		//将中线显示在图像 heibaiImage 上
		//中线及中线左右  色素 ---> 0
		for(uint8 i=0;i<Height;i++){
			heiBaiImage[i][midLine[i]]=0;
			//heiBaiImage[i][midLine[i]-1]=0;
			//heiBaiImage[i][midLine[i]+1]=0;
		}
	}
	
	/**
  * @brief  求车身偏差   Deviation
  * @param  void
  * @note   车身偏差有横向偏差  Lateral deviation  和 前瞻偏差 Forward deviation
  * @retval None
  */
	void FindDeviation(void){
		/****   F1为前Forward行的中线位置   F2为前2*Forward行中线位置   **************/
//		uint8_t F0=midLine[Height-1],F1=midLine[Height-Forward-1],F2=midLine[Height-2*Forward-1];
//		uint8_t lateralDeviation=midLine[Height-1]-(Width-1)/2;  //车身横向偏差
//		uint8_t forwardDeviation=0;                              //前瞻偏差
//		/***
//		* F0 - 近
//		* F1 - 中
//		* F2 - 远
//		*
//		** 四种情况
//		*    1. 远>中>近       右
//		*    2. 远<中<近       左
//		*    3. 远>中，中<近   先左后右
//		*    4. 远<中，中>近   先右后左
//		*
//		*/
//		
//		if(F2>=F1&&F1>=F0){
//			forwardDeviation=(F2-F0)/2;
//		}else if(F2<=F1&&F1<=F0){
//			forwardDeviation=(F2-F0)/2;
//		}else if(F2>=F1&&F1<=F0){
//			forwardDeviation=F1-F0;
//		}else{
//			forwardDeviation=F1-F0;
//		}
			/***      加权求偏差      两端低，中间高         */
			/**
			*     高度128  47~127 行 每10行取一个中线数据   一共9个点
			*     宽度160  标准位置为 Width/2=80
			*     第 47 57 67 77 87 97 107 117 127行 的权重为
			*         1  4  5  6  5  3  2   2   2=30
			*			定义一个 deviation数组存储每个数据的 偏差*权重
			*			最后 求和/30=err
			*			
			*			err<0 往左
			*			err>0 往右
			*/  
			
			uint8 base=Width/2;
			int deviation[9]={0};
			uint8 weight[9]={6,6,6,6,6,3,3,2,2};
			for(uint8 i=47;i<=127;i+=10){
				deviation[(i-47)/10]=(midLine[i]-base)*weight[(i-47)/10];
			}
			preErr=err;
			err=0;
			for(uint8 i=0;i<9;i++){
				err+=deviation[i];
			}
			err/=40;   
			errSum+=err;
	}
	
	/**
  * @brief  获取P I D
  * @param  void
  * @note   
  * @retval None
  */
	void getPID(void){
		int kP,kI,kD;
		kP=17;
		kI=0;
		kD=5;
		uint32 basePWM,PWM1,PWM2;
		basePWM=6500;
		PWM1=(kP*err+kI*errSum+kD*(err-preErr))+basePWM;
		PWM2=-(kP*err+kI*errSum+kD*(err-preErr))+basePWM;
		if(PWM1<1000) PWM1=1000;
		if(PWM1>9000) PWM1=9000;
		if(PWM2<1000) PWM2=1000;
		if(PWM2>9000) PWM2=9000;
		pwm_set_duty(TIM5_PWM_CH1_A0,PWM1);	
		pwm_set_duty(TIM5_PWM_CH2_A1,PWM2);
		
	}
	
	
	
