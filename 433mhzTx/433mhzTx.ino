/**
 * 433mhz Binary ASK encoded Radio Transmitter
 * 
 * Demostrates Binary, Network Order Binary, ACII & Struct Messaging
 *
 */

#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile

//RH_ASK driver; // defaults: uint16_t speed = 2000, uint8_t rxPin = 11, uint8_t txPin = 12, uint8_t pttPin = 10
RH_ASK driver(2000, 2, 4, 5); // ESP8266 GPIO / Pins D4, D2, D1



void setup()
{
    Serial.begin(115200);    // Debugging only
    if (!driver.init())
         Serial.println("init failed");
}

void txAsciiString() 
{
    // send ASCII string
    Serial.print(".");
    const char *msg = "Hello World!";
    driver.send((uint8_t *)msg, strlen(msg));
    driver.waitPacketSent();
    delay(1000);

}

void txAsciiFloat() 
{
    float data = 23.12;
    char buf[15]; // Bigger than the biggest possible ASCII
    snprintf(buf, sizeof(buf), "%f", data);

    if (!driver.send((uint8_t *)buf, strlen(buf) + 1)) // Include the trailing NUL
      Serial.println("send failed");

}

void txNetworkOrderBinary()
{
    // Sending a single 16 bit unsigned integer
    // in the transmitter:
    uint16_t data = htons(12345);
    if (!driver.send((uint8_t*)&data, sizeof(data)))
    {
      Serial.println("send failed");
    }
}

void txStruct()
{

    // Sending several 16 bit unsigned integers in a structure
    // in a common header for your project:
    typedef struct
    {
        uint16_t   dataitem1;
        uint16_t   dataitem2;
    } MyDataStruct;

    MyDataStruct data;
    data.dataitem1 = 12345;
    data.dataitem2 = 54321;
    if (!driver.send((uint8_t*)&data, sizeof(data)))
      Serial.println("send failed");

}

void loop()
{
    //txAsciiString();
    //txAsciiFloat();
    //txNetworkOrderBinary();
    txStruct();
}
