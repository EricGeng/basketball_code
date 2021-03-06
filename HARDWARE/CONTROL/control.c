#include "control.h"
#include "MPU6050.h"
#include "remote.h"
#include "beep.h"


struct robot robot_zqd;
u32 uart_data[3]={0,2000,1000};			//串口接收数据存储，x位置，深度，半径
u32 uart3_data[3]={0,2000,1000};		//串口接收数据存储，x位置，深度，半径
void control_init(void)
{
	robot_zqd.X = 0;		//机器人在坐标系中x坐标
	robot_zqd.Y = 0;		//机器人在坐标系中y坐标
	robot_zqd.theta = 0;	//机器人正方向和y轴夹角
	robot_zqd.Vx = 0;		//机器人在坐标系x方向速度
	robot_zqd.Vy = 0;		//机器人在坐标系y方向速度
	robot_zqd.W = 0;		//机器人角速度，顺时针正方向
	robot_zqd.w[1] = 0;		//第一个轮的角速度
	robot_zqd.w[2] = 0;		//第二个轮的角速度
	robot_zqd.w[0] = 0;		//第三个轮的角速度
	robot_zqd.v[1] = 0;		//第一个轮的速度
	robot_zqd.v[2] = 0;		//第二个轮的速度
	robot_zqd.v[0] = 0;		//第三个轮的速度
	robot_zqd.theta_dev = 0;
	robot_zqd.theta_offset = 0;
	
	control1_W(0);
	control2_W(0);
	control3_W(0);
	
	charge(0);
	
	MPU_Init();			//MPU6050初始化
	MPU_Init();
	MPU_Init();
	
	TIM_SetCompare2(TIM9,MOTOR_STATIC_2); 		//PE6
	TIM_SetCompare1(TIM9,MOTOR_STATIC_1);			//PE5
	
	LCD_Show_lcj();
	LCD_Show_v();
	LCD_Show_V();
	LCD_Show_position();
}

//电机1 转速W
void control1_W(float W)
{
	W=1000 - W;//W+=1000;
	if(W>=1900)
		W=1900;
	if(W<=100)
		W=100;
 	TIM_SetCompare1(TIM3,(uint32_t)W);			//PC6

}

//电机2 转速W
void control2_W(float W)
{
	W=1000 - W;//W+=1000;
	if(W>=1900)
		W=1900;
	if(W<=100)
		W=100;
 	TIM_SetCompare2(TIM3,(uint32_t)W);			//PC7

}

//电机3 转速W
void control3_W(float W)
{
	W=1000 - W;//W+=1000;
	if(W>=1900)
		W=1900;
	if(W<=100)
		W=100;
 	TIM_SetCompare3(TIM3,(uint32_t)W);			//PC8

}


//弹射初始化,GPIOG7
void shot_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);//使能GPIOE时钟
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; //
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOG, &GPIO_InitStructure);//初始化

	GPIO_ResetBits(GPIOG,GPIO_Pin_7);
}

//限位开关初始化
void xianwei_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//使能GPIOE时钟
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1; //
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//普通输入模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化GPIOE0

}

//红外开关初始化
void hongwai_init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);//使能GPIOE时钟
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//普通输入模式
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOF, &GPIO_InitStructure);//初始化GPIOF9

}


//获取红外状态
void get_hongwai(void)
{
	while(1){
		if(!GPIO_ReadInputDataBit(GPIOF,GPIO_Pin_9))
		{
			break;
		}
	}
}

//获取红外状态
void get_hongwai_dixian(float dis)
{
	while(1){
		if(!GPIO_ReadInputDataBit(GPIOF,GPIO_Pin_9))
		{
			break;
		}
		if(robot_zqd.X < dis)
		{
			break;
		}
	}
}


//坐标转换
void get_position(void)
{
	
	
	//根据速度运算球场坐标
	u8 i,j,k;
	float theta;
	float L_inv[3][3];
	float theta_inv[3][3];
	float tem[3][3];
	
	//取Theta采样中值
	if(fabs(robot_zqd.theta - robot_zqd.theta_dev) < PI)
		theta = (robot_zqd.theta + robot_zqd.theta_dev) / 2.0f;
	else
		theta = (robot_zqd.theta + robot_zqd.theta_dev) / 2.0f + PI;
	robot_zqd.theta_dev = robot_zqd.theta;
	
	//v(bmq)=L * Theta * V
	//theta_inv
	theta_inv[0][0]= cos(theta);	theta_inv[0][1] = -sin(theta);		theta_inv[0][2] = 0;
	theta_inv[1][0]= sin(theta);	theta_inv[1][1] = cos(theta);		theta_inv[1][2] = 0;
	theta_inv[2][0]= 0;				theta_inv[2][1] = 0;				theta_inv[2][2] = 1;
	//		[-cos(0)	-sin(0)		BMQ_L]-1
	//L_inv=[-cos(120) 	-sin(120)	BMQ_L]
	//		[-cos(-120)	-sin(-120)	BMQ_L]
	L_inv[0][0] = -0.666666666666667;		L_inv[0][1] =  0.333333333333333;		L_inv[0][2] = 0.333333333333333;
	L_inv[1][0] =  0;						L_inv[1][1] = -0.577350269189626;		L_inv[1][2] = 0.577350269189626;
	L_inv[2][0] =  1.666666666666667;		L_inv[2][1] =  1.666666666666667;		L_inv[2][2] = 1.666666666666667;
	
	for(i=0;i<3;i++)
	{
		for(j=0;j<3;j++)
		{
			tem[i][j] = 0;
			for(k=0;k<3;k++)
			{
				tem[i][j] += theta_inv[i][k] * L_inv[k][j];
			}
		}
	}
	
	robot_zqd.Vx = 0;
	for(j=0;j<3;j++){
		robot_zqd.Vx += tem[0][j] * robot_zqd.v[j];
	}
	robot_zqd.Vy = 0;
	for(j=0;j<3;j++){
		robot_zqd.Vy += tem[1][j] * robot_zqd.v[j];
	}
	robot_zqd.W = 0;
	for(j=0;j<3;j++){
		robot_zqd.W += tem[2][j] * robot_zqd.v[j];
	}
	
	robot_zqd.X += robot_zqd.Vx*0.01f;
	robot_zqd.Y += robot_zqd.Vy*0.01f;
	
	/*
	//根据里程计计算角度
	//用陀螺仪代替
	robot_zqd.theta += robot_zqd.W*0.01f;
	
	while(robot_zqd.theta<0)
	{
		robot_zqd.theta += 2*PI;
	}
	while(robot_zqd.theta > 2*PI)
	{
		robot_zqd.theta -= 2*PI;
	}
	*/
	
	
	
	
	/*
	//根据里程计累计值运算球场坐标
	u8 i,j,k;
	float L_inv[3][3];
	float theta_inv[3][3];
	float tem[3][3];
	//L*theta*V=R*w
	//theta_inv
	theta_inv[0][0]= 1;		theta_inv[0][1] = 0;		theta_inv[0][2] = 0;
	theta_inv[1][0]= 0;		theta_inv[1][1] = 1;		theta_inv[1][2] = 0;
	theta_inv[2][0]= 0;		theta_inv[2][1] = 0;		theta_inv[2][2] = 1;
	//L_inv
	
	L_inv[0][0] = -0.666666666666667;		L_inv[0][1] =  0.333333333333333;		L_inv[0][2] = 0.333333333333333;
	L_inv[1][0] =  0;						L_inv[1][1] = -0.577350269189626;		L_inv[1][2] = 0.577350269189626;
	L_inv[2][0] =  1.877934272300470;		L_inv[2][1] =  1.877934272300470;		L_inv[2][2] = 1.877934272300470;
	
	//L_inv[0][0] = -0.691823899371069;		L_inv[0][1] =  0.345911949685535;		L_inv[0][2] = 0.345911949685535;
	//L_inv[1][0] =  0;						L_inv[1][1] = -0.558469178636527;		L_inv[1][2] = 0.558469178636527;
	//L_inv[2][0] =  1.736203383824963;		L_inv[2][1] =  1.948799716538223;		L_inv[2][2] = 1.948799716538223;
	
	for(i=0;i<3;i++)
	{
		for(j=0;j<3;j++)
		{
			tem[i][j] = 0;
			for(k=0;k<3;k++)
			{
				tem[i][j] += theta_inv[i][k] * L_inv[k][j];
			}
		}
	}
	
	robot_zqd.X = 0;
	for(j=0;j<3;j++){
		robot_zqd.X += tem[0][j] * robot_zqd.w[j]*0.0001727875959474386f;
	}
	robot_zqd.Y = 0;
	for(j=0;j<3;j++){
		robot_zqd.Y += tem[1][j] * robot_zqd.w[j]*0.0001727875959474386f;
	}
	robot_zqd.Y *= 1.123f;
	robot_zqd.theta = 0;
	for(j=0;j<3;j++){
		robot_zqd.theta += tem[2][j] * robot_zqd.w[j]*0.0001727875959474386f;
	}
	robot_zqd.theta *= 1.14f;
	
	while(robot_zqd.theta<0)
	{
		robot_zqd.theta += 2*PI;
	}
	while(robot_zqd.theta > 2*PI)
	{
		robot_zqd.theta -= 2*PI;
	}
	*/
	/*
	//合
	u8 i;
	float L_inv[3][3];
	
	//L_inv
	L_inv[0][0] = -0.666666666666667;		L_inv[0][1] =  0.333333333333333;		L_inv[0][2] = 0.333333333333333;
	L_inv[1][0] =  0;						L_inv[1][1] = -0.577350269189626;		L_inv[1][2] = 0.577350269189626;
	L_inv[2][0] =  1.877934272300470;		L_inv[2][1] =  1.877934272300470;		L_inv[2][2] = 1.877934272300470;

	robot_zqd.X = 0;
	for(i=0;i<3;i++){
		robot_zqd.X += L_inv[0][i] * robot_zqd.w[i]*0.0001727875959474386f;
	}
	robot_zqd.X *= 1.123f;
	robot_zqd.Y = 0;
	for(i=0;i<3;i++){
		robot_zqd.Y += L_inv[1][i] * robot_zqd.w[i]*0.0001727875959474386f;
	}

	robot_zqd.W = 0;
	for(i=0;i<3;i++){
		robot_zqd.W += L_inv[2][i] * robot_zqd.v[i];
	}
	robot_zqd.theta += robot_zqd.W*0.01f;
	
	while(robot_zqd.theta<0)
	{
		robot_zqd.theta += 2*PI;
	}
	while(robot_zqd.theta > 2*PI)
	{
		robot_zqd.theta -= 2*PI;
	}
	*/
	
}

