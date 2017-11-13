/*
 * ringbuffer.h
 *
 * Created: 08/11/2017 10:29:53
 *  Author: VdBer
 */ 


#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <util/atomic.h>

#define  SIZE 256

class RingBuffer
{
	public:
		enum BufferStatus
		{
			Success,
			BufferFull,
			BufferEmpty
		};
		
		RingBuffer();
		
		BufferStatus Insert(uint8_t data);
		BufferStatus Retrieve(uint8_t* pData);

		const bool BufferIsEmpty(void);
		const bool BufferIsFull(void);
		
	protected:
	
	private:
		volatile uint8_t	m_iBegin;
		volatile uint8_t 	m_iEnd;
		volatile bool		m_bEmpty;

		uint8_t		m_pBuffer[SIZE];
};

#endif /* RINGBUFFER_H_ */