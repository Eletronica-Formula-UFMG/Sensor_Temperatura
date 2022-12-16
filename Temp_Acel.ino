//------------------------------------------------------------------------------------------------------
//Includes
#include "SdFat.h"
//Includes do sensor
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
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

const uint32_t SampleIntMs = 1000/SampleRate;

SdFat sd;
SdFile file;

#define error(msg) sd.errorHalt(F(msg))

#define IR1 0x5A
//------------------------------------------------------------------------------------------------------
//Specific component data
struct Data_t {
  uint32_t usec;
  float Acel_x;
  float Acel_y;
  float Acel_z;
  float VelAng_x;
  float VelAng_y;
  float VelAng_z;
  
  float Acel_x2;
  float Acel_y2;
  float Acel_z2;
  float VelAng_x2;
  float VelAng_y2;
  float VelAng_z2;

  float temp1;
} Tx_Data, Rx_Data;

Adafruit_MPU6050 mpu;
Adafruit_MPU6050 mpu2;
Adafruit_MLX90614 mlx1;
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
  sensors_event_t a, g, t;
  sensors_event_t a2, g2, temp2;  
  int cte = 1;

  while(1){
    if(xSemaphoreTake(timerSemaphore, portMAX_DELAY) == pdTRUE){
      mpu.getEvent(&a, &g, &t);
      mpu2.getEvent(&a2, &g2, &temp2);
      Tx_Data.usec = millis();
      Tx_Data.Acel_x = a.acceleration.x;
      Tx_Data.Acel_y = a.acceleration.y;
      Tx_Data.Acel_z = a.acceleration.z;
      Tx_Data.VelAng_x = g.gyro.x;
      Tx_Data.VelAng_y = g.gyro.y;
      Tx_Data.VelAng_z = g.gyro.z;
      Tx_Data.Acel_x2 = a2.acceleration.x;
      Tx_Data.Acel_y2 = a2.acceleration.y;
      Tx_Data.Acel_z2 = a2.acceleration.z;
      Tx_Data.VelAng_x2 = g2.gyro.x;
      Tx_Data.VelAng_y2 = g2.gyro.y;
      Tx_Data.VelAng_z2 = g2.gyro.z;

      Tx_Data.temp1 = mlx1.readObjectTempC();


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

    Serial.print("Sensor 1 no endereco 0x68: ");
    Serial.print(Rx_Data.Acel_x);
    Serial.print("\t");
    Serial.print(Rx_Data.Acel_y);
    Serial.print("\t");
    Serial.print(Rx_Data.Acel_z);
    Serial.print("\t");
    Serial.print(Rx_Data.VelAng_x);
    Serial.print("\t");
    Serial.print(Rx_Data.VelAng_y);
    Serial.print("\t");
    Serial.print(Rx_Data.VelAng_z);
    
    Serial.print("\t");
    Serial.print("Sensor 2 no endereco 0x69: ");
    Serial.print(Rx_Data.Acel_x2);
    Serial.print("\t");
    Serial.print(Rx_Data.Acel_y2);
    Serial.print("\t");
    Serial.print(Rx_Data.Acel_z2);
    Serial.print("\t");
    Serial.print(Rx_Data.VelAng_x2);
    Serial.print("\t");
    Serial.print(Rx_Data.VelAng_y2);
    Serial.print("\t");
    Serial.print(Rx_Data.VelAng_z2);

    Serial.print("\t");
    Serial.print("Sensor de temperatura: ");
    Serial.print(Rx_Data.temp1);
    Serial.print("\t");
    
    Serial.print(", ");
    Serial.println(Rx_Data.usec);
     
    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    //Serial.println(uxHighWaterMark);
  }
  vTaskDelete(NULL);
}
/*
void TaskSD(void *pvParam){
  (void) pvParam;
  while(1){
    if(xQueueReceive(DataQueue, &Rx_Data, portMAX_DELAY) != pdPASS){
      Serial.println("XQueueReceive is not working");
    }

    file.print(Rx_Data.Acel_x);
    file.print("\t");
    file.print(Rx_Data.Acel_y);
    file.print("\t");
    file.print(Rx_Data.Acel_z);
    file.print("\t");
    file.print(Rx_Data.VelAng_x);
    file.print("\t");
    file.print(Rx_Data.VelAng_y);
    file.print("\t");
    file.print(Rx_Data.VelAng_z);
    file.print(", ");
    file.println(Rx_Data.usec);

    if(!file.sync() || file.getWriteError()){
      error("write error");
    }  
      
    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    //Serial.println(uxHighWaterMark);
  }
  vTaskDelete(NULL); 
}
*/
//------------------------------------------------------------------------------------------------------
//Setup function
void setup() {
  
  Serial.begin(115200);
  
  //------------------------------------------------------------------------------------------------------
  //Queue inicialization
  DataQueue = xQueueCreate(10, sizeof(Data_t));
  if(DataQueue == NULL){
    Serial.println("Error creating queue");
  }

  //------------------------------------------------------------------------------------------------------
  //Hardware timers initialization
  timerSemaphore = xSemaphoreCreateBinary();
  timer = timerBegin(1, 80, true); //Initialize timer. param(hardware timer number [0-4], prescaler [80 MHz divisor], [true: count up, false: count down])
  timerAttachInterrupt(timer, &vTimerISR, true); //bind timer to handling function
  timerAlarmWrite(timer, SampleIntUs, true); //timer count value in us, depending on prescaler. true: reload counter
  timerAlarmEnable(timer);

  //------------------------------------------------------------------------------------------------------  
  //SD card initialization 
  //if(!sd.begin(chipSelect, SD_SCK_MHZ(10))){
  //  Serial.println("Erro no sd.begin"); 
  //  sd.initErrorHalt();
  //}

  //if(!file.open("Teste.csv", O_WRONLY | O_CREAT | O_EXCL)){
  //  error("file.open");
  //}

  //------------------------------------------------------------------------------------------------------  
  //Component initialization
    // Try to initialize!
if (!mpu2.begin(0x69) || !mpu.begin(0x68)) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050s Found!");

  if(mlx1.begin()){
    Serial.println("Sensor Inicializado!");
  }
  mlx1.AddrSet(IR1);

  Serial.println("");
  delay(100);
  //------------------------------------------------------------------------------------------------------
  //Tasks creation
  //Software timer task
  /*data_timer = xTimerCreate(
                      "Polling timer",                // Name of timer
                      SampleIntMs/portTICK_PERIOD_MS, // Period of timer (in ticks)
                      pdFALSE,                        // Auto-reload
                      (void *)0,                      // Timer ID
                      timerCallback);                 // Callback function
  */                    
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
             2 , 
             &xPrint);
  /*
  xTaskCreate(TaskSD,
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