//球场坐标速度转轮子的PWM
//vx：球场坐标的x轴速度
//vy：球场坐标的y轴速度
//w:机器人原地旋转的角速度
//注意最大速度！
void set_motor_vx_vy_w(float vx,float vy,float w)
{	
	u8 i,j,k;
	float L[3][3];
	float theta[3][3];
	float V[3][1];
	float tem[3][3];
					
	//v(PWM)=L*Theta*V
	//   cos(60)	sin(60)		-MOTOR_L
	//L= cos(180) 	sin(180)	-MOTOR_L
	//   cos(-60)	sin(-60)	-MOTOR_L
	L[0][0] =  0.5;					L[0][1] =  0.8660254037844386;		L[0][2] = -MOTOR_L;
	L[1][0] = -1;					L[1][1] =  0;						L[1][2] = -MOTOR_L;
	L[2][0] =  0.5;					L[2][1] = -0.8660254037844386;		L[2][2] = -MOTOR_L;
	//		 cos(theta)		sin(theta)	0
	//theta= -sin(theta)	cos(theta) 	0
	//		 0				0			1
	theta[0][0]= cos(robot_zqd.theta);	theta[0][1] = sin(robot_zqd.theta);	theta[0][2] = 0;
	theta[1][0]= -sin(robot_zqd.theta);	theta[1][1] = cos(robot_zqd.theta);	theta[1][2] = 0;
	theta[2][0]= 0;						theta[2][1] = 0;					theta[2][2] = 1;
	//V
	V[0][0] = -vx*10;
	V[1][0] = -vy;
	V[2][0] = -w;
	
	for(i=0;i<3;i++){
		for(j=0;j<3;j++){
			tem[i][j] = 0;
			for(k=0;k<3;k++){
				tem[i][j] += L[i][k] * theta[k][j];
			}
		}
	}
	
	for(i=0;i<3;i++){
		robot_zqd.pwm[i] = 0;
		for(j=0;j<3;j++){
			robot_zqd.pwm[i] += tem[i][j]*V[j][0];
		}
	}
	
	/*
	for(i = 0;i<3;i++)
	{
		if(robot_zqd.pwm[i] > 700 || robot_zqd.pwm[i] < -700)
		{
			robot_zqd.pwm[0] = robot_zqd.pwm[0] * 700 /robot_zqd.pwm[i];
			robot_zqd.pwm[1] = robot_zqd.pwm[1] * 700 /robot_zqd.pwm[i];
			robot_zqd.pwm[2] = robot_zqd.pwm[2] * 700 /robot_zqd.pwm[i];
		}
	}
	*/
	
	
	LCD_Show_pwm();
}


//球场坐标速度转轮子的PWM
//vx：球场坐标的x轴速度
//vy：球场坐标的y轴速度
//w:机器人原地旋转的角速度
//注意最大速度！
void set_motor_vx_vy_w_R(float vx,float vy,float w)
{	
	u8 i,j,k;
	float L[3][3];
	float theta[3][3];
	float V[3][1];
	float tem[3][3];
					
	//v(PWM)=L*Theta*V
	//L
	L[0][0] =  0.5;					L[0][1] =  8.660254037844386;		L[0][2] = -0.2013;
	L[1][0] = -1;					L[1][1] =  0;						L[1][2] = -0.2013;
	L[2][0] =  0.5;					L[2][1] = -8.660254037844386;		L[2][2] = -0.2013;
	//theta
	theta[0][0]= 1;						theta[0][1] = 0;					theta[0][2] = 0;
	theta[1][0]= 0;						theta[1][1] = 1;					theta[1][2] = 0;
	theta[2][0]= 0;						theta[2][1] = 0;					theta[2][2] = 1;
	//V
	V[0][0] = -vx;
	V[1][0] = -vy;
	V[2][0] = -w;
	
	for(i=0;i<3;i++){
		for(j=0;j<3;j++){
			tem[i][j] = 0;
			for(k=0;k<3;k++){
				tem[i][j] += L[i][k] * theta[k][j];
			}
		}
	}
	
	for(i=0;i<3;i++){
		robot_zqd.pwm[i] = 0;
		for(j=0;j<3;j++){
			robot_zqd.pwm[i] += tem[i][j]*V[j][0];
		}
	}
	
	
	LCD_Show_pwm();
}


//获取下降限位器状态
u8 xianwei_down(void)
{
	return GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_1);
}

//获取上升限位器状态
u8 xianwei_up(void)
{
	return GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_0);
}

//射球
int shot()
{
	if(xianwei_down()==1)
	{
		charge(0);
		delay_ms(10000);
		GPIO_SetBits(GPIOG,GPIO_Pin_7);
		LED1 = 1;
		delay_ms(10000);
		delay_ms(10000);
		delay_ms(10000);
		GPIO_ResetBits(GPIOG,GPIO_Pin_7);
		LED1 = 0;
		return 0;
	}
	else 
		return 1;
}

//机械臂下降
void jixiebi_down(void)
{
	//原来板子：3000
	//V1.0：1750
	u16 i,t;
	u16 W=3300;
	u16 nms=2000;
	
	if(xianwei_down()==1)
	{
		TIM_SetCompare2(TIM9,MOTOR_STATIC_2);
		TIM_SetCompare1(TIM9,MOTOR_STATIC_1);
		return;
	}
	
	//EXTIX_Enable(1);
	#ifdef ZQD_DEBUG
	BEEP = 1;
	#endif
	TIM_SetCompare2(TIM9,W);  			//PE6
	TIM_SetCompare1(TIM9,MOTOR_STATIC_1);			//PE5
	LED1 = 1;
	for(i=0;i<nms;i++)
	{	  
		if(xianwei_down()==1)
		{	
			for(t=0;t<0xff;t++);
			if(xianwei_down()==1)
			{
				TIM_SetCompare2(TIM9,MOTOR_STATIC_2);
				TIM_SetCompare1(TIM9,MOTOR_STATIC_1);
				break;
			}
		}
		for(t=0;t<0x4fff;t++)
			if(xianwei_down() == 1)
				break;
	}
	TIM_SetCompare2(TIM9,MOTOR_STATIC_2);
	TIM_SetCompare1(TIM9,MOTOR_STATIC_1);
	#ifdef ZQD_DEBUG
	BEEP = 0;
	#endif
}

//机械臂上升
void jixiebi_up(void)
{
	//原来板子：1960
	//V1.0:550
	u16 i,t;
	u16 W=3000;
	u16 nms=2000;
	
	if(xianwei_up()==1)
	{
		TIM_SetCompare2(TIM9,MOTOR_STATIC_2);
		TIM_SetCompare1(TIM9,MOTOR_STATIC_1);
		return;
	}
	
	//EXTIX_Enable(0);
	#ifdef ZQD_DEBUG
	BEEP = 1;
	#endif
	TIM_SetCompare1(TIM9,W);		//PE6
	TIM_SetCompare2(TIM9,MOTOR_STATIC_2);		//PE5
	for(i=0;i<nms;i++)
	{
		if(xianwei_up() == 1)
		{
			for(t=0;t<0xff;t++);
			if(xianwei_up() == 1)
			{
				TIM_SetCompare1(TIM9,MOTOR_STATIC_1);
				TIM_SetCompare2(TIM9,MOTOR_STATIC_2);
				break;
			}
		}
		for(t=0;t<0x4fff;t++)
			if(xianwei_up() == 1)
				break;
	}
	TIM_SetCompare1(TIM9,MOTOR_STATIC_1);
	TIM_SetCompare2(TIM9,MOTOR_STATIC_2);	
	#ifdef ZQD_DEBUG
	BEEP = 0;
	#endif
}

//机械臂下降、弹射、上升动作
int down_shot_up(void)
{
	
	jixiebi_down();
	delay_ms(30000);
	charge(0);
	delay_ms(30000);
	if(shot())
		return 1;
	delay_ms(45000);
	jixiebi_up();
	delay_ms(30000);
	return 0;
}

