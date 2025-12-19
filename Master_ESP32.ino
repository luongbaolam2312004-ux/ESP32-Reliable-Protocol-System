//Code for Master ESP32
#include "enhanced_protocol.h"
#include "uart_interface.h"
#include "spi_interface.h"
#include <SPI.h>

//UART Configuration
HardwareSerial SerialPort(2);
#define RX2 16
#define TX2 17

//SPI Configuration
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
#define SPI_CS 5

//Protocol instance
EnhancedProtocol protocol(true);
UARTInterface uart_interface(&SerialPort, 115200);
SPIInterface spi_interface(true, SPI_CS);//master mode

void send_ack(uint16_t seq_num)//Chuyển data frame thành ACK frame
{
    UartFrame ack_frame;
    uint8_t empty_data[1] = {0};
    
    if(protocol.create_frame(TYPE_ACK, empty_data, 0, &ack_frame))
    {
        ack_frame.sequence_num = seq_num;

        //Use current communication interface
        protocol.get_comm_interface()->send(&ack_frame);

        Serial.print("Sent ACK for frame ");
        Serial.print(seq_num);
    }
}

void send_nack(uint16_t seq_num)//Chuyển data frame thành NACK frame
{
    UartFrame nack_frame;
    uint8_t empty_data[1] = {0};
    
    if(protocol.create_frame(TYPE_NACK, empty_data, 0, &nack_frame))
    {
        nack_frame.sequence_num = seq_num;
        protocol.get_comm_interface()->send(&nack_frame);

        Serial.print("Sent NACK for frame ");
        Serial.println(seq_num);
    }

}

// =============================================== SPI FUNCTIONS ==========================================================
void send_test_data_spi()
{
    static uint16_t spi_test_counter = 0;
    static unsigned long last_throughput_check = 0;

    UartFrame frame;
    String message = "SPI_TEST_" + String(spi_test_counter) + " - Time: " + String(millis());

    if(protocol.create_frame(TYPE_DATA, (uint8_t*)message.c_str(), message.length(), &frame))
    {
        Serial.println("=== SPI MODE TEST ===");
        Serial.print("Sending frame ");
        Serial.print(frame.sequence_num);
        Serial.println(" via SPI");

        bool success = protocol.send_reliable(&frame);

        if(success)
        {
            Serial.println("SPI derivery comfirmed");
            delay(100);
        }
        else
        {
            Serial.println("SPI delivery failed");
        }
         spi_test_counter++;
    }
    else
    {
        Serial.println(("Failed to create SPI frame"));
    }

    if(millis() - last_throughput_check >  5000)
    {
        float throughput = protocol.get_performance_monitor().get_throughput_kbps();
        Serial.print("Current throughput: ");
        Serial.print(throughput);
        Serial.print(" kbps");
        last_throughput_check = millis();
    }
}

