/*
 * MIDTERM_2.c
 *
 * Created: 5/01/2019 10:06:39 PM
 * Author : victor
 */ 

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "I2C_MASTER_C.h"
#include "APDS_9960_LIB.h"

void USART_tx_string(char *data); 										// where it prints string USART
void USART_init( unsigned int ubrr ); 									// where USART initializes
void RGB_dect();														// used to read the colors and put their values
void APDS_9960_init();													// begins to use APDS sensor 

uint16_t baud_rate = MYUBRR;		// Int declaration
uint8_t red_low, red_high;		//
uint8_t blue_low, blue_high;	//
uint8_t green_low, green_high;	//
uint8_t config;					//

char outs[256];															// room we have to print

int main(void)
{
	uint16_t red = 0;														// Declare red
	uint16_t green = 0;														// Declare green
	uint16_t blue = 0;														// Declare blue
		
	i2c_init();																// i2C function call
	USART_init(MYUBRR);														// uart initialization function call
	APDS_9960_init();														// initialize 9960 sensor function call
	
	unsigned char AT_CHECK[] = "AT\r\n"; 									// AT Commands
	unsigned char CWMODE_SET[] = "AT+CWMODE=1\r\n"; 						// Set MODE
	unsigned char CWJAP_LOGIN[] = "AT+CWJAP=\"iPXSMax\",\"12345678\"\r\n"; 	// MUST CHANGE WIFI AND PASSWORD
	unsigned char CIPMUX_SET[] = "AT+CIPMUX=0\r\n";
	unsigned char CIPSTART_SEND[] = "AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n";
	unsigned char CIPSEND_FINISH[] = "AT+CIPSEND=100\r\n";

	_delay_ms(2500);
	USART_tx_string(AT_CHECK); 													//send commands
	_delay_ms(6000);
	USART_tx_string(CWMODE_SET); 												//set mode
	_delay_ms(6000);
	USART_tx_string(CWJAP_LOGIN); 												//connect to Wifi
	_delay_ms(16000);
	
		
	while (1)
	{
		
	///////////////////////// SENDING RED, GREEN, BLUE READINGS/////////////////////////
		
			USART_tx_string(CIPMUX_SET); 												//select MUX
			_delay_ms(10000);
			USART_tx_string(CIPSTART_SEND);												//connect TCP
			_delay_ms(10000);
			USART_tx_string(CIPSEND_FINISH);											//send size
			_delay_ms(6000);
		
			RGB_dect(&red, &blue, &green);												// Call colors function
		
			snprintf(outs,sizeof(outs),"GET https://api.thingspeak.com/update?api_key=5HICIFGYJQ1XUU90&field1=0%05u&field2=%05u&field3=%05u\r\n", red,green,blue); // print it
			USART_tx_string(outs);														//send data
			_delay_ms(10000);	
	}
}
/* INIT USART (RS-232) */
void USART_init( unsigned int ubrr ) {
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B |= (1 << TXEN0) | (1 << RXEN0)| ( 1 << RXCIE0); 				// Enable receiver, transmitter & RX interrupt
	UCSR0C |= (1<<UCSZ01) | (1 << UCSZ00);
}

void USART_tx_string( char *data ) 										// used to print on to the screen
{
	while ((*data != '\0')) {
		while (!(UCSR0A & (1 <<UDRE0)));
		UDR0 = *data;
		data++;
	}
}

void RGB_dect(uint16_t *red, uint16_t *blue, uint16_t *green)	 // Function used to read the colors for sensor (red, blue and green)
{
	i2c_readReg(APDS_WRITE, APDS9960_RDATAL, &red_low, 1);
	i2c_readReg(APDS_WRITE, APDS9960_RDATAH, &red_high, 1);
	i2c_readReg(APDS_WRITE, APDS9960_GDATAL, &green_low, 1);
	i2c_readReg(APDS_WRITE, APDS9960_GDATAH, &green_high, 1);
	i2c_readReg(APDS_WRITE, APDS9960_BDATAL, &blue_low, 1);
	i2c_readReg(APDS_WRITE, APDS9960_BDATAH, &blue_high, 1);
	*red = red_high << 8 | red_low;
	*blue = blue_high << 8 | blue_low;
	*green = green_high << 8 | green_low;
	
}

void APDS_9960_init()	// Function used to initialize the Sensor only for the RGB, no gesture or proximity used
{
	i2c_readReg(APDS_WRITE, APDS9960_ID, &config,1);
	
	if(config != APDS9960_ID_1)
	while(1)
	{
		config = 1 << 1 | 1 << 0 | 1 << 3 | 1 << 4;
	}
	
	i2c_writeReg(APDS_WRITE, APDS9960_ENABLE, &config, 1);
	config = DEFAULT_ATIME;
	
	i2c_writeReg(APDS_WRITE, APDS9960_ATIME, &config, 1);
	config = DEFAULT_WTIME;
	
	i2c_writeReg(APDS_WRITE, APDS9960_WTIME, &config, 1);
	config = DEFAULT_PROX_PPULSE;
	
	i2c_writeReg(APDS_WRITE, APDS9960_PPULSE, &config, 1);
	config = DEFAULT_POFFSET_UR;
	
	i2c_writeReg(APDS_WRITE, APDS9960_POFFSET_UR, &config, 1);
	config = DEFAULT_POFFSET_DL;
	
	i2c_writeReg(APDS_WRITE, APDS9960_POFFSET_DL, &config, 1);
	config = DEFAULT_CONFIG1;
	
	i2c_writeReg(APDS_WRITE, APDS9960_CONFIG1, &config, 1);
	config = DEFAULT_PERS;
	
	i2c_writeReg(APDS_WRITE, APDS9960_PERS, &config, 1);
	config = DEFAULT_CONFIG2;
	
	i2c_writeReg(APDS_WRITE, APDS9960_CONFIG2, &config, 1);
	config = DEFAULT_CONFIG3;
	
	i2c_writeReg(APDS_WRITE, APDS9960_CONFIG3, &config, 1);
}

