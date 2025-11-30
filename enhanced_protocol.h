#pragma once
#ifndef ENHANCED_PROTOCOL_H
#define ENHANCED_PROTOCOL_H

#include "protocol.h"
#include "communication_interface.h"
#include "auto_switch.h"
#include <SPI.h>

class EnhancedProtocol : public Protocol//Derived Class of Class Protocol
{
private:
	CommunicationInterface* comm_interface;
	AutoSwitchProtocol auto_switch;
	bool auto_switch_enable;

public:
	EnhancedProtocol(bool enable_auto_switch = true);
	virtual ~EnhancedProtocol() = default;

	void set_communication_interface(CommunicationInterface* interface);
	CommunicationInterface* get_comm_interface() { return comm_interface; }

	//Override methods for uinfied interface
	bool send_reliable(UartFrame* frame);
	bool wait_for_ack(uint16_t seq_num, uint32_t timeout_ms);

	//Mode management
	void switch_to_spi(uint8_t cs_pin, SPIClass* spi_instance, uint32_t frequency = 1000000);
	void switch_to_uart(HardwareSerial* serial_port, uint32_t frequency = 115200);
	CommunicationMode get_current_mode() const;

	uint32_t calculate_dynamic_timeout();
	

	//Auto_switch access
	void enable_auto_switch(bool enable) { auto_switch_enable = enable; }
	AutoSwitchProtocol& get_auto_switch() { return auto_switch; }
	void perform_auto_switch();//Manual trigger

private:
	bool send_frame(UartFrame* frame);
};
#endif // !ENHANCED_PROTOCOL_H
