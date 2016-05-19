#include<stc15.h>
#include<uart.h>
#include<string.h>
unsigned char LightValue = 3;
void Timer2Init(void)		//100΢��@11.0592MHz
{
	AUXR &= 0xFB;		//��ʱ��ʱ��12Tģʽ
	T2L = 0xAE;		//���ö�ʱ��ֵ
	T2H = 0xFB;		//���ö�ʱ��ֵ
	AUXR |= 0x10;		//��ʱ��2��ʼ��ʱ
	IE2 |= 0x04;
	EA = 1;
}
void Delay100ms()		//@11.0592MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 5;
	j = 52;
	k = 195;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}


void UART_Action(unsigned char *dat, unsigned char len)
{	
	unsigned char str[2] = "0\n";
//UART_SendString(dat, len);
	if(len <= 1)
		return;

	if(len >= 4 && strncmp(dat, "L:V", 3) == 0 && dat[3] >= '0' && dat[3] <= '9')
	{
		LightValue = dat[3] - '0';
		UART_SendString("M:L.V:", 5);
		str[0] = LightValue + '0';
		UART_SendString(str, 2);
	}
	else if(len >= 2 && strncmp(dat, "L:", 2) == 0)
	{
		//UART_SendString(dat, len);
		UART_SendString("N:L.SERR\n", 9);
	}
}
void main()
{
	UART_T1_Init();
	P3M0 = 0x20; P3M1 = 0x00;
	Timer2Init();

	WDT_CONTR = 0x37;
	while(1)
	{
		WDT_CONTR |= 0x10;
		Delay100ms();
		UART_Driver();
	}
}
void Interrupt_Timer2() interrupt 12
{
	static unsigned char PWMTimer = 0;
	//�˴�����Ϊ100us����������1ms�Ĳ�������Ӧ�ģ������ڵȴ�ʱ��������10����
	UART_RxMonitor(1);
	if(LightValue == 0)
		P35 = 0;
	else if(LightValue == 9)
		P35 = 1;
	else
	{
		PWMTimer++; 
		if(PWMTimer >= 100)
			PWMTimer = 0;
		if(PWMTimer % 10 < LightValue)
			P35 = 1;
		else 
			P35 = 0;
	}
}
