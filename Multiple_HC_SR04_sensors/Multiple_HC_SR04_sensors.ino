#include <NewPing.h>

/**
*Number of sensors
*/
#define SONAR_NUM 5
/**
*Maximum distance (in cm) to ping.
*/
#define MAX_DISTANCE 100
/**
*Maximum distance range (in cm)
*/
#define MAX_DISTANCE_2 400
/**
*Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).
*/
#define PING_INTERVAL 35
/**
*Ping frequency (in milliseconds), fastest we should ping is about 35ms per sensor
*/
#define PING_SPEED 250
/**
*Holds the times when the next ping should happen for each sensor.
*/
unsigned long pingTimer[SONAR_NUM]; 
/**
*Where the ping distances are stored.
*/
unsigned int cm[SONAR_NUM];          
/**
*Keeps track of which sensor is active.
*/
uint8_t currentSensor = 0;         
/**
*distance from quadriceps to ground.
*/
int setup_value;
/**
*number of values for the setup
*/
int values_number;
/**
*Echo Pins of the ultrasonic sensors
*/
int echoPins[SONAR_NUM]={
  A0,A1,A2,A3,A4};       
  /**
*vibrations
*/
int vibrations[SONAR_NUM]; 
/**
*Trigger pins of the ultrasonic sensors.
*/
int motPins[SONAR_NUM] = {
  5,3,9,6,10};  
  /**
*Sensor object array.

*\param NewPing(2,A0,MAX_DISTANCE)  leg-sx (high)
*\param NewPing(4,A1,MAX_DISTANCE)  leg-sx (low)
*\param NewPing(7,A2,MAX_DISTANCE)  leg-dx (high) -> for holes, stairs
*\param NewPing(8,A3,MAX_DISTANCE)  leg-dx (low)
*\param NewPing(12,A4,MAX_DISTANCE) belt

*/
NewPing sonar[SONAR_NUM] = { 
	
//Each sensor's trigger pin, echo pin, and max distance to ping.
	
  NewPing(2, A0, MAX_DISTANCE), 
  NewPing(4, A1, MAX_DISTANCE), 
  NewPing(7, A2, MAX_DISTANCE), 
  NewPing(8, A3, MAX_DISTANCE), 
  NewPing(12, A4, MAX_DISTANCE) 
  };  

  /**
*The setup() function is called when a sketch starts
*/
void setup() {
  Serial.begin(9600);

  set_ping_interval();
  set_setup_value();
}
/**
*The loop() function loops consecutively allowing your program to change and respond 
*/

void loop() {
  for (uint8_t i = 0; i < SONAR_NUM; i++) { //Loop through all the sensors.
    if (millis() >= pingTimer[i]) { 
      if(i == 0){
        pingTimer[i] += PING_SPEED;  //Make sensor 1 fire again "PING_SPEED" ms later   
      }
      else{
        pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;    //Make next sensor fire again "PING_INTERVAL" second after previous sensor fires 
      }

      if(i == 0 && currentSensor == SONAR_NUM - 1){   //When all the sensors has pinged 
        vibrate();
        printResults();
      }

      currentSensor = i;  //Sensor being accessed.                 

      cm[currentSensor] = sonar[currentSensor].ping_cm();      //Send a ping, returns the distance in centimeters or 0 (zero) if no ping echo within set distance limit

      if(currentSensor == 2)  //sensor that recognises holes
      {
        if(cm[currentSensor] == 0 || (cm[currentSensor] - 10) > setup_value || (cm[currentSensor] + 10) < setup_value)  //if the no ping echo or the distance is higher than 75
        {
          vibrations[currentSensor]=255; //vibrate
        }
        else
        {
          vibrations[currentSensor]=0;  //no vibrate
        }
      }
      else
      {
        if(cm[currentSensor] == 0){    //if the no ping echo
          reset();
          vibrations[currentSensor]=0;
        }
        else
        {
          if(cm[currentSensor] == 1){   //adjust minimum range to 2cm
            cm[currentSensor] = 2;
          }

          vibrations[currentSensor] = map(cm[currentSensor], 2, MAX_DISTANCE, 255, 80); //calculate vibration according to the distance 
        }
      }
    }
  }
 
}

/**
*The reset() function is called to avoid the interruption of the sensor
*/
void reset(){     
  pinMode( echoPins[currentSensor], OUTPUT );
  digitalWrite( echoPins[currentSensor], LOW );
  delayMicroseconds(10);
  pinMode( echoPins[currentSensor], INPUT); 
}

/**
* the printResults() prints the results on the Serial monitor (used by developers)
*/
void printResults() { 
  Serial.print("- ");

  for(uint8_t i = 0; i < SONAR_NUM; i++){
    Serial.print("SENSOR ");
    Serial.print(i);
    Serial.print(": ");

    Serial.print(cm[i]);
    Serial.print("cm - "); 

    Serial.print(vibrations[i]);
    Serial.print("vib - ");
  }

  Serial.println();
  Serial.println();
}

/**
*the vibrate() function makes vibrate all the vibration motors with respective vibrations
*/
void vibrate(){ 
  for(uint8_t i=0; i < SONAR_NUM; i++){
    analogWrite(motPins[i], vibrations[i]);
  }
}

/**
*the set_setup_value() function sets the setup_value on the basis of the height of the person
*/

void set_setup_value() { 
  int sum=0,temp,numbers[values_number];
  boolean ok=false;
  while(!ok) {
    for(int i=0;i<values_number;i++) // needed at least ten values
    {
      temp = sonar[2].ping_cm();  //send a ping
  
      if(temp != 0) //if the ping is not out of range
      {
        sum += temp; //add the value to the sum
        numbers[i]=temp;
      }
      else
      {
        i--;  //ping another value
      }
      Serial.println();
      Serial.println(temp);
      delay(35);
    }
  
    setup_value = sum/10; //average of the ten values pinged
    Serial.println(setup_value);
    
    if(calcola_varianza(numbers)) {
      ok=true;
    }
  }
}

/**
*the set_ping_interval() function sets the setup_value on the basis of the height of the person
*/
void set_ping_interval() {
  pingTimer[0] = millis() + PING_SPEED; //First ping starts at "PING_SPEED" ms, gives time for the Arduino to chill before starting.

  for (uint8_t i = 1; i < SONAR_NUM; i++)   //Set the starting time for each sensor.  
  {
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
  }
}

boolean calcola_varianza(int numbers[]) {
  int somma=0,varianza;
  for(int i=0;i<values_number;i++) {
    somma+=(numbers[i]-setup_value)^2;
  }
  
  varianza=somma/values_number;
  
  if(varianza>2) {
    return false;
  }
  else {
    return true;
  }
}

