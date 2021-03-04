/*
 * ESP32 GPS LoRA Receiver
 * 
 * Listens on LoRa 866E6 for comma delimited GPS msg in format
 * '50.72,-1.84,31.80,0.00,0.04'
 *
 * Tested on TTGO ESP32 OLED v1.3
 *
 */


//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);


unsigned long loraRxCount = 0;
char loraData[128];

const int b = 8;
char lat[b], lon[b], alt[b], course[b], speed[b];

bool displayRefresh = false;


void onReceive(int packetSize) {

  Serial.print("Received packet '");

  for (int i = 0; i < packetSize; i++) {
    loraData[i] = (char)LoRa.read();
    Serial.print(loraData[i]);
  }
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());

  ++loraRxCount;

  parseGPS(loraData);
  displayRefresh = true;
}

void updateOLED()
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  
  display.print("Lat: ");
  display.print(lat);
  
  display.setCursor(0,10);
  display.print("Lon: ");
  display.println(lon);
  
  display.setCursor(0,20);
  display.print("Alt: ");
  display.println(alt);
  
  display.setCursor(0,30);
  display.print("Course:");
  display.print(course);
  display.println(" (deg) ");
  
  display.setCursor(0,40);
  display.print("Speed:");
  display.print(speed);
  display.println(" (kmph)");
  
  
  display.setCursor(0,50);
  display.print("LoRa Rx:");
  display.println(loraRxCount);
  
  display.display();

}

void setup() { 

  Serial.begin(115200);

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

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA RECEIVER ");

  Serial.println("LoRa Receiver Test");
  
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("LoRa Receiver Callback");

  if (!LoRa.begin(866E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Uncomment the next line to disable the default AGC and set LNA gain, values between 1 - 6 are supported
  // LoRa.setGain(6);
  
  // register the receive callback
  LoRa.onReceive(onReceive);

  // put the radio into receive mode
  LoRa.receive();

  Serial.println("LoRa Initializing OK!");
  display.setCursor(0,10);
  display.println("LoRa Initializing OK!");
  display.display();  
}

void loop() 
{
  if (displayRefresh)
  {
    updateOLED();
    displayRefresh = false;
  }
}


void parseGPS(char buff[])
{

  Serial.println("parseGPS()");
  Serial.println(buff);

  char * token = strtok(buff,",");
  if (token != NULL)
  {
    strcpy(lat, token);
  }
  token = strtok(NULL,",");
  if (token != NULL)
  {
    strcpy(lon, token);
  }
  token = strtok(NULL,",");
  if (token != NULL)
  {
    strcpy(alt, token);
  }
  token = strtok(NULL,",");
  if (token != NULL)
  {
    strcpy(course, token);
  }
  token = strtok(NULL,",");
  if (token != NULL)
  {
    strcpy(speed, token);
  }

  Serial.println(lat);
  Serial.println(lon);
  Serial.println(alt);
  Serial.println(course);
  Serial.println(speed);

  /*
  int idx = 0;
  while(token){
     Serial.println(token);
     token = strtok(NULL,",");
  }
  */
}
