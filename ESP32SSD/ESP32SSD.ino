/**
 * ESP32 TTGO v2 1.6 SSD Card Test
 * 
 * @note sketch upload fails when SSD card connected
 * Remove SSD, upload sketch, insert SSD card, reset device
 * 
 */

#include <SD.h>
SPIClass spiSD(HSPI);
#define SD_CS 13
#define SDSPEED 16000000


void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}


void setup() {

 Serial.begin(115200);

 // Set SD Card SPI
 spiSD.begin(14,2,15,13);//SCK,MISO,MOSI,CS
 if(!SD.begin( SD_CS, spiSD, SDSPEED)){   // use only 12, 13, 14, 15 ou 25 26 if possible
   Serial.println("Card Mount Failed");
   return;
 }

 Serial.println("Card Mount OK");

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/data.txt");
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/data.txt", "Reading ID, Date, Hour, Temperature \r\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();

}

void loop() {
  // put your main code here, to run repeatedly:

}