//机器人直走,带加速度
//Y_I:直走距离
//V：速度
//distance:误差距离
//a_start:开始加速度
//a_stop:停止加速度
void robot_straight_Y(float Y_I,float V,float distance,u32 a_start,u32 a_stop)
{
	u32 i;
	for(i=0;i<=(V/a_start);i++)
	{
		if(fabs(Y_I-robot_zqd.Y)<distance)
			break;
		if(Y_I-robot_zqd.Y>0)
		{
			robot_zqd.pwm[0] = -(float)i*a_start;
			robot_zqd.pwm[2] = (float)i*a_start;
			robot_zqd.pwm[1] = 0;
		}
		else if(Y_I-robot_zqd.Y<0)
		{
			robot_zqd.pwm[0] = (float)i*a_start;
			robot_zqd.pwm[2] = -(float)i*a_start;
			robot_zqd.pwm[1] = 0;
		}
		control1_W(robot_zqd.pwm[0]);
		control2_W(robot_zqd.pwm[1]);
		control3_W(robot_zqd.pwm[2]);
		delay_ms(1000);
	}
	LCD_Show_pwm();
	while(fabs(Y_I-robot_zqd.Y)>distance);
	for(;i>0;i--)
	{
		if(fabs(Y_I-robot_zqd.Y)<distance)
			break;
		if(Y_I-robot_zqd.Y>0)
		{
			robot_zqd.pwm[0] = -(float)i*a_stop;
			robot_zqd.pwm[2] = (float)i*a_stop;
			robot_zqd.pwm[1] = 0;
		}
		else if(Y_I-robot_zqd.Y<0)
		{
			robot_zqd.pwm[0] = (float)i*a_stop;
			robot_zqd.pwm[2] = -(float)i*a_stop;
			robot_zqd.pwm[1] = 0;
		}
		control1_W(robot_zqd.pwm[0]);
		control2_W(robot_zqd.pwm[1]);
		control3_W(robot_zqd.pwm[2]);
		delay_ms(100);
	}
	set_motor_vx_vy_w(0,0,0);
	control1_W(robot_zqd.pwm[0]);
	control2_W(robot_zqd.pwm[1]);
	control3_W(robot_zqd.pwm[2]);
	
}





//直线行走,阶梯变速
//X_I:目标坐标的X
//Y_I:目标坐标的Y
//Theta_I:目标坐标的角度
void robot_straight_stage(float X_I,float Y_I,float Theta_I)
{
	float D_Theta,D_X,D_Y,Vw=0,sx,sy=0;
	D_Theta = Theta_I/180*PI - robot_zqd.theta;
	D_X = X_I - robot_zqd.X;
	D_Y = Y_I - robot_zqd.Y;
	
	while(fabs(D_Y) > 0.05f || fabs(D_X) > 0.05f)
	{
		if(D_Y > 0.05f)
		{
			
			if(D_Y >= 1.5f)
			{
				sy = 8;
				if(robot_zqd.Vy < 0.3f)
					sy = 2;
				else if(robot_zqd.Vy < 0.5f)
					sy = 4;
				else if(robot_zqd.Vy < 0.9f)
					sy = 6;
			}
			else
			{
				sy = 8;
			}
			if(D_Y < 1.5f){    
				sy = 6;
			}
			if(D_Y < 0.8f){    
				sy = 4;
			}
			if(D_Y < 0.3f){    
				sy = 2;
			}
			if(D_Y < 0.2f)		sy = 0.25;
			
		}
		else if(D_Y < -0.05f)
		{
			if(D_Y < -1.5f)
			{
				sy = -8;
				if(robot_zqd.Vy > -0.3f)
					sy = -2;
				else if(robot_zqd.Vy > -0.5f)
					sy = -4;
				else if(robot_zqd.Vy > -0.9f)
					sy = -6;
			}
			else
			{
				sy = -8;
			}
			if(D_Y > -1.5f){    
				sy = -6;
			}
			if(D_Y > -0.8f){    
				sy = -4;
			}
			if(D_Y > -0.3f){    
				sy = -2;
			}
			if(D_Y > -0.2f)	    sy = -0.25;
		}
		else 
			sy = 0;
		
		
		if(D_X > 0.05f)
		{
			if(D_X > 1.5f)
			{
				sx = 8;
				if(robot_zqd.Vx < 0.1f)
					sx = 0.5;
				else if(robot_zqd.Vx < 0.2f)
					sx = 1;
				else if(robot_zqd.Vx < 0.4f)
					sx = 2;
				else if(robot_zqd.Vx < 0.58f)
					sx = 4;
				else if(robot_zqd.Vx < 7.5f)
					sx = 6;
			}
			else if(D_X < 1.5f)
			{
				if(D_X > 0.2f)
				{
					sx = 2;
				}
				else if(D_X > 0.15f)
				{
					sx = 1;
				}
				else if(D_X > 0.1f)
				{
					sx = 0.5f;
				}
				else
					sx = 0.25f;
			}
		}
		else if(D_X < -0.05f)
		{
			if(D_X < -1.5f)
			{
				sx = -8;
				if(robot_zqd.Vx > -0.1f)
					sx = -0.5;
				else if(robot_zqd.Vx > -0.2f)
					sx = -1;
				else if(robot_zqd.Vx > -0.4f)
					sx = -2;
				else if(robot_zqd.Vx > -0.58f)
					sx = -4;
				else if(robot_zqd.Vx > -7.5f)
					sx = -6;
			}
			else if(D_X > -1.5f)
			{
				if(D_X < -0.2f)
				{
					sx = -2;
				}
				else if(D_X < -0.15f)
				{
					sx = -1;
				}
				else if(D_X < -0.1f)
				{
					sx = -0.5f;
				}
				else
					sx = -0.25f;
			}
		}
		else 
			sx = 0;
		
		
		if(D_Theta>0&&(D_Theta<PI))  
		{
			Vw=D_Theta*500;
		}
		else if(D_Theta>0&&(D_Theta>=PI)) 
		{
			D_Theta = 2*PI-D_Theta;
			Vw=-D_Theta*500;
		}
		else if(D_Theta<0&&(D_Theta>=-PI)) 
		{
			D_Theta = -D_Theta;
			Vw=-D_Theta*500;
		}
		else if(D_Theta<0&&(D_Theta<-PI)) 
		{
			D_Theta = 2*PI+D_Theta;
			Vw=D_Theta*500;
		}
		else 
			Vw=Vw;
		
		//小于60°大于30°匀速
		//实际PWM为201
		if(D_Theta > 0.523599f)
		{
			if(Vw>0)
				Vw = 300;
			else
				Vw = -300;
		}
		
		//小于30°大于5°	PWM40
		if(D_Theta < 0.523599f)
		{
			if(Vw>0)
				Vw = 150;
			else
				Vw = -150;
		}
		//小于5°	pwm8
		if(D_Theta < 0.0872665f)
		{
			if(Vw>0)
				Vw = 20;
			else
				Vw = -20;
		}
		
		
		
		
		set_motor_vx_vy_w(sx*12,sy*100,Vw);
		control1_W(robot_zqd.pwm[0]);
		control2_W(robot_zqd.pwm[1]);
		control3_W(robot_zqd.pwm[2]);
		D_Theta = Theta_I/180*PI - robot_zqd.theta;
		D_X = X_I - robot_zqd.X;
		D_Y = Y_I - robot_zqd.Y;
	}
	control1_W(0);
	control2_W(0);
	control3_W(0);
	delay_ms(1000);
	robot_turnOrigin_stage(Theta_I);
}


