/*
 * ESP32 GPS LoRA Sender (Multitask)
 * 
 * Demonstrates ESP32 / FreeRTOS features:
 *   - multi task SMP parrallel processing with CPU core affinity
 *   - task notification (IPC)
 *   - timer / ISR
 *   - queue (pointer)
 *   - semaphore mutex
 *
 * Tested on TTGO ESP32 v2 1.6
 *
 */

#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

HardwareSerial GPSSerial(1);

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// SD Card
#include <SD.h>
SPIClass spiSD(HSPI);
#define SD_CS 13
#define SDSPEED 16000000

#define SSD_SCK 14
#define SSD_MISO 2
#define SSD_MOSI 15
#define SSD_CS 13

bool sdCardEnabled = true;


// GPS position data
struct XPosit
{
    float Lat;
    float Lon;
    float Alt;
    float Course;
    float Speed;

} xPosit;


TinyGPSPlus gps;

static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;

// CPU Cores
static int taskCore0 = 0;
static int taskCore1 = 1;

// Semaphores to lock / serialise data structure IO
SemaphoreHandle_t sema_GPS_Gate; 
SemaphoreHandle_t sema_Posit;

// GPS position data queue 
QueueHandle_t xQ_Posit;

// times in ticks
TickType_t xTicksToWait0 = 0;
TickType_t xTicksToWait1000 = 1000;


// ISR timer
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
unsigned long isrCounter = 0; 


// task handles
static TaskHandle_t xGPSTask;
static TaskHandle_t xLoRATask;
static TaskHandle_t xSDWriteTask;


// Task Notification bits
#define LORA_TX_BIT    0x04
#define LORA_RX_BIT    0x06
#define SD_WRITE_BIT   0x08


HardwareSerial Serial1(1);

#define HWSERIAL_RX 12
#define HWSERIAL_TX 13
#define HWSERIAL_BAUD 9600


// LoRA Module
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

bool loraEnabled = true;
unsigned long loraLastEvent;
unsigned long loraSendInterval = 10000;
unsigned long loraCounter = 0; 


// 0.96 OLED Display
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


// LoRA Send Timer ISR
void IRAM_ATTR fLoRASendISR( void )
{
    portENTER_CRITICAL_ISR(&timerMux);

    Serial.println("fLoRASendISR()");

    ++isrCounter;
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Notify SD Write Task SD_WRITE BIT */
    xTaskNotifyFromISR( xSDWriteTask,
                       SD_WRITE_BIT,
                       eSetBits,
                       &xHigherPriorityTaskWoken );

    /* Notify LoRA send task to transmit by setting the TX_BIT */
    xTaskNotifyFromISR( xLoRATask,
                       LORA_TX_BIT,
                       eSetBits,
                       &xHigherPriorityTaskWoken );

    portEXIT_CRITICAL_ISR(&timerMux);
}

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

  Serial.println("Begin ESP32GPSLoRAMultiTask...");

  sema_GPS_Gate = xSemaphoreCreateBinary();

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
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
  }

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/data.txt");
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/data.txt", "Lat, Lon, Alt, Course, Speed \r\n");
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

 
  // create a pointer queue to pass position data
  xQ_Posit = xQueueCreate( 15, sizeof( &xPosit )  );

  Serial.print("Start Task fGPS_Parse() priority 0 on core 0");
  xTaskCreatePinnedToCore( fGPS_Parse, "fGPS_Parse", 1000, NULL, 0, &xGPSTask, taskCore0 );
  configASSERT( xGPSTask );

  Serial.println("Start Task fSD_Write() priority 4 on core 1");
  xTaskCreatePinnedToCore( fSD_Write, "fSD_Write", 1000, NULL, 4, &xSDWriteTask, taskCore1 ); // assigned to core 1
  configASSERT( xSDWriteTask );

  Serial.println("Start Task fLoRA_Send() priority 3 on core 1");
  // Tasks on Core 1 must have priority > 1 (priority of core 1 loop() task)
  xTaskCreatePinnedToCore( fLoRA_Send, "fLoRA_Send", 1000, NULL, 3, &xLoRATask, taskCore1 ); // assigned to core 1
  configASSERT( xLoRATask );

  sema_GPS_Gate = xSemaphoreCreateMutex();
  sema_Posit = xSemaphoreCreateMutex();
  
  xSemaphoreGive( sema_GPS_Gate );
  xSemaphoreGive( sema_Posit );

  // LoRA send & SD Card Write timer interupt 
  // Configure Prescaler to 80, as our timer runs @ 80Mhz
  // Giving an output of 80,000,000 / 80 = 1,000,000 ticks / second
  timer = timerBegin(0, 80, true);                
  timerAttachInterrupt(timer, &fLoRASendISR, true);    
    
  // Fire Interrupt every 10s (10 * 1 million ticks)
  timerAlarmWrite(timer, 10000000, true);
  timerAlarmEnable(timer);
}




/**
 * The task which runs setup() and loop() is created on core 1 with priority 1
 */
void loop() {
  //vTaskDelay(100); //  yields the CPU on core 1

  // delete loop() task and free resources
  vTaskDelete(NULL); 
}


