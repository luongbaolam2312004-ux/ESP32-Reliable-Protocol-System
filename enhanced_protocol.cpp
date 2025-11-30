#include "enhanced_protocol.h"
#include "uart_interface.h"
#include "spi_interface.h"
#include <Arduino.h>

EnhancedProtocol::EnhancedProtocol(bool enable_auto_switch) : 
	comm_interface(nullptr), 
	auto_switch(perf_monitor), 
	auto_switch_enable(enable_auto_switch) 
{}

void EnhancedProtocol::set_communication_interface(CommunicationInterface* interface)
{
	// For changing mode

	if (comm_interface)
	{
		comm_interface->reset_receiver();//Reset old interface
	}

	comm_interface = interface;
	if (comm_interface)
	{
		comm_interface->reset_receiver();//Reset new interface
		Serial.print("Communication interface set to: ");
		Serial.println(comm_interface->get_mode() == MODE_UART ? "UART" : "SPI");
	}
}

CommunicationMode EnhancedProtocol::get_current_mode() const
{
	return comm_interface ? comm_interface->get_mode() : MODE_UART;
}

bool EnhancedProtocol::send_reliable(UartFrame* frame)
{
	if (!comm_interface)
	{
		Serial.println("ERROR: No communication interface set!");
		return false;
	}

	int retries = MAX_RETRIES;
	while (retries > 0)
	{
		//Start timing
		start_packet_timing(frame->sequence_num);

		//Send frame
		if (!send_frame(frame))
		{
			record_retransmission();
			retries--;
			continue;
		}

		Serial.print("Sent frame ");
		Serial.print(frame->sequence_num);
		Serial.print(" via ");
		Serial.print(get_current_mode() == MODE_UART ? "UART" : "SPI");
		Serial.println(", waiting for ACK...");

		//Wait for ACK
		uint32_t dynamic_timeout = calculate_dynamic_timeout();
		if (wait_for_ack(frame->sequence_num, dynamic_timeout))
		{
			Serial.println("ACK received - SUCCESS");
			
			if (auto_switch_enable)
			{
				perform_auto_switch();
			}
			return true;
		}

		//Timeout - retry
		retries--;
		record_retransmission();//retransmissions++
		Serial.print("Timeout - Retransmitting frame ");
		Serial.print(frame-> sequence_num);
		Serial.print(", attemps left:  ");
		Serial.println(retries);
	}

	record_timeout();//timeouts++
	record_packet_lost(frame->sequence_num);//lost_packets++
	return false;
}

bool EnhancedProtocol::send_frame(UartFrame* frame)
{
	if (!comm_interface) return false;
	return comm_interface->send(frame);
}

bool EnhancedProtocol::wait_for_ack(uint16_t seq_num, uint32_t timeout_ms)
{
	//For checking packet type and send to Master
	if (!comm_interface) return false;

	uint32_t start_time = millis();
	while (millis() - start_time < timeout_ms)
	{
		if (comm_interface->available())
		{
			UartFrame response;
			if (comm_interface->receive(&response))
			{
				if (validate_frame(&response))
				{
					if (response.packet_type == TYPE_ACK && response.sequence_num == seq_num)
					{
						end_packet_timing(seq_num);
						Serial.println("VALID ACK RECEIVED");
						return true;
					}
					else if (response.packet_type == TYPE_NACK && response.sequence_num == seq_num)
					{
						Serial.println("NACK RECEIVED");
						return false;
					}
				}
			}
		}
		delay(1);
	}
	return false;
}

uint32_t EnhancedProtocol::calculate_dynamic_timeout()
{
	//For calculating base timeout

	uint32_t base_timeout = ACK_TIMEOUT_MS;

	PerformanceMonitor& perf = const_cast<EnhancedProtocol*>(this)->get_performance_monitor();
	float avg_latency = Protocol::perf_monitor.get_average_latency();

	if (get_current_mode() == MODE_SPI)
	{
		base_timeout = 1500;
	}

	if (avg_latency > 100.0)
	{
		base_timeout += 1000;
	}

	return base_timeout;
}

void EnhancedProtocol::perform_auto_switch()
{
	//For recommending mode for send packets
	if (!auto_switch_enable || !comm_interface) return;

	CommunicationMode recommended = auto_switch.recommend_mode();
	CommunicationMode current = get_current_mode();

	if (recommended != current)
	{
		Serial.print("Auto-switch recommended: ");
		Serial.println(recommended == MODE_UART ? "UART" : "SPI");
	}
}

void EnhancedProtocol::switch_to_spi(uint8_t cs_pin, SPIClass* spi_instance, uint32_t frequency)
{
	//For switching mode to SPI

	static SPIInterface spi_interface(cs_pin, spi_instance,frequency);
	spi_interface.begin();
	set_communication_interface(&spi_interface);
	Serial.println("Switched to SPI mode");
}

void EnhancedProtocol::switch_to_uart(HardwareSerial* serial_port, uint32_t baud_rate)
{
	//For switching mode to UART

	static UARTInterface uart_interface(serial_port,baud_rate);
	uart_interface.begin();
	set_communication_interface(&uart_interface);
	Serial.println("Switched to UART mode");
}