#include <NewPing.h>

#define SONAR_NUM 5 // Number of sensors.
#define MAX_DISTANCE 200 // Maximum distance (in cm) to ping.
#define MAX_DISTANCE_2 400 // Maximum distance range (in cm).
#define PING_INTERVAL 35 // Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).
#define PING_SPEED 250 // Ping frequency (in milliseconds), fastest we should ping is about 35ms per sensor

unsigned long pingTimer[SONAR_NUM]; // Holds the times when the next ping should happen for each sensor.
unsigned int cm[SONAR_NUM];         // Where the ping distances are stored.
uint8_t currentSensor = 0;          // Keeps track of which sensor is active.

int echoPins[SONAR_NUM]={A0,A1,A2,A3,A4};       // Echo Pins of the ultrasonic sensors

int vibrations[SONAR_NUM];  //vibrations
int motPins[SONAR_NUM] = {3,5,6,9,10};  //Trigger pins of the vibration motors

NewPing sonar[SONAR_NUM] = {   // Sensor object array.
  NewPing(2, A0, MAX_DISTANCE), // Each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(4, A1, MAX_DISTANCE),
  NewPing(7, A2, MAX_DISTANCE_2),
  NewPing(8, A3, MAX_DISTANCE),
  NewPing(12, A4, MAX_DISTANCE)
};

void setup() {
  Serial.begin(9600);
  
  pingTimer[0] = millis() + PING_SPEED;           // First ping starts at 100ms, gives time for the Arduino to chill before starting.
  for (uint8_t i = 1; i < SONAR_NUM; i++) // Set the starting time for each sensor.
  {
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
  } 
}

void loop() {
  for (uint8_t i = 0; i < SONAR_NUM; i++) { // Loop through all the sensors.
    if (millis() >= pingTimer[i]) { 
      if(i == 0){
        pingTimer[i] += PING_SPEED;     // Make sensor 1 fire again "PING_SPEED" second later
      }
      else{
        pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;     // Make next sensor fire again "PING_INTERVAL" second after previous sensor fires
      }
      
      if(i == 0 && currentSensor == SONAR_NUM - 1){     // When all the sensors has pinged 
        vibrate();  //vibrate function is called
        printResults();  //printResults function is called
      }
      
      currentSensor = i;                          // Sensor being accessed.
      cm[currentSensor] = sonar[currentSensor].ping_cm();     // Send a ping, returns the distance in centimeters or 0 (zero) if no ping echo within set distance limit 
      
      if(currentSensor == 2)  //sensor that recognises ground's irregularities
      {
        if(cm[currentSensor] == 0 || cm[currentSensor] > 75)  //if the no ping echo or the distance is higher than 75
        {
          vibrations[currentSensor]=255;  //vibration
        }
        else
        {
          vibrations[currentSensor]=0;  //no vibration
        }
      }
      else
      {
        if(cm[currentSensor] == 0){     // if the no ping echo
          reset();  // reset function is called
          vibrations[currentSensor]=0;  //vibration = 0
        }
        else
        {
          if(cm[currentSensor] == 1){  // adjust minimum range to 2cm
          cm[currentSensor] = 2;
          }
          
          vibrations[currentSensor] = map(cm[currentSensor], 2, MAX_DISTANCE, 255, 80);  //calculate vibration according to the distance
        }
      }
    }
  }
  // Other code that *DOESN'T* analyze ping results can go here.
}

void reset(){     // This function is called to avoid the interruption of the sensor
  pinMode( echoPins[currentSensor], OUTPUT );
  digitalWrite( echoPins[currentSensor], LOW );
  delayMicroseconds(10);
  pinMode( echoPins[currentSensor], INPUT); 
}

void printResults() { // Results are printed on the Serial monitor
  Serial.print("- ");
  
  for(uint8_t i = 0; i < SONAR_NUM; i++){
    Serial.print("SENSOR ");
    Serial.print(i);
    Serial.print(": ");
    
    if(cm[i] == 0){
      Serial.print("RESET - ");
    }
    else{
      Serial.print(cm[i]);
      Serial.print("cm - "); 
    }
    
    Serial.print(vibrations[i]);
    Serial.print("vib - ");
  }
  
  Serial.println();
  Serial.println();
}

void vibrate(){
  for(uint8_t i=0; i < SONAR_NUM; i++){
      analogWrite(motPins[i], vibrations[i]);
  }
}
