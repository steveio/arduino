#include <SPI.h>
#include <SD.h>

File myFile;
const int chipSelect = 53;


void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  Serial.print("Initializing SD card...");


  if (!SD.begin()) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  /*
  myFile = SD.open("test.txt", FILE_WRITE);

  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("testing 1, 2, 3.");
    myFile.close();
    Serial.println("done.");
  } else {
    Serial.println("error opening test.txt");
  }
  */

  // re-open the file for reading:
  myFile = SD.open("sensor.txt");
  if (myFile) {
    Serial.println("sensor.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

}

void loop()
{
  // nothing happens after setup
}
