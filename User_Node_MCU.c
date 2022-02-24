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


#define KEY_PRT 	PORTC
#define KEY_DDR		DDRC
#define KEY_PIN		PINC


#define F_CPU 8000000
#define BaudRate 9600
#define BR_Calc ((F_CPU/16/BaudRate)-1) //Baudrate calculator


#define USART_RX_BUFFER_SIZE 128   /* 2,4,8,16,32,64,128 or 256 bytes */
char USART_RxBuf[USART_RX_BUFFER_SIZE] = "\0";

unsigned char new_user_read_char; // reception one character at a time
unsigned char user_rv_index=-1;
#define ERROR "Wrong Input!"
#define Welcome "WELCOME"
#define MENU1_0 "                    "
#define MENU1_1 "1-30ms"
#define MENU1_2 "2-250ms"
#define MENU1_3 "3-500ms"

#define MENU2_1 "4-0.5s"
#define MENU2_2 "5-1s"
#define MENU2_3 "6-2s"

#define MENU3_1 "7-MD"
#define MENU3_2 "8-LE"
#define MENU3_3 "9-Rest"


volatile static uint8_t uart_tx_busy = 1;
unsigned char input;

unsigned char keypad[4][3] =
{'3','2','1',
	'6','5','4',
	'9','8','7',
'#','0','*'};

unsigned char colloc, rowloc;
unsigned char menuCount = 0;


void USART_init(void);
void USART_Transmit(unsigned char user_data);
void USART_Transmit_Packet(char *data_packet);

void displayStringLCD(char *data_packet,unsigned char flag);
void displayCharLCD(unsigned char data);
void displayMenu();

char keyfind(unsigned char input_array);
void displayResult(void);
void inputCheck(void);
void UserTrBufferInit();
unsigned char hexToChar(unsigned char val);

