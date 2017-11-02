/*
 * RailComDetector.cpp
 *
 * Created: 02/11/2017 19:14:21
 * Author : VdBer
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#include <string.h>

#include "RailComEncoding.h"

//	USART1 = RailCom
//			 250kBaud, 8N1

//	PB2 = INT2 = /RailComGap

//	RailCom Baud rate = 2500kBaud
#define RAILCOM_BAUDRATE (250000UL)

volatile uint8_t dataCounter = 0;
volatile uint8_t data[8];
volatile bool dataReady = false;

union uAddress
{
	uint16_t	Address;
	struct
	{
		uint8_t AddrHigh;
		uint8_t AddrLow;
	};
};
		
uAddress MyAddress;

ISR(INT2_vect)
{
	if((1 << PINB2) == (PINB & (1 << PINB2)))
	{
		//	Rising Edge --> End of RailCom Gap
		EICRA = (1 << ISC21);	//	INT2 falling Edge Interrupt Request
		UCSR1B &= ~(1 << RXEN1);	//	Receiver Disable
		if (dataCounter > 0)
		{
			dataReady = true;
		}
	}
	else
	{
		//	Falling Edge --> Start of RailCom Gap
		EICRA = (1 << ISC21) | (1 << ISC20);	//	INT2 Rising Edge Interrupt Request
		UCSR1B = (1 << RXCIE1) | (1 << RXEN1);	// Receiver Enable, RX Complete Interrupt Enable
	}
}

ISR(USART1_RX_vect)
{
	data[dataCounter++] = UDR1;
}

int main(void)
{
//    DDRB = 0x00;	//	PortB All Inputs
	 
	EICRA = (1 << ISC21);	//	INT2 falling Edge Interrupt Request
	EIMSK = (1 << INT2);	//	External Interrupt Request 2 Enable
	
	UBRR1 = (F_CPU / (16 * RAILCOM_BAUDRATE)) - 1;	//	Baudrate
	UCSR1C = (3 << UCSZ10);	//	8-bit Data Size, No Parity, 1 Stop-bit
//	UCSR1B = (1 << RXCIE1);
	
	sei();	//	Enable Global Interrupts
		
    while (1) 
    {
		while(!dataReady);
		cli();
		uint8_t RailcomBuffer[8];
		memset(RailcomBuffer, 0, 8);
		uint8_t RailcomCount = dataCounter;
		uint8_t RailComMessage[8];
		memset(RailComMessage, 0, 8);
		dataCounter = 0;
		dataReady = false;
		bool ErrorInFrame = false;
		memcpy(RailcomBuffer, (const void*)data, RailcomCount);
		sei();
		for(uint8_t index = 0; index < RailcomCount; index++)
		{
			RailComMessage[index] = RailComEncoding[RailcomBuffer[index]];
			if(RailComMessage[index] & 0x80)
			{
				ErrorInFrame = true;
			}
		}
		
		if(!ErrorInFrame)
		{
			uint8_t MessageID = (RailComMessage[0] & 0x3C) >> 2;
			switch(MessageID)
			{
				case 0: //	Channel 2 POM
				{
					break;
				}
				
				case 1:	//	app:adr_low
				{
					uint16_t OldAddress = MyAddress.Address;
					MyAddress.AddrLow = ((RailComMessage[0] & 0x03) << 6) + (RailComMessage[1] & 0x3F);
					break;
				}
				
				case 2:	//	app:adr_high
				{
					uint16_t OldAddress = MyAddress.Address;
					MyAddress.AddrHigh = ((RailComMessage[0] & 0x03) << 6) + (RailComMessage[1] & 0x3F);
					break;
				}
				
				case 3:	//	Channel 2 app:ext
				{
					break;
				}
				
				case 7:	//	Channel 2 app:dyn
				{
					break;
				}
				
				case 12:	//	Channel 2 app:subID
				{
					break;
				}
				
				default:
				{
					
				}
			}
		}	// !ErrorInFrame
    }
}

