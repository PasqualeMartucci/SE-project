#include <Arduino_FreeRTOS.h>
#include <Wire.h>
#include "Adafruit_HTU21DF.h"
#include <LiquidCrystal.h> 
#include "Adafruit_CCS811.h"
#include <semphr.h>

#define HTU 1
#define CCS 2
#define STAMP 0

Adafruit_CCS811 ccs;
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
SemaphoreHandle_t mutex;
SemaphoreHandle_t s_CCS;
SemaphoreHandle_t s_HTU;
SemaphoreHandle_t s_COM;  

void taskCCS811(void *pvParameters);
void taskHTU21D(void *pvParameters);
void taskComunication(void *pvParameters);


int arrayVariables[4]; // array di float per immagazzinare le variabili 


int n_CCS,n_COM,n_HTU;
int b_CCS,b_COM,b_HTU;
int stato;
/*funzioni*/


void startLetturaCCS(void *pvParameters){
  xSemaphoreTake(mutex,( TickType_t ) 100 );
   if(n_CCS || stato != CCS ){
    b_CCS++;
    Serial.println("SENSORE IN ATTESA\n");
   }else{
    n_CCS++;
    xSemaphoreGive(s_CCS);
   }
  xSemaphoreGive(mutex);
  xSemaphoreTake(s_CCS,( TickType_t ) 100 ); 
  Serial.println("SENSORE ATTIVO");
}



void read_data_CCS(){
 
    if(!ccs.readData()){
    arrayVariables[2] = ccs.geteCO2();
    arrayVariables[3] = ccs.getTVOC();
    }else{
      Serial.println("ERRORE\n");
    }
  
}

void endLetturaCCS(void *pvParameters){
  xSemaphoreTake(mutex,( TickType_t ) 100 );
  n_CCS--;
  stato = STAMP;
  Serial.println("SENSORE disattivato");
   if(b_COM){
   xSemaphoreGive(s_COM);  
  }
  xSemaphoreGive(mutex);
}




void startLetturaHTU(void *pvParameters){
  xSemaphoreTake(mutex,( TickType_t ) 100 );
   if(n_HTU || stato != HTU){
    b_HTU++;
    Serial.println("SENSORE IN ATTESA\n");
   }else{
    n_HTU++;
    xSemaphoreGive(s_HTU);
   }
  xSemaphoreGive(mutex);
  xSemaphoreTake(s_HTU,( TickType_t ) 100 ); 
  Serial.println("SENSORE ATTIVO");
}

void read_data_HTU(){
  arrayVariables[0] = htu.readTemperature();
  arrayVariables[1] = htu.readHumidity();
}


void endLetturaHTU(void *pvParameters){
  xSemaphoreTake(mutex,( TickType_t ) 100 );
  n_HTU--;
  stato = CCS;
  Serial.println("SENSORE disattivato");
   if(b_CCS){
     b_CCS--;
     n_CCS++;
   xSemaphoreGive(s_CCS);  
  }
  xSemaphoreGive(mutex);
}



void startComunication(void *pvParameters){
  xSemaphoreTake(mutex,( TickType_t ) 100 );
  if(n_COM || stato != STAMP){
    b_COM++;
    Serial.println("COM IN ATTESA");
    xSemaphoreGive(mutex);
    xSemaphoreTake(s_COM,( TickType_t ) 100 );
    b_COM--; 
  }
  n_COM++;
  xSemaphoreGive(mutex);
  Serial.println("INIZIO A STAMPARE");
  }


  void stamp (){
    Serial.println("TEMP -->");
    Serial.println(arrayVariables[0]);
    Serial.println("HUM -->");
    Serial.println(arrayVariables[1]);
    Serial.println("CO2 -->");
    Serial.println(arrayVariables[2]);
    Serial.println("TVOC -->");
    Serial.println(arrayVariables[3]);
  }


void endComunication(void *pvParameters){
  xSemaphoreTake(mutex,( TickType_t ) 100 );
  n_COM--;
  stato= HTU;
  Serial.println("resetto lo stato a read");
  
  if(b_HTU){
    b_HTU--;
    n_HTU++;
    xSemaphoreGive(s_HTU);  
  }else{
    xSemaphoreGive(mutex);
  }
  }




void setup() { 


Serial.begin(9600);
  for(int i=0;i<4;i++){       // function init prossima implementazione                     
  arrayVariables[i]=-1;                            
 }
  stato = HTU;
Serial.print("Setup");
  n_CCS = n_COM = n_HTU = 0 ;
  b_CCS = b_COM = b_HTU = 0 ;            

 
  if ( mutex == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
  {
    mutex = xSemaphoreCreateMutex();  
  }
  if(s_COM == NULL){
    s_COM = xSemaphoreCreateBinary();
  }
   if(s_CCS == NULL){
    s_CCS = xSemaphoreCreateBinary();
  }
   if(s_HTU == NULL){
    s_HTU = xSemaphoreCreateBinary();
  }

  if(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
  }
  while(!ccs.available()){ 
  }
  Serial.println("Sensore pronto");

  
   xTaskCreate(
    taskCCS811
    ,  "task-temp"
    ,  128
    ,  NULL
    ,  1  // priority
    ,  NULL );


            xTaskCreate(
    taskComunication
    ,  "task-com"
    , 128
    ,  NULL
    ,  1  // priority
    ,  NULL );
      
     xTaskCreate(
    taskHTU21D
    ,  "task-temp"
    ,  128
    ,  NULL
    ,  1  // priority
    ,  NULL );

    
}


void taskCCS811(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
    
  for (;;)
  {
    startLetturaCCS(pvParameters);  
    vTaskDelay( 100 / portTICK_PERIOD_MS ); // wait for one second
    read_data_CCS();
    vTaskDelay( 100 / portTICK_PERIOD_MS );
    endLetturaCCS(pvParameters);   
  }
}


void taskComunication(void *pvParameters)  
{
  (void) pvParameters;
  
  for (;;) 
  {
    startComunication(pvParameters);  
    vTaskDelay( 100 / portTICK_PERIOD_MS ); // wait for one second
    stamp();
    vTaskDelay( 100 / portTICK_PERIOD_MS ); // wait for one second
    endComunication(pvParameters);
  }
}

void taskHTU21D(void *pvParameters)  
{
  (void) pvParameters;
   htu.begin();
  for (;;) 
  {
    startLetturaHTU(pvParameters);  
    vTaskDelay( 100 / portTICK_PERIOD_MS ); // wait for one second
    read_data_HTU();
    vTaskDelay( 100 / portTICK_PERIOD_MS );
    endLetturaHTU(pvParameters);   
  }
}



void loop() {    
 //empty loop   
}
