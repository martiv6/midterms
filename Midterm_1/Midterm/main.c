/*
 * Midterm.c
 *
 * Created: 4/1/2019 1:47:30 PM
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

void read_adc(void);  													// where the adc is read
void adc_initializer(void);  											// where the ADC initializes
void USART_init( unsigned int ubrr ); 									// where USART initializes
void USART_tx_string(char *data); 										// where it prints string USART
volatile unsigned int room_temp;										// where we hold the value of the ADC
char outs[256];															// room we have to print

int main(void) {

	adc_initializer();													// Initialize the ADC (Analog / Digital Converter)
	USART_init(MYUBRR); 												// Initialize the USART (RS232 interface)
	_delay_ms(500); 													// wait a bit
	sei(); 																// global interrupt

	while(1)
	{
	}
}

void adc_initializer(void) 												//initialize ADC
{
	
	ADMUX = (0<<REFS1)| (1<<REFS0)|(0<<ADLAR)| 							// Reference Selection Bits
	(0<<MUX2)| (1<<MUX1)| (0<<MUX0);									// avcc - external cap at AREF
																		// ADC Left Adjust Result
																		// Analog Channel Selection Bits
																		// PC2 where we read from

	ADCSRA = (1<<ADEN)| (0<<ADSC)| (0<<ADATE)| (0<<ADIF)|				// ADC ENable
	(0<<ADIE)| (1<<ADPS2)|(0<<ADPS1)| (1<<ADPS0);
																		// ADC Start Conversion
																		// ADC Auto Trigger Enable
																		// ADC Interrupt Flag
																		// ADC Interrupt Enable
																		// ADC Prescaler Select Bits
																		// ADC Prescaler select bits
																		// ADC input
																		// Timer/Counter1 Interrupt Mask Register
	TIMSK1 |= (1<<TOIE1); 												// enable overflow interrupt
	TCCR1B |= (1<<CS12)|(1<<CS10); 										// clock
	TCNT1 = 49911; 														//((16MHz/1024)*1)-1 = 15624

}

void read_adc(void)														// initialize to begin to read from the sensor
{
	unsigned char i =4;													// to keep track of input
	room_temp = 0;
	while (i--) {
		ADCSRA |= (1<<ADSC);										    // used to start convo with chips
		while(ADCSRA & (1<<ADSC));
		room_temp+= ADC;
		_delay_ms(50);
	}
	room_temp = room_temp / 8; 											// gets correct value displayed
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

ISR(TIMER1_OVF_vect) 														// timer overflow interrupt to delay for 1 second
{
	unsigned char AT_CHECK[] = "AT\r\n"; 									// AT Commands
	unsigned char CWMODE_SET[] = "AT+CWMODE=1\r\n"; 						// Set MODE
	unsigned char CWJAP_LOGIN[] = "AT+CWJAP=\"MY_WIFI\",\"12345678\"\r\n"; 	// MUST CHANGE WIFI AND PASSWORD
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
	USART_tx_string(CIPMUX_SET); 												//select MUX
	_delay_ms(10000);
	USART_tx_string(CIPSTART_SEND);												//connect TCP
	_delay_ms(10000);
	USART_tx_string(CIPSEND_FINISH);											//send size
	_delay_ms(6000);
	
	read_adc(); 																//read ADC
	snprintf(outs,sizeof(outs),"GET https://api.thingspeak.com/update?api_key=XA1ZWJOD2BRUEVKD&field2=%3d\r\n", room_temp);// print it
	USART_tx_string(outs);														//send data
	_delay_ms(10000);
	TCNT1 = 49911; 																//reset
}