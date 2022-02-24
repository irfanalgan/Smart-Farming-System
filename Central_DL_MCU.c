#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/wdt.h>
#include <xc.h>
#include <util/delay.h>

#define F_CPU 8000000
#define BaudRate 9600
#define BR_Calc ((F_CPU/16/BaudRate)-1) //Baudrate calculator
#define IntMemStrt 0x500
#define RX_BUFFER_SIZE 255
#define EEPROM_ADDRESS 0x00

#define ERROR "Wrong Input!"

volatile static uint8_t uart0_tx_busy = 1;
volatile static uint8_t uart1_tx_busy = 1;

unsigned char MST_WD;
unsigned char SLV_WD;
unsigned char lastAddressOfEEPROM;

unsigned char T;
unsigned char M;
unsigned char W;
unsigned char B;
unsigned char flag = 0;

#define USER_TR_BUFFER_SIZE 128
#define USER_RV_BUFFER_SIZE 2
#define SENSOR_RV_BUFFER_SIZE 5
#define SENSOR_TR_BUFFER_SIZE 2

#define USART_RX_BUFFER_SIZE 128   /* 2,4,8,16,32,64,128 or 256 bytes */
char USART0_RxBuf[USART_RX_BUFFER_SIZE] = "\0";


#define USART_TX_BUFFER_SIZE 128     /* 2,4,8,16,32,64,128 or 256 bytes */
#define USART_RX_BUFFER_MASK ( USART_RX_BUFFER_SIZE - 1 )
#define USART_TX_BUFFER_MASK ( USART_TX_BUFFER_SIZE - 1 )

volatile unsigned char USART_RxHead;
volatile unsigned char USART_RxTail;
volatile unsigned char USART_TxHead;
volatile unsigned char USART_TxTail;

unsigned char USART1_RxBuf[USART_RX_BUFFER_SIZE];
volatile unsigned char USART1_RxHead;
volatile unsigned char USART1_RxTail;
volatile unsigned char USART1_TxHead;
volatile unsigned char USART1_TxTail;

///////////////////////////////
unsigned char user_rv_buffer;
unsigned char user_rv_index = 0; // keeps track of character index in buffer

char user_tr_buffer[USER_TR_BUFFER_SIZE] = "";
unsigned char user_tr_index = 0; // keeps track of character index in buffer

unsigned char sensor_rv_buffer[SENSOR_RV_BUFFER_SIZE] = "";
unsigned char sensor_rv_index=-1;

unsigned char sensor_tr_buffer[SENSOR_TR_BUFFER_SIZE] = "";;
unsigned char sensor_tr_index=-1;

unsigned char new_user_read_char; // reception one character at a time
unsigned char new_sensor_read_char; //

///////////////////////////


unsigned char data[3200];
unsigned short dataPointer = 0;
unsigned char TOS = 0;
unsigned char TOS_STATE;
unsigned short timerCounter = 0;

char saved;
char wdSaved;
char watchDogSetting;



unsigned char delayValue;
unsigned char interruptCounter=0;

void promtUserWD();

char ReadFromEEPROM(short address);
void WriteToEEPROM(short address, char data);

void USART1_Transmit(unsigned char user_data);
void USART1_Transmit_Packet(char *data_packet);

void USART0_Transmit(unsigned char user_data);
void USART0_Transmit_Packet(char *data_packet);

void USART0_Receive( void );
void USART1_Receive( void );

void USART_init (void);

void ConfigMasterWD(void);
void ConfigSlaveWD(void);
void MemoryDump(void);
void LastEntry(void);
void Restart(void);

void timerStart(char timer);
void timerStop(char timer);


void Sleep_and_Wait(void);
void UserTrBufferInit(void);
void sensorTBufferInit(void);
void start(void);
unsigned char input;

int main(void)
{
	char *MENU0 = malloc(20 * sizeof(char));
	strcpy(MENU0,ERROR);
	input = '1';
	lastAddressOfEEPROM = 4;
	USART_init();
	_delay_ms(10000);
	sei();
	//USART1_Transmit(input);
	while(1){
		USART_init();
		sei();
		Sleep_and_Wait();
		promtUserWD();
		_delay_ms(5000);
	}
	
}
void start(void){
	_delay_ms(500);
}

