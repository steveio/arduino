/*
 * ESP32 GPS LoRA Sender
 * 
 * Demonstrates ESP32 / FreeRTOS features:
 *   - multi task SMP parrallel processing with CPU core affinity
 *   - task notification (IPC)
 *   - timer / ISR
  *   - queue
 *   - semaphores to serialise / lock data structure read/write

 *
 */

#include <TinyGPS++.h>
#include <HardwareSerial.h>

HardwareSerial GPSSerial(1);

// GPS position data
struct XPosit
{
    float Lat;
    float Lon;
    float MPH;
    float KPH;
    float Alti;
    float Hdg;

} xPosit;

TinyGPSPlus GPS;

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

// Task Notification bits
#define LORA_TX_BIT    0x01
#define LORA_RX_BIT    0x02

unsigned long loraSendCounter = 0; 

// LoRA Send Timer ISR
void IRAM_ATTR fLoRASendISR( void )
{
    portENTER_CRITICAL_ISR(&timerMux);

    Serial.println("fLoRASendISR()");

    ++isrCounter;
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* Notify LoRA send task to transmit by setting the TX_BIT */
    xTaskNotifyFromISR( xLoRATask,
                       LORA_TX_BIT,
                       eSetBits,
                       &xHigherPriorityTaskWoken );
    
    portEXIT_CRITICAL_ISR(&timerMux);
}


void setup() {
  Serial.begin(115200);

  Serial.println("Begin ESP32GPSMultiTask...");

  sema_GPS_Gate = xSemaphoreCreateBinary();

  //GPSSerial.begin ( 9600, SERIAL_8N1, 15, 2 ); // begin GPS hardware serial

  // create a pointer queue to pass position data
  xQ_Posit = xQueueCreate( 15, sizeof( &xPosit )  );

  Serial.print("Start Task fGPS_Parse() priority 0 on core 0");
  xTaskCreatePinnedToCore( fGPS_Parse, "fGPS_Parse", 1000, NULL, 0, &xGPSTask, taskCore0 );

  configASSERT( xGPSTask );

  Serial.println("Start Task fLoRA_Send() priority 3 on core 1");
  // Tasks on Core 1 must have priority > 1 (priority of core 1 loop() task)
  xTaskCreatePinnedToCore( fLoRA_Send, "fLoRA_Send", 1000, NULL, 3, &xLoRATask, taskCore1 ); // assigned to core 1
  configASSERT( xLoRATask );
  
  sema_GPS_Gate = xSemaphoreCreateMutex();
  sema_Posit = xSemaphoreCreateMutex();
  
  xSemaphoreGive( sema_GPS_Gate );
  xSemaphoreGive( sema_Posit );

  // LoRA send timer interupt 
  // Configure Prescaler to 80, as our timer runs @ 80Mhz
  // Giving an output of 80,000,000 / 80 = 1,000,000 ticks / second
  timer = timerBegin(0, 80, true);                
  timerAttachInterrupt(timer, &fLoRASendISR, true);    
    
  // Fire Interrupt every 10s (10 * 1 million ticks)
  timerAlarmWrite(timer, 10000000, true);
  timerAlarmEnable(timer);
}


/**
 * The task which runs setup() and loop() is created on core 1 with priority 1.
 * That task is currently running loop() forever
 * 
 */
