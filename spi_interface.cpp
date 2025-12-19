#include "spi_interface.h"
#include <Arduino.h>

SPIInterface::SPIInterface(bool master, uint8_t cs) :
	is_master(master),
	cs_pin(cs),
	spi(nullptr),
	data_ready(false),
	last_packet_time(0),
	packet_start_time(0),
	master_sequence_counter(0)

{
	if (is_master)
	{
		spi = new SPIClass(HSPI);
		spi_settings = SPISettings(SPI_CLOCK_SPEED, SPI_BIT_ORDER, SPI_MODE_CONFIG);
	}
	else
	{
		spi = &SPI;
	}
}

SPIInterface::~SPIInterface()
{
	if (spi && is_master)
	{
		spi->end();
		delete spi;
	}

}

void SPIInterface::begin()
{

	if (is_master)
	{
		begin_master();
	}
	else
	{
		begin_slave();
	}
	Serial.printf("SPI %s initialized on CS:%d\n", is_master ? "Master" : "Slave", cs_pin);
}

void SPIInterface::begin_master()
{
	Serial.println("[MASTER] Initializing SPI Master...");

	//Simple MASTER: CS pin always be controlled
	pinMode(cs_pin, OUTPUT);
	digitalWrite(cs_pin, HIGH);//Test CS Signal
	Serial.println("[DEBUG] Setting CS HIGH");
	Serial.printf("CS Sending Signal: %s\n", (digitalRead(SPI_CS) == LOW) ? "LOW" : "HIGH");

	spi->begin(SPI_SCK, SPI_MISO, SPI_MOSI, -1);


	Serial.println("[MASTER] Testing SPI connection...");

	//Sent a simple bytes
	digitalWrite(cs_pin, LOW);
	Serial.println("[DEBUG] Setting CS LOW");
	Serial.printf("CS Sending Signal: %s\n", (digitalRead(SPI_CS) == LOW) ? "LOW" : "HIGH");
	delayMicroseconds(50);

	spi->beginTransaction(spi_settings);
	uint8_t test_send = 0xAA;
	uint8_t test_receive = spi->transfer(test_send);
	spi->endTransaction();

	delayMicroseconds(50);
	digitalWrite(cs_pin, HIGH);
	Serial.println("[DEBUG] Setting CS HIGH");
	Serial.printf("CS Sending Signal: %s\n", (digitalRead(SPI_CS) == LOW) ? "LOW" : "HIGH");

	Serial.printf("[MASTER] Test transfer: Sent 0x%02X, Received 0x%02X\n", test_send, test_receive);
}

void SPIInterface::begin_slave()
{
	//Simple SLAVE
	pinMode(cs_pin, INPUT_PULLUP);
	SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, cs_pin);
	spi->setBitOrder(SPI_BIT_ORDER);
	spi->setDataMode(SPI_MODE_CONFIG);
	spi->setFrequency(SPI_CLOCK_SPEED);

	Serial.println("SPI Slave ready (polling mode)");
	Serial.printf("[SLAVE] CS pin: %d (state: %d)\n", cs_pin, digitalRead(cs_pin));
}

bool SPIInterface::send(const UartFrame* frame)
{
	if (!frame)
	{
		Serial.println("have not had frame yet");
		return false;
	}

	memcpy(tx_buffer, frame, sizeof(UartFrame));//Chuyen frame vao tx_frame

	if (is_master)
	{
		return send_master();
	}
	else
	{
		return send_slave();
	}
}

bool SPIInterface::send_master()
{
	digitalWrite(cs_pin, LOW);
	Serial.println("[DEBUG] Setting CS LOW");
	Serial.printf("CS Sending Signal: %s\n", (digitalRead(SPI_CS) == LOW) ? "LOW" : "HIGH");
	delayMicroseconds(SPI_CS_DELAY_US * 2);

	spi->beginTransaction(spi_settings);
	spi->transferBytes(tx_buffer, rx_buffer, sizeof(UartFrame));
	spi->endTransaction();

	delayMicroseconds(SPI_CS_DELAY_US * 2);
	digitalWrite(cs_pin, HIGH);
	Serial.println("[DEBUG] Setting CS HIGH");
	Serial.printf("CS Sending Signal: %s\n", (digitalRead(SPI_CS) == LOW) ? "LOW" : "HIGH");

	if (rx_buffer[0] == START_MARKER)
	{
		data_ready = true;
		return true;
	}

	Serial.printf("[MASTER] Invalid response: 0x%02X\n", rx_buffer[0]);
	return false;
}

bool SPIInterface::send_slave()
{
	if (digitalRead(cs_pin) == LOW)
	{
		spi->beginTransaction(SPISettings(SPI_CLOCK_SPEED, SPI_BIT_ORDER, SPI_MODE0));
		for (size_t i = 0; i < sizeof(UartFrame); i++)
		{
			rx_buffer[i] = spi->transfer(tx_buffer[i]);
		}

		spi->endTransaction();

		if (rx_buffer[0] == START_MARKER)
		{
			data_ready = true;
			return true;
		}
	}
	return false;
}

bool SPIInterface::receive(UartFrame* frame)
{
	if (!frame || !data_ready) return false;
	memcpy(frame, rx_buffer, sizeof(UartFrame));
	data_ready = false;
	return true;
}

bool SPIInterface::available() { return data_ready; }