void promtUserWD(){
	
	unsigned char i = 0;
	
	
	if(USART0_RxBuf[0] == '0'){
		watchDogSetting = 0x00;
	}
	else if(USART0_RxBuf[0] == '1' || USART0_RxBuf[0] == '2' || USART0_RxBuf[0] == '3' ){
		ConfigMasterWD();
	}
	else if(USART0_RxBuf[0] == '4' || USART0_RxBuf[0] == '5' || USART0_RxBuf[0] == '6' ){
		ConfigSlaveWD();
		USART1_Transmit('1');
	} 
	else if(USART0_RxBuf[0] == '7'){
		
		MemoryDump();
	}
	else if(USART0_RxBuf[0] == '8'){
		
		LastEntry();
	}
	else if(USART0_RxBuf[0] == '9'){
		Restart();
	}
	
	if(sensor_rv_buffer != "\0"){
		while(i < 4){
			WriteToEEPROM((EEPROM_ADDRESS + lastAddressOfEEPROM),sensor_rv_buffer[i]);
			i++;
			lastAddressOfEEPROM++;
		}
	}
	
	WriteToEEPROM(EEPROM_ADDRESS, 0x00);
	WriteToEEPROM(EEPROM_ADDRESS + 1, (timerCounter&0xFF00)>>8);
	WriteToEEPROM(EEPROM_ADDRESS + 2, (timerCounter&0x00FF));
	WriteToEEPROM(EEPROM_ADDRESS + 3, watchDogSetting);
	timerCounter = 65536 - timerCounter;
}
char ReadFromEEPROM(short address){
	while((EECR & (1<<EEWE)) == 2)
	_delay_ms(1);
	EEARH = address&0xFF00;
	EEARL = address&0x00FF;
	EECR = (1<<EERE);
	return EEDR;
}

void WriteToEEPROM(short address, char data){
	while((EECR & (1<<EEWE)) == 2)
	_delay_ms(1);
	
	EEARH = address&0xFF00;
	EEARL = address&0x00FF;
	
	EEDR = data;
	EECR = (1<<EEMWE);
	EECR = (1<<EEWE);
}

void MemoryDump(){
	unsigned char i = 0;
	
	while(1){
		if(lastAddressOfEEPROM == 4){
			strcpy(user_tr_buffer,"No data");
			USART0_Transmit_Packet(user_tr_buffer);
			break;
		}
		else{
			user_tr_buffer[i] = ReadFromEEPROM(EEPROM_ADDRESS + (i+4));
			i++;
			if((i+4) == lastAddressOfEEPROM){
				USART0_Transmit_Packet(user_tr_buffer);
				break;
			}
		}
	}
}