//避障直行
//X_I:目标坐标的X
//Y_I:目标坐标的Y
//Theta_I:目标坐标的角度
//使用时注意机器人运动朝向与theta角尽量一致
void robot_straight_ObsAvoidance(float X_I,float Y_I,float Theta_I)
{
	float D_Theta,D_X,D_Y,Vw,sx,sy=0;
	float d_x,d_y,dis;
	float deg;
	D_Theta = Theta_I/180*PI - robot_zqd.theta;
	D_X = X_I - robot_zqd.X;
	D_Y = Y_I - robot_zqd.Y;
	
	//防止无效数据
	while(receive3 != 1);
	receive3 = 0;
	USART3_RX_STA = 0;
	
	while(fabs(D_Y) > 0.05f || fabs(D_X) > 0.05f)
	{
		if(D_Y > 0.05f)
		{
			if(D_Y >= 1.5f)
			{
				sy = 8;
				if(robot_zqd.Vy < 0.3f)
					sy = 2;
				else if(robot_zqd.Vy < 0.5f)
					sy = 4;
				else if(robot_zqd.Vy < 0.9f)
					sy = 6;
			}
			else
			{
				sy = 8;
			}
			if(D_Y < 1.5f){    
				sy = 6;
			}
			if(D_Y < 0.8f){    
				sy = 4;
			}
			if(D_Y < 0.3f){    
				sy = 2;
			}
			if(D_Y < 0.2f)		sy = 0.25;
			
		}
		else if(D_Y < -0.05f)
		{
			if(D_Y < -1.5f)
			{
				sy = -8;
				if(robot_zqd.Vy > -0.3f)
					sy = -2;
				else if(robot_zqd.Vy > -0.5f)
					sy = -4;
				else if(robot_zqd.Vy > -0.9f)
					sy = -6;
			}
			else
			{
				sy = -8;
			}
			if(D_Y > -1.5f){    
				sy = -6;
			}
			if(D_Y > -0.8f){    
				sy = -4;
			}
			if(D_Y > -0.3f){    
				sy = -2;
			}
			if(D_Y > -0.2f)	    sy = -0.25;
		}
		else 
			sy = 0;
		
		
		if(D_X > 0.05f)
		{
			if(D_X > 1.5f)
			{
				sx = 8;
				if(robot_zqd.Vx < 0.1f)
					sx = 0.5;
				else if(robot_zqd.Vx < 0.2f)
					sx = 1;
				else if(robot_zqd.Vx < 0.4f)
					sx = 2;
				else if(robot_zqd.Vx < 0.58f)
					sx = 4;
				else if(robot_zqd.Vx < 7.5f)
					sx = 6;
			}
			else if(D_X < 1.5f)
			{
				if(D_X > 0.2f)
				{
					sx = 2;
				}
				else
					sx = 0.25f;
			}
		}
		else if(D_X < -0.05f)
		{
			if(D_X < -1.5f)
			{
				sx = -8;
				if(robot_zqd.Vx > -0.1f)
					sx = -0.5;
				else if(robot_zqd.Vx > -0.2f)
					sx = -1;
				else if(robot_zqd.Vx > -0.4f)
					sx = -2;
				else if(robot_zqd.Vx > -0.58f)
					sx = -4;
				else if(robot_zqd.Vx > -7.5f)
					sx = -6;
			}
			else if(D_X > -1.5f)
			{
				if(D_X < -0.2f)
				{
					sx = -2;
				}
				else
					sx = -0.25f;
			}
		}
		else 
			sx = 0;
		
		
		if(D_Theta>0&&(D_Theta<PI))  
		{
			Vw=D_Theta*500;
		}
		else if(D_Theta>0&&(D_Theta>=PI)) 
		{
			D_Theta = 2*PI-D_Theta;
			Vw=-D_Theta*500;
		}
		else if(D_Theta<0&&(D_Theta>=-PI)) 
		{
			D_Theta = -D_Theta;
			Vw=-D_Theta*500;
		}
		else if(D_Theta<0&&(D_Theta<-PI)) 
		{
			D_Theta = 2*PI+D_Theta;
			Vw=D_Theta*500;
		}
		else 
			Vw=Vw;
		
		//小于60°大于30°匀速
		//实际PWM为201
		if(D_Theta > 0.523599f)
		{
			if(Vw>0)
				Vw = 300;
			else
				Vw = -300;
		}
		
		//小于30°大于5°	PWM40
		if(D_Theta < 0.523599f)
		{
			if(Vw>0)
				Vw = 150;
			else
				Vw = -150;
		}
		//小于5°	pwm8
		if(D_Theta < 0.0872665f)
		{
			if(Vw>0)
				Vw = 20;
			else
				Vw = -20;
		}
		
		if(uart_getLaser())
		{
			if(uart3_data[1] < fabs(D_X)*1000 || uart3_data[1] < fabs(D_Y)*1000)
			{
				deg = uart3_data[0] - (float)MID_LASER;
				deg = (uart3_data[0] - (float)MID_LASER) * PI /180;
				if(deg < 0 )
					deg = -deg;
				if(uart3_data[1] < 2500 && uart3_data[1] * sin(deg) <4000)
				{
					control1_W(robot_zqd.pwm[0]);
					control2_W(robot_zqd.pwm[0]);
					control3_W(robot_zqd.pwm[0]);
					
					dis = uart3_data[1] + 1500;
					d_x = robot_zqd.X;
					d_y = robot_zqd.Y;
					D_X = d_x - robot_zqd.X;
					D_Y = d_y - robot_zqd.Y;
					while(1)
					{
						while(receive3 != 1);
		
						if(!uart_getLaser())
						{	
							set_motor_vx_vy_w_R(0,0,0);
							control1_W(robot_zqd.pwm[0]);
							control2_W(robot_zqd.pwm[1]);
							control3_W(robot_zqd.pwm[2]);
							continue;
						}
						
						if(fabs(robot_zqd.Vx) > 0.5f || robot_zqd.Vy > 0.5f)
						{
							set_motor_vx_vy_w_R(0,0,0);
							control1_W(robot_zqd.pwm[0]);
							control2_W(robot_zqd.pwm[1]);
							control3_W(robot_zqd.pwm[2]);
							delay_ms(30000);
							continue;
						}
						
						if(D_Y*1000 < -dis || D_Y*1000 > dis || D_X*1000 < -dis || D_X*1000 > dis)
						{
							set_motor_vx_vy_w_R(0,0,0);
							control1_W(robot_zqd.pwm[0]);
							control2_W(robot_zqd.pwm[1]);
							control3_W(robot_zqd.pwm[2]);
							break;
						}
						
						deg = (uart3_data[0] - (float)MID_LASER) * PI /180;
						if(uart3_data[1] < 2500 && deg < 0 && uart3_data[1] * sin(deg) > -400)
						{
							set_motor_vx_vy_w_R(80,25,Vw);
						}
						else if(uart3_data[1] < 2500 && deg > 0 && uart3_data[1] * sin(deg) <400)
						{
							set_motor_vx_vy_w_R(-80,25,Vw);
						}
						else
						{
							set_motor_vx_vy_w_R(0,25,Vw);
						}
						
						control1_W(robot_zqd.pwm[0]);
						control2_W(robot_zqd.pwm[1]);
						control3_W(robot_zqd.pwm[2]);
						
						
						//计算距离
						D_Theta = Theta_I/180*PI - robot_zqd.theta;
						D_X = d_x - robot_zqd.X;
						D_Y = d_y - robot_zqd.Y;
						
						if(D_Theta>0&&(D_Theta<PI))  
						{
							Vw=D_Theta*500;
						}
						else if(D_Theta>0&&(D_Theta>=PI)) 
						{
							D_Theta = 2*PI-D_Theta;
							Vw=-D_Theta*500;
						}
						else if(D_Theta<0&&(D_Theta>=-PI)) 
						{
							D_Theta = -D_Theta;
							Vw=-D_Theta*500;
						}
						else if(D_Theta<0&&(D_Theta<-PI)) 
						{
							D_Theta = 2*PI+D_Theta;
							Vw=D_Theta*500;
						}
						else 
							Vw=Vw;
						
						//小于60°大于30°匀速
						//实际PWM为201
						if(D_Theta > 0.523599f)
						{
							if(Vw>0)
								Vw = 300;
							else
								Vw = -300;
						}
						
						//小于30°大于5°	PWM40
						if(D_Theta < 0.523599f)
						{
							if(Vw>0)
								Vw = 150;
							else
								Vw = -150;
						}
						//小于5°	pwm8
						if(D_Theta < 0.0872665f)
						{
							if(Vw>0)
								Vw = 20;
							else
								Vw = -20;
						}
					}
					sx = 0;
					sy = 0;
					Vw = 0;
				}
			}
		}
		
		set_motor_vx_vy_w(sx*12,sy*100,Vw);
		control1_W(robot_zqd.pwm[0]);
		control2_W(robot_zqd.pwm[1]);
		control3_W(robot_zqd.pwm[2]);
		D_Theta = Theta_I/180*PI - robot_zqd.theta;
		D_X = X_I - robot_zqd.X;
		D_Y = Y_I - robot_zqd.Y;
	}
	control1_W(0);
	control2_W(0);
	control3_W(0);
	delay_ms(1000);
	robot_turnOrigin_stage(Theta_I);
}



//直线行走,带坐标修正
//X_I:目标坐标的X
//Y_I:目标坐标的Y
//V：前进的速度
//distance:电机停止的距离,单位m
void robot_straight_I(float X_I,float Y_I,float Theta,float V,float W,float distance)
{
	float D_X,D_Y,D_Theta,T_Theta,V_Theta=0,Vx,Vy,Vw=0,length=0;
	T_Theta=Theta;
	Theta=Theta/180*PI;  //机器人在基坐标系中下一次  姿态角Theta值  基坐标系x轴正方向X_I与机器人坐标系x轴正方向X_R的夹角
	do{
		D_X=X_I-robot_zqd.X;
		D_Y=Y_I-robot_zqd.Y;
		D_Theta=Theta-robot_zqd.theta;	
		if(D_X==0){
			if(D_Y>0) 
				V_Theta=90.0/180.0*PI;
			else if(D_Y<0) 
				V_Theta=270.0/180.0*PI;
		}
		else if(D_Y==0){
			if(D_X>0) 
				V_Theta=0.0;
			else if(D_X<0) 
				V_Theta=PI;
		}
		else{
			if(D_Y>0&&D_X>0) 
				V_Theta=atan(D_Y/D_X);
			else if(D_Y>0&&D_X<0) 
				V_Theta=PI-atan(D_Y/-D_X);
			else if(D_Y<0&&D_X<0) 
				V_Theta=atan(D_Y/D_X)+PI;
			else if(D_Y<0&&D_X>0) 
				V_Theta=2*PI-atan(-D_Y/D_X);
		}
		length=sqrt(D_Y*D_Y+D_X*D_X);
		
		Vx=V*cos(V_Theta);
		Vy=V*sin(V_Theta);
		if(D_Theta>0&&fabs(D_Theta)<=PI)  
			Vw=W;
		else if(D_Theta>0&&fabs(D_Theta)>PI) 
			Vw=-W;
		else if(D_Theta<0&&fabs(D_Theta)<=PI) 
			Vw=-W;
		else if(D_Theta<0&&fabs(D_Theta)>PI) 
			Vw=W;
		else 
			Vw=0;
		
		set_motor_vx_vy_w(Vx,Vy,Vw);		

		control1_W(robot_zqd.pwm[0]);
		control2_W(robot_zqd.pwm[1]);	
		control3_W(robot_zqd.pwm[2]);
		length=sqrt(D_Y*D_Y+D_X*D_X);
	}while(length>distance);
	
	control1_W(0);
	control2_W(0);
	control3_W(0);
	while(robot_zqd.Vx!=0||robot_zqd.Vy!=0||robot_zqd.W!=0);
	delay_ms(10);
	robot_turnOrigin(400,T_Theta,10);
}

//自旋运动
//W自旋角速度 Theta 目标姿态角 dis姿态角偏差
void robot_turnOrigin(float W,float Theta,float dis)
{ 
	float D_Theta,Vw=0;        //W大于0 逆时针
	Theta=Theta/180.0f*PI;
	dis=dis/180.0f*PI;
	D_Theta=Theta-robot_zqd.theta;
	if(D_Theta>0&&fabs(D_Theta)<PI)  
		Vw=W;
	else if(D_Theta>0&&fabs(D_Theta)>=PI) 
		Vw=-W;
	else if(D_Theta<0&&fabs(D_Theta)<=PI) 
		Vw=-W;
	else if(D_Theta<0&&fabs(D_Theta)>PI) 
		Vw=W;
	else 
		Vw=Vw;
	
	if(fabs(D_Theta) > PI)
		D_Theta = fabs(D_Theta) - PI;
	
	while(fabs(D_Theta)>dis){
		set_motor_vx_vy_w(0,0,Vw);		
		control1_W(robot_zqd.pwm[0]);
		control2_W(robot_zqd.pwm[1]);
		control3_W(robot_zqd.pwm[2]);		
		
		D_Theta=Theta-robot_zqd.theta;	
		if(fabs(D_Theta) > PI)
			D_Theta = fabs(D_Theta) - PI;
	}
	control1_W(0);
	control2_W(0);		
	control3_W(0);
	robot_zqd.theta = Theta;
}

