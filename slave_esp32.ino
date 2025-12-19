#include "enhanced_protocol.h"
#include "uart_interface.h"
#include "spi_interface.h"
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LED_BUILTIN 2

//LCD Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);

//UART Configuration
HardwareSerial SerialPort(2);
#define RX2 16
#define TX2 17

//SPI Configuration
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
#define SPI_CS 5

EnhancedProtocol protocol(false);//Slave no need auto_switch
UARTInterface uart_interface(&SerialPort, 115200);
SPIInterface spi_interface(false, SPI_CS);//Slave SPI

static unsigned long last_data_time = 0;

void display_on_lcd(const String& line1, const String& line2 = "")//Hàm hiển thị cho màn lcd
{
    lcd.clear();
    lcd.setCursor(0, 0);

    if(line1.length() <= 16)
    {
        lcd.print(line1);
    }
    else
    {
        lcd.print(line1.substring(0, 16));
    }

    if(line2 != "")
    {
        lcd.setCursor(0, 1);
        if(line2.length() <= 16)
        {
            lcd.print(line2);
        }
        else
        {
            lcd.print(line2.substring(0, 16));
        }
    }
}

void display_performance_metrics()//Hiển thị thông số đo lường chính trên màn lcd 
{
    PerformanceMonitor& perf = protocol.get_performance_monitor();

    String line1 = "T:" + String(perf.get_throughput_kbps(), 1) + "k";
    line1 += " L:" + String(perf.get_average_latency(), 1) + "ms";

    String line2 = "RX:" + String(perf.get_packet_received());
    line2 += " E:" + String(perf.get_crc_errors());

    display_on_lcd(line1, line2);
}

void display_current_mode()
{
    String mode_str = (protocol.get_current_mode() == MODE_UART) ? "UART" : "SPI";
    String line2 = "Mode: " + mode_str;
    display_on_lcd("SLAVE READY", line2);
}

void display_received_data(const String& data, uint16_t seq_num)
{
    String display_line = "RX#" + String(seq_num);
    String data_display = data.substring(0, 12);
    display_on_lcd(display_line, data_display);

    last_data_time = millis();
}

void send_ack(uint16_t seq_num)
{
    UartFrame ack_frame;
    uint8_t empty_data[1] = {0};
    
    if(protocol.create_frame(TYPE_ACK, empty_data, 0, &ack_frame))
    {
        ack_frame.sequence_num = seq_num;
        protocol.get_comm_interface()->send(&ack_frame);

        Serial.print("Sent ACK for frame ");
        Serial.println(seq_num);

        display_on_lcd("Sent ACK", "Seq: " + String(seq_num));//Hiển thị đã gửi ACK lên lcd
    }

}

void send_nack(uint16_t seq_num)
{
    UartFrame nack_frame;
    uint8_t empty_data[1] = {0};
    
    if(protocol.create_frame(TYPE_NACK, empty_data, 0, &nack_frame))
    {
        nack_frame.sequence_num = seq_num;
        protocol.get_comm_interface()->send(&nack_frame);

        Serial.print("Sent NACK for frame ");
        Serial.println(seq_num);

        display_on_lcd("Sent NACK", "Seq: " + String(seq_num));//Hiển thị đã gửi NACK lên lcd
    }

}

void process_received_frame(UartFrame* frame)
{
    Serial.print("<<< SLAVE RECEIVED [");
    Serial.print(protocol.get_current_mode() == MODE_UART ? "UART" : "SPI");
    Serial.print("]: ");
    protocol.print_frame_info(frame);
    Serial.println();

    if(protocol.validate_frame(frame))
    {
        switch(frame->packet_type)
        {
            case TYPE_DATA:
            {
                String received_data = "";
                String mode_info = protocol.get_current_mode() == MODE_UART ? "UART" : "SPI";
                
                for(int i = 0; i < frame->data_length; i++)
                {
                    received_data += (char)frame->data[i];
                }
                Serial.print("Data via ");
                Serial.print(mode_info);
                Serial.print(": ");
                Serial.println(received_data);

                display_received_data(received_data, frame->sequence_num);

                Serial.print("Sending ACK for seq: ");
                Serial.println(frame->sequence_num);
                send_ack(frame->sequence_num);
                break;
            }
            case TYPE_ACK:
                Serial.println("ACK processed - THIS SHOULD NOT HAPPEN ON SLAVE");
                break;
            case TYPE_NACK:
                Serial.println("NACK processed - will retry");
                break;
            case TYPE_PING:
                Serial.println("PING received");
                break;
            case TYPE_PONG:
                Serial.println("PONG received");
                break;
        }
    }
    else
    {
        Serial.println("INVALID FRAME - CRC ERROR");
        Serial.print("Sending NACK for seq: ");
        Serial.println(frame->sequence_num);
        send_nack(frame->sequence_num);
        display_on_lcd("CRC ERROR!", "Seq: " + String(frame->sequence_num));
    }
}

