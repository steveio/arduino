/*
  AnalogReadSerial

  Reads an analog input on pin 0, prints the result to the Serial Monitor.
  Graphical representation is available using Serial Plotter (Tools > Serial Plotter menu).
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/AnalogReadSerial
*/

#include <DHT.h>

#define dht_apin A0

DHT dht(dht_apin, DHT11);

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(19200);
  delay(500);//Delay to let system boot
  Serial.println("DHT11 Humidity & temperature Sensor\n\n");
  delay(1000);//Wait before accessing Sensor

  dht.begin();
}

// the loop routine runs over and over again forever:
void loop() {

  // Wait a few seconds between measurements.
  delay(10000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  Serial.println("DHT11 Humidity:");
  Serial.println(h);
  Serial.println("DHT11 Temperature (celsuis):");
  Serial.println(t);
  Serial.println("DHT11 Temperature (fahrenheit):");
  Serial.println(f);


}
