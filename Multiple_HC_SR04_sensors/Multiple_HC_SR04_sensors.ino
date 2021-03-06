#include <NewPing.h>

/**
   Number of ultrasonic sensors
*/
#define SONAR_NUM 5
/**
   Maximum distance (in cm)
*/
#define MAX_DISTANCE 150
/**
   Maximum distance range (for the sensor that recognises irregularities)
*/
#define MAX_DISTANCE_2 400
/**
   Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).
*/
#define PING_INTERVAL 35
/**
   Ping frequency (in milliseconds), fastest we should ping is about 35ms per sensor
*/
#define PING_SPEED 50
/**
   number of distance measures needed for each ultrasonic sensor
*/
#define MEASURES_NUM 4
/**
   Number of values for the setup
*/
#define VALUES_NUM 10
/**
   Number of the sensor that recognises irregularities
*/
#define SENSOR_NUM 4
/**
   Holds the times when the next ping should happen for each sensor.
*/
unsigned long ping_timer[SONAR_NUM][MEASURES_NUM];
/**
   Where the ping distances are stored.
*/
unsigned int cm[SONAR_NUM][MEASURES_NUM];
/**
   Keeps track of which sensor is active.
*/
uint8_t current_sensor = 0;
/**
   Actual distance measured from each ultrasonic sensor (after 3 distance measures)
*/
unsigned int actual_cm[SONAR_NUM];
/**
   Distance from the belt to the ground (for the sensor that recognises irregularities)
*/
unsigned int setup_value;
/**
   Echo Pins of the ultrasonic sensors
*/
uint8_t echo_pins[SONAR_NUM] = {
  A0, A1, A2, A3, A4
};
/**
   Vibrations' values
*/
uint8_t vibrations[SONAR_NUM];
/**
   Trigger pins of the ultrasonic sensors.
*/
uint8_t mot_pins[SONAR_NUM] = {
  5, 3, 9, 6, 10
};
/**
   Sensor object array.

   \param NewPing(2,A0,MAX_DISTANCE)  leg-dx (high)
   \param NewPing(4,A1,MAX_DISTANCE)  leg-dx (low)
   \param NewPing(7,A2,MAX_DISTANCE)  leg-sx (high)
   \param NewPing(8,A3,MAX_DISTANCE)  leg-sx (low)
   \param NewPing(12,A4,MAX_DISTANCE) belt -> for holes, stairs



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
   The "setup" function is called when a sketch starts
*/
void setup() {
  Serial.begin(9600);

  set_ping_interval();
  set_setup_value();
}

/**
   The "loop" function loops consecutively allowing your program to change and respond
*/
void loop() {
  for (uint8_t i = 0; i < SONAR_NUM; i++) { //Loop through all the sensors.
    if (i == 0 && current_sensor == SONAR_NUM - 1) { //When all the sensors has pinged
      print_results();
    }

    current_sensor = i;  //Sensor being accessed.

    for (uint8_t j = 0; j < MEASURES_NUM; j++) {
      if (millis() >= ping_timer[i][j]) {

        if (i >= 0 && j > 0) {
          ping_timer[i][j] = ping_timer[i][j - 1] + PING_INTERVAL; //Make next sensor fire again "PING_INTERVAL" second after
        }
        else if (j == 0 && i > 0) {
          ping_timer[i][j] = ping_timer[i - 1][SONAR_NUM - 1] + PING_INTERVAL; //Make next sensor fire again "PING_INTERVAL" second after (first sensor's measure)
        }
        else if (i == 0 && j == 0) {
          ping_timer[i][j] += PING_SPEED;  //Make sensor 1 fire again "PING_SPEED" ms later
        }

        cm[current_sensor][j] = sonar[current_sensor].ping_cm();      //Send a ping, returns the distance in centimeters or 0 (zero) if no ping echo within set distance limit

        if (cm[current_sensor][j] == 0) { //If the measure is not valid the "reset" function is called
          reset();
        }
      }
    }

    actual_cm[current_sensor] = get_actual_value(cm[current_sensor]); //Store for each sensor the actual value from the "MEASURES_NUMBER" measures

    if (current_sensor == SENSOR_NUM) { //Sensor that recognises irregularities
      if (actual_cm[current_sensor] == 0 || (actual_cm[current_sensor] - 10) > setup_value || (actual_cm[current_sensor] + 10) < setup_value) { //If the no ping echo or the distance is higher than 75
        change_vibration(255);
        vibrations[current_sensor] = 255; //Vibration
      }
      else {
        change_vibration(0);
        vibrations[current_sensor] = 0; //No vibration
      }
    }
    else {
      if (actual_cm[current_sensor] == 0) {  //If the no ping echo
        change_vibration(0);
        vibrations[current_sensor] = 0;
      }
      else {
        if (actual_cm[current_sensor] == 1) { //Adjust minimum range to 2cm
          actual_cm[current_sensor] = 2;
        }

        vibrations[current_sensor] = map(actual_cm[current_sensor], 2, MAX_DISTANCE, 255, 80); //Calculate vibration according to the distance
        change_vibration(vibrations[current_sensor]);
      }
    }
  }
}