//自旋运动，根据误差角度，自动调节
void robot_turnOrigin_stage(float theta)
{
	float D_Theta,Vw=0;        //W大于0 逆时针
	theta=theta/180.0f*PI;
	D_Theta=theta-robot_zqd.theta;
	
	//大于30°线性控制
	if(D_Theta>0&&(D_Theta<PI))  
	{
		Vw=D_Theta*500;
	}
	else if(D_Theta>0&&(D_Theta>=PI)) 
	{
		D_Theta = 2*PI-D_Theta;
		Vw=-D_Theta*500;
	}
	else if(D_Theta<0&&(D_Theta>=-PI)) 
	{
		D_Theta = -D_Theta;
		Vw=-D_Theta*500;
	}
	else if(D_Theta<0&&(D_Theta<-PI)) 
	{
		D_Theta = 2*PI+D_Theta;
		Vw=D_Theta*500;
	}
	else 
		Vw=Vw;
	
	//小于60°大于30°匀速
	//实际PWM为100
	if(D_Theta < 2.0944f)
	{
		if(Vw>0)
			Vw = 1000;
		else
			Vw = -1000;
	}
	
	//小于30°大于5°
	if(D_Theta < 0.523599f)
	{
		if(Vw>0)
			Vw = 200;
		else
			Vw = -200;
	}
	//小于5°
	if(D_Theta < 0.0872665f)
	{
		if(Vw>0)
			Vw = 40;
		else
			Vw = -40;
	}
	
	while(D_Theta>0.015f){
		set_motor_vx_vy_w(0,0,Vw);		
		control1_W(robot_zqd.pwm[0]);
		control2_W(robot_zqd.pwm[1]);
		control3_W(robot_zqd.pwm[2]);		
		
		D_Theta=theta-robot_zqd.theta;
		//大于30°线性控制
		if(D_Theta>0&&(D_Theta<PI))  
		{
			Vw=D_Theta*500;
		}
		else if(D_Theta>0&&(D_Theta>=PI)) 
		{
			D_Theta = 2*PI-D_Theta;
			Vw=-D_Theta*500;
		}
		else if(D_Theta<0&&(D_Theta>=-PI)) 
		{
			D_Theta = -D_Theta;
			Vw=-D_Theta*500;
		}
		else if(D_Theta<0&&(D_Theta<-PI)) 
		{
			D_Theta = 2*PI+D_Theta;
			Vw=D_Theta*500;
		}
		else 
			Vw=Vw;
		
		//小于60°大于30°匀速
		//实际PWM为201
		if(D_Theta < 2.0944f)
		{
			if(Vw>0)
				Vw = 1000;
			else
				Vw = -1000;
		}
		
		//小于30°大于5°	PWM40
		if(D_Theta < 0.523599f)
		{
			if(Vw>0)
				Vw = 200;
			else
				Vw = -200;
		}
		//小于5°	pwm8
		if(D_Theta < 0.0872665f)
		{
			if(Vw>0)
				Vw = 40;
			else
				Vw = -40;
		}
	}
	control1_W(0);
	control2_W(0);		
	control3_W(0);
	set_motor_vx_vy_w(0,0,0);
	while(robot_zqd.W);
}

void get_red_basketball()
{
	USART_SendData(USART1, '1'); 
}

void get_red_volleyball()
{
	USART_SendData(USART1, '4'); 
}

void get_blue_basketball()
{
	USART_SendData(USART1, '2'); 
}

void get_blue_volleyball()
{
	USART_SendData(USART1, '3'); 
}

void get_dingweizhu()
{
	USART_SendData(USART1, '5'); 
}

void get_lankuang()
{
	USART_SendData(USART1, '9'); 
}
	
	
void get_weizhi_dis()
{
	USART_SendData(USART1, '6'); 
}

void get_border()
{
	USART_SendData(USART1, '7'); 
}
void get_along_border()
{
	USART_SendData(USART1, '8'); 
}

u8 uart_getData(void)
{
	
	if(USART_RX_STA&0x8000)
	{					   
		//得到此次接收到的数据长度
		//len=USART_RX_STA&0x3fff;

		/*
		//原来的视觉找球
		//球左右像素的位置 0-640
		if(USART_RX_BUF[0]!=' ')
			uart_data[0]=(USART_RX_BUF[0]-'0')*100;
		else 
			uart_data[0]=0;
		USART_RX_BUF[0] = ' ';
		if(USART_RX_BUF[1]!=' ')
			uart_data[0]+=(USART_RX_BUF[1]-'0')*10;
		USART_RX_BUF[1] = ' ';
		if(USART_RX_BUF[2]!=' ')
			uart_data[0]+=(USART_RX_BUF[2]-'0');
		USART_RX_BUF[2] = ' ';
		
		//深度信息
		if(USART_RX_BUF[3]!=' ')
			uart_data[1]=(USART_RX_BUF[3]-'0')*1000;
		else 
			uart_data[1]=0;
		USART_RX_BUF[3] = ' ';
		if(USART_RX_BUF[4]!=' ')
			uart_data[1]+=(USART_RX_BUF[4]-'0')*100;
		USART_RX_BUF[4] = ' ';
		if(USART_RX_BUF[5]!=' ')
			uart_data[1]+=(USART_RX_BUF[5]-'0')*10;
		USART_RX_BUF[5] = ' ';
		if(USART_RX_BUF[6]!=' ')
			uart_data[1]+=(USART_RX_BUF[6]-'0');
		USART_RX_BUF[6] = ' ';
		
		//半径信息
		if(USART_RX_BUF[7]!=' ')
			uart_data[2]=(USART_RX_BUF[7]-'0')*100;
		else 
			uart_data[2]=0;
		USART_RX_BUF[7] = ' ';
		if(USART_RX_BUF[8]!=' ')
			uart_data[2]+=(USART_RX_BUF[8]-'0')*10;
		USART_RX_BUF[8] = ' ';
		if(USART_RX_BUF[9]!=' ')
			uart_data[2]+=(USART_RX_BUF[9]-'0');
		USART_RX_BUF[9] = ' ';
		*/
		
		//坐标位置信息
		if(USART_RX_BUF[0]!=' ')
			uart_data[0]=(USART_RX_BUF[0]-'0')*100;
		else 
			uart_data[0]=0;
		USART_RX_BUF[0] = ' ';
		if(USART_RX_BUF[1]!=' ')
			uart_data[0]+=(USART_RX_BUF[1]-'0')*10;
		USART_RX_BUF[1] = ' ';
		if(USART_RX_BUF[2]!=' ')
			uart_data[0]+=(USART_RX_BUF[2]-'0');
		USART_RX_BUF[2] = ' ';
		
		//深度信息
		if(USART_RX_BUF[3]!=' ')
			uart_data[1]=(USART_RX_BUF[3]-'0')*1000;
		else 
			uart_data[1]=0;
		USART_RX_BUF[3] = ' ';
		if(USART_RX_BUF[4]!=' ')
			uart_data[1]+=(USART_RX_BUF[4]-'0')*100;
		USART_RX_BUF[4] = ' ';
		if(USART_RX_BUF[5]!=' ')
			uart_data[1]+=(USART_RX_BUF[5]-'0')*10;
		USART_RX_BUF[5] = ' ';
		if(USART_RX_BUF[6]!=' ')
			uart_data[1]+=(USART_RX_BUF[6]-'0');
		USART_RX_BUF[6] = ' ';

		LCD_ShowString(30+200,380,200,16,16,"View :pix");	
		LCD_ShowNum(30+200+48+8+45,380,uart_data[0],4,16);		
		LCD_ShowString(30+200,400,200,16,16,"View :length");	
		LCD_ShowNum(30+200+48+8+45,400,uart_data[1],4,16);	
		
		USART_RX_STA=0;
		receive=0;
	}
	
	if(uart_data[0]<10 || uart_data[0] >630)
		return 0;
	if(uart_data[1]<50)
		return 0;
	if(uart_data[2]==0)
		return 0;
	
	
	
	return 1;
}


//激光处理数据
u8 uart_getLaser(void)
{
	
	if(USART3_RX_STA&0x8000)
	{					   
		//得到此次接收到的数据长度
		//len=USART_RX_STA&0x3fff;

		
		//篮筐深度
		if(USART3_RX_BUF[0]!=' ')
			uart3_data[1]=(USART3_RX_BUF[0]-'0')*1000;
		else 
			uart3_data[1]=0;
		USART3_RX_BUF[0] = ' ';
		if(USART3_RX_BUF[1]!=' ')
			uart3_data[1]+=(USART3_RX_BUF[1]-'0')*100;
		USART3_RX_BUF[1] = ' ';
		if(USART3_RX_BUF[2]!=' ')
			uart3_data[1]+=(USART3_RX_BUF[2]-'0')*10;
		USART3_RX_BUF[2] = ' ';
		if(USART3_RX_BUF[3]!=' ')
			uart3_data[1] +=(USART3_RX_BUF[3]-'0');
		USART3_RX_BUF[3] = ' ';
		
		//篮筐角度
		if(USART3_RX_BUF[4]!=' ')
			uart3_data[0]=(USART3_RX_BUF[4]-'0')*100;
		else 
			uart3_data[0]=0;
		USART3_RX_BUF[4] = ' ';
		if(USART3_RX_BUF[5]!=' ')
			uart3_data[0]+=(USART3_RX_BUF[5]-'0')*10;
		USART3_RX_BUF[5] = ' ';
		if(USART3_RX_BUF[6]!=' ')
			uart3_data[0]+=(USART3_RX_BUF[6]-'0');
		USART3_RX_BUF[6] = ' ';
		
		

		LCD_ShowString(30+200,420,200,16,16,"Radar:rad");	
		LCD_ShowNum(30+200+48+8+45,420,uart3_data[0],4,16);		
		LCD_ShowString(30+200,440,200,16,16,"Radar:length");	
		LCD_ShowNum(30+200+48+8+45,440,uart3_data[1],4,16);	
		
		USART3_RX_STA=0;
		receive3=0;
	}
	
	if(uart_data[0]<240 && uart_data[0] >300)
		return 0;
	if(uart_data[1]>4000)
		return 0;
	
	
	return 1;
}


