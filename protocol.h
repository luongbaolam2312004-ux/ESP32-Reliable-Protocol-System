#pragma once
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "crc16.h"
#include "performance.h"

#define START_MARKER 0xAA
#define END_MARKER 0x55
#define MAX_DATA_LEN 128
#define PROTOCOL_VERSION 0x01
#define MAX_RETRIES 3
#define ACK_TIMEOUT_MS 1000

typedef enum
{
	TYPE_DATA = 0x01,
	TYPE_ACK = 0x02,
	TYPE_NACK = 0x03,
	TYPE_PING = 0x04,
	TYPE_PONG = 0x05,
}PacketType;

typedef struct
{
	uint8_t start_marker;
	uint8_t version;
	uint8_t packet_type;
	uint16_t sequence_num;
	uint16_t data_length;
	uint8_t data[MAX_DATA_LEN];
	uint16_t crc16;
	uint8_t end_marker;
}UartFrame;

class Protocol
{
private:
	uint16_t sequence_counter;

protected:
	PerformanceMonitor perf_monitor;

public:
	Protocol();
	virtual ~Protocol() = default;

	//Frame creation & validation
	bool create_frame(PacketType type, const uint8_t* data, uint16_t data_len, UartFrame* frame);
	bool validate_frame(UartFrame* frame);

	//Reliable transmission
	bool send_reliable(UartFrame* frame, HardwareSerial& serial);
	bool wait_for_ack(uint16_t seq_num, HardwareSerial& serial, uint32_t timeout_ms);

	//Performance monitoring
	PerformanceMonitor& get_performance_monitor() { return perf_monitor; }

	//Statistics and utilities
	void print_frame_info(UartFrame* frame);
	void print_statistics();
	uint16_t get_next_sequence();

	//Error tracking
	void record_crc_error() { perf_monitor.crc_error(); }//crc_errors++
	void record_timeout() { perf_monitor.timeout_occurred(); }//timeouts++
	void record_retransmission() { perf_monitor.retransmission_occurred(); }//retransmissions++
	void record_packet_lost(uint16_t seq) { perf_monitor.packet_lost(seq); }//lost_packets++
	void start_packet_timing(uint16_t seq_num) { perf_monitor.start_latency_measurement(seq_num); }
	void end_packet_timing(uint16_t seq_num) { perf_monitor.end_latency_measurement(seq_num); }
};

#endif
