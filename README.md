# ESP32-Reliable-Protocol-System
# System Structure:
- Protocol: Default Protocol (UART).
- EnhancedProtocol: Enhanced Protocol with Auto-switch.
- PerformanceMonitor: Monitoring and measuring metrics.
- AutoSwitchProtocol: UART/SPI auto-switching.
- UARTInterface: UART communicating interface.
- SPIInterface: SPI communicating interface.
- CRC16: Data error detection.

# Header Files:
- protocol.h
- enhanced_protocol.h
- performance.h
- auto_switch.h
- communication_inferface.h
- uart_interface.h
- spi_interface.h
- crc16.h

# Implementation Files:
- protocol.cpp
- enhanced_protocol.cpp
- performance.cpp
- auto_switch.cpp
- uart_interface.cpp
- spi_interface.cpp
- crc16.cpp

# Application Files:
- master_esp32.ino: For ESP32 Master.
- slave_esp32.ino: For ESP32 Slave.

# Main Features:
Reliable transmission:
- Frame Structure: Start marker + packet type + sequence number + data + CRC + end marker.
- Error Detection: CRC16 for error detecting.
- ACK/NACK: Comfirming and retransmitting mechanism.
- Retransmission: Automatically retransmitting when system loses packets (3 times max).

Performance Monitoring:
- Throughput (kbps).
- Latency (ms): Including average, min, max value.
- Jitter: Latency fluctuations.
- Error Rate.
- Success Rate.
- Packet Loss.

Auto-switch Protocol:
- UART -> SPI: When Throughput increases, Latency and Error Rate decreases.
- SPI -> UART: When Error Rate increases twice or Throughput decreases by 70%.
- Dynamic Threshold: Configurable switching threshold.
- Manual Override: Switching mode can be forced by button.

Display Statistics:
- LCD I2C 16x2: Displaying real-time metrics and mode.
- Serial Monitor: Debug and print statistics
- LED Indicator: System activating status

# Configuration
Master ESP32:
- Auto-switch enabled
- UART Baudrate: 115200 Hz
- SPI Baudrate: 2 MHz
- Default inteface: UART
- UART Connection: GPIO 16 for RX, 17 for TX.
- SPI Connection: GPIO 23 for MOSI, 19 for MISO, 18 for SCK, 2 for CS.

Slave ESP32:
- Auto-switch disabled
- UART Baudrate: 115200 Hz
- SPI Baudrate: 2 MHz
- Default inteface: UART
- UART Connection: GPIO 16 for RX, 17 for TX.
- SPI Connection: GPIO 23 for MOSI, 19 for MISO, 18 for SCK, 2 for CS.

Packet Types:
- TYPE_DATA (0x01): Default data.
- TYPE_ACK (0x02): Acknowledgement data.
- TYPE_NACK (0x03): Negative acknowledgement data.
- TYPE_STATS_REQUEST (0x04): Request for statistics.
- TYPE_STATS_RESPONSE (0x04): Response for statistics.