/*bool perform_handshake(uint32_t timeout_ms = 2000)
{
	if (!is_master) return false;

	Serial.println("[HANDSHAKE] Attemping to contact slave...");

	uint32_t start_time = millis();
	uint8_t attempts = 0;

	while (millis() - start_time < timeout_ms && attempts < 5)
	{
		//Create ping frame
		UartFrame ping_frame;
		uint8_t ping_data[] = "HANDSHAKE";

		if (create_frame(TYPE_PING, ping_data, sizeof(ping_data) - 1, master_sequence_counter++, &ping_frame))
		{
			digitalWrite(cs_pin, LOW);
			Serial.println("[DEBUG] Setting CS LOW");
			Serial.printf("CS Sending Signal: %s\n", (digitalRead(SPI_CS) == LOW) ? "LOW" : "HIGH");
			delayMicroseconds(50);

			spi->beginTransaction(spi_settings);
			spi->transferBytes((uint8_t*)&ping_frame, rx_buffer, sizeof(SpiFrame));
			spi->endTransaction();

			delayMicroseconds(50);
			digitalWrite(cs_pin, HIGH);
			Serial.println("[DEBUG] Setting CS HIGH");
			Serial.printf("CS Sending Signal: %s\n", (digitalRead(SPI_CS) == LOW) ? "LOW" : "HIGH");

			//Check response
			UartFrame* response = (UartFrame*)rx_buffer;
			if (response->start_marker == START_MARKER && response->packet_type == TYPE_PONG)
			{
				Serial.println("[HANDSHAKE] Slave is ready");
				return true;
			}
		}
		attempts++;
		delay(100);
	}

	Serial.println("[HANDSHAKE] Slave not responding");
	return false;
}

bool SPIInterface::send_and_wait_ack(const UartFrame* frame, uint32_t timeout_ms = ACK_TIMEOUT_MS)
{
	if (!send_frame(frame)) return false;

	uint32_t start_time = millis();
	while (millis() - start_time < timeout_ms)
	{
		if (available())
		{
			UartFrame response;
			if (receive_frame(&response))
			{
				if (validate_frame(&response))
				{
					if (response.packet_type == TYPE_ACK && response.sequence_num == frame->sequence_num)
					{
						performance.ack_received++;
						return true;
					}
					else if (response.packet_type == TYPE_NACK)
					{
						performance.nack_received++;
						return false;
					}
				}
				else
				{
					performance.crc_errors++;
				}
			}
		}
		delay(1);
	}

	performance.timeouts++;
	return false;
}

bool SPIInterface::wait_for_slave_ready(uint32_t timeout_ms)
{
	uint8_t test_pattern[4] = { 0xAA, 0x55, 0xAA, 0x55 };
	uint8_t response[4];

	while (millis() - start_time < timeout_ms)
	{
		digitalWrite(cs_pin, LOW);
		delayMicroseconds(20);

		spi->beginTransaction(spi_settings);
		for (int i = 0; i < 4; i++)
		{
			response[i] = spi->transfer(test_pattern[i]);
		}
		spi->endTransaction();

		delayMicroseconds(20);
		digitalWrite(cs_pin, HIGH);

		//Check response from slave
		if (response[0] == 0x55 && response[1] == 0xAA)
		{
			Serial.println("Slave is ready");
			return true;
		}

		delay(10);
	}
	return false;
}

*/

void test_spi_performance()
{
    Serial.println("STARTING SPI PERFORMANCE TEST");
    Serial.println("=============================");

    for(int i = 0; i < 5; i++)
    {
        send_test_data_spi();
        delay(500);// Send 500ms to test throughput
    }

    Serial.println("=============================");
    Serial.println("SPI PERFORMANCE TEST COMPLETED\n");
}
//=======================================================UART FUNCTIONS ===========================================================================

void send_test_data()//Gửi thông tin data
{
    static uint16_t test_counter = 0;
    static unsigned long last_throughput_check = 0;

    UartFrame frame;
    String message = "Test " +String(test_counter) + " - Time: " + String(millis());

    if(protocol.create_frame(TYPE_DATA, (uint8_t*)message.c_str(), message.length(), &frame))//Tạo message frame
    {
        bool success = protocol.send_reliable(&frame);//Kiểm tra frame

        if(success)
        {
            Serial.print("Reliable delivery comfirmed via ");
            Serial.println(protocol.get_current_mode() == MODE_UART ? "UART" : "SPI");
        }
        else
        {
            Serial.print("Delivery failed for packet ");
            Serial.println(frame.sequence_num);
        }
    }
    test_counter++;

    if(millis() - last_throughput_check >  5000)
    {
        float throughput = protocol.get_performance_monitor().get_throughput_kbps();
        Serial.print("Current throughput: ");
        Serial.print(throughput);
        Serial.print(" kbps");
        last_throughput_check = millis();
    }
}

void receive_frames()//Nhan frame
{
    UartFrame frame;
    if (protocol.get_comm_interface() -> receive(&frame))
    {
        process_received_frame(&frame);
    }
}

