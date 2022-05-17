#include <Arduino.h>
#include <HardwareSerial.h>

HardwareSerial Serial;

/* Feeding Grass by Sam */

/* Configuration */
#define APP_VERSION             "01.00.00" // Version application

#define SENSOR_TARGET_LEVEL     600        // Level for target level of moisture in grass
#define SENSOR_MIN_LEVEL        950        // Level for minimum level for moisture in grass
#define TIMEOUT_POMP            5000       // Timeout in millisecond
#define NBE_RETRY               3          // Number of retry when timeout

// Declaration typdef
typedef enum {
  STATE_IDLE,
  STATE_IS_FEEDING, 
  STATE_IS_FINISH,
  STATE_FORCE_FEEDING,
}state_t;



// Declaration global variable
static int            sensorPin   = A0;             // Arduino pin analog A0
static int            waterPomp   = 7;              // Arduino Pin digital 7 
static state_t        state       = STATE_IDLE;     // Initial state
static unsigned long  timeout     = 0;              // Timeout global
static int            error       = 0;              // Error global 
static int            retry       = 0;              // Number of retry count on error 

/* Entry function for setup uC*/
void setup() {

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // Init pin for water pomp for relay
  pinMode(waterPomp, OUTPUT);
  digitalWrite(waterPomp, HIGH);
}


/* Entry point for main loop function */
void loop() {

  // Pooling check moisture sensor
  int sensorValue = analogRead(sensorPin);

  // Traitement asking serial message
  if(Serial.available()){
    char car = Serial.read();
    switch (car){
      
      // Message to ask sensor value
      case 'c':
        Serial.print("Captor value : ");
        Serial.println(sensorValue);
        break;

      // Message to force feed grass
      case 'f':
        if( state == STATE_IDLE ){
          state = STATE_FORCE_FEEDING;
          Serial.println("State : Force feed");
          timeout = millis();
          digitalWrite(waterPomp, LOW);
        }
        else
          Serial.println("Forbidden command at this moment");
        break;

      // List all command
      case 'h':
        Serial.println("");
        Serial.println("****** Feed Grass App ******");
        Serial.print("App version : ");
        Serial.println(APP_VERSION);
        Serial.println("c : Read captor");
        Serial.println("f : Force one time feeding");
        Serial.println("s : Stop all command");
        Serial.println("v : Read version");
        break;

      // Force stop
      case 's':
        state = STATE_IS_FINISH;
        Serial.println("Force Stop");
        break;

      // Ask Version
      case 'v':
        Serial.println(APP_VERSION);
        break;

      default:
        break;
    }
  }

  // Main FSM
  switch( state ) {  
    
    // Read continously cap^tor and verify level of moisture
    case STATE_IDLE:

      // Check if moisture is low
      if( (sensorValue > SENSOR_MIN_LEVEL) && (retry < NBE_RETRY)){
      
        // Active pomp for feeding
        digitalWrite(waterPomp, LOW);
        state = STATE_IS_FEEDING;
        timeout = millis();
        Serial.println("State : is feeding...");
      }

      // reset error because grass is normaly feeding 
      if( (sensorValue < SENSOR_TARGET_LEVEL) && (retry >= NBE_RETRY)){
        retry = 0;
        error = 0;
      }
      break;


    /* Feeding is progress state. Detected here timeout or target value for sensor */
    case STATE_IS_FEEDING:
      
      // check if level is ok 
      if( sensorValue < SENSOR_TARGET_LEVEL){
        digitalWrite(waterPomp, HIGH);
        state = STATE_IS_FINISH;
        Serial.println("State : is finish");
      }

      // Timeout security for no pomp all water 
      if( millis() > ( timeout + TIMEOUT_POMP)) {
        digitalWrite(waterPomp, HIGH);
        state = STATE_IS_FINISH;
        error = 1;
        Serial.println("State : is finish");
      }

      break;


    /* State for force feeding by serial message */ 
    case STATE_FORCE_FEEDING:
       // Timeout security for no pomp all water 
      if( millis() > ( timeout + TIMEOUT_POMP)) {
        digitalWrite(waterPomp, HIGH);
        state = STATE_IS_FINISH;
        Serial.println("State : is finish");
        error = 0;
        retry = 0;
      }
      break; 


    /* End state */
    case STATE_IS_FINISH:
      if (error){
        Serial.print("State : Error ");
        Serial.print(error);
        Serial.println(" : Timeout");
        Serial.println("State : Retry Feeding");
        retry++;
        error = 0;
      }
      else{
        retry = 0;
      }
      
      digitalWrite(waterPomp, HIGH);  // For security
      state = STATE_IDLE;
      break;
  }
}