void main(){
	char *MENU0 = malloc(20 * sizeof(char));
	strcpy(MENU0,MENU1_0);

	DDRD = 0xFF;
	DDRA = 0xFF;
	Lcd8_Init();
	USART_init();
	
	sei();
	while(1){
		displayMenu();
		input = keyfind(input);
		inputCheck();
		_delay_ms(10000);
		Lcd8_Set_Cursor(2,0);
		Lcd8_Write_String(MENU0);
		USART_Transmit(input);
		_delay_ms(1000);
		//displayResult();
		displayResult();
		_delay_ms(1000);
		UserTrBufferInit();
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

void inputCheck(){
	char *error = malloc(20 * sizeof(char));
	strcpy(error,ERROR);
	char *MENU0 = malloc(20 * sizeof(char));
	strcpy(MENU0,MENU1_0);
	while(1){
		if(menuCount == 0){
			if(input == '1'){
				displayCharLCD(input);
				menuCount++;
				break;
			}
			else if(input == '2'){
				displayCharLCD(input);
				menuCount++;
				break;
			}
			else if(input == '3'){
				displayCharLCD(input);
				menuCount++;
				break;
			}
			else if(input == '0'){
				displayCharLCD(input);
				menuCount++;
				break;
			}
			else{
				Lcd8_Set_Cursor(2,0);
				Lcd8_Write_String(error);
			}
		}
		else if(menuCount == 1){
			if(input == '4'){
				displayCharLCD(input);
				menuCount++;
				break;
			}
			else if(input == '5'){
				displayCharLCD(input);
				menuCount++;
				break;
			}
			else if(input == '6'){
				displayCharLCD(input);
				menuCount++;
				break;
			}
			else if(input == '0'){
				displayCharLCD(input);
				menuCount++;
				break;
			}
			else{
				Lcd8_Set_Cursor(2,0);
				Lcd8_Write_String(error);
			}
		}
		else if(menuCount == 2){
			if(input == '7'){
				displayCharLCD(input);
				break;
			}
			else if(input == '8'){
				displayCharLCD(input);
				break;
			}
			else if(input == '9'){
				displayCharLCD(input);
				break;
			}
			else{
				Lcd8_Set_Cursor(2,0);
				Lcd8_Write_String(error);
			}
		}
		input = keyfind(input);
		Lcd8_Set_Cursor(2,0);
		Lcd8_Write_String(MENU0);
	}
	if(input == '9'){
		menuCount = 0;
		Lcd8_Set_Cursor(3,0);
		Lcd8_Write_String(MENU0);
		Lcd8_Set_Cursor(4,0);
		Lcd8_Write_String(MENU0);
		_delay_ms(10000);
	}
}
void displayResult(void){
	unsigned char out[4] = "  \0";
	unsigned char i ;
	unsigned char size = strlen(USART_RxBuf);
	Lcd8_Set_Cursor(3,0);
	if (input == '7'){
		while(i <size){
			out[0] = hexToChar(USART_RxBuf[i]>>4);
			out[1] = hexToChar(USART_RxBuf[i]&0x0F);
			Lcd8_Write_String(out);
			i++;
			_delay_ms(500);
		}
	}
	else if (input == '8'){
		Lcd8_Set_Cursor(4,0);
		out[0] = hexToChar(USART_RxBuf[size-1]>>4);
		out[1] = hexToChar(USART_RxBuf[size-1]&0x0F);
		Lcd8_Write_String(out);
		_delay_ms(500);
	}
	
	
	
	
}
void UserTrBufferInit(){
	unsigned char i=0;
	while(i<128){
		USART_RxBuf[i]='\0';
		i++;
	}
	user_rv_index =-1;
	
}


void displayMenu(){
	char *MENU1 = malloc(20 * sizeof(char));
	char *MENU2 = malloc(20 * sizeof(char));
	char *MENU3 = malloc(20 * sizeof(char));
	char *MENU0 = malloc(20 * sizeof(char));
	
	strcpy(MENU0,MENU1_0);
	Lcd8_Set_Cursor(1,0);
	Lcd8_Write_String(MENU0);
	if(menuCount == 0){
		strcpy(MENU1,MENU1_1);
		strcpy(MENU2,MENU1_2);
		strcpy(MENU3,MENU1_3);
		Lcd8_Set_Cursor(1,0);
		Lcd8_Write_String(MENU1);
		Lcd8_Set_Cursor(1,6);
		Lcd8_Write_String(MENU2);
		Lcd8_Set_Cursor(1,13);
		Lcd8_Write_String(MENU3);
		
		
	}
	else if(menuCount == 1){
		strcpy(MENU1,MENU2_1);
		strcpy(MENU2,MENU2_2);
		strcpy(MENU3,MENU2_3);
		Lcd8_Set_Cursor(1,0);
		Lcd8_Write_String(MENU1);
		Lcd8_Set_Cursor(1,7);
		Lcd8_Write_String(MENU2);
		Lcd8_Set_Cursor(1,13);
		Lcd8_Write_String(MENU3);
	}
	else if(menuCount == 2){
		strcpy(MENU1,MENU3_1);
		strcpy(MENU2,MENU3_2);
		strcpy(MENU3,MENU3_3);
		Lcd8_Set_Cursor(1,0);
		Lcd8_Write_String(MENU1);
		Lcd8_Set_Cursor(1,6);
		Lcd8_Write_String(MENU2);
		Lcd8_Set_Cursor(1,12);
		Lcd8_Write_String(MENU3);
	}
	
	
	
	
}
void displayStringLCD(char *data_packet,unsigned char flag){
	if(flag == 1){
		Lcd8_Set_Cursor(1,0);
		Lcd8_Write_String(data_packet);
	}
	if(flag == 0){
		Lcd8_Set_Cursor(2,0);
		Lcd8_Write_String(data_packet);
	}
	if(flag == 2){
		Lcd8_Set_Cursor(2,10);
		Lcd8_Write_String(data_packet);
	}
	
	
}
void displayCharLCD(unsigned char data){
		Lcd8_Set_Cursor(2,0);
		Lcd8_Write_Char(data);

}


void USART_init(void){
	
	
	UBRR0H = (BR_Calc >> 8);
	UBRR0L = BR_Calc;
	
	
	UCSR0B |= (1<<RXEN0)  | (1<<TXEN0) | (1<<RXCIE0) | (1<<TXCIE0);
	UCSR0C |=(1<<UCSZ00)|(1<<UCSZ01);

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


char keyfind(unsigned char input_array)
{
	while(1)
	{
		KEY_DDR = 0xF0;           /* set port direction as input-output */
		KEY_PRT = 0xFF;

		do
		{
			KEY_PRT &= 0x0F;      /* mask PORT for column read only */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F); /* read status of column */
		}while(colloc != 0x0F);
		
		do
		{
			do
			{
				_delay_ms(20);             /* 20ms key debounce time */
				colloc = (KEY_PIN & 0x0F); /* read status of column */
				}while(colloc == 0x0F);        /* check for any key press */
				
				_delay_ms (40);	            /* 20 ms key debounce time */
				colloc = (KEY_PIN & 0x0F);
			}while(colloc == 0x0F);

			/* now check for rows */
			KEY_PRT = 0xEF;            /* check for pressed key in 1st row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 0;
				break;
			}

			KEY_PRT = 0xDF;		/* check for pressed key in 2nd row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 1;
				break;
			}
			
			KEY_PRT = 0xBF;		/* check for pressed key in 3rd row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 2;
				break;
			}

			KEY_PRT = 0x7F;		/* check for pressed key in 4th row */
			asm("NOP");
			colloc = (KEY_PIN & 0x0F);
			if(colloc != 0x0F)
			{
				rowloc = 3;
				break;
			}
		}

		if(colloc == 0x0E)
		return(keypad[rowloc][0]);
		else if(colloc == 0x0D)
		return(keypad[rowloc][1]);
		else if(colloc == 0x0B)
		return(keypad[rowloc][2]);
		else
		return(keypad[rowloc][3]);
		
	}



ISR(USART0_RX_vect)
{
	while (!(UCSR0A & (1<<RXC0) )){}; // Double checking flag
	new_user_read_char = UDR0;
	user_rv_index++;
	USART_RxBuf[user_rv_index]= new_user_read_char;
		
		

}

ISR(USART0_TX_vect)
{
	uart_tx_busy = 1;
}
