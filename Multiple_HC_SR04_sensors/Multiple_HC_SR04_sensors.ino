#include <NewPing.h>

/**
*Number of ultrasonic sensors
*/
#define SONAR_NUM 5
/**
*Maximum distance (in cm)
*/
#define MAX_DISTANCE 150
/**
*Maximum distance range (for the sensor that recognises irregularities)
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
*Distance from the belt to the ground (for the sensor that recognises irregularities)
*/
int setup_value;
/**
*Number of values for the setup
*/
int values_number=10;
/**
*Number of the sensor that recognises irregularities
*/
int sensor_number = 4;
/**
*Echo Pins of the ultrasonic sensors
*/
int echoPins[SONAR_NUM]={
  A0,A1,A2,A3,A4};       
/**
*Vibrations' values
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
*\param NewPing(12,A4,MAX_DISTANCE) leg-dx (high)
*\param NewPing(8,A3,MAX_DISTANCE)  leg-dx (low)
*\param NewPing(7,A2,MAX_DISTANCE)  belt -> for holes, stairs

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
  unsigned long initial_time = millis();
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

      if(currentSensor == sensor_number)  //Sensor that recognises holes
      {
        if(cm[currentSensor] == 0 || (cm[currentSensor] - 10) > setup_value || (cm[currentSensor] + 10) < setup_value)  //If the no ping echo or the distance is higher than 75
        {
          vibrations[currentSensor]=255; //Vibrate
        }
        else
        {
          vibrations[currentSensor]=0;  //No vibrate
        }
      }
      else
      {
        if(cm[currentSensor] == 0){    //If the no ping echo
          reset();
          vibrations[currentSensor]=0;
        }
        else
        {
          if(cm[currentSensor] == 1){   //Adjust minimum range to 2cm
            cm[currentSensor] = 2;
          }

          vibrations[currentSensor] = map(cm[currentSensor], 2, MAX_DISTANCE, 255, 80); //Calculate vibration according to the distance 
        }
      }
    }
  }
  unsigned long final_time = millis();
  unsigned long difference = final_time - initial_time;
  Serial.print("Time needed to execute the loop:");
  Serial.println(difference);
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
*The printResults() prints the results on the Serial monitor (used by developers)
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
*The vibrate() function makes vibrate all the vibration motors with respective vibrations
*/
void vibrate(){ 
  for(uint8_t i=0; i < SONAR_NUM; i++){
    analogWrite(motPins[i], vibrations[i]);
  }
}

/**
*The set_setup_value() function sets the setup_value on the basis of the height of the person
*/

void set_setup_value() { 
  int sum=0,temp,numbers[values_number];
  boolean ok=false;
  double average=0.0;
  while(!ok) {
    sum=0;
    Serial.println("Values");
    for(int i=0;i<values_number;i++) // Needed at least ten values
    {
      temp = sonar[sensor_number].ping_cm();  //Send a ping
  
      if(temp != 0) //If the ping is not out of range
      {
        sum += temp; //Add the value to the sum
        numbers[i]=temp;
      }
      else
      {
        i--;  //Ping another value
      }
      Serial.println(temp);
      delay(35);
    }
  
    average = (double)sum/(double)values_number; //Average of the ten values pinged
    Serial.print("Average: ");
    Serial.println(average);
    setup_value = (int)average;
    Serial.print("Setup value: ");
    Serial.println(setup_value);
    
    if(calculate_variance(numbers)) {
      ok=true;
    }
  }
}

/**
*The set_ping_interval() function sets the setup_value on the basis of the height of the person
*/
void set_ping_interval() {
  pingTimer[0] = millis() + PING_SPEED; //First ping starts at "PING_SPEED" ms, gives time for the Arduino to chill before starting.

  for (uint8_t i = 1; i < SONAR_NUM; i++)   //Set the starting time for each sensor.  
  {
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;
  }
}

/**
*The calculate_variance() function calculates the variance to set the setup_value
**/
boolean calculate_variance(int numbers[]) {
  double sum=0.0;
  double variance=0.0;
  for(int i=0;i<values_number;i++) {
    sum+=pow(numbers[i]-setup_value,2);
  }
  
  variance=(double)sum/(double)values_number;
  Serial.print("variance: ");
  Serial.println(variance);
  
  if(variance>2) {
    return false;
  }
  else {
    return true;
  }
}

