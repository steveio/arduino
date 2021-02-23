/**
 * 
 * ESP32 TTGO LoRA v2 1.6 GPS Sender w/ SSD Card
 * 
 * V1 Single Task Process
 * 
 */

#include <SPI.h>
#include <LoRa.h>

#include <TinyGPS.h>
#include <HardwareSerial.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#include <SD.h>
SPIClass spiSD(HSPI);
#define SD_CS 13
#define SDSPEED 16000000

#define SSD_SCK 14
#define SSD_MISO 2
#define SSD_MOSI 15
#define SSD_CS 13


HardwareSerial Serial1(1);

#define HWSERIAL_RX 12
#define HWSERIAL_TX 13
#define HWSERIAL_BAUD 9600


//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 866E6

bool loraEnabled = false;
unsigned long loraLastEvent;
unsigned long loraTxDelay = 10000;
int loraCounter = 0;


// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);


// long  lat,lon; // create variable for latitude and longitude object
float lat = 50.7192,lon = -1.8808; 

TinyGPS gps; // create gps object


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

void setup()
{

  Serial.begin(115200); // connect serial

  Serial.println("LoRA GPS Transmitter");

  Serial1.begin(HWSERIAL_BAUD, SERIAL_8N1, HWSERIAL_RX, HWSERIAL_TX);

  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  
  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }


  // Initialize SD card
  // Set SD Card SPI
  spiSD.begin(SSD_SCK, SSD_MISO, SSD_MOSI, SSD_CS);//SCK,MISO,MOSI,CS
  if(!SD.begin( SD_CS, spiSD, SDSPEED)){   // use only 12, 13, 14, 15 ou 25 26 if possible
    Serial.println("Card Mount Failed");
  }

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

  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);
  
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initializing OK!");
  display.setCursor(0,10);
  display.print("LoRa Initializing OK!");

  loraLastEvent = millis();

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LoRA GPS Transmitter");
  display.display();

}
 
void loop()
{

  bool newData = false;

  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {

    while (Serial1.available())
    {
      gps.encode(Serial1.read());
      newData = true;
    }
  }

  if (newData)
  {
    gps.f_get_position(&lat,&lon); // get latitude and longitude

    //if (lat != TinyGPS::GPS_INVALID_F_ANGLE && lon != TinyGPS::GPS_INVALID_F_ANGLE)
    //{
      
      float gps_alt = gps.f_altitude();
      float gps_course = gps.f_course();
      float gps_speed = gps.f_speed_kmph();
  
      Serial.println(" ");
      Serial.println("GPS Signal");
      Serial.println("Position: ");
      Serial.print("Latitude:");
      Serial.print(lat,6);
      Serial.print("; ");
      Serial.print("Longitude:");
      Serial.println(lon,6);     
      Serial.print("Altitude:");
      Serial.println(gps_alt,6); 
      Serial.print("Course:");
      Serial.println(gps_course,6); 
      Serial.print("Speed (kmph):");
      Serial.println(gps_speed,6); 
      Serial.println(" "); 
    
      if (loraEnabled)
      {
        Serial.print("LoRA Sending packet: ");
        Serial.println(loraCounter);
    
        int sz = 64;
        char msg[sz];
        sprintf(msg, "%f,%f,%f,%f,%f" , lat, lon, gps_alt, gps_course, gps_speed);
    
        Serial.print("LoRA Msg: ");
        Serial.println(msg);
  
        //Send LoRa packet to receiver
        LoRa.beginPacket();
        LoRa.print(msg);
        LoRa.endPacket();  
        loraCounter++;
        loraLastEvent = millis();
  
        clearBuffer(msg, sz);
  
      }
  
  
      // Display information
      display.clearDisplay();
      display.setCursor(0,0);
      display.print("Lat: ");
      display.print(lat);
    
      display.setCursor(0,10);
      display.print("Lon: ");
      display.print(lon);
      
      display.setCursor(0,20);
      display.print("Alt: ");
      display.print(gps_alt);
      
      display.setCursor(0,30);
      display.print("Course:");
      display.print(gps_course);
      display.print(" (deg) ");
    
      display.setCursor(0,40);
      display.print("Speed:");
      display.print(gps_speed);
      display.print(" (kmph)");
    
      display.setCursor(0,50);
      display.print("LoRA Tx:");
      display.print(loraCounter);
    
      display.display();
  
    //}

    unsigned long chars = 0;
    unsigned short sentences = 0, failed = 0;

    gps.stats(&chars, &sentences, &failed);

    Serial.print("Chars:");
    Serial.println(chars);

    Serial.print("Words:");
    Serial.println(sentences);

    Serial.print("Failed:");
    Serial.println(failed);

    if (chars == 0)
      Serial.println("** No characters received from GPS: check wiring **");

  }
}


void clearBuffer(char * buff, int sz)
{
  for(int i=0; i<sz; i++)
  {
    buff[i] = '\0';
  }
}
