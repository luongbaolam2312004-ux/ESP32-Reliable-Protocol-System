#pragma once
#ifndef COMMUNICATION_INTERFACE_H
#define COMMUNICATION_INTERFACE_H

#include "protocol.h"

//Communication Mode
enum CommunicationMode
{
	MODE_UART = 0x01,
	MODE_SPI = 0x02
};

//Receiver State Machine
enum ReceiverState
{
	STATE_WAITING_START,
	STATE_RECEIVING_FRAME
};

class CommunicationInterface
{
public:
	virtual ~CommunicationInterface() {};

	//Core communication methods
	virtual bool send(const UartFrame* frame) = 0;
	virtual bool receive(UartFrame* frame) = 0;
	virtual bool available() = 0;

	//Mode and configuration
	virtual CommunicationMode get_mode() = 0;
	virtual void begin() = 0;
	virtual void reset_receiver() = 0;

	//Performance monitoring
	virtual uint32_t get_baud_rate() const = 0;
	virtual bool is_connected() const = 0;
	
};

#endif // !COMMUNICATION_INTERFACE_H
