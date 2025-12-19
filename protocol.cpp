#include "protocol.h"
#include <Arduino.h>
#include <cstring>

Protocol::Protocol() : sequence_counter(0)
{
	sequence_counter = 0;
}

uint16_t Protocol::get_next_sequence()
{
	uint16_t seq = sequence_counter;
	sequence_counter = (sequence_counter + 1) % 65535;
	return seq;
}

bool Protocol::create_frame(PacketType type, const uint8_t* data, uint16_t data_len, UartFrame* frame)
{
	if (data_len > MAX_DATA_LEN || !frame) return false;

	frame->start_marker = START_MARKER;
	frame->version = PROTOCOL_VERSION;
	frame->packet_type = type;
	frame->sequence_num = get_next_sequence();
	frame->data_length = data_len;
	frame->end_marker = END_MARKER;

	//Clear and copy data
	memset(frame->data, 0, MAX_DATA_LEN);
	if (data_len > 0)
	{
		memcpy(frame->data, data, data_len);
	}

	//Calculate CRC
	uint16_t data_part_size = 1 + 1 + 2 + 2 + data_len;//Version + type + seq + len + data
	frame->crc16 = CRC16::calculate((uint8_t*)&frame->version, data_part_size);

	perf_monitor.packet_sent(sizeof(UartFrame));
	return true;
}

bool Protocol::validate_frame(UartFrame* frame)
{
	if (!frame) return false;

	//Check marker
	if (frame->start_marker != START_MARKER || frame->end_marker != END_MARKER)
	{
		return false;
	}

	//Verify CRC
	uint16_t data_part_size = 1 + 1 + 2 + 2 + frame->data_length;
	uint16_t calculated_crc = CRC16::calculate((uint8_t*)&frame->version, data_part_size);

	if (calculated_crc != frame->crc16)
	{
		record_crc_error();
		return false;
	}

	perf_monitor.packet_received(sizeof(UartFrame));
	return true;
}

bool Protocol::send_reliable(UartFrame* frame, HardwareSerial& serial)
{
	int retries = MAX_RETRIES;
	
	while (retries > 0)
	{
		//Start timing
		start_packet_timing(frame->sequence_num);

		//Send frame
		serial.write((uint8_t*)frame, sizeof(UartFrame));
		serial.flush();
		delay(2);

		Serial.print("Sent frame ");
		Serial.print(frame->sequence_num);
		Serial.println(", waiting for ACK...");

		if (wait_for_ack(frame->sequence_num, serial, ACK_TIMEOUT_MS))
		{
			Serial.println("ACK received - SUCCESS");
			return true;
		}

		//Timeout - retry
		retries--;
		record_retransmission();
		Serial.print("Timeout - Retransmitting frame ");
		Serial.print(frame->sequence_num);
		Serial.print(", Attemps left: ");
		Serial.println(retries);
	}

	record_timeout();
	record_packet_lost(frame->sequence_num);
	return false;
}

bool Protocol::wait_for_ack(uint16_t seq_num, HardwareSerial& serial, uint32_t timeout_ms)
{
	uint32_t start_time = millis();

	while (millis() - start_time < timeout_ms)
	{
		if (serial.available() >= sizeof(UartFrame))
		{
			UartFrame response;
			serial.readBytes((uint8_t*)&response, sizeof(UartFrame));

			Serial.print("Received frame - Type: ");
			Serial.print(response.packet_type);
			Serial.print(", Seq: ");
			Serial.print(response.sequence_num);
			Serial.print(", Expected Seq: ");
			Serial.println(seq_num);

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
				else
				{
					Serial.println("UNEXPECTED FRAME TYPE OR SEQUENCE");
				}
			}
			else
			{
				Serial.println("INVALID FRAME CRC");
			}
		}
		delay(1);
	}
	Serial.println("ACK TIMEOUT");
	return false;
}

void Protocol::print_frame_info(UartFrame* frame)
{
	Serial.print("Frame[");
	Serial.print(frame->sequence_num);
	Serial.print("] Type:");

	switch (frame->packet_type)
	{
	case TYPE_DATA: Serial.print("DATA"); break;
	case TYPE_ACK: Serial.print("ACK"); break;
	case TYPE_NACK: Serial.print("NACK"); break;
	case TYPE_PING: Serial.print("PING"); break;
	case TYPE_PONG: Serial.print("PONG"); break;
	default: Serial.print("UNKNOWN"); break;
	}

	Serial.print(" Len: "); Serial.print(frame->data_length);
	Serial.print(" CRC: 0x"); Serial.print(frame->crc16, HEX);
	Serial.print(" Valid: "); Serial.print(validate_frame(frame) ? "YES" : "NO");
}

void Protocol::print_statistics()
{
	perf_monitor.print_statistics();
}