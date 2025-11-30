#pragma once
#ifndef AUTO_SWITCH_H
#define AUTO_SWITCH_H

//Auto Switch Protocol

#include "performance.h"
#include "communication_interface.h"

class AutoSwitchProtocol
{
private:
	PerformanceMonitor& perf;//Monitoring and measuring performance Class
	float throughput_threshold;
	float max_latency_threshold;
	float max_error_rate;
	unsigned long min_measurement_time;

public:
	AutoSwitchProtocol(PerformanceMonitor& monitor, float threshold = 50.0, float latency_thresh = 100.0, float error_thresh = 5.0);
	bool should_switch_to_spi();
	bool should_switch_to_uart();
	CommunicationMode recommend_mode();


	void set_threshold(float throughput, float latency, float error) 
	{ 
		throughput_threshold = throughput;
		max_latency_threshold = latency;
		max_error_rate = error;
	}
	void get_current_metrics(float& throughput, float& latency, float& error_rate) const;

private:
	bool has_sufficient_data() const;
	float calculate_switch_score() const;

};

#endif // !AUTO_SWITCH_H
