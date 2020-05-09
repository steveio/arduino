/**
 * 433mhz Binary ASK encoded Radio Receiver
 *  
 * Demostrates Binary, Network Order Binary, ACII & Struct Messaging
 */

#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile

RH_ASK driver;

long counter = 0;


void setup()
{
    Serial.begin(115200);  // Debugging only
    if (!driver.init())
         Serial.println("init failed");
}

void rxAsciiString()
{

    uint8_t buf[12];
    uint8_t buflen = sizeof(buf);
    if (driver.recv(buf, &buflen)) // Non-blocking
    {
      int i;
      // Message with a good checksum received, dump it.
      Serial.print("Message: ");
      Serial.println(counter);
      Serial.println((char*)buf);         
      counter++;
    }

}

void rxAsciiFloat()
{
    float data;
    uint8_t buf[15]; // Bigger than the biggest possible ASCII
    uint8_t buflen = sizeof(buf);
    if (driver.recv(buf, &buflen))
    {
        // Have the data, so do something with it
        float data = atof(buf); // String to float
        Serial.print("Message: ");
        Serial.println(counter);
        Serial.println(data);
        counter++;
    }
}

void rxNetworkOrderBinary()
{
    uint16_t data;
    uint8_t datalen = sizeof(data);
    if (   driver.recv((uint8_t*)&data, &datalen)
        && datalen == sizeof(data))
    {
        // Have the data, so do something with it
        uint16_t xyz = ntohs(data);
        Serial.print("Message: ");
        Serial.println(counter);
        Serial.println(xyz);
        counter++;
    }
}

void rxStruct()
{
    typedef struct
    {
        uint16_t   dataitem1;
        uint16_t   dataitem2;
    } MyDataStruct;

    MyDataStruct data;
    uint8_t datalen = sizeof(data);
    if (   driver.recv((uint8_t*)&data, &datalen)
        && datalen == sizeof(data))
    {
        // Have the data, so do something with it
        uint16_t pqr = data.dataitem1;
        uint16_t xyz = data.dataitem2;  
        Serial.print("Message: ");
        Serial.println(counter);
        Serial.println(pqr);
        Serial.println(xyz);
        counter++;
    }
}

void loop()
{
    //rxAsciiString();
    //rxAsciiFloat();
    //rxNetworkOrderBinary();
    rxStruct();
}
