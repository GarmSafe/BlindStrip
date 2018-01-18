#include <NewPing.h>

/**
 * Number of ultrasonic sensors
 */
#define SONAR_NUM 5
/**
 * Maximum distance (in cm)
 */
#define MAX_DISTANCE 150
/**
 * Maximum distance range (for the sensor that recognises irregularities)
 */
#define MAX_DISTANCE_2 400
/**
 * Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).
 */
#define PING_INTERVAL 35
/**
 * Ping frequency (in milliseconds), fastest we should ping is about 35ms per sensor
 */
#define PING_SPEED 50
/**
 * number of distance measures needed for each ultrasonic sensor
 */
#define MEASURES_NUM 3
/**
 * Number of values for the setup
 */
#define VALUES_NUM 10
/**
 * Number of the sensor that recognises irregularities
 */
#define SENSOR_NUM 4
/**
 * Holds the times when the next ping should happen for each sensor.
 */
unsigned long ping_timer[SONAR_NUM][MEASURES_NUM];
/**
 * Where the ping distances are stored.
 */
unsigned int cm[SONAR_NUM][MEASURES_NUM];
/**
 * Keeps track of which sensor is active.
 */
uint8_t current_sensor = 0;
/**
 * Actual distance measured from each ultrasonic sensor (after 3 distance measures)
 */
unsigned int actual_cm[SONAR_NUM];
/**
 * Distance from the belt to the ground (for the sensor that recognises irregularities)
 */
int setup_value;
/**
 * Echo Pins of the ultrasonic sensors
 */
int echo_pins[SONAR_NUM] = {
  A0, A1, A2, A3, A4
};
/**
 * Vibrations' values
 */
int vibrations[SONAR_NUM];
/**
 * Trigger pins of the ultrasonic sensors.
 */
int mot_pins[SONAR_NUM] = {
  5, 3, 9, 6, 10
};
/**
 * Sensor object array.
 * 
 * \param NewPing(2,A0,MAX_DISTANCE)  leg-dx (high)
 * \param NewPing(4,A1,MAX_DISTANCE)  leg-dx (low)
 * \param NewPing(7,A2,MAX_DISTANCE)  leg-sx (high)
 * \param NewPing(8,A3,MAX_DISTANCE)  leg-sx (low)
 * \param NewPing(12,A4,MAX_DISTANCE) belt -> for holes, stairs
 * 
 * 
 * 
 */
NewPing sonar[SONAR_NUM] = {

  //Each sensor's trigger pin, echo pin, and max distance to ping.

  NewPing(2, A0, MAX_DISTANCE),
  NewPing(4, A1, MAX_DISTANCE),
  NewPing(7, A2, MAX_DISTANCE),
  NewPing(8, A3, MAX_DISTANCE),
  NewPing(12, A4, MAX_DISTANCE_2)
  };

  /**
   * The setup() function is called when a sketch starts
   */
  void setup() {
    Serial.begin(9600);

    set_ping_interval();
    set_setup_value();
  }
/**
 * The loop() function loops consecutively allowing your program to change and respond
 */

void loop() {
  for (uint8_t i = 0; i < SONAR_NUM; i++) { //Loop through all the sensors.
    if (i == 0 && current_sensor == SONAR_NUM - 1) { //When all the sensors has pinged
      vibrate();
      printResults();
    }

    current_sensor = i;  //Sensor being accessed.

    for (uint8_t j = 0; j < MEASURES_NUM; j++) {
      if (millis() >= ping_timer[i][j]) {

        if (i == 0 && j == 0) {
          ping_timer[i][j] += PING_SPEED;  //Make sensor 1 fire again "PING_SPEED" ms later
        }
        else if(i >= 0 && j > 0) {
          ping_timer[i][j] = ping_timer[i][j - 1] + PING_INTERVAL; //Make next sensor fire again "PING_INTERVAL" second after previous sensor fires
        }
        else if(j == 0 && i > 0){
          ping_timer[i][j] = ping_timer[i - 1][SONAR_NUM - 1] + PING_INTERVAL; //Make next sensor fire again "PING_INTERVAL" second after previous sensor fires (first sensor measure)
        }

        cm[current_sensor][j] = sonar[current_sensor].ping_cm();      //Send a ping, returns the distance in centimeters or 0 (zero) if no ping echo within set distance limit

        if (cm[current_sensor][j] == 0) {
          reset();
        }
      }
    }

    actual_cm[current_sensor] = get_actual_value(cm[current_sensor]);

    if (current_sensor == SENSOR_NUM) //Sensor that recognises irregularities
    {
      if (actual_cm[current_sensor] == 0 || (actual_cm[current_sensor] - 10) > setup_value || (actual_cm[current_sensor] + 10) < setup_value) //If the no ping echo or the distance is higher than 75
      {
        vibrations[current_sensor] = 255; //Vibration
      }
      else
      {
        vibrations[current_sensor] = 0; //No vibration
      }
    }
    else
    {
      if (actual_cm[current_sensor] == 0) {  //If the no ping echo
        reset();
        vibrations[current_sensor] = 0;
      }
      else
      {
        if (actual_cm[current_sensor] == 1) { //Adjust minimum range to 2cm
          actual_cm[current_sensor] = 2;
        }

        vibrations[current_sensor] = map(actual_cm[current_sensor], 2, MAX_DISTANCE, 255, 80); //Calculate vibration according to the distance
      }
    }
  }
}