//=======================================================COMMON FUNCTIONS ===========================================================================
void process_received_frame(UartFrame* frame)//Xử lí frame đã nhận
{
    Serial.print("<<< MASTER RECEIVED: ");
    protocol.print_frame_info(frame);//In thông tin frame
    Serial.println();

    if(protocol.validate_frame(frame))//Kiểm tra frame
    {
        switch(frame->packet_type)
        {
            case TYPE_DATA:
                Serial.print("Data: ");
                for(int i = 0; i < frame->data_length; i++)
                {
                    Serial.print((char)frame->data[i]);
                }
                Serial.println();
                send_ack(frame->sequence_num);
                break;
            case TYPE_ACK:
                Serial.println("ACK processed");
                break;
            case TYPE_NACK:
                Serial.println("NACK processed - will retry");
                break;
            case TYPE_PING:
                Serial.println("PING processed - signal is up");
                break;
            case TYPE_PONG:
                Serial.println("PONG processed - signal is down");
                break;

        }
    }
    else
    {
        Serial.println("INVALID FRAME");
        send_nack(frame->sequence_num);
    }
}


void check_and_switch_mode()
{
    CommunicationMode recommended = protocol.get_auto_switch().recommend_mode();
    CommunicationMode current = protocol.get_current_mode();

    if(recommended == MODE_SPI && current == MODE_UART)
    {
        Serial.println("Auto-switching to SPI for better performance...");
        protocol.switch_to_spi(SPI_CS, &SPI, 1000000);
    }
    else if (recommended == MODE_UART && current == MODE_SPI)
    {
        Serial.println("Switching back to UART...");
        protocol.switch_to_uart(&SerialPort, 115200);
    }
}

void display_current_mode()
{
    static CommunicationMode last_displayed_mode = MODE_UART;
    CommunicationMode current = protocol.get_current_mode();

    if(current != last_displayed_mode)
    {
        Serial.print("=== CURRENT MODE: ");
        Serial.print(current == MODE_UART ? "UART ===" : "SPI ===");
        last_displayed_mode = current;
    }
}

void setup()
{
    //Initialize  UART
    Serial.begin(115200);//Cho Serial Monitor để Debug
    SerialPort.begin(115200, SERIAL_8N1, RX2, TX2);//Cấu hình cho kết nối UART
    uart_interface.begin();


    //Initialize SPI
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);

    //Set default interface (UART)
    protocol.set_communication_interface(&spi_interface);
    protocol.get_performance_monitor().reset_statistics();

    //Wait for Serial
    unsigned long startWait = millis();
    while(!Serial && (millis() - startWait) < 5000) { delay(10);}//Delay 10 ms

    Serial.println("\n MASTER ESP32 - RELIABLE PROTOCOL");
    Serial.println("========================================");
    Serial.println("Auto-switch: ENABLE");
    Serial.println("Initial Mode: SPI");
    Serial.println("========================================");

    delay(1000);
}

void loop()
{
    static unsigned long last_send = 0;
    static unsigned long last_mode_check = 0;
    static unsigned long last_stats = 0;
    static unsigned long last_mode_display = 0;
    static bool led_state = false;

    //Gửi test data mỗi 2s
    //if(millis() - last_send > 2000){ send_test_data();last_send = millis(); }
    if(millis() - last_send > 2000)
    {
        test_spi_performance();
        last_send = millis();
    }
    
    //Nhận và xử lí frame được nhận
    receive_frames();
    
    //In thông số mỗi 15s
    if(millis() - last_stats > 15000)
    {
        protocol.print_statistics();

        //Display metrics auto-switch
        float throughput, latency, error_rate;
        protocol.get_auto_switch().get_current_metrics(throughput, latency, error_rate);
        Serial.print("Auto-switch Metric - T: ");
        Serial.print(throughput);
        Serial.print("kbps L: ");
        Serial.print(latency);
        Serial.print("ms E: ");
        Serial.print(error_rate);
        Serial.println("%");

        

        last_stats = millis();
    }

    //Auto-switch check every 10s
    /*if(millis() - last_mode_check > 15000)
    {
        check_and_switch_mode();
        last_mode_check = millis();
    }*/

    delay(10);//Delay để tránh watchdog timer
}