#include <Arduino.h>

/* Feeding Grass by Sam */

/* Configuration */
#define APP_VERSION             "01.00.00" // Version application

#define SENSOR_TARGET_LEVEL     600        // Level for target level of moisture in grass
#define SENSOR_MIN_LEVEL        950        // Level for minimum level for moisture in grass
#define TIMEOUT_PUMP            5000       // Timeout in millisecond
#define NBE_RETRY               3          // Number of retry when timeout
#define PUMP_PIN                7          // Pin number for pump

// Declaration typdef
typedef enum {
  STATE_IDLE,
  STATE_IS_FEEDING, 
  STATE_IS_FINISH,
  STATE_FORCE_FEEDING,
}state_t;



// Declaration global variable
static int            sensorPin   = A0;             // Arduino pin analog A0
static state_t        state       = STATE_IDLE;     // Initial state
static unsigned long  timeout     = 0;              // Timeout global
static int            pump_error  = 0;              // Error global 
static int            retry       = 0;              // Number of retry count on error 

// Class for water manage  water pump
class WaterPump  {
  public: 
    WaterPump(pin_size_t argpin = 7 ){
      pin = argpin;
    }
    void begin(){
      pinMode(pin, OUTPUT);
      digitalWrite(pin, HIGH);
    }
    void On(){
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(pin, LOW);  
    }
    void Off(){
      digitalWrite(LED_BUILTIN, LOW);
      digitalWrite(pin, HIGH); 
    }

  private:
    pin_size_t pin; 
};


// Instance Pump
WaterPump Pump (PUMP_PIN);


/* Entry function for setup uC*/
void setup() {
  
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // Init pin for water pump for relay
  Pump.begin();
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
          Pump.On();
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
  switch(state){  
    
    // Read continously cap^tor and verify level of moisture
    case STATE_IDLE:

      // Check if moisture is low
      if( (sensorValue > SENSOR_MIN_LEVEL) && (retry < NBE_RETRY)){
      
        // Active pump for feeding
        Pump.On();
        state = STATE_IS_FEEDING;
        timeout = millis();
        Serial.println("State : is feeding...");
      }

      // reset error because grass is normaly feeding 
      if( (sensorValue < SENSOR_TARGET_LEVEL) && (retry >= NBE_RETRY)){
        retry = 0;
        pump_error = 0;
      }
      break;


    /* Feeding is progress state. Detected here timeout or target value for sensor */
    case STATE_IS_FEEDING:
      
      // check if level is ok 
      if( sensorValue < SENSOR_TARGET_LEVEL){
        Pump.Off();
        state = STATE_IS_FINISH;
        Serial.println("State : is finish");
      }

      // Timeout security for no pump all water 
      if( millis() > ( timeout + TIMEOUT_PUMP)) {
        Pump.Off();
        state = STATE_IS_FINISH;
        pump_error = 1;
        Serial.println("State : is finish");
      }

      break;


    /* State for force feeding by serial message */ 
    case STATE_FORCE_FEEDING:
       // Timeout security for no pump all water 
      if( millis() > ( timeout + TIMEOUT_PUMP)) {
        Pump.Off();
        state = STATE_IS_FINISH;
        Serial.println("State : is finish");
        pump_error = 0;
        retry = 0;
      }
      break; 


    /* End state */
    case STATE_IS_FINISH:
      if (pump_error){
        Serial.print("State : Error ");
        Serial.print(pump_error);
        Serial.println(" : Timeout");
        Serial.println("State : Retry Feeding");
        retry++;
        pump_error = 0;
      }
      else{
        retry = 0;
      }
      
      Pump.Off();
      state = STATE_IDLE;
      break;
  }
}