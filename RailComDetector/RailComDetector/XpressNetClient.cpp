/* 
* XpressNetClient.cpp
*
* Created: 07/11/2017 19:20:19
* Author: VdBer
*/

#include "XpressNetClient.h"

uint8_t m_CallByte;
RingBuffer m_Buffer;
volatile uint8_t m_TransmitCounter;

void XpressNetClientSetup(const uint8_t& XpressNetAddress /* = XPRESSNETADDRESS */)
{
	m_CallByte = 0x40 + XpressNetAddress;
	for(uint8_t x = 0x40; x != 0; x >>= 1)
	{
		if(m_CallByte & x)
		{
			m_CallByte ^= 0x80;
		}
	}
	
	//	PORTD bit 7 used for RS485 Direction
	PORTD &= !0x80;
	DDRD = 0x80;
	
	//	USART0 initialization (ExpressNet)
	UBRR0 = (F_CPU / (16 * XPRESSNET_BAUDRATE)) - 1;
	UCSR0A = (1 << MPCM0);
	UCSR0C = (0 << USBS0) | (3 << UCSZ00);
	UCSR0B = (1 << UCSZ02) | (1 << RXEN0) | (1 << RXCIE0);
}

RingBuffer::BufferStatus XpressNetClientRespond(const uint8_t* const pResponse)
{
	RingBuffer::BufferStatus status = RingBuffer::Success;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		uint8_t msgLength = pResponse[0];
		for(uint8_t i = 0; i < msgLength; i++)
		{
			status = m_Buffer.Insert(pResponse[i]);
			if(status != RingBuffer::Success)
			{
				return status;
			}
		}
	}
	return status;
}

ISR(USART0_RX_vect)
{
	uint8_t RxErrors = UCSR0A & ((1 << FE0) | (1 << DOR0) | (1 << UPE0));
	uint8_t data = UDR0;
	if(!RxErrors)
	{
		if(UCSR0A & (1 << MPCM0))
		{
			if(data == m_CallByte)
			{
				if(!(m_Buffer.BufferIsEmpty()))
				{
					if(0 == m_TransmitCounter)
					{
						m_Buffer.Retrieve((uint8_t*)&m_TransmitCounter);
						m_TransmitCounter--;
						uint8_t firstData;
						m_Buffer.Retrieve(&firstData);
						m_TransmitCounter--;
						PORTD |= (1 << PORTD7);
						UCSR0B |= (1 << TXEN0) | (1 << TXCIE0);
						UDR0 = firstData;
					}
				}
			}
			else
			{
				
			}
		}
		else
		{
			//	
		}
	}	//	!RxErrors
	else
	{
		UCSR0A |= (1 << MPCM0);
	}
}

ISR(USART0_TX_vect)
{
	if(m_TransmitCounter)
	{
		uint8_t data;
		m_Buffer.Retrieve(&data);
		m_TransmitCounter--;
		UDR0 = data;
	}
	else
	{
		UCSR0B &= ~((1 << TXEN0) | (1 << TXCIE0));
		PORTD &= ~(1 << PORTD7);
	}
}