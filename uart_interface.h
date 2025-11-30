#pragma once
#ifndef UART_INTERFACE_H
#define UART_INTERFACE_H

#include "communication_interface.h"
#include <HardwareSerial.h>

class UARTInterface : public CommunicationInterface 
{
private:
	HardwareSerial* serial;
	uint32_t baud_rate;

	//State Machine Variables
	ReceiverState rx_state;
	uint8_t rx_buffer[sizeof(UartFrame)];
	uint8_t rx_index;
	unsigned long last_byte_time;

public:
	UARTInterface(HardwareSerial* serial_port,uint32_t baud = 115200);

	//CommunicationInterface implement
	bool send(const UartFrame* frame) override;
	bool receive(UartFrame* frame) override;
	bool available() override;

	CommunicationMode get_mode() override { return MODE_UART; }
	void begin() override;
	void reset_receiver() override;

	uint32_t get_baud_rate() const override { return baud_rate; }
	bool is_connected() const override { return serial != nullptr; }

private:
	bool check_timeout();
};

#endif
