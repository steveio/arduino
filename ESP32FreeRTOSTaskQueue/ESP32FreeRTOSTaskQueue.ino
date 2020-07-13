/**
 * Demonstrates ESP32 FreeRTOS Task Scheduling & Queues
 * 
 * - task create
 * - task parameter data
 * - task priority 
 * - task IPC via fifo queue
 * - task control: suspend / resume via timer ISR
 * 
 */


/* structure that hold queue msg counter data*/
typedef struct{
  int sender;
  int counter;
}Data;

/* this variable hold queue handle */
xQueueHandle xQueue;

/* task details */
typedef struct{
  int id;
  char * name;
  TaskHandle_t th;
} taskData;

taskData t1; 
taskData t2; 


hw_timer_t * timer = NULL;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

volatile int interruptCounter = 0;
int numberOfInterrupts = 0;

volatile int taskState = 1; // 0 = idle, 1 = running

/* ISR - Timer */
void IRAM_ATTR taskControl() {
  portENTER_CRITICAL_ISR(&mux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
} 

void setup() {

  Serial.begin(112500);
  /* create the queue which size can contains 5 elements of Data */
  xQueue = xQueueCreate(5, sizeof(Data));

  /* setup & create 2 tasks */
  t1.id = 1;
  t1.name = "task1";
  t1.th = NULL;
  
  t2.id = 2;
  t2.name = "task2";
  t2.th = NULL;

  xTaskCreate(
      sendTask,       /* Task function. */
      t1.name,        /* name of task. */
      10000,          /* Stack size of task */
      (void*)&t1,     /* parameter of the task */
      3,              /* priority of the task */
      &t1.th);        /* Task handle to keep track of created task */

  xTaskCreate(
      sendTask,  
      t2.name,    
      10000,     
      (void*)&t2, 
      2,         
      &t2.th);    

  xTaskCreate(
      receiveTask,           /* Task function. */
      "receiveTask",        /* name of task. */
      10000,                    /* Stack size of task */
      NULL,                     /* parameter of the task */
      1,                        /* priority of the task */
      NULL);                    /* Task handle to keep track of created task */

  // setup timer ISR to schedule task suspend / resume
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &taskControl, true);
  timerAlarmWrite(timer, 5000000, true);
  timerAlarmEnable(timer);

}

void loop() {
  if(interruptCounter > 0){
 
      portENTER_CRITICAL(&mux);
      interruptCounter--;
      portEXIT_CRITICAL(&mux);
 
      numberOfInterrupts++;
      Serial.print("An interrupt has occurred. Total: ");
      Serial.println(numberOfInterrupts);

      Serial.print("ISR Time:");
      Serial.print(millis());

      if (taskState == 1)
      {
        Serial.println(" : Suspend");
        vTaskSuspend( t1.th );
        vTaskSuspend( t2.th );
        taskState = 0;
      } else {
        Serial.println(" : Resume");
        xTaskResumeFromISR( t1.th );
        xTaskResumeFromISR( t2.th );
        taskState = 1;
      }
  }

}

void sendTask( void * parameter )
{

  taskData t = *((taskData *) parameter);

  Serial.print(F("Task Id:"));
  Serial.print(t.id);
  Serial.print(F(" ,Name:"));
  Serial.print(t.name);
  Serial.print(F(" ,Priority:"));
  Serial.print(uxTaskPriorityGet(t.th));
  Serial.println();

  int taskId = t.id;
    
  /* keep the status of sending data */
  BaseType_t xStatus;
  /* time to block the task until the queue has free space */
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);

  /* create data to send */
  Data data;
  data.sender = taskId;
  data.counter = 1;

  for(;;){

    /* send data to front of the queue */
    xStatus = xQueueSendToFront( xQueue, &data, xTicksToWait );

    /* check whether sending is ok or not */
    if( xStatus == pdPASS ) {
      /* increase task counter */
      data.counter = data.counter + 1;
    }

    /* we delay here so that receiveTask has chance to receive data */
    delay(1000);
  }
  vTaskDelete( NULL );
}

void receiveTask( void * parameter )
{

  /* keep the status of receiving data */
  BaseType_t xStatus;
  /* time to block the task until data is available */
  const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
  Data data;
  int lastTaskPrioritySwitch = 0;

  for(;;){
    /* receive data from the queue */
    xStatus = xQueueReceive( xQueue, &data, xTicksToWait );
    /* check whether receiving is ok or not */
    if(xStatus == pdPASS){
      /* print the data to terminal */
      Serial.print("receiveTask got data: ");
      Serial.print("sender = ");
      Serial.print(data.sender);
      Serial.print(" counter = ");
      Serial.println(data.counter);
    }

    /* switch send task priorities every n counts */
    if((data.counter % 10) == 0 && lastTaskPrioritySwitch != data.counter){
      if (uxTaskPriorityGet(t1.th) < uxTaskPriorityGet(t2.th))
      { 
        vTaskPrioritySet( t1.th, 4 );
        vTaskPrioritySet( t2.th, 2 );
      } else {
        vTaskPrioritySet( t1.th, 2 );
        vTaskPrioritySet( t2.th, 4 );        
      }
      Serial.print(F("t1 New Priority:"));
      Serial.println(uxTaskPriorityGet(t1.th));
      Serial.print(F("t2 New Priority:"));
      Serial.println(uxTaskPriorityGet(t2.th));
      lastTaskPrioritySwitch = data.counter;

    }

  }
  vTaskDelete( NULL );
}