void LastEntry(){
	if(lastAddressOfEEPROM == 4){
		strcpy(user_tr_buffer,"No data");
		USART0_Transmit('a');
	}
	else{
		USART0_Transmit(ReadFromEEPROM(EEPROM_ADDRESS + (lastAddressOfEEPROM)));
	}
	
}
void Restart(){
	unsigned char i=lastAddressOfEEPROM;
	while(i != 0){
		WriteToEEPROM(EEPROM_ADDRESS + (i),'\0');
		i--;
	}
	USART1_Transmit('0');
}
void ConfigSlaveWD(){
	if(user_rv_buffer == '4'){
		WDTCR |= ((1<<WDE) | (1<<WDP2) | (1<<WDP0));
		UserTrBufferInit();
		WriteToEEPROM(EEPROM_ADDRESS + 3, 0x1F4); //0.45s
	}
	else if(user_rv_buffer == '5'){
		WDTCR |= ((1<<WDE) | (1<<WDP2)| (1<<WDP1));
		UserTrBufferInit();
		WriteToEEPROM(EEPROM_ADDRESS + 3, 0x3E8); //0.9s
	}
	else if(user_rv_buffer == '6'){
		WDTCR |= ((1<<WDE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0));
		UserTrBufferInit();
		WriteToEEPROM(EEPROM_ADDRESS + 3, 0x7D0); //1.8
	}
}
void ConfigMasterWD(){
	while(interruptCounter==0);
	if(user_rv_buffer =='1'){
		WDTCR |= ((1<<WDE) | (1<<WDP0));
		UserTrBufferInit();
		WriteToEEPROM(EEPROM_ADDRESS + 3, 0x1E);  //30ms
	}
	else if(user_rv_buffer =='2'){
		WDTCR |= ((1<<WDE) | (1<<WDP2));
		UserTrBufferInit();
		WriteToEEPROM(EEPROM_ADDRESS + 3, 0xFA);	//250ms
	}
	else if(user_rv_buffer =='3'){
		WDTCR |= ((1<<WDE) | (1<<WDP2) | (1<<WDP0));
		UserTrBufferInit();
		WriteToEEPROM(EEPROM_ADDRESS + 3, 0x1F4); //500ms
	}
}
//wait until came interrupt
void Sleep_and_Wait(){
	
	sleep_enable(); // arm sleep mode
	sei(); // global interrupt enable
	sleep_cpu(); // put CPU to sleep
	sleep_disable(); // disable sleep once an interrupt wakes CPU up
}
void UserTrBufferInit(){
	unsigned char i=0;
	while(i<128){
		user_tr_buffer[i]='\0';
		i++;
	}
	user_tr_index =0;
	
}

void sensorTBufferInit(){
	unsigned char i=0;
	while(i<128){
		sensor_tr_buffer[i]='\0';
		i++;
	}
	sensor_tr_index =0;
	

}

void USART_init(void){
	
	UBRR0H = (BR_Calc >> 8);
	UBRR0L = BR_Calc;
	
	
	UCSR0B |= (1<<RXEN0)  | (1<<TXEN0) | (1<<RXCIE0) | (1<<TXCIE0);
	UCSR0C |=(1<<UCSZ00)|(1<<UCSZ01);
	
	
	UBRR1H = (BR_Calc >> 8);
	UBRR1L = BR_Calc;
	
	UCSR1B = (1<<RXEN1)  | (1<<TXEN1) | (1<<RXCIE1) | (1<<TXCIE1);
	UCSR1C = (1<<UCSZ10) | (1<<UCSZ11);
	
}




void USART1_Transmit(unsigned char user_data) {
	while(uart1_tx_busy == 0);
	uart1_tx_busy = 0;
	UDR1 = user_data;
	
}

void USART1_Transmit_Packet(char *data_packet) {
	uint16_t i = 0;
	
	do{
		
		USART1_Transmit(data_packet[i]);
		i++;
		
	}while(data_packet[i] != '\0');
	USART1_Transmit(data_packet[i]);

}

void USART0_Transmit(unsigned char user_data) {
	while(uart0_tx_busy == 0);
	uart0_tx_busy = 0;
	UDR0 = user_data;
	
}

void USART0_Transmit_Packet(char *data_packet) {
	uint16_t i = 0;
	
	do{
		
		USART0_Transmit(data_packet[i]);
		i++;
		
	}while(data_packet[i] != '\0');
	USART0_Transmit(data_packet[i]);

}


ISR(USART0_RX_vect)
{
	while (!(UCSR0A & (1<<RXC0) )){}; // Double checking flag
	new_user_read_char = UDR0;
	//user_rv_index++;
	USART0_RxBuf[user_rv_index]= new_user_read_char;
	
}

ISR(USART1_RX_vect)
{
	while (!(UCSR1A & (1<<RXC1) )){}; // Double checking flag
	
	new_sensor_read_char = UDR1;
	sensor_rv_index ++;
	sensor_rv_buffer[sensor_rv_index]=new_sensor_read_char;
}


ISR(USART1_TX_vect){
	uart1_tx_busy = 1;
}

ISR(USART0_TX_vect){
	uart0_tx_busy = 1;
}
