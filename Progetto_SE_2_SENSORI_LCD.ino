#include <Arduino_FreeRTOS.h>
#include <Wire.h>
#include "Adafruit_HTU21DF.h"
#include <LiquidCrystal.h> 
#include "Adafruit_CCS811.h"
#include <semphr.h>

#define HTU 1
#define CCS 2
#define STAMP 0
#define LCD 3
#define LED 4


int Contrast=120;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);  
Adafruit_CCS811 ccs;
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
SemaphoreHandle_t mutex;
SemaphoreHandle_t s_CCS;
SemaphoreHandle_t s_HTU;
SemaphoreHandle_t s_COM;  
SemaphoreHandle_t s_LCD; 
SemaphoreHandle_t s_LED; 

void taskCCS811(void *pvParameters);
void taskHTU21D(void *pvParameters);
void taskComunication(void *pvParameters);
void taskLCD(void *pvParameters);
//void taskLED(void *pvParameters);

int arrayVariables[4]; // array di float per immagazzinare le variabili 


int n_CCS,n_COM,n_HTU,n_LCD,n_LED;
int b_CCS,b_COM,b_HTU,b_LCD,b_LED;
int stato;
float time;
float time_nuovo;
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
  stato= LCD;
  Serial.println("resetto lo stato a read");
  
  if(b_LCD){
    b_LCD--;
    n_LCD++;
    xSemaphoreGive(s_LCD);  
  }else{
    xSemaphoreGive(mutex);
  }
  }

void startLCD(void *pvParameters){
xSemaphoreTake(mutex,( TickType_t ) 100 );

if(n_LCD || stato != LCD){
  b_LCD++;
  Serial.println("LCD IN ATTESA");
}
else {
  n_LCD++;
  xSemaphoreGive(s_LCD);
  Serial.println("LCD ATTIVO");
}

xSemaphoreGive(mutex);
xSemaphoreTake(s_LCD,( TickType_t ) 100 );
}

void stamp_LCD(){
lcd.setCursor(0,0);
lcd.print("T:");
lcd.setCursor(3,0);
lcd.print(String(arrayVariables[0]));


lcd.setCursor(8,0);
lcd.print("H:");
lcd.setCursor(10,0);
lcd.print(String(arrayVariables[1]));


lcd.setCursor(0,1);
lcd.print("CO2:");
lcd.setCursor(4,1);
lcd.print(String(arrayVariables[2]));

lcd.setCursor(8,1);
lcd.print("TVOC:");
lcd.setCursor(13,1);
lcd.print(String(arrayVariables[3]));

}


void endLCD(void *pvParameters){
xSemaphoreTake(mutex,( TickType_t ) 100 );
n_LCD--;
stato= HTU;
if(b_HTU){
  b_HTU--;
  xSemaphoreGive(s_HTU);
}
xSemaphoreGive(mutex);
}


void setup() { 

Serial.begin(9600);

  for(int i=0;i<4;i++){       // function init prossima implementazione                     
  arrayVariables[i]=-1;                            
 }
  stato = HTU;
  n_CCS = n_COM = n_HTU = n_LCD = n_LED = 0 ;
  b_CCS = b_COM = b_HTU = b_LCD = b_LED = 0 ;            
 
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
 if(s_LCD == NULL){
    s_LCD = xSemaphoreCreateBinary();
  }
  if(s_LED == NULL){
    s_LED = xSemaphoreCreateBinary();
  }

  if(!ccs.begin()){
    Serial.println("Failed to start sensor! Please check your wiring.");
  }else{
    delay(4000); // delay for ccs in order to start
  }

/*time =0;
  time_nuovo = 0; 
  time = millis();         
  while(!ccs.available())  {}
  time_nuovo = millis() - time;    TEMPO STIMATO PER L'AVVIO DEL SENSORE, CIRCA 3963 MS
  Serial.println("Tempo stimato: ");
  Serial.println(time_nuovo); */


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
    ,  128
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

         xTaskCreate(
    taskLCD
    ,  "task-lcd"
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


void taskLCD(void *pvParameters)
{
  (void) pvParameters;
  analogWrite(7,Contrast);
   lcd.begin(16, 2);
   for(;;){
    startLCD(pvParameters);
    vTaskDelay( 100 / portTICK_PERIOD_MS ); // wait for one second
    stamp_LCD();
    vTaskDelay( 100 / portTICK_PERIOD_MS ); // wait for one second
    endLCD(pvParameters);
   }
}



void loop() {    
 //empty loop   
}