void fSD_Write( void *pvParameter )
{

  Serial.print("fSD_Write() Task running on core ");
  Serial.println(xPortGetCoreID());

  BaseType_t xResult;
  uint32_t ulNotifiedValue;

  for( ;; )
  {

    /* block until task notification (from timer ISR occurs) */
    xResult = xTaskNotifyWait( SD_WRITE_BIT,
                         ULONG_MAX,        /* Clear all bits on exit. */
                         &ulNotifiedValue, /* Stores the notified value. */
                         portMAX_DELAY );  /* Block indefinately */
  
    if( xResult == pdPASS )
    {
       if( ( ulNotifiedValue & SD_WRITE_BIT ) != 0 )
       {
          Serial.println("fSD_Write() notify / unblock");

          struct XPosit xPosit, *pxPosit;

          if( xQueueReceive( xQ_Posit,
                             &( pxPosit ),
                             ( TickType_t ) 10 ) == pdPASS )
          {

              if (sdCardEnabled)
              {
                int sz = 64;
                char msg[sz];
                sprintf(msg, "%f,%f,%f,%f,%f" , pxPosit->Lat, pxPosit->Lon, pxPosit->Alt, pxPosit->Course, pxPosit->Speed);

                Serial.print("SD Card Log: ");
                Serial.println(msg);

                writeFile(SD, "/data.txt", msg);
                clearBuffer(msg, sz);
              }

          }

          taskYIELD();
       }  
    }
    else
    {
    }
  }
}


void fLoRA_Send( void *pvParameter )
{

  Serial.print("fLoRA_Send() Task running on core ");
  Serial.println(xPortGetCoreID());

  BaseType_t xResult;
  uint32_t ulNotifiedValue;

  for( ;; )
  {

    /* block until task notification (from timer ISR occurs) */
    xResult = xTaskNotifyWait( LORA_TX_BIT,
                         ULONG_MAX,        /* Clear all bits on exit. */
                         &ulNotifiedValue, /* Stores the notified value. */
                         portMAX_DELAY );  /* Block indefinately */
  
    if( xResult == pdPASS )
    {
       if( ( ulNotifiedValue & LORA_TX_BIT ) != 0 )
       {
          Serial.println("fLoRA_Send() LORA_TX_BIT notify / unblock");

          struct XPosit xPosit, *pxPosit;

          if( xQueueReceive( xQ_Posit,
                             &( pxPosit ),
                             ( TickType_t ) 10 ) == pdPASS )
          {

             Serial.println("LoRA Send:");
             Serial.print("Lat:");
             Serial.println(pxPosit->Lat);
             Serial.print("Lon:");
             Serial.println(pxPosit->Lon);

              int sz = 64;
              char msg[sz];
          
              if (loraEnabled && millis() > loraLastEvent + loraSendInterval)
              {
                Serial.print("LoRA Sending packet: ");
                Serial.println(loraCounter);
              
                sprintf(msg, "%f,%f,%f,%f,%f" , pxPosit->Lat, pxPosit->Lon, pxPosit->Alt, pxPosit->Course, pxPosit->Speed);
              
                Serial.print("LoRA Msg: ");
                Serial.println(msg);
              
                //Send LoRa packet to receiver
                LoRa.beginPacket();
                LoRa.print(msg);
                LoRa.endPacket();  
                loraCounter++;
                loraLastEvent = millis();
              
                clearBuffer(msg, sz);
            
                loraLastEvent = millis();
              }

          }

          taskYIELD();
       }
  
       if( ( ulNotifiedValue & LORA_RX_BIT ) != 0 )
       {
          /* The RX ISR has set a bit. */
       }
    }
    else
    {
       /* Did not receive a notification within the expected time. */
    }
  }
}


void fGPS_Parse(  void *pvParameters )
{

  Serial.print("fGPS_Parse() Task running on core ");
  Serial.println(xPortGetCoreID());

  struct XPosit *pxPosit;

  for (;;)
  {    

    if (gps.location.isValid())
    {
      
      Serial.println(" ");
      Serial.println("GPS Signal");
      Serial.println("Position: ");
      Serial.print("Latitude:");
      Serial.print(gps.location.lat(),6);
      Serial.print("; ");
      Serial.print("Longitude:");
      Serial.println(gps.location.lng(),6);     
      Serial.print("Altitude:");
      Serial.println(gps.altitude.meters(),6); 
      Serial.print("Course:");
      Serial.println(gps.course.deg(),6); 
      Serial.print("Speed (kmph):");
      Serial.println(gps.speed.kmph(),6); 
      Serial.println(" "); 
  
  
      if ( xSemaphoreTake( sema_GPS_Gate, xTicksToWait0 ) == pdTRUE )
      {
  
        if ( xSemaphoreTake( sema_Posit, xTicksToWait1000 ) == pdTRUE )
        {
          xPosit.Lat = gps.location.lat();
          xPosit.Lon = gps.location.lng();
          xPosit.Alt = gps.altitude.meters();
          xPosit.Course = gps.course.deg();
          xPosit.Speed = gps.speed.kmph();

          xSemaphoreGive( sema_Posit );
        }
  
  
        if ( xSemaphoreTake( sema_Posit, xTicksToWait1000 ) == pdTRUE )
        {
          Serial.println("xQueueSend()");
          pxPosit = &xPosit;
          xQueueSend( xQ_Posit, ( void * ) &pxPosit, ( TickType_t ) 0 );  
          xSemaphoreGive( sema_Posit );
        }
  
        xSemaphoreGive( sema_GPS_Gate );
      }  
    } else {

      Serial.print("Chars:");
      Serial.print(gps.charsProcessed());
      Serial.print("  Sentances:");
      Serial.print(gps.sentencesWithFix());
      Serial.print("  Failed: ");
      Serial.print(gps.failedChecksum());
      Serial.println();
  
      if (millis() > 5000 && gps.charsProcessed() < 10)
        Serial.println(F("No GPS data received: check wiring"));

      delay(1000);
    }
  } // for (;;)
  vTaskDelete( NULL );
} // void fGPS_Parse(  void *pvParameters )


static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (Serial1.available())
    {
      gps.encode(Serial1.read());
    }
  } while (millis() - start < ms);
}



void clearBuffer(char * buff, int sz)
{
  for(int i=0; i<sz; i++)
  {
    buff[i] = '\0';
  }
}
