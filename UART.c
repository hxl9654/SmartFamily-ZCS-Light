/*//////////////////////////GPL开源许可证////////////////////////////////////////////////
    Copyright (C) <2014>  <Xianglong He>
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

	文件名：UART.c
	作者：何相龙
	邮箱：qwgg9654@gmail.com
		  admin@hxlxz.com
	功能描述：串口字符串通信模块
	备注：尽量使用封装好的函数进行操作，而不要使用直接对串口进行操作。
        使用该模块，请在config.h中定义UART_BUFF_MAX常量为数据缓存数组最大长度。
            如 #define UART_BUFF_MAX 64
        使用该模块，请在config.h中定义XTAL常量为晶振频率（单位：兆赫兹）。
            如 #define XTAL 11.059200
*////////////////////////////////////////////////////////////////////////////////////////
#include<stc15.h>
#include<UART.h>

#ifndef UART_BUFF_MAX
#define UART_BUFF_MAX 20
#endif 

#ifndef XTAL
#define XTAL 11.059200
#endif //如果没有定义晶振频率，则默认为11.0592M晶振

extern void UART_Action(unsigned char *dat, unsigned char len);
//此函数须另行编写：当串口完成一个字符串结束后会自动调用

unsigned char UART_Buff[UART_BUFF_MAX];     //串口接收缓冲区
unsigned char UART_BuffIndex = 0;           //串口接收缓冲区当前位置

bit SendingFlag = 0;
bit ResivingFlag = 0;
unsigned char DATA = 0x00;
bit UART_ResiveStringEndFlag;               //串口字符串接收全部完成标志
bit UART_ResiveStringFlag;                  //串口字符串正在接收标志

/*///////////////////////////////////////////////////////////////////////////////////
*函数名：UART_SendString
*函数功能：向串口发送一个字符串
*参数列表：
*   *dat
*       参数类型：unsigned char型指针
*       参数描述：要发送的字符串的首地址
*   len
*       参数类型：unsigned char型数据
*       参数描述：要发送的字符串的长度
*返回值：无
*版本：1.0
*作者：何相龙
*日期：2014年12月9日
*////////////////////////////////////////////////////////////////////////////////////
void UART_SendString(unsigned char *dat, unsigned char len)
{
	while(len)
	{
		len --;                     //每发送一位，长度减1
		while(ResivingFlag);
		DATA = *dat;
		SendingFlag = 1;
		while(SendingFlag);
		dat ++;                     //数据指针移向下一位
	}
}
/*///////////////////////////////////////////////////////////////////////////////////
*函数名：UART_Driver
*函数功能：串口通信监控函数，在主循环中调用。
*           如果接收到字符串，会自动调用另行编写的UART_Action(unsigned char *dat,unsigned char len)
*参数列表：
*   无
*返回值：无
*版本：1.0
*作者：何相龙
*日期：2014年12月9日
*////////////////////////////////////////////////////////////////////////////////////
void UART_Driver()
{
	if(UART_ResiveStringEndFlag)            	//如果串口接收到一个完整的字符串
	{
		UART_ResiveStringEndFlag = 0;   		//清空接收完成标志
		UART_Action(UART_Buff, UART_BuffIndex); //调用用户编写的UART_Action函数，将接收到的数据及数据长度作为参数
		UART_BuffIndex = 0; 
	}
}
/*///////////////////////////////////////////////////////////////////////////////////
*函数名：UART_RxMonitor
*函数功能：串口字符串接收结束判断，在定时器0中断函数中调用
*参数列表：
*   ms
*       参数类型：unsigned char型数据
*       参数描述：定时器延时时长（单位：ms）
*返回值：无
*版本：1.0
*作者：何相龙
*日期：2014年12月9日
*////////////////////////////////////////////////////////////////////////////////////
void UART_RxMonitor(unsigned char ms)
{
	static unsigned char MS = 0;                    //毫秒计时
	static unsigned char UART_BuffIndex_Backup;     //串口数据暂存数组位置备份
	if(! UART_ResiveStringFlag)return ;             //如果当前没有在接受数据，直接退出函数
    MS += ms;                                       //每一次定时器中断，表示时间过去了若干毫秒
	if(UART_BuffIndex_Backup != UART_BuffIndex)     //如果串口数据暂存数组位置备份不等于串口接收缓冲区当前位置（接收到了新数据位）
	{
		UART_BuffIndex_Backup = UART_BuffIndex;     //记录当前的串口接收缓冲区位置
		MS = 0;                                     //复位毫秒计时
	}
	else if(MS > 200)                                //毫秒到了
	{
		MS = 0;                                 	//复位毫秒计时
		UART_ResiveStringEndFlag = 1;           	//设置串口字符串接收全部完成标志
		UART_ResiveStringFlag = 0;              	//清空串口字符串正在接收标志
	}
}



//定时器1+外部中断4模拟串口

//void UART_T1_Init(unsigned int Baud)
void UART_T1_Init()
{
	AUXR |= 0x80;		//定时器时钟1T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x82;		//设置定时初值
	TH0 = 0xFB;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	PT0 = 1;
	ET0 = 1;
	EA = 1;

	INT_CLKO |= 0x40;
}

void Interrupt_INT4() interrupt 16
{
	ResivingFlag = 1;
}
void Interrupt_Timer0() interrupt 1
{
	static unsigned char mask = 0x00;
	if((!SendingFlag) && (!ResivingFlag))
		return ;
	
	if(ResivingFlag)
	{
		if(mask == 0x33)
		{
			mask = 0x00;
			ResivingFlag = 0;
			
			UART_Buff[UART_BuffIndex] = DATA;   //将接收到的数据放到暂存数组
			UART_ResiveStringFlag = 1;          //设置串口字符串正在接收标志
			UART_BuffIndex ++;                  //串口接收缓冲区当前位置右移
			
			return;
		}
		else if(mask == 0x00)
		{
			DATA = 0x00;
			mask = 0x01;
			return;
		}

		if(P30)
			DATA |= mask;
		mask <<= 1;
		if(mask == 0x00)
		{
			mask = 0x33;
		}

	}
	else if(SendingFlag)
	{
		if(mask == 0x44)
		{
			P31 = 1;
			SendingFlag = 0;
			mask = 0x00;
			return;
		}
		if(mask == 0x00)
		{
			P31 = 0;
			mask = 0x01;
			return ;
		}
		P31 = DATA & mask;
		mask <<= 1;
		if(mask == 0x00)
		{			
			mask = 0x44;
		}
	}
	
}
