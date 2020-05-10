/**
 * 433mhz Binary ASK encoded Radio Transmitter
 * 
 * Demostrates Binary, Network Order Binary, ACII & Struct Messaging
 *
 */

#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile
#include <Wire.h>
#include <DHT.h>

#define dht_pin 12
DHT dht(dht_pin, DHT11);

typedef struct
{
    float  humidity;
    float  tempC;
    float  tempF;
} SensorData;


RH_ASK driver(2000, 2, 4, 5); // ESP8266 GPIO / Pins D4, D2, D1



void setup()
{
  Serial.begin(115200);    // Debugging only
  if (!driver.init())
       Serial.println("init failed");

  dht.begin();

}

void txData(SensorData data)
{
  if (!driver.send((uint8_t*)&data, sizeof(data)))
    Serial.println("send failed");
}

void loop()
{
  float h, tc, tf; // humidity, temp c, temp f

  Serial.println("DHT11: Temp, Humidity ");

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  tc = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  tf = dht.readTemperature(true);

  if (isnan(h) || isnan(tc)) {
    Serial.println("DHT11 Sensor ERROR");
  }

  Serial.println("Humidity:");
  Serial.println(h);
  Serial.println("Temperature (celsuis):");
  Serial.println(tc,1);
  Serial.println("Temperature (fahrenheit):");
  Serial.println(tf);

  SensorData data;
  data.humidity = h;
  data.tempC = tc;
  data.tempF = tf;

  txData(data);

  delay(3000);

}
