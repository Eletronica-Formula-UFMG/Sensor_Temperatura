//------------------------------------------------------------------------------------------------------
//Includes
#include "SdFat.h"
#include <Wire.h>
#include <SPI.h>
//#include <SD.h>
#include <Adafruit_MLX90614.h>


//------------------------------------------------------------------------------------------------------
//Basic variables used for polling, data communication, etc
const int SampleRate = 100;
//const uint32_t SampleIntUs = 1000000/SampleRate;
//const uint32_t SampleIntMs = 1000/SampleRate;
int SampleIntUs = 1000000/SampleRate;

UBaseType_t uxHighWaterMark;
QueueHandle_t DataQueue;

SemaphoreHandle_t timerSemaphore;

//static TimerHandle_t data_timer = NULL;
hw_timer_t *timer = NULL; //Hardware timer

//------------------------------------------------------------------------------------------------------
//SD card configurations
// SD chip select pin.  Be sure to disable any other SPI devices such as Enet.
const uint8_t chipSelect = 15;

SdFat sd;
SdFile file;

#define error(msg) sd.errorHalt(F(msg))

#define IR1 0x5A
#define IR2 0x5B
#define IR3 0x5C
#define IR4 0x5D

//------------------------------------------------------------------------------------------------------
//Specific component data
struct Data_t {
  uint32_t usec;
  float temp1;
  float temp2;
  float temp3;
  float temp4;

} Tx_Data, Rx_Data;

Adafruit_MLX90614 mlx1;
Adafruit_MLX90614 mlx2;
Adafruit_MLX90614 mlx3;
Adafruit_MLX90614 mlx4;

//------------------------------------------------------------------------------------------------------
//Tasks declaration
void TaskGet(void *pvParam);
void TaskPrint(void *pvParam);
void TaskSD(void *pvParam);

TaskHandle_t xGetData;
TaskHandle_t xPrint;
TaskHandle_t xSD;

//------------------------------------------------------------------------------------------------------
////Interrupt services routines (Hardware timer)
void IRAM_ATTR vTimerISR(){ //interrupt handle routine must always have IRAM_ATTR attribute
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
}
/*
//Software tiemer handler
void timerCallback(TimerHandle_t xTimer){
  xSemaphoreGive(timerSemaphore);
}*/

//------------------------------------------------------------------------------------------------------
//Tasks definition
void TaskGet(void *pvParam){
  (void) pvParam; 

  while(1){
    if(xSemaphoreTake(timerSemaphore, portMAX_DELAY) == pdTRUE){

      // Pega as temperaturas dos 4 sensores
      Tx_Data.usec = millis();
      Tx_Data.temp1 = mlx1.readObjectTempC();
      Tx_Data.temp2 = mlx2.readObjectTempC();
      Tx_Data.temp3 = mlx3.readObjectTempC();
      Tx_Data.temp4 = mlx4.readObjectTempC();
      
      if(xQueueSend(DataQueue, &Tx_Data, portMAX_DELAY) != pdPASS){
        Serial.println("XQueueSend is not working");
      }
      //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
      //Serial.println(uxHighWaterMark);
    } 
  }  
  vTaskDelete(NULL);
}

void TaskPrint(void *pvParam){
  (void) pvParam;

  while(1){
    if(xQueueReceive(DataQueue, &Rx_Data, portMAX_DELAY) != pdPASS){
      Serial.println("XQueueReceive is not working");
    }
    // Printa as temperaturas dos 4 sensores
    Serial.print(Rx_Data.temp1);
    Serial.print("\t");
    Serial.print(Rx_Data.temp2);
    Serial.print("\t");
    Serial.print(Rx_Data.temp3);
    Serial.print("\t");
    Serial.print(Rx_Data.temp4);
    Serial.print(", ");
    Serial.println(Rx_Data.usec);
     
    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    //Serial.println(uxHighWaterMark);
  }
  vTaskDelete(NULL);
}

void TaskSD(void *pvParam){
  (void) pvParam;
  while(1){
    if(xQueueReceive(DataQueue, &Rx_Data, portMAX_DELAY) != pdPASS){
      Serial.println("XQueueReceive is not working");
    }

    file.print(Rx_Data.temp1);
    file.print("\t");
    file.print(Rx_Data.temp2);
    file.print("\t");
    file.print(Rx_Data.temp3);
    file.print("\t");
    file.print(Rx_Data.temp4);
    file.write(',');
    file.println(Rx_Data.usec);

    if(!file.sync() || file.getWriteError()){
      error("write error");
    }  
      
    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    //Serial.println(uxHighWaterMark);
  }
  vTaskDelete(NULL); 
}

//------------------------------------------------------------------------------------------------------
//Setup function
void setup() {
  
  Serial.begin(115200);
  
  timerSemaphore = xSemaphoreCreateBinary();
  //------------------------------------------------------------------------------------------------------
  //Queue inicialization
  DataQueue = xQueueCreate(10, sizeof(Data_t));
  if(DataQueue == NULL){
    Serial.println("Error creating queue");
  }

  //------------------------------------------------------------------------------------------------------
  //Hardware timers initialization
  timer = timerBegin(1, 80, true); //Initialize timer. param(hardware timer number [0-4], prescaler [80 MHz divisor], [true: count up, false: count down])
  timerAttachInterrupt(timer, &vTimerISR, true); //bind timer to handling function
  timerAlarmWrite(timer, SampleIntUs, true); //timer count value in us, depending on prescaler. true: reload counter
  timerAlarmEnable(timer);

  //------------------------------------------------------------------------------------------------------  
  //SD card initialization 
 // if(!sd.begin(chipSelect, SD_SCK_MHZ(50))){
 //   sd.initErrorHalt();
 // }

 // if(!file.open("Teste.csv", O_WRONLY | O_CREAT | O_EXCL)){
 //   error("file.open");
 // }
  
  //------------------------------------------------------------------------------------------------------  
  //Component initialization
  if(mlx1.begin() | mlx2.begin() | mlx3.begin() | mlx4.begin()){
    Serial.println("Sensor Inicializado!");
  }
  mlx1.AddrSet(IR1);
  mlx2.AddrSet(IR2);
  mlx3.AddrSet(IR3);
  mlx4.AddrSet(IR4);
  //------------------------------------------------------------------------------------------------------
  //Tasks creation
  //Software timer task
  /*data_timer = xTimerCreate(
                      "Polling timer",                // Name of timer
                      SampleIntMs/portTICK_PERIOD_MS, // Period of timer (in ticks)
                      pdFALSE,                        // Auto-reload
                      (void *)0,                      // Timer ID
                      timerCallback);                 // Callback function*/
                      
  xTaskCreate(TaskGet,
             "Read sensor",
              1024 * 4,
              NULL,
              2, 
              &xGetData);
  
  xTaskCreate(TaskPrint,
            "Serial monitor print",
             1024 * 4,
             NULL,
             3, 
             &xPrint);

  /*xTaskCreate(TaskSD,
            "Write on SD card",
             1024 * 4,
             NULL,
             3, 
             &xSD);*/

}

//------------------------------------------------------------------------------------------------------
//empty loop task
void loop() {
  // put your main code here, to run repeatedly:

}