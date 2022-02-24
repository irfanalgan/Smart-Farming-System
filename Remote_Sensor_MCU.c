#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/wdt.h>
#include <xc.h>
#include <util/delay.h>
#include "lcd.h"


#define F_CPU 8000000
#define BaudRate 9600
#define BR_Calc ((F_CPU/16/BaudRate)-1) //Baudrate calculator

volatile static uint8_t uart_tx_busy = 1;



#define USART_RX_BUFFER_SIZE 2   /* 2,4,8,16,32,64,128 or 256 bytes */
unsigned char USART_RxBuf[USART_RX_BUFFER_SIZE];


unsigned char sensors[4];


unsigned char adc_channel=0;

unsigned char user_rv_index=0;
unsigned char signal;

void USART_init(void);
void USART_Transmit(unsigned char user_data);
void USART_Transmit_Packet(char *data_packet);
void displayStringLCD();
void displayCharLCD(unsigned char data);


unsigned char USART0_Receive( void );

unsigned char hexToChar(unsigned char val);

void adc_Init(void);
void ReadADC(void);
void motor_Init(void);
void send(void);
void timer_con();
void motor_start();
unsigned char level(unsigned char val);
int motor_flag = 0;void Sleep_and_Wait();
void main(){
	
	
	
	DDRD = 0xFF;
	DDRA = 0xFF;
	DDRF = 0;
	DDRC = 0xFF; 
	DDRB = 0xFF;
	USART_init();
	Lcd8_Init();
	
	adc_Init();
	
	sei();
	while(1){
		if (signal == '9')
		{
			while(1){
				if(signal == '4'){
					break;
				}
			}
		}
		else{
			ReadADC();
			timer_con();
			motor_Init();
			motor_start();
		}
		
		
	}
	
	
}
void Sleep_and_Wait(){
	sleep_enable(); // arm sleep mode
	sei(); // global interrupt enable
	sleep_cpu(); // put CPU to sleep
}
void send(void){
	unsigned char T;
	unsigned char M;
	unsigned char W;
	unsigned char B;
	unsigned char *packet = malloc(4 * sizeof(char));
	T = sensors[0]|(1<<7);
	M = sensors[1]|(1<<7)|(1<<5);
	W = sensors[2]|(1<<7)|(1<<6);
	B = sensors[3]|(1<<7)|(1<<6)|(1<<5);
	
	packet[0] = T;
	packet[1] = M;
	packet[2] = W;
	packet[3] = B;
	USART_Transmit_Packet(packet);
}

void motor_Init(){
	DDRC |= (1<<2); 
	OCR0 = 0; 
	TCCR0 = 0x65;
}
unsigned char level(unsigned char val)
{
	float x = val;
	x = (x/32)*255;
	return x;
}

void motor_start(){
	unsigned char moisturelevel = level(sensors[1]);
	DDRC |= (1<<2);
	OCR0 = moisturelevel;
	TCCR0 = 0x65;
}

/* Read and write functions */
void adc_Init(void){
	 ADCSRA = 0x87; // make ADC enable and select ck/128
	 ADMUX = 0x20; // 2.56 V Vref, ADC0 single ended
}

void ReadADC(void)
{
	unsigned char sensor;
	ADCSRA |= (1<<ADSC);
	// Wait for the AD conversion to complete
	while ((ADCSRA & (1<<ADIF))==0);
	ADCSRA |= (1<<ADIF);
	sensor = ADCH;
	sensor = sensor & 0xF8;
	sensor = sensor >>3;
	sensors[adc_channel] = sensor;
	if (adc_channel < 1)
	{
		adc_channel++;
		ADMUX++;
	}
	else
	{
		displayStringLCD();
		send();
		_delay_ms(70000);
		adc_channel = 0;
		ADMUX = 0x20;	
	}
}


unsigned char hexToChar(unsigned char val)
{
	if (val> 0x09)
	val+=0x37;
	else
	val|=0x30;
	return val;
}


void displayStringLCD(){
	unsigned char out[4] = "  \0";
	Lcd8_Set_Cursor(1,0);
	Lcd8_Write_Char('T');
	Lcd8_Set_Cursor(1,1);
	Lcd8_Write_Char('=');
	Lcd8_Set_Cursor(1,3);
	out[0] = hexToChar(sensors[0]>>4);
	out[1] = hexToChar(sensors[0]&0x0F);
	Lcd8_Write_String(out);
	
	Lcd8_Set_Cursor(1,8);
	Lcd8_Write_Char('M');
	Lcd8_Set_Cursor(1,9);
	Lcd8_Write_Char('=');
	Lcd8_Set_Cursor(1,11);
	out[0] = hexToChar(sensors[1]>>4);
	out[1] = hexToChar(sensors[1]&0x0F);
	Lcd8_Write_String(out);
	
	
	Lcd8_Set_Cursor(2,0);
	Lcd8_Write_Char('W');
	Lcd8_Set_Cursor(2,1);
	Lcd8_Write_Char('=');
	Lcd8_Set_Cursor(2,3);
	out[0] = hexToChar(sensors[2]>>4);
	out[1] = hexToChar(sensors[2]&0x0F);
	Lcd8_Write_String(out);
	
	
	Lcd8_Set_Cursor(2,8);
	Lcd8_Write_Char('B');
	Lcd8_Set_Cursor(2,9);
	Lcd8_Write_Char('=');
	Lcd8_Set_Cursor(2,11);
	out[0] = hexToChar(sensors[3]>>4);
	out[1] = hexToChar(sensors[3]&0x0F);
	Lcd8_Write_String(out);
	
}
void displayCharLCD(unsigned char data){
	
	//Lcd8_Write_String(data_packet);
	Lcd8_Set_Cursor(1,5);
	Lcd8_Write_Char(data);

}


void USART_init(void){


	UBRR0H = (BR_Calc >> 8);
	UBRR0L = BR_Calc;
	
	UCSR0B |= (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0) | (1<<TXCIE0);
	UCSR0C |= (1<<UCSZ00)|(1<<UCSZ01);
	
	
	
}

void USART_Transmit(unsigned char user_data) {
	while(uart_tx_busy == 0);
	uart_tx_busy = 0;
	UDR0 = user_data;
	
}

void USART_Transmit_Packet(char *data_packet) {
	uint16_t i = 0;
	
	do{
		
		USART_Transmit(data_packet[i]);
		i++;
		
	}while(data_packet[i] != '\0');
	USART_Transmit(data_packet[i]);

}
void timer_con(){
	DDRA |= (1<<7);
	TCCR1A = 0x00; //00
	TCCR1B = 0x0D; //1D
	OCR1AH = 0x9A; //98
	OCR1AL = 0xB3; //96
	TIMSK = (1<<OCIE1A);
	sei ();

}

ISR(TIMER1_COMPA_vect) {
	motor_flag = !motor_flag;
}
ISR(USART0_RX_vect)
{
	while (!(UCSR0A & (1<<RXC0) )){}; // Double checking flag
	signal = UDR0;
	//user_rv_index++;
	USART_RxBuf[user_rv_index]= signal;
	 

}


ISR(USART0_TX_vect)
{
	uart_tx_busy = 1;
}