unsigned int get_actual_value(unsigned int values[MEASURES_NUM]) {
  int zero_count = 0;
  unsigned int actual_value = 0;

  for (uint8_t i = 0; i < MEASURES_NUM; i++) {
    if (values[i] == 0) {
      zero_count++;
    }
    else {
      actual_value = values[i];
    }
  }

  return actual_value;
}

/**
 * The reset() function is called to avoid the interruption of the sensor
 */
void reset() {
  pinMode( echo_pins[current_sensor], OUTPUT );
  digitalWrite( echo_pins[current_sensor], LOW );
  delayMicroseconds(10);
  pinMode( echo_pins[current_sensor], INPUT);
}

/**
 * The printResults() prints the results on the Serial monitor (used by developers)
 */
void printResults() {
  for (uint8_t i = 0; i < SONAR_NUM; i++) {
    for (uint8_t j = 0; j < MEASURES_NUM; j++) {
      Serial.print("SENSOR ");
      Serial.print(i);
      Serial.print(": ");

      Serial.print(cm[i][j]);
      Serial.print("cm - ");

      Serial.print(vibrations[i]);
      Serial.print("vib - ");
    }
    
    Serial.println("actual cm: ");
    Serial.print(actual_cm[i]);
  }

  Serial.println();
  Serial.println();
}

/**
 * The vibrate() function makes vibrate all the vibration motors with respective vibrations
 */
void vibrate() {
  for (uint8_t i = 0; i < SONAR_NUM; i++) {
    analogWrite(mot_pins[i], vibrations[i]);
  }
}

/**
 * The set_setup_value() function sets the setup_value on the basis of the height of the person
 */

void set_setup_value() {
  int sum = 0, temp, numbers[VALUES_NUM];
  boolean ok = false;
  double average = 0.0;
  while (!ok) {
    sum = 0;
    Serial.println("Values");
    for (int i = 0; i < VALUES_NUM; i++) // Needed at least ten values
    {
      temp = sonar[SENSOR_NUM].ping_cm();  //Send a ping

      if (temp != 0) //If the ping is not out of range
      {
        sum += temp; //Add the value to the sum
        numbers[i] = temp;
      }
      else
      {
        i--;  //Ping another value
      }
      Serial.println(temp);
      delay(PING_INTERVAL);
    }

    average = (double)sum / (double)VALUES_NUM; //Average of the ten values pinged
    Serial.print("Average: ");
    Serial.println(average);
    setup_value = (int)average;
    Serial.print("Setup value: ");
    Serial.println(setup_value);

    if (calculate_variance(numbers)) {
      ok = true;
    }
  }
}

/**
 * The set_ping_interval() function sets the setup_value on the basis of the height of the person
 */
void set_ping_interval() {
  ping_timer[0][0] = millis() + PING_SPEED; //First ping starts at "PING_SPEED" ms, gives time for the Arduino to chill before starting.

  for (uint8_t i = 0; i < SONAR_NUM; i++)   //Set the starting time for each sensor.
  {
    for (uint8_t j = 0; j < MEASURES_NUM; j++) {
      if(i >= 0 && j > 0) {
        ping_timer[i][j] = ping_timer[i][j - 1] + PING_INTERVAL; //Make next sensor fire again "PING_INTERVAL" second after previous sensor fires
      }
      else if(j == 0 && i > 0){
        ping_timer[i][j] = ping_timer[i - 1][SONAR_NUM - 1] + PING_INTERVAL; //Make next sensor fire again "PING_INTERVAL" second after previous sensor fires (first sensor measure)
      }
    }
  }
}

/**
 * The calculate_variance() function calculates the variance to set the setup_value
 **/
boolean calculate_variance(int numbers[]) {
  double sum = 0.0;
  double variance = 0.0;
  for (int i = 0; i < VALUES_NUM; i++) {
    sum += pow(numbers[i] - setup_value, 2);
  }

  variance = (double)sum / (double)VALUES_NUM;
  Serial.print("variance: ");
  Serial.println(variance);

  if (variance > 2) {
    return false;
  }
  else {
    return true;
  }
}


