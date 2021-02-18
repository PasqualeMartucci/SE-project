#include <Arduino_FreeRTOS.h>
#include <Wire.h>
#include "Adafruit_HTU21DF.h"
#include <LiquidCrystal.h> 
#include "Adafruit_CCS811.h"
#include <semphr.h>

#define READ 1
#define STAMP 0

Adafruit_HTU21DF htu = Adafruit_HTU21DF();
SemaphoreHandle_t mutex;
SemaphoreHandle_t s_HTU;
SemaphoreHandle_t s_COM;  

void taskHTU21D(void *pvParameters);
void taskComunication(void *pvParameters);


float arrayVariables[2]; // array di float per immagazzinare le variabili 


int n_HTU,n_COM;
int b_HTU,b_COM;
int stato;
/*funzioni*/


void startLetturaHTU(void *pvParameters){
  xSemaphoreTake(mutex,( TickType_t ) 100 );
   if(n_COM || n_HTU || stato != READ){
    b_HTU++;
    //Serial.println("SENSORE IN ATTESA\n");
   }else{
    n_HTU++;
    xSemaphoreGive(s_HTU);
   }
  xSemaphoreGive(mutex);
  xSemaphoreTake(s_HTU,( TickType_t ) 100 ); 
  //Serial.println("SENSORE ATTIVO");
}

void endLetturaHTU(void *pvParameters){
  xSemaphoreTake(mutex,( TickType_t ) 100 );
  n_HTU--;
  stato = STAMP;
  //Serial.println("SENSORE disattivato");
   if(b_COM){
   xSemaphoreGive(s_COM);  
  }
  xSemaphoreGive(mutex);
}

void read_data(){
  arrayVariables[0] = htu.readTemperature();
  arrayVariables[1] = htu.readHumidity();
}



void startComunication(void *pvParameters){
  xSemaphoreTake(mutex,( TickType_t ) 100 );
  if(n_COM || n_HTU || stato != STAMP){
    b_COM++;
    //Serial.println("COM IN ATTESA");
    xSemaphoreGive(mutex);
    xSemaphoreTake(s_COM,( TickType_t ) 100 );
    b_COM--; 
  }
  n_COM++;
  xSemaphoreGive(mutex);
  //Serial.println("INIZIO A STAMPARE");
  }


  void stamp (){
    Serial.print("TEMP -->");
    Serial.print(arrayVariables[0]);
    Serial.print("HUM -->");
    Serial.print(arrayVariables[1]);
  }


void endComunication(void *pvParameters){
  xSemaphoreTake(mutex,( TickType_t ) 100 );
  n_COM--;
  stato= READ;
  //Serial.println("resetto lo stato a read");
  
  if(b_HTU){
    b_HTU--;
    n_HTU++;
    xSemaphoreGive(s_HTU);  
  }else{
    xSemaphoreGive(mutex);
  }
  }




void setup() { 

  for(int i=0;i<2;i++){       // function init prossima implementazione                     
  arrayVariables[i]=-1;                            
 }
  stato = READ;

  n_HTU  = n_COM = 0 ;
  b_HTU  = b_COM = 0 ;            
 
  if ( mutex == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
  {
    mutex = xSemaphoreCreateMutex();  
  }
  if(s_COM == NULL){
    s_COM = xSemaphoreCreateBinary();
  }
   if(s_HTU == NULL){
    s_HTU = xSemaphoreCreateBinary();
  }
   xTaskCreate(
    taskHTU21D
    ,  "task-temp"
    ,  128 
    ,  NULL
    ,  1  // priority
    ,  NULL );


            xTaskCreate(
    taskComunication
    ,  "task-com"
    ,  128 
    ,  NULL
    ,  1  // priority
    ,  NULL );
      
    
}


void taskHTU21D(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
   htu.begin();
  for (;;)
  {
    startLetturaHTU(pvParameters);  
    vTaskDelay( 100 / portTICK_PERIOD_MS ); // wait for one second
    read_data();
    vTaskDelay( 100 / portTICK_PERIOD_MS );
    endLetturaHTU(pvParameters);   
  }
}


void taskComunication(void *pvParameters)  
{
  (void) pvParameters;
  Serial.begin(9600);
  for (;;) 
  {
    startComunication(pvParameters);  
    vTaskDelay( 100 / portTICK_PERIOD_MS ); // wait for one second
    stamp();
    vTaskDelay( 100 / portTICK_PERIOD_MS ); // wait for one second
    endComunication(pvParameters);
  }
}





void loop() {    
 //empty loop   
}
