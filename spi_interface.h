#pragma once
#ifndef SPI_INTERFACE_H
#define SPI_INTERFACE_H

#include "communication_interface.h"
#include "protocol.h"
#include <SPI.h>

#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
#define SPI_CS 5

#define SPI_CLOCK_SPEED 1000000//1MHz
#define SPI_MODE_CONFIG SPI_MODE0
#define SPI_BIT_ORDER MSBFIRST
#define SPI_CS_DELAY_US 10

class SPIInterface : public CommunicationInterface
{
private:
	bool is_master;
	uint8_t cs_pin;
	SPIClass* spi;
	SPISettings spi_settings;

	//Buffer for slave response
	uint8_t tx_buffer[sizeof(UartFrame)];
	uint8_t rx_buffer[sizeof(UartFrame)];
	volatile bool data_ready;
	
	uint32_t last_packet_time;
	uint32_t packet_start_time;

	uint16_t master_sequence_counter;

public:
	SPIInterface(bool master_mode = true, uint8_t cs_pin = SPI_CS);
	~SPIInterface();

	//CommunicationInterface implementation
	bool send(const UartFrame* frame) override;
	bool send_master();
	bool send_slave();
	bool receive(UartFrame* frame) override;
	bool available() override;

	CommunicationMode get_mode() override { return MODE_SPI; }
	void begin() override;
	void begin_master();
	void begin_slave();

	void reset_receiver() override { data_ready = false; }
	bool is_connected() const override { return spi != nullptr; }
	uint32_t get_baud_rate() const override { return SPI_CLOCK_SPEED; }
};

#endif // !SPI_INTERFACE_H

