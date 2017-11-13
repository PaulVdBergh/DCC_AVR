/* 
* XpressNetClient.h
*
* Created: 07/11/2017 19:20:19
* Author: VdBer
*/


#ifndef __XPRESSNETCLIENT_H__
#define __XPRESSNETCLIENT_H__

#include <avr/io.h>
#include <avr/interrupt.h>


//	USART0 = XpressNet
//			 RS485, 62500Baud, 9N1

#define XPRESSNET_BAUDRATE	( 62500UL)	//	XpressNet Baud rate = 62500Baud

#ifndef XPRESSNETADDRESS
#define XPRESSNETADDRESS 31
#endif

#include <stdint.h>

#include "ringbuffer.h"

void XpressNetClientSetup(const uint8_t& XpressNetAddress);
		
RingBuffer::BufferStatus XpressNetClientRespond(const uint8_t* const pResponse);

#endif //__XPRESSNETCLIENT_H__
