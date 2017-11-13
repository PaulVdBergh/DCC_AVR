/*
 * ringbuffer.cpp
 *
 * Created: 08/11/2017 11:08:12
 *  Author: VdBer
 */ 

#include "ringbuffer.h"

RingBuffer::RingBuffer()
:	m_iBegin(0),
	m_iEnd(0),
	m_bEmpty(true)
{
}

RingBuffer::BufferStatus RingBuffer::Insert(uint8_t data)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if((m_iEnd == m_iBegin) && !m_bEmpty)
		return BufferFull;

		m_pBuffer[m_iEnd++] = data;

		if(m_iEnd > SIZE - 1)
		m_iEnd = 0;

		m_bEmpty = false;
		
		return Success;
	}
}

RingBuffer::BufferStatus RingBuffer::Retrieve(uint8_t* pData)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if(m_bEmpty)
		{
			return BufferEmpty;
		}
		
		*pData = m_pBuffer[m_iBegin];
		
		if(++m_iBegin > SIZE - 1)
		{
			m_iBegin = 0;
		}
		
		if(m_iBegin == m_iEnd)
		{
			m_bEmpty = true;
		}
		
		return Success;
	}
}

const bool RingBuffer::BufferIsEmpty(void)
{
	return m_bEmpty;
}

const bool RingBuffer::BufferIsFull(void)
{
	return ((m_iEnd == m_iBegin) && !m_bEmpty);
}