//视觉找球
//限制4米以内
void find_ball(u8 ball)
{
	float w=200;
	u8 time = 1;
	float theta = robot_zqd.theta,D_theta = 0;
	switch(ball)
	{
		case 1:
			get_red_basketball();
			delay_ms(10000);
			get_red_basketball();
			delay_ms(10000);
			get_red_basketball();
			break;
		case 2:
			get_blue_basketball();
			delay_ms(10000);
			get_blue_basketball();
			delay_ms(10000);
			get_blue_basketball();
			break;
		case 3:
			get_blue_volleyball();
			delay_ms(10000);
			get_blue_volleyball();
			delay_ms(10000);
			get_blue_volleyball();
			break;
		case 4:
			get_red_volleyball();
			delay_ms(10000);
			get_red_volleyball();
			delay_ms(10000);
			get_red_volleyball();
			break;
		case 6:
			get_weizhi_dis();
			delay_ms(10000);
			get_weizhi_dis();
			delay_ms(10000);
			get_weizhi_dis();
			break;
		case 9:
			get_lankuang();
			delay_ms(10000);
			get_lankuang();
			delay_ms(10000);
			get_lankuang();
			break;
	}
	control1_W(0);
	control2_W(0);
	control3_W(0);
	LCD_Show_pwm();
	while(receive != 1);
	USART_RX_STA=0;
	receive=0;
	do{
		while(receive != 1);
		
		if(!uart_getData())
		{	
			if(time == 0)
			{
			}
			else if(time++ <5)
			{
				set_motor_vx_vy_w_R(0 ,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
				continue;
			}
			else if(time !=0)
				time = 0;
		}
		else
			time = 1;
		
		if(time == 0)
		{
			D_theta = robot_zqd.theta - theta;
			if((D_theta > PI/6.0f && D_theta < PI) || (D_theta < -PI && D_theta > -PI*11.0f/6.0f))
			{
				w = -200;
			}
			if((D_theta < -PI/6.0f && D_theta > -PI) || (D_theta > PI && D_theta < PI*11.0f/6.0f))
			{
				w = 200;
			}
			set_motor_vx_vy_w(0,0,w);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[1] > 400)
		{
			set_motor_vx_vy_w(0,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart_data[0]< MID_VIEW-30) && uart_data[1]>130)
		{
			set_motor_vx_vy_w_R(-50,10,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart_data[0] > MID_VIEW+30) && uart_data[1] > 130)
		{
			set_motor_vx_vy_w_R(50,10,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[1] > 130)
		{
			set_motor_vx_vy_w_R(0,20,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart_data[0]< MID_VIEW-20) && uart_data[1] > 70)
		{
			set_motor_vx_vy_w_R(-40,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart_data[0] > MID_VIEW+20) && (uart_data[1] >70))
		{
			set_motor_vx_vy_w_R(40,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[1]>70)
		{
			set_motor_vx_vy_w_R(0,12,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[0]< MID_VIEW-30)
		{
			set_motor_vx_vy_w_R(-30,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[0] > MID_VIEW+30)
		{
			set_motor_vx_vy_w_R(30,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[0] <= MID_VIEW+30 && uart_data[0] > MID_VIEW+10)
		{
			set_motor_vx_vy_w_R(15,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[0] >= MID_VIEW-30 && uart_data[0] < MID_VIEW -10)
		{
			set_motor_vx_vy_w_R(-15,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else 
		{
				control1_W(0);
				control2_W(0);
				control3_W(0);
				jixiebi_down();
				set_motor_vx_vy_w_R(0,7,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
				break;
			
		}
	}while(1);
	get_hongwai();
	robot_zqd.pwm[0] = 0;
	robot_zqd.pwm[1] = 0;
	robot_zqd.pwm[2] = 0;
	control1_W(0);
	control2_W(0);
	control3_W(0);
	jixiebi_up();
	LCD_Show_pwm();
	
}

//视觉、雷达找球结合
//限制4m以内
void find_ball_sanfen(u8 ball)
{
	float w=200;
	u8 time = 1;
	float theta = robot_zqd.theta,D_theta = 0;
	switch(ball)
	{
		case 1:
			get_red_basketball();
			delay_ms(10000);
			get_red_basketball();
			delay_ms(10000);
			get_red_basketball();
			break;
		case 2:
			get_blue_basketball();
			delay_ms(10000);
			get_blue_basketball();
			delay_ms(10000);
			get_blue_basketball();
			break;
		case 3:
			get_blue_volleyball();
			delay_ms(10000);
			get_blue_volleyball();
			delay_ms(10000);
			get_blue_volleyball();
			break;
		case 4:
			get_red_volleyball();
			delay_ms(10000);
			get_red_volleyball();
			delay_ms(10000);
			get_red_volleyball();
			break;
		case 6:
			get_weizhi_dis();
			delay_ms(10000);
			get_weizhi_dis();
			delay_ms(10000);
			get_weizhi_dis();
			break;
		case 9:
			get_lankuang();
			delay_ms(10000);
			get_lankuang();
			delay_ms(10000);
			get_lankuang();
			break;
	}
	control1_W(0);
	control2_W(0);
	control3_W(0);
	LCD_Show_pwm();
	
	//清空视觉串口数据
	while(receive != 1);
	USART_RX_STA=0;
	receive=0;
	
	//清空串口接收数据缓存
	receive3 = 0;
	USART3_RX_STA = 0;
	
	control1_W(0);
	control2_W(0);
	control3_W(0);
	LCD_Show_pwm();
	
	//防止无效数据
	while(receive3 != 1);
	receive3 = 0;
	USART3_RX_STA = 0;
	
	do{
		while(receive != 1);
		while(receive3 != 1);
		
		if(!uart_getData())
		{	
			if(time == 0)
			{
			}
			else if(time++ <5)
			{
				set_motor_vx_vy_w_R(0 ,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
				continue;
			}
			else if(time !=0)
				time = 0;
		}
		else
			time = 1;
		uart_getLaser();
		
		if(time == 0)
		{
			D_theta = robot_zqd.theta - theta;
			if((D_theta > PI/6.0f && D_theta < PI) || (D_theta < -PI && D_theta > -PI*11.0f/6.0f))
			{
				w = -200;
			}
			if((D_theta < -PI/6.0f && D_theta > -PI) || (D_theta > PI && D_theta < PI*11.0f/6.0f))
			{
				w = 200;
			}
			set_motor_vx_vy_w(0,0,w);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[1]>400)
		{
			set_motor_vx_vy_w_R(0,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart_data[0]< MID_VIEW-30) && uart_data[1]>130)
		{
			set_motor_vx_vy_w_R(-50,10,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart_data[0] > MID_VIEW+30) && uart_data[1] > 130)
		{
			set_motor_vx_vy_w_R(50,10,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[1] > 130)
		{
			set_motor_vx_vy_w_R(0,20,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else
			break;
	}while(1);

	
	//精确瞄准
	if(uart3_data[0] > MID_LASER-15 && uart3_data[0] < MID_LASER+15)
	{
		LCD_ShowString(30+200,460,200,16,16,"Radar!");
		//雷达数据相同，按照雷达数据寻找
		while(1)
		{
			while(receive3 != 1);
			if(!uart_getLaser())
			{	
				set_motor_vx_vy_w_R(0,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
				continue;
			}
			if((uart3_data[0]< MID_LASER-10) && uart3_data[1] >700)
			{
				set_motor_vx_vy_w_R(-100,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if((uart3_data[0] > MID_LASER+10) && uart3_data[1] >700)
			{
				set_motor_vx_vy_w_R(100,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart3_data[1]>700)
			{
				set_motor_vx_vy_w_R(0,14,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart3_data[0]< MID_LASER-5)
			{
				set_motor_vx_vy_w_R(-100,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart3_data[0] > MID_LASER+5)
			{
				set_motor_vx_vy_w_R(100,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else 
			{
				control1_W(0);
				control2_W(0);
				control3_W(0);
				jixiebi_down();
				set_motor_vx_vy_w_R(0,7,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
				if(uart3_data[1] < 350)
					break;
			}
			
		}
	}
	else
	{
		LCD_ShowString(30+200,460,200,16,16,"View!");
		//雷达数据不同时，按照视觉数据寻找
		while(1)
		{
			while(receive != 1);
			if(!uart_getData())
			{	
				set_motor_vx_vy_w_R(0,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
				continue;
			}
			if(uart_data[0]< MID_VIEW-20)
			{
				set_motor_vx_vy_w_R(-40,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[0] > MID_VIEW+20)
			{
				set_motor_vx_vy_w_R(40,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[1]>70)
			{
				set_motor_vx_vy_w_R(0,12,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[0]< MID_VIEW-30)
			{
				set_motor_vx_vy_w_R(-30,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[0] > MID_VIEW+30)
			{
				set_motor_vx_vy_w_R(30,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[0] <= MID_VIEW+30 && uart_data[0] > MID_VIEW+10)
			{
				set_motor_vx_vy_w_R(15,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[0] >= MID_VIEW-30 && uart_data[0] < MID_VIEW -10)
			{
				set_motor_vx_vy_w_R(-15,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else 
			{
					control1_W(0);
					control2_W(0);
					control3_W(0);
					jixiebi_down();
					set_motor_vx_vy_w_R(0,7,0);
					control1_W(robot_zqd.pwm[0]);
					control2_W(robot_zqd.pwm[1]);
					control3_W(robot_zqd.pwm[2]);
					break;
			}
		}
	}
		
		
	get_hongwai();
	robot_zqd.pwm[0] = 0;
	robot_zqd.pwm[1] = 0;
	robot_zqd.pwm[2] = 0;
	control1_W(0);
	control2_W(0);
	control3_W(0);
	jixiebi_up();
	LCD_Show_pwm();
	LCD_ShowString(30+200,460,200,16,16,"       ");
}

//利用激光找球
//不用区分颜色
//限制3m以内
void find_ball_laser(void)
{
	float w=300;
	float theta = robot_zqd.theta,D_theta = 0;
	
	get_lankuang();
	get_lankuang();
	get_lankuang();
	
	control1_W(0);
	control2_W(0);
	control3_W(0);
	LCD_Show_pwm();
	do{
		while(receive3 != 1);
		
		if(!uart_getLaser())
		{	
			set_motor_vx_vy_w_R(0,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
			continue;
		}
		LED0 = !LED0;
		
		if(uart3_data[1] < 10)
			continue;
		if(uart3_data[1] > 3000)
		{			
			D_theta = robot_zqd.theta - theta;
			if((D_theta > PI/6.0f && D_theta < PI) || (D_theta < -PI && D_theta > -PI*11.0f/6.0f))
			{
				w = -300;
			}
			if((D_theta < -PI/6.0f && D_theta > -PI) || (D_theta > PI && D_theta < PI*11.0f/6.0f))
			{
				w = 300;
			}
			set_motor_vx_vy_w(0,0,w);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0]< MID_LASER-15)
		{
			set_motor_vx_vy_w_R(0,0,200);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0] > MID_LASER+15)
		{
			set_motor_vx_vy_w_R(0,0,-200);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[1] > 1000)
		{
			set_motor_vx_vy_w_R(0,30,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart3_data[0]< MID_LASER-10) && uart3_data[1] >700)
		{
			set_motor_vx_vy_w_R(0,0,150);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart3_data[0] > MID_LASER+10) && uart3_data[1] >700)
		{
			set_motor_vx_vy_w_R(0,0,-150);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[1]>700)
		{
			set_motor_vx_vy_w_R(0,20,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0]< MID_LASER-5)
		{
			set_motor_vx_vy_w_R(-80,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0] > MID_LASER+5)
		{
			set_motor_vx_vy_w_R(80,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else 
		{
			control1_W(0);
			control2_W(0);
			control3_W(0);
			jixiebi_down();
			set_motor_vx_vy_w_R(0,7,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
			if(uart3_data[1] < 350)
				break;
		}
	}while(1);
	get_hongwai();
	robot_zqd.pwm[0] = 0;
	robot_zqd.pwm[1] = 0;
	robot_zqd.pwm[2] = 0;
	control1_W(0);
	control2_W(0);
	control3_W(0);
	//jixiebi_up(1950,1100);
	LCD_Show_pwm();
	
}

//中圈、三分线找球
//如果视野中没有球会自动旋转
//find_ball_laser()改进
//限制3m以内
void find_ball_zhongquan(void)
{
	float w=300;
	float theta = robot_zqd.theta,D_theta = 0;
	
	//清空串口接收数据缓存
	receive3 = 0;
	USART3_RX_STA = 0;
	
	control1_W(0);
	control2_W(0);
	control3_W(0);
	LCD_Show_pwm();
	
	//防止无效数据
	while(receive3 != 1);
	receive3 = 0;
	USART3_RX_STA = 0;
	
	do{
		while(receive3 != 1);
		
		if(!uart_getLaser())
		{	
			set_motor_vx_vy_w_R(0,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
			LED1 = !LED1;
			continue;
		}
		LED0 = !LED0;
		
		if(uart3_data[1] < 10)
			continue;
		
		if(uart3_data[1] > 3000)
		{			
			D_theta = robot_zqd.theta - theta;
			if((D_theta > PI/6.0f && D_theta < PI) || (D_theta < -PI && D_theta > -PI*11.0f/6.0f))
			{
				w = -300;
			}
			if((D_theta < -PI/6.0f && D_theta > -PI) || (D_theta > PI && D_theta < PI*11.0f/6.0f))
			{
				w = 300;
			}
			set_motor_vx_vy_w(0,0,w);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0]< MID_LASER-15)
		{
			set_motor_vx_vy_w_R(0,0,200);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0] > MID_LASER+15)
		{
			set_motor_vx_vy_w_R(0,0,-200);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[1] > 1000 )
		{
			set_motor_vx_vy_w_R(0,17,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart3_data[0]< MID_LASER-10) && uart3_data[1] >700)
		{
			set_motor_vx_vy_w_R(0,0,150);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart3_data[0] > MID_LASER+10) && uart3_data[1] >700)
		{
			set_motor_vx_vy_w_R(0,0,-150);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[1]>700)
		{
			set_motor_vx_vy_w_R(0,14,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0]< MID_LASER-5)
		{
			set_motor_vx_vy_w_R(-80,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0] > MID_LASER+5)
		{
			set_motor_vx_vy_w_R(80,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0] <= MID_LASER+5 && uart3_data[0] > MID_LASER+2)
		{
			set_motor_vx_vy_w_R(30,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0] >= MID_LASER-5 && uart3_data[0] < MID_LASER -2)
		{
			set_motor_vx_vy_w_R(-30,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else 
		{
			control1_W(0);
			control2_W(0);
			control3_W(0);
			jixiebi_down();
			set_motor_vx_vy_w_R(0,7,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
			if(uart3_data[1] < 350)
				break;
		}
	}while(1);
	get_hongwai();
	robot_zqd.pwm[0] = 0;
	robot_zqd.pwm[1] = 0;
	robot_zqd.pwm[2] = 0;
	control1_W(0);
	control2_W(0);
	control3_W(0);
	jixiebi_up();
	LCD_Show_pwm();
	
}


//中圈、三分线找球
//如果视野中没有球会自动旋转
//find_ball_laser()改进
//限制3m以内
u8 find_ball_dixian(void)
{
	float w=300;
	float theta = robot_zqd.theta,D_theta = 0;
	
	//清空串口接收数据缓存
	receive3 = 0;
	USART3_RX_STA = 0;
	
	control1_W(0);
	control2_W(0);
	control3_W(0);
	LCD_Show_pwm();
	
	//防止无效数据
	while(receive3 != 1);
	receive3 = 0;
	USART3_RX_STA = 0;
	
	do{
		while(receive3 != 1);
		
		if(!uart_getLaser())
		{	
			set_motor_vx_vy_w_R(0,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
			LED1 = !LED1;
			continue;
		}
		LED0 = !LED0;
		
		if(uart3_data[1] < 10)
			continue;
		
		if(uart3_data[1] > 3000)
		{			
			D_theta = robot_zqd.theta - theta;
			if((D_theta > PI/6.0f && D_theta < PI) || (D_theta < -PI && D_theta > -PI*11.0f/6.0f))
			{
				w = -300;
			}
			if((D_theta < -PI/6.0f && D_theta > -PI) || (D_theta > PI && D_theta < PI*11.0f/6.0f))
			{
				w = 300;
			}
			set_motor_vx_vy_w(0,0,w);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0]< MID_LASER-15)
		{
			set_motor_vx_vy_w_R(0,0,200);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0] > MID_LASER+15)
		{
			set_motor_vx_vy_w_R(0,0,-200);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[1] > 1000 )
		{
			set_motor_vx_vy_w_R(0,17,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart3_data[0]< MID_LASER-10) && uart3_data[1] >700)
		{
			set_motor_vx_vy_w_R(0,0,150);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart3_data[0] > MID_LASER+10) && uart3_data[1] >700)
		{
			set_motor_vx_vy_w_R(0,0,-150);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[1]>700)
		{
			set_motor_vx_vy_w_R(0,14,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0]< MID_LASER-5)
		{
			set_motor_vx_vy_w_R(-80,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0] > MID_LASER+5)
		{
			set_motor_vx_vy_w_R(80,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0] <= MID_LASER+5 && uart3_data[0] > MID_LASER+2)
		{
			set_motor_vx_vy_w_R(30,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[0] >= MID_LASER-5 && uart3_data[0] < MID_LASER -2)
		{
			set_motor_vx_vy_w_R(-30,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(robot_zqd.Y < 0.9f)
		{
			robot_zqd.pwm[0] = 0;
			robot_zqd.pwm[1] = 0;
			robot_zqd.pwm[2] = 0;
			control1_W(0);
			control2_W(0);
			control3_W(0);
			return 1;
		}
		else 
		{
			control1_W(0);
			control2_W(0);
			control3_W(0);
			jixiebi_down();
			set_motor_vx_vy_w_R(0,7,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
			if(uart3_data[1] < 350)
				break;
		}
	}while(1);
	get_hongwai_dixian(0.9f);
	robot_zqd.pwm[0] = 0;
	robot_zqd.pwm[1] = 0;
	robot_zqd.pwm[2] = 0;
	control1_W(0);
	control2_W(0);
	control3_W(0);
	jixiebi_up();
	LCD_Show_pwm();
	return 0;
	
}


void find_lankuang(void)
{ 
	/*
	//纯雷达找篮筐，备用
	//清空串口接收数据缓存
	receive3 = 0;
	USART3_RX_STA = 0;
	
	control1_W(0);
	control2_W(0);
	control3_W(0);
	LCD_Show_pwm();
	
	//防止无效数据
	while(receive3 != 1);
	receive3 = 0;
	USART3_RX_STA = 0;
	
	do{
		while(receive3 != 1);
		
		if(!uart_getLaser())
		{	
			set_motor_vx_vy_w_R(0 ,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
			continue;
		}
		LED0 = !LED0;
		
		if(uart_data[1] > (DIS_LASER+500))
		{
			set_motor_vx_vy_w_R(0,40,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[1] <(DIS_LASER-500))
		{
			set_motor_vx_vy_w_R(0,-40,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[0] > MID_LASER+10 )
		{
			set_motor_vx_vy_w_R(-100,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[0] < MID_LASER - 10)
		{
			set_motor_vx_vy_w_R(100,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		if(uart_data[1]>(DIS_LASER+100))
		{
			set_motor_vx_vy_w_R(0,20,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[1]<(DIS_LASER-100))
		{
			set_motor_vx_vy_w_R(0,-20,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[0]> MID_LASER + 2)
		{
			set_motor_vx_vy_w_R(50,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[0] < MID_LASER - 2 )
		{
			set_motor_vx_vy_w_R(-50,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		
		else
		{
			set_motor_vx_vy_w_R(0,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
			break;
		}
	}while(1);
	*/
	float w=200;
	u8 time = 1;
	float theta = robot_zqd.theta,D_theta = 0;
	
	get_dingweizhu();
	delay_ms(10000);
	get_dingweizhu();
	delay_ms(10000);
	get_dingweizhu();
	
	control1_W(0);
	control2_W(0);
	control3_W(0);
	LCD_Show_pwm();
	
	//清空视觉串口数据
	while(receive != 1);
	USART_RX_STA=0;
	receive=0;
	
	//清空串口接收数据缓存
	receive3 = 0;
	USART3_RX_STA = 0;
	
	control1_W(0);
	control2_W(0);
	control3_W(0);
	LCD_Show_pwm();
	
	//防止无效数据
	while(receive3 != 1);
	receive3 = 0;
	USART3_RX_STA = 0;
	
	do{
		while(receive != 1);
		while(receive3 != 1);
		
		if(!uart_getData())
		{	
			if(time == 0)
			{
			}
			else if(time++ <5)
			{
				set_motor_vx_vy_w_R(0 ,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
				continue;
			}
			else if(time !=0)
				time = 0;
		}
		else
			time = 1;
		uart_getLaser();
		
		if(time == 0)
		{
			D_theta = robot_zqd.theta - theta;
			if((D_theta > PI/6.0f && D_theta < PI) || (D_theta < -PI && D_theta > -PI*11.0f/6.0f))
			{
				w = -200;
			}
			if((D_theta < -PI/6.0f && D_theta > -PI) || (D_theta > PI && D_theta < PI*11.0f/6.0f))
			{
				w = 200;
			}
			set_motor_vx_vy_w(0,0,w);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[1] > DIS_VIEW + 25)
		{
			set_motor_vx_vy_w_R(0,20,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[1] < DIS_VIEW -25)
		{
			set_motor_vx_vy_w_R(0,-20,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[0]< MID_VIEW-20)
		{
			set_motor_vx_vy_w_R(-50,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart_data[0] > MID_VIEW+20)
		{
			set_motor_vx_vy_w_R(50,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else
		{
			set_motor_vx_vy_w_R(0,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
			break;
		}
	}while(1);

	
	//精确瞄准
	if(uart3_data[0] > MID_LASER-9 && uart3_data[0] < MID_LASER+9)
	{
		LCD_ShowString(30+200,460,200,16,16,"Radar!");
		//雷达数据相同，按照雷达数据寻找
		while(1)
		{
			while(receive3 != 1);
			if(!uart_getLaser())
			{	
				set_motor_vx_vy_w_R(0,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
				continue;
			}
			if((uart3_data[0]< MID_LASER-10))
			{
				set_motor_vx_vy_w_R(-100,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if((uart3_data[0] > MID_LASER+10))
			{
				set_motor_vx_vy_w_R(100,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart3_data[1]>DIS_LASER + 100)
			{
				set_motor_vx_vy_w_R(0,20,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart3_data[1] < DIS_LASER - 100)
			{
				set_motor_vx_vy_w_R(0,-20,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart3_data[0]< MID_LASER-5)
			{
				set_motor_vx_vy_w_R(-50,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart3_data[0] > MID_LASER+5)
			{
				set_motor_vx_vy_w_R(50,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart3_data[0] <= MID_LASER+5 && uart3_data[0] > MID_LASER+2)
			{
				set_motor_vx_vy_w_R(30,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart3_data[0] >= MID_LASER-5 && uart3_data[0] < MID_LASER -2)
			{
				set_motor_vx_vy_w_R(-30,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else 
			{
				control1_W(0);
				control2_W(0);
				control3_W(0);
				break;
			}
			
		}
	}
	else
	{
		LCD_ShowString(30+200,460,200,16,16,"View!");
		//雷达数据不同时，按照视觉数据寻找
		while(1)
		{
			while(receive != 1);
			if(!uart_getData())
			{	
				set_motor_vx_vy_w_R(0,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
				continue;
			}
			if(uart_data[0]< MID_VIEW-20)
			{
				set_motor_vx_vy_w_R(-50,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[0] > MID_VIEW+20)
			{
				set_motor_vx_vy_w_R(50,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[1]>DIS_VIEW+10)
			{
				set_motor_vx_vy_w_R(0,15,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[1]<DIS_VIEW -10)
			{
				set_motor_vx_vy_w_R(0,-15,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[0]< MID_VIEW-30)
			{
				set_motor_vx_vy_w_R(-45,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[0] > MID_VIEW+30)
			{
				set_motor_vx_vy_w_R(45,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[0] <= MID_VIEW+30 && uart_data[0] > MID_VIEW+10)
			{
				set_motor_vx_vy_w_R(30,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else if(uart_data[0] >= MID_VIEW - 30 && uart_data[0] < MID_VIEW -10)
			{
				set_motor_vx_vy_w_R(-30,0,0);
				control1_W(robot_zqd.pwm[0]);
				control2_W(robot_zqd.pwm[1]);
				control3_W(robot_zqd.pwm[2]);
			}
			else 
			{
					control1_W(0);
					control2_W(0);
					control3_W(0);
					break;
			}
		}
	}
		
	
	//防止无效数据
	while(receive3 != 1);
	receive3 = 0;
	USART3_RX_STA = 0;
	
	//精确瞄准
	while(1)
	{
		while(receive3 != 1);
		if(!uart_getLaser())
		{	
			set_motor_vx_vy_w_R(0,0,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
			continue;
		}
		if((uart3_data[0]< MID_LASER-1))
		{
			set_motor_vx_vy_w_R(0,0,100);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if((uart3_data[0] > MID_LASER+1))
		{
			set_motor_vx_vy_w_R(0,0,-100);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		//距离精度调整至50mm
		else if(uart3_data[1]>DIS_LASER + 30)
		{
			set_motor_vx_vy_w_R(0,7,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else if(uart3_data[1] < DIS_LASER - 30)
		{
			set_motor_vx_vy_w_R(0,-7,0);
			control1_W(robot_zqd.pwm[0]);
			control2_W(robot_zqd.pwm[1]);
			control3_W(robot_zqd.pwm[2]);
		}
		else 
		{
			control1_W(0);
			control2_W(0);
			control3_W(0);
			break;
		}
		
	}
		
	robot_zqd.pwm[0] = 0;
	robot_zqd.pwm[1] = 0;
	robot_zqd.pwm[2] = 0;
	LCD_Show_pwm();
	LCD_ShowString(30+200,460,200,16,16,"       ");
}


void remote_control(void)
{	
	int16_t vx=0,vy=0,w=0;
	u8 key;
	vx = 0;
	vy = 0;
	w = 0;
	while(1)
	{
		key = Remote_Scan();
		if(key == KEY_UP)
		{
			vy += 3;
		}
		else if(key == KEY_DOWN)
		{
			vy -= 3;
		}
		else if(key == KEY_LEFT)
		{
			vx -= 20;
		}
		else if(key == KEY_RIGHT)
		{
			vx += 20;
		}
		else if(key == KEY_VOL_A)
		{
			w += 40;
		}
		else if(key == KEY_VOL_D)
		{
			w -= 40;
		}
		else if(key == KEY_PLAY)
		{
			control_init();
		}
		else if(key == KEY_POWER)
		{
			control1_W(0);
			control2_W(0);
			control3_W(0);
			set_motor_vx_vy_w_R(0,0,0);
			break;
		}
		set_motor_vx_vy_w_R(vx,vy,w);
		control1_W(robot_zqd.pwm[0]);
		control2_W(robot_zqd.pwm[1]);
		control3_W(robot_zqd.pwm[2]);
	}
}


//弹射充电开关初始化
void charge_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);//使能GPIOG时钟

  //GPIOG_6初始化设置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOG, &GPIO_InitStructure);//初始化
	
	GPIO_ResetBits(GPIOG,GPIO_Pin_6);//GPIOG6
}


//弹射充电开关状态调整
void charge(u8 state)
{
	if(state)
		GPIO_SetBits(GPIOG,GPIO_Pin_6);
	else
		GPIO_ResetBits(GPIOG,GPIO_Pin_6);
}


