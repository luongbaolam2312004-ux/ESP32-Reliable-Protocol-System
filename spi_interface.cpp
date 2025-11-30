#include "spi_interface.h"
#include <Arduino.h>

SPIInterface::SPIInterface(uint8_t chip_select_pin, SPIClass* spi_instance, uint32_t frequency) :
	cs_pin(chip_select_pin),
	spi(spi_instance),
	spi_frequency(frequency),
	rx_state(STATE_WAITING_START),
	last_transfer_time(0),
	frame_received(false)
{
	memset(rx_buffer, 0, sizeof(UartFrame));
	memset(tx_buffer, 0, sizeof(UartFrame));
}

void SPIInterface::begin()
{
	//Turn on signal light on ESP32
	pinMode(cs_pin, OUTPUT);
	digitalWrite(cs_pin, HIGH);

	//Setting parameter
	if (spi)
	{
		spi->begin();
		spi->setFrequency(spi_frequency);
		spi->setDataMode(SPI_MODE0);
		spi->setBitOrder(MSBFIRST);
	}

	reset_receiver();
}

void SPIInterface::reset_receiver()
{
	//Resetting for taking new packet

	rx_state = STATE_WAITING_START;
	memset(rx_buffer, 0, sizeof(UartFrame));
	memset(tx_buffer, 0, sizeof(UartFrame));
	last_transfer_time = 0;
	frame_received = false;
}

bool SPIInterface::send(const UartFrame* frame)
{
	if (!spi || !frame) return false;

	UartFrame empty_tx;
	memset(&empty_tx, 0, sizeof(UartFrame));
	empty_tx.start_marker = START_MARKER;

	UartFrame rx_frame;
	if (spi_transfer(&empty_tx, &rx_frame))
	{
		if (rx_frame.start_marker == START_MARKER && rx_frame.end_marker == END_MARKER)
		{
			memcpy(&rx_buffer, &rx_frame, sizeof(UartFrame));
			return true;
		}
	}
	return false;
}

bool SPIInterface::available()
{
	if (check_timeout())
	{
		reset_receiver();
		return false;
	}
	return frame_received;//True if framed packet is received
}

bool SPIInterface::spi_transfer(const UartFrame* tx_frame, UartFrame* rx_frame)
{
	if (!spi) return false;

	digitalWrite(cs_pin, LOW);
	delayMicroseconds(5);

	//By checking value of tx_frame and rx_frame, system chooses way for transfering
	if (tx_frame && rx_frame)
	{
		spi->transferBytes((const uint8_t*)tx_frame, (uint8_t*)rx_frame, sizeof(UartFrame));
	}
	else if (tx_frame)
	{
		spi->writeBytes((const uint8_t*)tx_frame, sizeof(UartFrame));
	}
	else if (rx_frame)
	{
		spi->transferBytes(nullptr, (uint8_t*)rx_frame, sizeof(UartFrame));
	}

	digitalWrite(cs_pin, HIGH);
	last_transfer_time = millis();
	return true;//Transfer is complete
}

bool SPIInterface::receive(UartFrame*frame)
{
	if (!spi || !frame) return false;
	
	if (check_timeout())
	{
		reset_receiver();
		return false;
	}

	UartFrame temp_rx;
	memset(&temp_rx, 0, sizeof(UartFrame));

	if (spi_transfer(nullptr, &temp_rx))
	{
		if (temp_rx.start_marker == START_MARKER && temp_rx.end_marker == END_MARKER)
		{
			memcpy(&rx_buffer, &temp_rx, sizeof(UartFrame));
			frame_received = true;
			return true;
		}
		
	}

	frame_received = false;
	return false;
}

static const uint32_t SPI_TIMEOUT_MS = 2000;

bool SPIInterface::check_timeout()
{
	//Checking timeout
	return (millis() - last_transfer_time) > SPI_TIMEOUT_MS;
}