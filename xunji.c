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
  * @brief   ���������ͷѰ������
	* @note    ����ͼ��������С��Ҫ����ƫ��ĺ���
  ******************************************************************************
  */ 

extern uint8 mt9v03x_image[MT9V03X_H][MT9V03X_W];

uint8 midLine[Height];            //�洢���ߵ����� 

uint8 image[Height][Width];       //ԭʼ�Ҷ�ͼ��
uint8 Pixle[Height][Width];       //��ֵ�����ͼ��
uint8 heiBaiImage[Height][Width]; //��ֵ����ĺڰ�ͼ�� ��main��������ʾ��Ҳ�����ͼ��

uint32 Histogram[256];            //ֱ��ͼ
uint8 threshold;                  //��ֵ

int err,errSum,preErr;           //�˴ε�ƫ��  ƫ��Ļ���  ��һ�ε�ƫ��


void findRoad(void){
	 binodicization();   //���ͼ���ֵ��
	 imageProcessing();  //ͼ����
	 FindMidLine();      //��������
	 FindDeviation();    //����ƫ��
	 getPID();           //��pid
}


/**
  * @brief  ��򷨶�ֵ��ͼ��
  * @param  void
  * @note   ��
  * @retval None
  */
	void binodicization(void ){
		memcpy(image,mt9v03x_image,Height*Width);
		//Histogram={0};
		for(uint16 i=0;i<256;i++){
			Histogram[i]=0;
		}
		//��60*80����ĻҶ�ֵ����Histogram����
		for(int i=0;i<Height;i++){
			for(int j=0;j<Width;j++){
				Histogram[image[i][j]]++;
			}
		}
		uint8_t D1,D2,D3;     //D1 ��ߵ�λ�� D2 �θߵ�λ�� D3��͵�λ��
		uint32 H1=0,H2,H3=20000;
		H2=0;		                   //H1 ��ߵ��ֵ 
		//Ѱ��ֱ��ͼ��ߵ�
		for(int i=0;i<256;i++){
			if(Histogram[i]>H1){
					H1=Histogram[i];
					D1=i;
			}
		}
		//Ѱ�Ҵθߵ�
		uint8_t OK=0;//���ҳɹ���־λ
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
		//������͵�
		if(D1>D2){
			//D1>D2�������������͵�
			for(int i=D1;i>=D2;i--){
				if(Histogram[i]<H3){
					H3=Histogram[i];
					D3=i;
				}
			}
		}else{
			//D1<D2�������Ҳ�����͵�
			for(int i=D2;i>=D1;i--){
				if(Histogram[i]<H3){
					H3=Histogram[i];
					D3=i;
				}
			}
		}
		//�趨��ֵ
		threshold=D3;
		//��ֵ��
		for(int i=0;i<Height;i++){
			for(int j=0;j<Width;j++){
				Pixle[i][j]=(image[i][j]>threshold) ? 1 : 0;
			}
		}
		//�� Pixle��ֵ�����0 1ͼ��ת��Ϊ 0 255��ͼ��
//		for(int i=0;i<Height;i++){
//			for(int j=0;j<Width;j++){
//				heiBaiImage[i][j]=(Pixle[i][j]==1) ? 255 : 0;
//			}
//		}
		
	}
	/**
  * @brief  ͼ����
  * @param  void
  * @note   ����A B C D �㣬���ж�ʮ��·�� ������
	*          A ---  Width-1 �����ڵ�      B --- Width-1 ���Ҳ�ڵ�
	*          C --- ʮ��·�����½Ǻڵ�       C --- ʮ��·�����½Ǻڵ�
  * @retval None
  */
	
	void imageProcessing(void){
		uint8_t Ax,Ay,Bx,By,Cx,Cy,Dx,Dy;
		/**  ����Ax Ay   **/
		uint8 midPoint=Width/2,rightPx=Width-1;
		int leftPx=0;
		//Ѱ����߽�ڵ� ---> A
		for(int i=midPoint;i>0;i--){
			if(Pixle[i]-Pixle[i-1]==1){
				leftPx=i-1;
				break;
			}
		} 
		//Ѱ���ұ߽�ڵ� ---> B
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
		/****     ���� C D ��           ****/
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
		//D ��
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
		
	//�ж�ʮ��·��
		/***
		*    ȷ�� C D ������ߵĵ��������
		*    ��������������10�кڵ�ĸ���
		*    ȡ�� 0 2 4 6 8 ��
		*    ÿ��ȡ 0 5 10 ... Width ��
		*		 80/5=16 *10=160
		*/
		if(abs(Cy-Dy)<20&&abs(Cx-Dx)>=40){
			uint8 minY;             //C D ������ߵ�һ��
			minY=(Cy<Dy) ? Cy:Dy; 
			uint8 blackCount=0;     //������ߵ�һ��~10�кڵ�ĸ���   
			for(uint8 i=minY;i>minY-20;i-=2){
				for(uint8 j=0;j<Width;j+=5){
					if(Pixle[i][j]==0)
						blackCount++;
				}
			}
		//�ڵ�ĸ���С�� 10% ��Ϊǰ����ʮ��·��
			/*** ����
			* ȡA��B����ĺ�������������б��
			* ȡC��D����ĺ�����������Ҳ�б��
			* leftK=(Cx-Ax)/(Cy-Ay)
			* rightK=(Dx-Bx)/(Dy-By)
			*/ 
			/*
			60 20
			40 40
			(60-40)/(20-40)=-1
			*/
			float leftK=-1,rightK=1;     //������������б��
			if(blackCount<=(20*Width/5/10)){
				leftK=(float)(Cx-Ax)/(float)(Cy-Ay);
				rightK=(float)(Dx-Bx)/(float)(Dy-By);			
				//�� C D ���� i �ϴ�ĵ㿪ʼ���ߣ������� minY-20�� ���� i-=1��>=0
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
		//�� Pixle��ֵ�����0 1ͼ��ת��Ϊ 0 255��ͼ��
		for(int i=0;i<Height;i++){
			for(int j=0;j<Width;j++){
				heiBaiImage[i][j]=(Pixle[i][j]==1) ? 255 : 0;
			}
		}
	}
	

	
	
/**
  * @brief  ��������
  * @param  void
  * @note   ��
  * @retval None
  */
	void FindMidLine(void){
		uint8_t midPoint=Width/2;//��ʼ�����һ���е�
		uint8_t leftPoint=0,rightPoint=Width-1;
		for(int i=Height-1;i>=0;i--){
			leftPoint=0;
			rightPoint=Width-1;
			//�е��������leftPoint			
			for(int j=midPoint;j>0;j--){
				if(Pixle[i][j]-Pixle[i][j-1]==1){
					leftPoint=j-1;
					break;
				}
			}
			//�е����Ҳ���rightPoint
			for(int j=midPoint;j<Width-1;j++){
				if(Pixle[i][j]-Pixle[i][j+1]==1){
					rightPoint=j+1;
					break;
				}
			}
			midPoint=(leftPoint+rightPoint)/2;
			midLine[i]=midPoint;
			//������߽���ұ߽缫С�����  ��ǰ�������  ���� һ�а�ɫ��������ռ�ȱ�С  С����ֵ
			//����һ�����ϵ������е����߾Ͱ�����һ�еĸ�ֵ
			//��� break ����ѭ��
			//��ɫ�ĸ�����leftPoint-rightPoint  ��ֵ��Width/8 
			if(abs(leftPoint-rightPoint)<=Width/8){
				for(uint8 j=i-1;j>=0;j--){
					midLine[j]=midPoint;
				}
				break;
			}
		}
		//��������ʾ��ͼ�� heibaiImage ��
		//���߼���������  ɫ�� ---> 0
		for(uint8 i=0;i<Height;i++){
			heiBaiImage[i][midLine[i]]=0;
			//heiBaiImage[i][midLine[i]-1]=0;
			//heiBaiImage[i][midLine[i]+1]=0;
		}
	}
	
	/**
  * @brief  ����ƫ��   Deviation
  * @param  void
  * @note   ����ƫ���к���ƫ��  Lateral deviation  �� ǰհƫ�� Forward deviation
  * @retval None
  */
	void FindDeviation(void){
		/****   F1ΪǰForward�е�����λ��   F2Ϊǰ2*Forward������λ��   **************/
//		uint8_t F0=midLine[Height-1],F1=midLine[Height-Forward-1],F2=midLine[Height-2*Forward-1];
//		uint8_t lateralDeviation=midLine[Height-1]-(Width-1)/2;  //�������ƫ��
//		uint8_t forwardDeviation=0;                              //ǰհƫ��
//		/***
//		* F0 - ��
//		* F1 - ��
//		* F2 - Զ
//		*
//		** �������
//		*    1. Զ>��>��       ��
//		*    2. Զ<��<��       ��
//		*    3. Զ>�У���<��   �������
//		*    4. Զ<�У���>��   ���Һ���
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
			/***      ��Ȩ��ƫ��      ���˵ͣ��м��         */
			/**
			*     �߶�128  47~127 �� ÿ10��ȡһ����������   һ��9����
			*     ���160  ��׼λ��Ϊ Width/2=80
			*     �� 47 57 67 77 87 97 107 117 127�� ��Ȩ��Ϊ
			*         1  4  5  6  5  3  2   2   2=30
			*			����һ�� deviation����洢ÿ�����ݵ� ƫ��*Ȩ��
			*			��� ���/30=err
			*			
			*			err<0 ����
			*			err>0 ����
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
  * @brief  ��ȡP I D
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
	
	
	
