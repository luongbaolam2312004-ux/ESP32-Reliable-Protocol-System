#include "auto_switch.h"

AutoSwitchProtocol::AutoSwitchProtocol(PerformanceMonitor& monitor, float throughput_thresh, float latency_thresh, float error_rate) : 
	perf(monitor),
	throughput_threshold(throughput_thresh),
	max_latency_threshold(latency_thresh),
	max_error_rate(error_rate), 
	min_measurement_time(30000) 
{}

bool AutoSwitchProtocol::should_switch_to_spi()
{
	if (!has_sufficient_data()) return false;//Not having enough packets => false

	//Take metrics
	float throughput = perf.get_throughput_kbps();
	float latency = perf.get_average_latency();
	float error_rate = perf.get_error_rate();

	//Switch to SPI condition: high throughtput, low latency, low error rate.
	bool throughput_ok = throughput > throughput_threshold;
	bool latency_ok = latency < max_latency_threshold;
	bool error_ok = error_rate < max_error_rate;

	Serial.print("SPI Switch Check - Throughput: ");
	Serial.print(throughput);
	Serial.print("kbps, Latency: ");
	Serial.print(latency);
	Serial.print("ms, Error: ");
	Serial.print(error_rate);
	Serial.print("% -> ");
	Serial.println(throughput_ok && latency_ok && error_ok ? "YES" : "NO");

	return throughput_ok && latency_ok && error_ok;
}

bool AutoSwitchProtocol::should_switch_to_uart()
{
	if (!has_sufficient_data()) return false;//Not having enough packets => false

	//Take metrics
	float throughput = perf.get_throughput_kbps();
	float error_rate = perf.get_error_rate();

	//UART Switch Condition: low throughput or high error
	bool low_throughput = throughput < (throughput_threshold * 0.7); // Throughput Limit: 70%
	bool high_errors = error_rate > (max_error_rate * 2.0);// Errors x2

	Serial.print("UART Switch Check - Throughput:");
	Serial.print(throughput);
	Serial.print("kbps, Error: ");
	Serial.print(error_rate);
	Serial.print("% -> ");
	Serial.println(low_throughput || high_errors ? "YES" : "NO");

	return low_throughput || high_errors;
}

CommunicationMode AutoSwitchProtocol::recommend_mode()
{
	//Choose mode for conmmunicating
	if (should_switch_to_spi())
	{
		return MODE_SPI;
	}
	else if (should_switch_to_uart())
	{
		return MODE_UART;
	}
	return MODE_UART;//Default mode
}

void AutoSwitchProtocol::get_current_metrics(float& throughput, float& latency, float& error_rate) const
{
	//Take metrics of performance
	throughput = perf.get_throughput_kbps();
	latency = perf.get_average_latency();
	error_rate = perf.get_error_rate();
}

bool AutoSwitchProtocol::has_sufficient_data() const
{
	//perf.get_packet_sent = total_packet_sent
	return perf.get_packet_sent() >= 10;// Take at least 10 packets
}

float AutoSwitchProtocol::calculate_switch_score() const
{
	if (!has_sufficient_data()) return 0.0f;

	//Take metrics
	float throughput = perf.get_throughput_kbps();
	float latency = perf.get_average_latency();
	float error_rate = perf.get_error_rate();

	//Calculate score
	float throughput_score = (throughput / throughput_threshold) * 50.0f;
	float latency_score = (1.0f - (latency / max_latency_threshold)) * 30.0f;
	float error_score = (1.0f - (error_rate / max_error_rate)) *  20.0f;

	//Score Limit
	throughput_score = constrain(throughput_score, 0.0f, 50.0f);
	latency_score = constrain(latency_score, 0.0f, 30.0f);
	error_score = constrain(error_score, 0.0f, 20.0f);

	return throughput_score + latency_score + error_score;
}