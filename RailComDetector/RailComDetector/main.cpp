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
#include "XpressNetClient.h"

//	USART1 = RailCom
//			 250kBaud, 8N1

//	PB2 = INT2 = /RailComGap

#define RAILCOM_BAUDRATE	(250000UL)	//	RailCom Baud rate = 250kBaud

volatile uint8_t Ch1dataCounter = 0;
volatile uint8_t Ch1RawRailcomMessage[8];
volatile uint8_t Ch2dataCounter = 0;
volatile uint8_t Ch2RawRailcomMessage[8];
volatile bool bCh1RailComDataReady = false;
volatile bool bCh2RailComDataReady = false;
volatile bool bRxError = false;
volatile bool bLowAddressValid = false;
volatile bool bHighAddressValid = false;
volatile bool bAddressResponded = false;
volatile bool bChannel2 = false;

volatile bool bXpressNetMessageFromHostReady = false;
uint8_t XpressNetMessageFromHost[32];


uint8_t RailcomBuffer[8];	//	Message in 4/8 Format
uint8_t RailComMessage[8];	//	Message in 6-bit format

union uAddress
{
	uint16_t	Address;
	struct
	{
		uint8_t AddrHigh;
		uint8_t AddrLow;
	}__attribute__((packed));
};
		
uAddress MyAddress;

volatile bool bOccupied = false;
bool bPreviousOccupied = false;

ISR(INT0_vect)	//	Occupancy detection : enter occupied state
{
	TIFR0 = 0xFF;			//	Clear all pending interrupts from TimerCounter0
	TIMSK0 |= (1 << TOIE0);	//	Enable TC0 Overflow Interrupt
	TCNT0 = 0;				//	start new delay
	bOccupied = true;
}

ISR(INT2_vect)	//	RailComGap detection
{	
	if(EICRA & (1 << ISC20))	//	Rising or Falling edge ?
	{
		//	Rising Edge --> End of RailCom Gap
		EICRA = (2 << ISC20) | (2 << ISC00);	//	INT2 & INT0 falling Edge Interrupt Request
		EIMSK = (1 << INT0) | (1 << INT2);		//	External Interrupt Request 2 and 0 Enable
		UCSR1B &= ~(1 << RXEN1);				//	Receiver Disable
		
		if (!bRxError && (Ch2dataCounter > 0))
		{
			bCh2RailComDataReady = true;
			
			memset(RailcomBuffer, 0, 8);
			memset(RailComMessage, 0, 8);

//			cli();
			uint8_t RailcomCount = Ch2dataCounter;
			Ch2dataCounter = 0;
			bCh2RailComDataReady = false;
			memcpy(RailcomBuffer, (const void*)Ch2RawRailcomMessage, RailcomCount);
//			sei();

			bool ErrorInFrame = false;
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
					case 0:		//	Channel 2 POM
					{
						uint8_t message[] = {0x09, 0x76, 0xE1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
						if(bHighAddressValid && bLowAddressValid)
						{
							message[3] = MyAddress.AddrHigh;
							message[4] = MyAddress.AddrLow;
						}
						message[7] = ((RailComMessage[0] & 0x03) << 6) + (RailComMessage[1] & 0x3F);
						message[8] = message[1] ^ message[2] ^ message[3] ^ message[4] ^ message[5] ^ message[6] ^ message[7];
						XpressNetClientRespond(message);
						break;
					}
												
					case 3:		//	Channel 2 app:ext
					{
						break;
					}
							
					case 7:		//	Channel 2 app:dyn
					{
						break;
					}
							
					case 12:	//	Channel 2 app:subID
					{
						break;
					}
							
					default:
					{
						break;
					}
				}
			}	//	!ErrorInFrame
		}
	
		if (!bRxError && (Ch1dataCounter > 0))
		{
			bCh1RailComDataReady = true;
		
			memset(RailcomBuffer, 0, 8);
			memset(RailComMessage, 0, 8);

			//			cli();
			uint8_t RailcomCount = Ch1dataCounter;
			Ch1dataCounter = 0;
			bCh1RailComDataReady = false;
			memcpy(RailcomBuffer, (const void*)Ch1RawRailcomMessage, RailcomCount);
			//			sei();

			bool ErrorInFrame = false;
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
					case 1:		//	app:adr_low
					{
						MyAddress.AddrLow = ((RailComMessage[0] & 0x03) << 6) + (RailComMessage[1] & 0x3F);
						bLowAddressValid = true;
						break;
					}
				
					case 2:		//	app:adr_high
					{
						MyAddress.AddrHigh = ((RailComMessage[0] & 0x03) << 6) + (RailComMessage[1] & 0x3F);
						bHighAddressValid = true;
						break;
					}
				
					default:
					{
					
					}
				}
			
				if((true == bHighAddressValid) && (true == bLowAddressValid) && (false == bAddressResponded))
				{
					//	Send new address to master
					uint8_t msg[] = {0x08, 0x75, 0xF2, 0x00, 0x01, 0x00, 0x00, 0x00 };
				
					msg[5] = MyAddress.AddrHigh;
					msg[6] = MyAddress.AddrLow;
					msg[7] = msg[1] ^ msg[2] ^ msg[3] ^ msg[4] ^ msg[5] ^ msg[6];
				
					XpressNetClientRespond(msg);
				
					bAddressResponded = true;
				}
			}	// !ErrorInFrame
		}
	}
	else
	{
		//	Falling Edge --> Start of RailCom Gap
		bRxError = false;
		EICRA = (3 << ISC20);					//	INT2 Rising Edge Interrupt Request
		EIMSK = (1 << INT2);					//	this clears INT0 Interrupt enable mask
		UCSR1B = (1 << RXCIE1) | (1 << RXEN1);	//	Receiver Enable, RX Complete Interrupt Enable
		
		bChannel2 = false;
		TIFR2 = 0xFF;							//	Clear all pending interrupts for TC2
		TIMSK2 = (1 << OCIE2B);					//	Timer/Counter2 Output Compare Match B Interrupt Enable
		TCNT2 = 0;								//	Start new delay
	}
}

