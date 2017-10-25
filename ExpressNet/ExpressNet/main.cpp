/*
 * ExpressNet.cpp
 *
 * Created: 1/10/2017 22:09:37
 * Author : Paul
 */ 

/*
	UART1	= Unix link
	UART0	= RS485
	PD7		= RS485 Direction
*/

#define LINUX_BAUDRATE (62500ul)
#define RS485_BAUDRATE (62500ul)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

volatile bool bMessageReady = false;
volatile uint8_t messageFromLinux[32];

ISR(USART1_RX_vect)
{
	static uint8_t newMsg[32];
	static uint8_t* pNextByte = newMsg;
	*pNextByte++ = UDR1;
	if(pNextByte - newMsg == newMsg[0])
	{
		memcpy((void*)messageFromLinux, newMsg, newMsg[0]);
		bMessageReady = true;
		pNextByte = newMsg;
	}
}

int main(void)
{
	//	initialization
	messageFromLinux[0] = 0x00;
	
	//	PORTD Bit 7 used for RS485 Direction
	PORTD &= !0x80;
	DDRD = 0x80;
	
	//	USART0 used for RS485 comm
	UBRR0 = (F_CPU / ( 16 * RS485_BAUDRATE)) - 1;
	UCSR0C = (0 << USBS0) | (3 << UCSZ00);
	UCSR0B = (1 << UCSZ02) | (1 << RXEN0) | (1 << TXEN0);
	
	//	USART1 used for Linux comm
	UBRR1 = (F_CPU / ( 16 * LINUX_BAUDRATE)) - 1;
	UCSR1C = (0 << USBS1) | (3 << UCSZ10);
	UCSR1B = (1 << RXEN1) | (1 << TXEN1 | 1 << RXCIE1);
	
    uint8_t XPNAddress;
	uint8_t CallByte;
	uint8_t RxData[255];
	uint8_t* pRxData = RxData;
	
	sei();
	
    while (1) 
    {
		for(XPNAddress = 1; XPNAddress < 32; XPNAddress++)
		{
			if (bMessageReady)
			{
				PORTD |= 0x80;
				for(uint8_t i = 1; i < messageFromLinux[0]; i++)
				{
					if(i == 1)
					{
						UCSR0B |= (1 << TXB80);
					}
					while(!(UCSR0A & (1 << UDRE0)));
					UDR0 = messageFromLinux[i];
					if(i == 1)
					{
						UCSR0B &= ~(1 << TXB80);
					}
				}
				while(!(UCSR0A & (1 << TXC0)));
				PORTD &= !0x80;
				UCSR0A |= (1 << TXC0);
				bMessageReady = false;
			} 


			CallByte = 0x40 + XPNAddress;
			for(uint8_t x = 0x40; x!=0; x = x >> 1)
			{
				if(CallByte & x)
				{
					CallByte ^= 0x80;
				}
			}
			PORTD |= 0x80;
			while(!(UCSR0A & (1 << UDRE0)));
			UCSR0B |= (1 << TXB80);
			UDR0 = CallByte;
			while(!(UCSR0A & (1 << TXC0)));
			UCSR0B &= ~(1 << TXB80);
			PORTD &= !0x80;
			UCSR0A |= (1 << TXC0);
			
			uint8_t timeout = 0;
 			do
			{
				if(UCSR0A & (1<<RXC0))
				{
					*pRxData++ = UDR0;
					timeout = 0;
				}
			}
			while(0 < ++timeout);
			
			if(RxData != pRxData)
			{
				uint8_t msgLength = pRxData - RxData;
				uint8_t check = 0;
				for(uint8_t i = 0; i < msgLength; i++)
				{
					check ^= RxData[i];
				}
				if(!check)
				{
					while(!(UCSR1A & (1 << UDRE1)));
					UDR1 = msgLength + 2;
					while(!(UCSR1A & (1 << UDRE1)));
					UDR1 = XPNAddress;
					for(uint8_t x = 0; x < msgLength; x++)
					{
						while(!(UCSR1A & (1 << UDRE1)));
						UDR1 = RxData[x];
					}
				}
				
				pRxData = RxData;
			}
		}
    }
}

