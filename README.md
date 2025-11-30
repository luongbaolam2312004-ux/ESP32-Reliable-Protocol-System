# ESP32-Reliable-Protocol-System
System Structure:
- Protocol: Default Protocol (UART)
- EnhancedProtocol: Enhanced Protocol with Auto-switch
- PerformanceMonitor: Monitoring and measuring metrics
- AutoSwitchProtocol: UART/SPI auto-switching
- UARTInterface: UART communicating interface
- SPIInterface: SPI communicating interface
- CRC16: Checking and validating data

* Header Files:
- protocol.h
- enhanced_protocol.h
- performance.h
- auto_switch.h
- communication_inferface.h
- uart_interface.h
- spi_interface.h
- crc16.h

* Implementation Files:
- protocol.cpp
- enhanced_protocol.cpp
- performance.cpp
- auto_switch.cpp
- uart_interface.cpp
- spi_interface.cpp
- crc16.cpp

* Application Files:
- master_esp32.ino: For ESP32 Master
- slave_esp32.ino: For ESP32 Slave

* Main Features:
- Reliable transmission
- Performance Monitoring
- Auto-switch Protocol
- Display Statistics