void loop() {
  //vTaskDelay(100); //  yields the CPU on core 1

  // delete loop() task and free resources
  vTaskDelete(NULL); 
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
  float lat, lon;
  xPosit.Lat = 51.123;
  xPosit.Lon = -1.123;

  for (;;)
  {    
  
    if ( xSemaphoreTake( sema_GPS_Gate, xTicksToWait0 ) == pdTRUE )
    {

      if ( xSemaphoreTake( sema_Posit, xTicksToWait1000 ) == pdTRUE )
      {
        xPosit.Lat = xPosit.Lat + 0.015;
        xPosit.Lon = xPosit.Lon + 0.015;
        xSemaphoreGive( sema_Posit );
      }

      Serial.println("fGPS_Parse()");
      Serial.print("Lat:");
      Serial.println(xPosit.Lat);
      Serial.print("Lon:");
      Serial.println(xPosit.Lon);


      if ( xSemaphoreTake( sema_Posit, xTicksToWait1000 ) == pdTRUE )
      {
        Serial.println("xQueueSend()");
        pxPosit = &xPosit;
        xQueueSend( xQ_Posit, ( void * ) &pxPosit, ( TickType_t ) 0 );
        //xQueueOverwrite( xQ_Posit, ( void * ) &pxPosit);

        xSemaphoreGive( sema_Posit );
      }

      xSemaphoreGive( sema_GPS_Gate );
    }


    delay(1000);
      

    /*
    //xEventGroupWaitBits (eg, evtGPS_Parse, pdTRUE, pdTRUE, portMAX_DELAY) ;
    if ( xSemaphoreTake( sema_GPS_Gate, xTicksToWait0 ) == pdTRUE )
    {
      //query GPS: has a new complete chunk of data been received?
      if ( GPSSerial.available() > 1 )
      {
        if ( GPS.encode(GPSSerial.read()) )
        {
          if (  GPS.location.isValid())
          {
            if ( xSemaphoreTake( sema_Posit, xTicksToWait1000 ) == pdTRUE )
            {
              xPosit.Lat = GPS.location.lat();// store data into structure
              xPosit.Lon = GPS.location.lng();
              xSemaphoreGive( sema_Posit );
            }
          } // if (  GPS.location.isValid())
          if (GPS.speed.isValid())
          {
            if ( xSemaphoreTake( sema_Posit, xTicksToWait1000 ) == pdTRUE )
            {
              xPosit.MPH = GPS.speed.mph();
              xPosit.KPH = GPS.speed.kmph();
              xSemaphoreGive( sema_Posit );
            }
          } //  if (GPS.speed.isValid())
          if (  GPS.altitude.isValid() )
          {
            if ( xSemaphoreTake( sema_Posit, xTicksToWait1000 ) == pdTRUE )
            {
              xPosit.Alti = GPS.altitude.meters();
              xSemaphoreGive( sema_Posit );
            }
          } //  if (  GPS.altitude.isValid() )
          if ( GPS.course.isUpdated() )
          {
            if ( xSemaphoreTake( sema_Posit, xTicksToWait1000 ) == pdTRUE )
            {
              xPosit.Hdg = GPS.course.deg();
              xSemaphoreGive( sema_Posit );
            }
          } // if ( GPS.course.isUpdated() )
          if (GPS.time.isValid())
          {
            if ( xSemaphoreTake( sema_Time, xTicksToWait1000 ) == pdTRUE )
            {
              xTime.iSeconds = GPS.time.second();
              xTime.iMinutes = GPS.time.minute();
              xTime.iHours = GPS.time.hour();
              xSemaphoreGive( sema_Time );
            }
          } // if (GPS.time.isValid())
          if (GPS.date.isValid())
          {
            if ( xSemaphoreTake( sema_Date, xTicksToWait1000 ) == pdTRUE )
            {
              xDate.iMonth = GPS.date. month();
              xDate.iDay = GPS.date.day();
              xDate.iYear = GPS.date.year();
              xSemaphoreGive( sema_Date );
            }
          } // if (GPS.date.isValid())
          if ( xSemaphoreTake( sema_Posit, xTicksToWait1000 ) == pdTRUE )
          {
            // version of xQueueSendToBack() that will write to the queue even if the queue is full
            xQueueOverwrite( xQ_Posit, (void *) &xPosit );
            xSemaphoreGive( sema_Posit );
          }
        } // if ( GPS.encode(GPSSerial.read()))
      } // if ( GPSSerial.available() > 0 )
      xSemaphoreGive( sema_GPS_Gate );
    }
    */

  } // for (;;)
  vTaskDelete( NULL );
} // void fGPS_Parse(  void *pvParameters )
