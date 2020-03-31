/**
 * Arduino SD Card Module read/write example 
 * Uses RTC to date stamp filename in format YYYYMMDD.txt 
 * 
 */
 
#include <DS3232RTC.h>
#include <DateTime.h>
#include <SPI.h>
#include <SD.h>

DS3232RTC rtc;
#define RTC_SDA_PIN 20 // Uno 18
#define RTC_SCL_PIN 21 // Uno 19
DateTime dt;

File myFile;
char logFileName[] = "00000000.txt";
const int chipSelect = 53;


void getLogFileName()
{
  sprintf(logFileName, "%02d%02d%02d.txt", dt.year(), dt.month(), dt.day());
}

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);


  pinMode(RTC_SDA_PIN, INPUT);
  pinMode(RTC_SCL_PIN, INPUT);
  rtc.begin();

  dt = rtc.get();

  Serial.print(dt.year());
  Serial.print(dt.month());
  Serial.print(dt.day());
  Serial.println();

  Serial.print("Init SD card...");

  getLogFileName();
  Serial.println(logFileName);

  if (!SD.begin()) {
    Serial.println("init failed!");
    return;
  }
  Serial.println("initialization done.");

  myFile = SD.open("weather.txt", FILE_WRITE);

  if (myFile) {
    Serial.print("Writing to weather.txt...");
    myFile.println("testing 1, 2, 3.");
    myFile.close();
    Serial.println("done.");
  } else {
    Serial.println("error opening test.txt");
  }

  // re-open the file for reading:
  myFile = SD.open("weather.txt");
  if (myFile) {
    Serial.println("file contents:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }

}

void loop()
{
  // nothing happens after setup
}
