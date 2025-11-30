#include "uart_interface.h"
#include <Arduino.h>

UARTInterface::UARTInterface(HardwareSerial* serial_port, uint32_t baud) : serial(serial_port), baud_rate(baud), rx_state(STATE_WAITING_START), rx_index(0) 
{
	memset(rx_buffer, 0, sizeof(UartFrame));
}

void UARTInterface::begin()
{
	if (serial)
	{
		serial->begin(baud_rate, SERIAL_8N1);
		//UART already initialized in main code, just set buffer size
		serial->setRxBufferSize(512);
	}
	reset_receiver();
}

void UARTInterface::reset_receiver()
{
	//Resetting for new UART transfer
	rx_state = STATE_WAITING_START;
	rx_index = 0;
	memset(rx_buffer, 0, sizeof(UartFrame));
	last_byte_time = 0;
}

bool UARTInterface::send(const UartFrame* frame)
{
	if (!serial) return false;

	size_t bytes_written = serial->write((uint8_t*)frame, sizeof(UartFrame));
	serial->flush();
	return bytes_written == sizeof(UartFrame);//Sending completed
}

bool UARTInterface::receive(UartFrame* frame)
{
	//Using Receiver State Machine for receiving
	
	if (!serial) return false;

	while (serial->available())
	{
		uint8_t byte = serial->read();
		last_byte_time = millis();

		switch (rx_state)
		{
		case STATE_WAITING_START:
			if (byte == START_MARKER)
			{
				rx_index = 0;
				rx_buffer[rx_index++] = byte;
				rx_state = STATE_RECEIVING_FRAME;
			}
			break;

		case STATE_RECEIVING_FRAME:
			rx_buffer[rx_index++] = byte;

			if (rx_index >= sizeof(UartFrame))
			{
				//Copy to output frame
				memcpy(frame, rx_buffer, sizeof(UartFrame));

				if (frame->end_marker == END_MARKER)
				{
					rx_state = STATE_WAITING_START;
					return true;//Valid frame
				}
				else
				{
					Serial.println("Invalid end marker");
					reset_receiver();
					return false;//framing error
				}
			}
			break;
		}
	}

	if (rx_state == STATE_RECEIVING_FRAME && check_timeout())
	{
		reset_receiver();
	}

	return false;//No complete frame available yet
}

bool UARTInterface::available()
{
	return serial->available() > 0;//UART Mode is ready
}

bool UARTInterface::check_timeout()
{
	return (millis() - last_byte_time) > 500;
}