void display_detailed_mode()
{
    CommunicationMode current = protocol.get_current_mode();
    String mode_str = (current == MODE_UART) ? "UART" : "SPI";
    String line1 = "MODE: " + mode_str;

    PerformanceMonitor& perf = protocol.get_performance_monitor();
    String line2 = "T:" + String(perf.get_throughput_kbps(),1) +  "L:" + String(perf.get_average_latency(), 0);

    display_on_lcd(line1, line2);
}

void receive_frames()
{
    UartFrame frame;
    if(protocol.get_comm_interface()->receive(&frame))
    {
        process_received_frame(&frame);
    }
}

void handle_mode_switch()
{
    //Slave just response by master chosen mode
    //But it doesn't switch automatically, it can display notification
    static CommunicationMode last_mode = MODE_UART;
    CommunicationMode current_mode = protocol.get_current_mode();
    
    if(current_mode != last_mode)
    {
        Serial.print("Communication mode changed to: ");
        Serial.println(current_mode == MODE_UART ? "UART" : "SPI");

        display_on_lcd("Mode Changed", current_mode == MODE_UART ? "UART" : "SPI");
        delay(1000);

        last_mode = current_mode;
    }
}

void setup()
{
    Serial.begin(115200);
    SerialPort.begin(115200, SERIAL_8N1, RX2, TX2);

    //CSpin configuration
    pinMode(SPI_CS, OUTPUT);
    digitalWrite(SPI_CS, HIGH);

    //CS pin Configuration
    pinMode(SPI_CS, OUTPUT);
    digitalWrite(SPI_CS, HIGH);
    
    //Initial SPI
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS);

    //Set default interface(UART)
    protocol.set_communication_interface(&spi_interface);
    uart_interface.begin();

    //Khởi tạo cấu hình cho màn lcd
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("ESP32 Enhance Receiver");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");

    unsigned long startWait = millis();

    while(!Serial && (millis() - startWait) < 5000)
    {
        delay(10);
    }

    Serial.println("\nSLAVE ESP32 - RELIABLE PROTOCOL (UART+SPI)");
    Serial.println("====================================");
    Serial.println("Ready to receive data...\n");
    Serial.print("Mode: ");Serial.println((protocol.get_current_mode() == MODE_SPI) ? "SPI" : "UART");
    Serial.println("====================================");

    delay(2000);
    display_current_mode();
}

void loop()
{
    static unsigned long last_stats_display = 0;//Bộ đếm hiển thị
    static unsigned long last_mode_display = 0;
    static unsigned long last_lcd_update = 0;//Bộ đếm update lcd
    static unsigned long last_data_time = 0;
    static unsigned long last_led = 0;//Bộ đếm đèn hiệu
    static bool led_state = false;//Trạng thái đèn hiệu

    //Nhận và xử lí frame
    receive_frames();

    //Mode changing check every 2s
    handle_mode_switch();

    //Display current mode of LCD
    if(millis() -  last_mode_display > 5000)
    {
        display_current_mode();
        last_mode_display = millis();
    }

    //Kiểm tra nếu đã 3s chưa nhận data thì hiển thị metrics
    if(millis() - last_data_time > 3000 && millis() - last_lcd_update > 2000 )
    {
        display_performance_metrics();
        last_lcd_update = millis();
    }

    //Print statistics
    if(millis() - last_stats_display > 20000)
    {
        protocol.print_statistics();
        last_stats_display = millis();
    }

    if(millis() - last_led > 300)
    {
        led_state = !led_state;
        digitalWrite(LED_BUILTIN, led_state);
        last_led = millis();
    }

    delay(10);//Small delay to prevent watchdog timer
}