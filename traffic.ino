/*
  Web client

  This sketch connects to a website (http://www.google.com)
  using the WiFi module.

  This example is written for a network using WPA encryption. For
  WEP or WPA, change the WiFi.begin() call accordingly.

  This example is written for a network using WPA encryption. For
  WEP or WPA, change the WiFi.begin() call accordingly.


  created 13 July 2010
  by dlf (Metodo2 srl)
  modified 31 May 2012
  by Tom Igoe

  Find the full UNO R4 WiFi Network documentation here:
  https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples#wi-fi-web-client
 */

#include "WiFiS3.h"
#include <Servo.h>
#include "ArduinoJson.h"

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(17, 18);              // SPI 버스에 nRF24L01 라디오를 설정하기 위해 CE, CSN를 선언.
const byte address[6] = "00001"; // 주소값을 5가지 문자열로 변경할 수 있으며, 송신기와 수신기가 동일한 주소로 해야됨.

#include "arduino_secrets.h"
#define r 14
#define y 15
#define g 16
// 세그먼트 디지털 핀 2~8
int pins[] = {2, 3, 4, 5, 6, 7, 8, 9};
// 7-segment common-anode
//   a
// f     b
//   g
// e     c
//   d
// 숫자 0~9까지
int ON = LOW;
int OFF = HIGH;

// 0~9까지 숫자 표시를 위한 세그먼트의 점멸 패턴
int digits[10][8] = {
    {ON, ON, ON, ON, ON, ON, OFF, OFF},
    {OFF, ON, ON, OFF, OFF, OFF, OFF, OFF},
    {ON, ON, OFF, ON, ON, OFF, ON, OFF},
    {ON, ON, ON, ON, OFF, OFF, ON, OFF},
    {OFF, ON, ON, OFF, OFF, ON, ON, OFF},
    {ON, OFF, ON, ON, OFF, ON, ON, OFF},
    {ON, OFF, ON, ON, ON, ON, ON, OFF},
    {ON, ON, ON, OFF, OFF, ON, OFF, OFF},
    {ON, ON, ON, ON, ON, ON, ON, OFF},
    {ON, ON, ON, OFF, OFF, ON, ON, OFF}};

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = "iPhone 13 Pro (4)"; // your network SSID (name)
char pass[] = "12345678";          // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                  // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
// IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
char server[] = "172.20.10.7"; // name address for Google (using DNS)

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;
/* -------------------------------------------------------------------------- */

Servo myservo;
int servoPin = 10;

void setup()
{

    /* -------------------------------------------------------------------------- */
    // Initialize serial and wait for port to open:
    Serial.begin(9600);
    myservo.attach(servoPin);

    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    radio.begin();
    radio.openWritingPipe(address); // 이전에 설정한 5글자 문자열인 데이터를 보낼 수신의 주소를 설정
    radio.setPALevel(RF24_PA_MIN);  // 전원공급에 관한 파워레벨을 설정합니다. 모듈 사이가 가까우면 최소로 설정합니다.

    // 거리가 가까운 순으로 RF24_PA_MIN / RF24_PA_LOW / RF24_PA_HIGH / RF24_PA_MAX 등으로 설정할 수 있습니다.

    // 높은 레벨(거리가 먼 경우)은 작동하는 동안 안정적인 전압을 가지도록 GND와 3.3V에 바이패스 커패시터 사용을 권장함.

    radio.stopListening(); // 모듈을 송신기로 설정

    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("Communication with WiFi module failed!");
        // don't continue
        while (true)
            ;
    }

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION)
    {
        Serial.println("Please upgrade the firmware");
    }

    // attempt to connect to WiFi network:
    while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }

    printWifiStatus();

    Serial.println("\nStarting connection to server...");
    // if you get a connection, report back via serial:
    if (client.connect(server, 80))
    {
        Serial.println("connected to server");
        // Make a HTTP request:
        client.println("GET / HTTP/1.1");
        client.println("Host: www.google.com");
        client.println("Connection: close");
        client.println();
    }
    pinMode(r, OUTPUT);
    pinMode(y, OUTPUT);
    pinMode(g, OUTPUT);

    for (int i = 0; i < 8; i++)
    {
        // 연결 핀을 출력으로 설정
        pinMode(pins[i], OUTPUT);
    }
}