/**
  The "get_actual_value" function is called to calcualte the actual value for each ultrasonic sensor from the "MEASURES_NUM" measures detected
*/
unsigned int get_actual_value(unsigned int values[MEASURES_NUM]) {
  int zero_count = 0;
  unsigned int actual_value = 0;

  for (uint8_t i = 0; i < MEASURES_NUM; i++) { //The measures are not valid if they are all 0
    if (values[i] == 0) { //If the measure is not valid
      zero_count++;
    }
    else {
      actual_value = values[i];
    }
  }

  return actual_value;
}

/**
   The "reset" function is called to avoid the interruption of the sensor
*/
void reset() {
  pinMode( echo_pins[current_sensor], OUTPUT );
  digitalWrite( echo_pins[current_sensor], LOW );
  delayMicroseconds(10);
  pinMode( echo_pins[current_sensor], INPUT);
}

/**
   The "printResults" prints the results on the Serial monitor (used by developers)
*/
void print_results() {
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

    Serial.print("Actual cm: ");
    Serial.println(actual_cm[i]);
  }

  Serial.println();
  Serial.println();
}

/**
   The "change_vibration" function change the vibration of the current sensor
*/
void change_vibration(uint8_t vibration) {
    analogWrite(mot_pins[current_sensor], vibration);
}

/**
   The "set_setup_value" function sets the setup_value on the basis of the height of the person
*/
void set_setup_value() {
  int sum = 0, temp, values[VALUES_NUM];
  boolean ok = false;
  double average = 0.0;
  while (!ok) { //While the setup value is not valid
    sum = 0;
    Serial.println("Values");
    for (uint8_t i = 0; i < VALUES_NUM; i++) { // Needed at least ten values
      temp = sonar[SENSOR_NUM].ping_cm();  //Send a ping

      if (temp != 0) { //If the ping is not out of range
        sum += temp; //Add the value to the sum
        values[i] = temp;
      }
      else {
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

    if (calculate_variance(values) <= 1) { //If the variance is bigger than 1, the values detected are not valid
      ok = true;
    }
  }
}

/**
   The "set_ping_interval" function sets the setup_value on the basis of the height of the person
*/
void set_ping_interval() {
  ping_timer[0][0] = millis() + PING_SPEED; //First ping starts at "PING_SPEED" ms, gives time for the Arduino to chill before starting.

  for (uint8_t i = 0; i < SONAR_NUM; i++) { //Set the starting time for each sensor.
    for (uint8_t j = 0; j < MEASURES_NUM; j++) {
      if (i >= 0 && j > 0) {
        ping_timer[i][j] = ping_timer[i][j - 1] + PING_INTERVAL; //Make next sensor fire again "PING_INTERVAL" second after previous sensor fires
      }
      else if (j == 0 && i > 0) {
        ping_timer[i][j] = ping_timer[i - 1][SONAR_NUM - 1] + PING_INTERVAL; //Make next sensor fire again "PING_INTERVAL" second after previous sensor fires (first sensor measure)
      }
    }
  }
}

/**
   The "calculate_variance" function calculates the variance to set the setup_value
 **/
double calculate_variance(int values[]) {
  double sum = 0.0;
  double variance = 0.0;
  for (uint8_t i = 0; i < VALUES_NUM; i++) {
    sum += pow(values[i] - setup_value, 2);
  }

  variance = (double)sum / (double)VALUES_NUM;
  Serial.print("Variance: ");
  Serial.println(variance);

  return variance;
}