ISR(TIMER0_OVF_vect)	//	Occupancy detection timeout : enter idle state
{
	TIFR0 = 0xFF;	//	Clear all pending interrupts
	TIMSK0 = 0;		//	Disable further interrupts from timer
	bOccupied = false;
	bHighAddressValid = false;
	bLowAddressValid = false;
	bAddressResponded = false;
}

ISR(TIMER2_COMPB_vect)
{
	bChannel2 = true;
	TIMSK2 = 0;			//	Disable further interrupts from timer
}

ISR(USART1_RX_vect)
{
	uint8_t RxErrors = UCSR1A & ((1 << FE1) | (1 << DOR1) | (1 << UPE1));
	if(bChannel2)
	{
		Ch2RawRailcomMessage[Ch2dataCounter++] = UDR1;
	}
	else
	{
		Ch1RawRailcomMessage[Ch1dataCounter++] = UDR1;
	}
	
	if(RxErrors)
	{
		Ch1dataCounter = Ch2dataCounter = 0;
		bRxError = true;
	}
}

uint8_t resetSource = 0;

int main(void)
{
	resetSource = MCUSR;
	MCUSR = 0;
	
	EICRA = (2 << ISC20) | (2 << ISC00);	//	INT2 and INT0 falling Edge Interrupt Request
	EIMSK = (1 << INT2) | (1 << INT0);	//	External Interrupt Request 2 and 0 Enable

	
	//	USART1 Initialization (RailCom)
	UBRR1 = (F_CPU / (16 * RAILCOM_BAUDRATE)) - 1;	//	Baudrate
	UCSR1C = (3 << UCSZ10);	//	8-bit Data Size, No Parity, 1 Stop-bit
	
	//	TimerCounter0 Initialization (Occupancy Detection)
	TCCR0B = (5 << CS00);	//	1024 x prescaling
	TIMSK0 = (1 << TOIE0);	//	Enable TCO overflow interrupt
	
	//	TimerCounter2 Initialization (RailCom channel1/channel2)
	TCCR2B = (3 << CS20);	//	32 x prescaling
	TCCR2A = (3 << COM2B0);	//	normal counter mode, set OC2B on Compare Match
	OCR2B  = 104;			//	167�S delay
	

	XpressNetClientSetup(XPRESSNETADDRESS);
	
	sei();	//	Enable Global Interrupts
		
    while (1) 
    {
		if(bOccupied != bPreviousOccupied)
		{
			bPreviousOccupied = bOccupied;
			uint8_t IdleMessage[] = {0x6, 0x73, 0xF0, 0x00, 0x01, 0x00};
				
			if(bOccupied)
			{
				IdleMessage[2] = 0xF1;
			}
			
			IdleMessage[5] = IdleMessage[1] ^ IdleMessage[2] ^ IdleMessage[3] ^ IdleMessage[4];
			XpressNetClientRespond(IdleMessage);
			
		}	//	new occupancy state
		
		if(bXpressNetMessageFromHostReady)
		{
			
		}	//	bXpressNetMessageFromHostReady
		
/*
		if(bCh1RailComDataReady)
		{

			memset(RailcomBuffer, 0, 8);
			memset(RailComMessage, 0, 8);

			cli();
			uint8_t RailcomCount = Ch1dataCounter;
			Ch1dataCounter = 0;
			bCh1RailComDataReady = false;
			memcpy(RailcomBuffer, (const void*)Ch1RawRailcomMessage, RailcomCount);
			sei();

			bool ErrorInFrame = false;
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
					case 1:		//	app:adr_low
					{
						MyAddress.AddrLow = ((RailComMessage[0] & 0x03) << 6) + (RailComMessage[1] & 0x3F);
						bLowAddressValid = true;
						break;
					}
				
					case 2:		//	app:adr_high
					{
						MyAddress.AddrHigh = ((RailComMessage[0] & 0x03) << 6) + (RailComMessage[1] & 0x3F);
						bHighAddressValid = true;
						break;
					}
				
					default:
					{
						
					}
				}
				
				if((true == bHighAddressValid) && (true == bLowAddressValid) && (false == bAddressResponded))
				{
					//	Send new address to master
					uint8_t msg[] = {0x08, 0x75, 0xF2, 0x00, 0x01, 0x00, 0x00, 0x00 };
						
					msg[5] = MyAddress.AddrHigh;
					msg[6] = MyAddress.AddrLow;
					msg[7] = msg[1] ^ msg[2] ^ msg[3] ^ msg[4] ^ msg[5] ^ msg[6];
					
					XpressNetClientRespond(msg);
					
					bAddressResponded = true;
				}
			}	// !ErrorInFrame
		}
*/

/*				
		if(bCh2RailComDataReady)
		{

			memset(RailcomBuffer, 0, 8);
			memset(RailComMessage, 0, 8);

			cli();
			uint8_t RailcomCount = Ch2dataCounter;
			Ch2dataCounter = 0;
			bCh2RailComDataReady = false;
			memcpy(RailcomBuffer, (const void*)Ch2RawRailcomMessage, RailcomCount);
			sei();

			bool ErrorInFrame = false;
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
					case 0:		//	Channel 2 POM
					{
						uint8_t message[] = {0x09, 0x76, 0xE1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
						if(bHighAddressValid && bLowAddressValid)
						{
							message[3] = MyAddress.AddrHigh;
							message[4] = MyAddress.AddrLow;
						}
						message[7] = ((RailComMessage[0] & 0x03) << 6) + (RailComMessage[1] & 0x3F);
						message[8] = message[1] ^ message[2] ^ message[3] ^ message[4] ^ message[5] ^ message[6] ^ message[7];
						XpressNetClientRespond(message);
						break;
					}
												
					case 3:		//	Channel 2 app:ext
					{
						break;
					}
							
					case 7:		//	Channel 2 app:dyn
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
			}	//	!ErrorInFrame
		}
*/		
    }
}