/* just wrap the received data up to 80 columns in the serial print*/
/* -------------------------------------------------------------------------- */
static int sig;
static int t;
static int send;
static bool red_flag = false;
static bool green_flag = false;
void read_response()
{

    /* -------------------------------------------------------------------------- */
    char c;
    String str;
    uint32_t received_data_num = 0;
    while (client.available())
    {
        /* actual data reception */
        c = client.read();
        /* print data to serial port */
        // Serial.print(c);
        str += c;
        /* wrap data to 80 columns*/
        received_data_num++;
        if (received_data_num % 80 == 0)
        {
            Serial.println();
        }
    }
    int first = str.indexOf("{");
    int end = str.indexOf("}");
    str = str.substring(first - 1, end) + "}";
    str.replace("\n", "");
    str.replace(" ", "");
    StaticJsonBuffer<200> jsonBuffer;

    JsonObject &root = jsonBuffer.parseObject(str);
    Serial.print(str);

    sig = root["signal"];
    Serial.print("signal : ");
    Serial.print(sig);
    t = 6;
    if (root["time"] != 0)
        t = root["time"];
    Serial.print("time : ");
    Serial.print(t);

    switch (sig)
    {
    case r:
        digitalWrite(r, HIGH);
        digitalWrite(y, LOW);
        digitalWrite(g, LOW);

        send = 'r';
        radio.write(&send, sizeof(send)); // 해당 메시지를 수신자에게 보냄
        // 7개의 세그먼트 led를 나타내는 j
        if (t == 6)
        {
            for (int j = 0; j < 8; j++)
            {
                digitalWrite(pins[j], HIGH);
            }
        }
        else
        {

            for (int j = 0; j < 8; j++)
            {
                digitalWrite(pins[j], digits[t][j]);
            }
        }
        if (!red_flag)
        {
            myservo.write(100);
            red_flag = true;
        }

        break;
    case y:
        green_flag = false;
        digitalWrite(r, LOW);
        digitalWrite(y, HIGH);
        digitalWrite(g, LOW);

        send = 'y';
        radio.write(&send, sizeof(send)); // 해당 메시지를 수신자에게 보냄
        for (int j = 0; j < 8; j++)
        {
            digitalWrite(pins[j], HIGH);
        }
        break;
    case g:
        red_flag = false;
        digitalWrite(r, LOW);
        digitalWrite(y, LOW);
        digitalWrite(g, HIGH);

        send = 'g';
        if (!green_flag)
        {
            myservo.write(10);
            green_flag = true;
        }

        radio.write(&send, sizeof(send)); // 해당 메시지를 수신자에게 보냄
        if (t == 5)
        {
            for (int j = 0; j < 8; j++)
            {
                digitalWrite(pins[j], HIGH);
            }
        }
        else
        {

            for (int j = 0; j < 8; j++)
            {
                digitalWrite(pins[j], digits[t][j]);
                // delay(50);
            }
        }
        break;
    case 0:
        break;
    default:

        digitalWrite(r, LOW);
        digitalWrite(y, LOW);
        digitalWrite(g, LOW);
        for (int j = 0; j < 8; j++)
        {
            digitalWrite(pins[j], HIGH);
            // delay(50);
        }
        break;
    }
}

/* -------------------------------------------------------------------------- */
void loop()
{
    /* -------------------------------------------------------------------------- */

    if (client.connect(server, 80))
    {
        Serial.println("connected to server");
        // Make a HTTP request:
        client.println("GET / HTTP/1.1");
        client.println("Host: www.google.com");
        client.println("Connection: close");
        client.println();

        client.connected();

        read_response();
    }
}

/* -------------------------------------------------------------------------- */
void printWifiStatus()
{
    /* -------------------------------------------------------------------------- */
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}