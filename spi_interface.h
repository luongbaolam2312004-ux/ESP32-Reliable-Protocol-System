#pragma once
#ifndef SPI_INTERFACE_H
#define SPI_INTERFACE_H

#include "communication_interface.h"
#include <SPI.h>

class SPIInterface : public CommunicationInterface
{
private:
	uint8_t cs_pin;
	SPIClass* spi;
	uint32_t spi_frequency;

	ReceiverState rx_state;
	uint8_t rx_buffer[sizeof(UartFrame)];
	uint8_t tx_buffer[sizeof(UartFrame)];
	unsigned long last_transfer_time;
	bool frame_received;

public:
	SPIInterface(uint8_t chip_select_pin, SPIClass* spi_instance, uint32_t frequency = 1000000);//1MHz

	//CommunicationInterface implementation
	bool send(const UartFrame* frame) override;
	bool receive(UartFrame* frame) override;
	bool available() override;

	CommunicationMode get_mode() override { return MODE_SPI; }
	void begin() override;
	void reset_receiver() override;

	//SPI-specific methods
	uint32_t get_baud_rate() const override { return spi_frequency; }
	bool is_connected() const override { return spi != nullptr; }

private:
	bool spi_transfer(const UartFrame* tx_frame, UartFrame* rx_frame);
	bool check_timeout();
};

#endif // !SPI_INTERFACE_H

