/*
 * MT2_transmit.c
 *
 * Created: 4/24/2018 3:50:05 PM
 * Author : kiaer
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "nrf24l01.h"
#define BAUD 9600 //Baud Rate
#define MYUBRR F_CPU/16/BAUD-1 //calculate Baud

void setup_timer(void);
nRF24L01 *setup_rf(void);
volatile bool rf_interrupt = false;
volatile bool send_message = false;
void read_adc(void);  //Read ADC
void adc_init(void);  //initialize ADC
void USART_init( unsigned int ubrr ); //initialize USART
void USART_tx_string(char *data); //Print String USART
volatile unsigned int adc_temp;
char outs[256]; //array
//char CWJAP[256];

volatile char received_data;


int main(void) {
	uint8_t to_address[5] = { 0x20, 0x30, 0x40, 0x51, 0x61 };
	bool on = false;
	adc_init(); // Initialize the ADC (Analog / Digital Converter)
	USART_init(MYUBRR); // Initialize the USART (RS232 interface)
	USART_tx_string("Connected!\r\n"); // shows theres a connection with USART
	_delay_ms(125); // wait a bit
	
	sei();
	nRF24L01 *rf = setup_rf();
	setup_timer();

	while (true) {
		if (rf_interrupt) {
			//	USART_tx_string("Connected1!\r\n");
			rf_interrupt = false;
			int success = nRF24L01_transmit_success(rf);
			if (success != 0)
			nRF24L01_flush_transmit_message(rf);
			//USART_tx_string(rf);
		}
		if (send_message) {
			send_message = false;
			//USART_tx_string("Connected2!\r\n");
			on = !on;
			nRF24L01Message msg;
		//	USART_tx_string(&msg);
			if (on) memcpy(msg.data, "ON", 3);
			else memcpy(msg.data, "OFF", 4);
			USART_tx_string(msg.data);
			read_adc();
				snprintf(outs,sizeof(outs),"%3d F \r\n", adc_temp);// print it
				USART_tx_string(outs);
			//	TCNT1 = 49911; //reset tcnt value for delay
			msg.length = strlen((char *)msg.data) + 1;
			nRF24L01_transmit(rf, to_address, &msg);
			USART_tx_string(msg.data);
		}
	}
	return 0;
}
nRF24L01 *setup_rf(void) {
	nRF24L01 *rf = nRF24L01_init();

rf->ss.port = &PORTB;
rf->ss.pin = PB2;
rf->ce.port = &PORTB;
rf->ce.pin = PB1;
rf->sck.port = &PORTB;
rf->sck.pin = PB5;
rf->mosi.port = &PORTB;
rf->mosi.pin = PB3;
rf->miso.port = &PORTB;
rf->miso.pin = PB4;
// interrupt on falling edge of INT0 (PD2)
EICRA |= _BV(ISC01);
EIMSK |= _BV(INT0);
nRF24L01_begin(rf);
return rf;
}
// setup timer to trigger interrupt every second when at 1MHz
void setup_timer(void) {
	TCCR1B |= _BV(WGM12);
	TIMSK1 |= _BV(OCIE1A);
	OCR1A = 15624;
	TCCR1B |= _BV(CS10) | _BV(CS11);
}
void adc_init(void) //initialize ADC
{
	
	ADMUX = (0<<REFS1)| // Reference Selection Bits

	(1<<REFS0)| // AVcc - external cap at AREF
	(0<<ADLAR)| // ADC Left Adjust Result
	(0<<MUX2)| // ANalog Channel Selection Bits
	(1<<MUX1)| // ADC2 (PC2 PIN25)
	(0<<MUX0);

	ADCSRA = (1<<ADEN)| // ADC ENable

	(0<<ADSC)| // ADC Start Conversion
	(0<<ADATE)| // ADC Auto Trigger Enable
	(0<<ADIF)| // ADC Interrupt Flag
	(0<<ADIE)| // ADC Interrupt Enable
	(1<<ADPS2)| // ADC Prescaler Select Bits
	(0<<ADPS1)|
	(1<<ADPS0);



}


void read_adc(void) {
	unsigned char i =4;
	adc_temp = 0; //initialize
	while (i--) {
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & (1<<ADSC));
		adc_temp+= ADC;
		_delay_ms(50);
	}
	adc_temp = adc_temp / 4; // Average a few samples

}

/* INIT USART (RS-232) */
void USART_init( unsigned int ubrr ) {
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B |= (1 << TXEN0) | (1 << RXEN0)| ( 1 << RXCIE0); // Enable receiver, transmitter & RX interrupt
	UCSR0C |= (1<<UCSZ01) | (1 << UCSZ00);
}

void USART_tx_string( char *data ) {
	while ((*data != '\0')) {
		while (!(UCSR0A & (1 <<UDRE0)));
		UDR0 = *data;
		data++;
	}
}

// each one second interrupt
ISR(TIMER1_COMPA_vect) {
	send_message = true;
}
// nRF24L01 interrupt
ISR(INT0_vect) {
	rf_interrupt = true;
}
