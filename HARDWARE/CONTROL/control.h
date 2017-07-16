#ifndef __CONTROL_H__
#define __CONTROL_H__

#include "sys.h"
#include "lcd.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include <math.h>

#define PI 3.141592654f
#define BMQ_DEC 4.0f		//��������Ƶ
#define BMQ_MAX 500.0f		//������������
#define BMQ_T 0.01f   		//���������㵱ǰ�ٶ�ʱ��
#define BMQ_L 0.2006f		//�������ֵ����������ĵľ���
#define BMQ_R 0.0275f		//���������Ӱ뾶
#define RAD 0.1570796327f	//������һ�������Ӧ�ĽǶ� pi/500/4/0.01
#define MOTOR_L 0.2013f		//�ֵ����������ĵľ���
#define MOTOR_R 0.0508f		//���ӵİ뾶

#define MOTOR_STATIC_1 3990		//TIM9 CH1 PE5
#define MOTOR_STATIC_2 3986  	//TIM9 CH2 PE6

#define MID_LASER 272		//���ⶨλ����
#define MID_VIEW 320		//�Ӿ���λ����
#define DIS_LASER 2500			//�����״ﶨλ����
#define DIS_VIEW 280	//�����Ӿ���λ����

struct robot{
	float X;		//������������ϵ��x����
	float Y;		//������������ϵ��y����
	float theta;	//�������������y��н�
	float Vx;		//������������ϵx�����ٶ�
	float Vy;		//������������ϵy�����ٶ�
	float W;		//�����˽��ٶȣ�˳ʱ��������
	float w[3];		//��������ʵ�ʼ���/4
	float v[3];		//���������ٶ�
	float pwm[3];	//���ӵ�pwm
	float theta_dev;	//��һʱ�̣�������theta��
	float theta_offset;	//�Ƕ�ƫ�����
};

extern struct robot robot_zqd;
extern u32 uart_data[3];
extern u8 sanfen_state;
extern u8 zhongquan_state;
extern u8 dixian_state;
extern u8 lankuang_state;

void control_init(void);
void control3_W(float W);
void control2_W(float W);
void control1_W(float W);
void shot_init(void);		//�����ʼ��
void xianwei_init(void);	//��λ���س�ʼ��
void hongwai_init(void);	//���⿪�س�ʼ��
void get_position(void);	//����ת��
void charge_init(void);		//�����翪�س�ʼ��
//�������ٶ�ת���ӵ�PWM
//vx���������x���ٶ�
//vy���������y���ٶ�
//w:������ԭ����ת�Ľ��ٶ�
void set_motor_vx_vy_w(float vx,float vy,float w);
void set_motor_vx_vy_w_R(float vx, float vy, float w);
void jixiebi_down(void);
void jixiebi_up(void);
int down_shot_up(void);		//��е���½���Ͷ�򣬻�е������
u8 get_hongwai(void);			//��ȡ����״̬
//ֱ������
//X_I:Ŀ�������X
//Y_I:Ŀ�������Y
//V��ǰ�����ٶ�
//distance:���ֹͣ�ľ���
void robot_straight_I(float X_I,float Y_I,float Theta,float V,float W,float distance);
void robot_straight_Y(float Y_I,float V,float distance,u32 a_start,u32 a_stop);
void robot_straight_stage(float X_I,float Y_I,float Theta_I);
void robot_straight_ObsAvoidance(float X_I,float Y_I,float Theta_I);
//�����˶�
//W�������ٶ� Theta Ŀ����̬�� dis��̬��ƫ��
void robot_turnOrigin(float W,float Theta,float dis);
void robot_turnOrigin_stage(float theta);
void get_red_basketball(void);
void get_red_volleyball(void);
void get_blue_basketball(void);
void get_blue_volleyball(void);
void get_dingweizhu(void);
void get_weizhi_dis(void);
void get_border(void);
void get_along_border(void);

void find_ball(u8 ball);
void find_ball_laser(void);
void find_ball_zhongquan(void);
void find_lankuang(void);
void find_ball_sanfen(u8 ball);
u8 find_ball_dixian(void);
int shot(void);
void charge(u8 state);		//�����翪��

void remote_control(void);
u8 xianwei_down(void);
u8 xianwei_up(void);

u8 uart_getLaser(void);
u8 uart_getData(void);

#endif