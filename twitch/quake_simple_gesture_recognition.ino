
 /*
  www.aifes.ai
  https://github.com/Fraunhofer-IMS/AIfES_for_Arduino
  Copyright (C) 2020-2022  Fraunhofer Institute for Microelectronic Circuits and Systems.
  All rights reserved.

  AIfES is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  AIfES-Express simple gesture recognition demo
  --------------------

    Versions:
    1.0.0   Initial version

  AIfES-Express is a simplified API for AIfES, which is directly integrated. So you can simply decide which variant you want to use.

  What does the example:
  - This demo can train and classify three different simple gestures
  - The data is recorded via an acceleration threshold. No user input is required
  - For the classification of the gestures, an artificial neural network is trained directly in the controller

  What gestures can be trained?
  - Simple gestures can be trained, such as an arm flex, a punch, a twist or a quick sideways movement

  Instructions:
  - Make sure that the board is always held in the same position in your hand!
  - After the UART (baud 115200) connection is established you are directly in the data acquisition mode
  - The output guides you through the data acquisition
  - Perform your first gesture and repeat it 5 times
    * Make sure that you always perform the gesture in the same way and that you take a short break between repetitions
    * Make sure that the gesture also passes the acceleration threshold
  - Repeat the procedure for the second and third gesture
  - The training starts automatically
  - Finally, the classification mode is started automatically

  The example is based on a project from EloquentArduino:
  - https://eloquentarduino.github.io/2019/12/how-to-do-gesture-identification-on-arduino/
  - https://github.com/eloquentarduino/EloquentArduino/blob/master/examples/MicromlGestureIdentificationExample/MicromlGestureIdentificationExample.ino
  - The complete data acquisition was taken from the project

  Calibration:
  - There is a simple calibration procedure to remove the fixed offset due to gravity
  - By default the calibration is commented out

  Hardware:
    Arduino Nano 33 BLE Sense
    Arduino Nano 33 BLE
    
  You can find more AIfES tutorials here:
  https://create.arduino.cc/projecthub/aifes_team
  */

#include <Arduino_LSM6DS3.h>

#include <Adafruit_ISM330DHCX.h>

#include <aifes.h>

#define NUM_SAMPLES 20  //15 30
#define NUM_AXES 3
#define TRUNCATE 20
#define ACCEL_THRESHOLD 4 //5
#define INTERVAL 30

#define SAMPLES_PER_CLASS 5

#define NUMBER_OF_DATA  3 * SAMPLES_PER_CLASS

// Defines for AIfES FNN
#define FNN_3_LAYERS    3
#define INPUTS          NUM_SAMPLES * NUM_AXES
#define NEURONS         4 
#define OUTPUTS         3 // 3 gestures
#define PRINT_INTERVAL  10

// FNN structure
uint32_t FNN_structure[FNN_3_LAYERS] = {INPUTS,NEURONS,OUTPUTS};

// Activation functions
AIFES_E_activations FNN_activations[FNN_3_LAYERS - 1];

// Pointer for the weights
float *FlatWeights;

// AIfES-Express model parameter
AIFES_E_model_parameter_fnn_f32 FNN;

// Baseline for calibration
float baseline[NUM_AXES];

// Features (inputs) of a single gesture
float features[INPUTS];

// Array for the labels (outputs)of a single gesture
// Three gestures can be trained
// 1,0,0 --> gesture 1
// 0,1,0 --> gesture 2
// 0,0,1 --> gesture 3
int labels_print[3] = {1,0,0};

// 2D array for training (inputs)
// Here the features of the gestures from the data recording are stored
float training_data[NUMBER_OF_DATA][INPUTS];

// 2d array for training (target data/labels)
// Here the labels of the gestures from the data recording are stored
float labels[NUMBER_OF_DATA][OUTPUTS];

int training_data_counter = 0;

int class_counter = 0;

// Counter for the samples per gesture (5 samples per gesture)
int sample_counter = 0;

uint32_t global_epoch_counter = 0;

Adafruit_ISM330DHCX ism330dhcx;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("AIfES simple gesture recognition demo");
  Serial.println("Data recording Mode:");
  Serial.print(NUM_SAMPLES * NUM_AXES);
  Serial.println(" features and 3 gestures");
  Serial.println("Make sure that the board is always held in the same position in your hand!");

/*
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
*/

  Wire1.setPins(SDA1, SCL1);
  if (!ism330dhcx.begin_I2C(0x6A, &Wire1)) {
    Serial.println("Failed to find ISM330DHCX chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("ISM330DHCX Found!");

  ism330dhcx.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);
  ism330dhcx.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);  
  ism330dhcx.setAccelDataRate(LSM6DS_RATE_12_5_HZ);
  ism330dhcx.setGyroDataRate(LSM6DS_RATE_12_5_HZ);  
  ism330dhcx.configInt1(false, false, true); // accelerometer DRDY on INT1
  ism330dhcx.configInt2(false, true, false); // gyro DRDY on INT2

  //IMPORTANT
  //AIfES requires random weights for training
  //Here the random seed is generated by the noise of an analog pin
  srand(analogRead(A1));

  // Gravity calibration
  //calibrate();
  
  float ax, ay, az; 

  Serial.println("Repeat the first gesture five times!");

  bool motion_detected = false;

  while(class_counter <= (OUTPUTS - 1)){
    // Wait for the acceleration to be higher than the threshold 
    while(!motion_detected){ 
      /*       
        if (IMU.accelerationAvailable()) {
          IMU.readAcceleration(ax, ay, az);
        }
        */
      delay(10);

      sensors_event_t accel;
      sensors_event_t gyro;
      sensors_event_t temp;
      ism330dhcx.getEvent(&accel, &gyro, &temp);

      ax = accel.acceleration.x;
      ay = accel.acceleration.y;
      az = accel.acceleration.z;

      ax = constrain(ax - baseline[0], -TRUNCATE, TRUNCATE);
      ay = constrain(ay - baseline[1], -TRUNCATE, TRUNCATE);
      az = constrain(az - baseline[2], -TRUNCATE, TRUNCATE);
  
    
      if (motionDetected(ax, ay, az)) {
        motion_detected = true;
      }
      else{
        delay(10);
      }
    }

    if(class_counter <= (OUTPUTS - 1)){
      recordIMU();
      printAndSafeFeatures();
      motion_detected = false;
    }


    sample_counter = sample_counter + 1;
    training_data_counter = training_data_counter + 1;
    
    if(sample_counter == SAMPLES_PER_CLASS){
      sample_counter = 0;
      class_counter = class_counter + 1;
      
      if(class_counter == 1){
        Serial.println("Repeat the second gesture five times!");
        labels_print[0] = 0;
        labels_print[1] = 1;
        labels_print[2] = 0;
      }
      if(class_counter == 2){
        Serial.println("Repeat the third gesture five times!");
        labels_print[0] = 0;
        labels_print[1] = 0;
        labels_print[2] = 1;
      }
      if(class_counter > 2){
        Serial.println("Data recording finished!");
      }
      
    }

    delay(50);
  }

  // This is where the AIfES model is built. The function is defined in the tab creation_and_training.
  // The different layers are defined and storage is reserved for the trainable parameters (like weights, bias, ...) of the model.
  build_AIfES_model();

  //A simple countdown to give the user time to read the UART outputs
  //countdown();

  // The AIfES model is trained with the captured data. This function is also defined in the tab creation_and_training
  // Not only is the training carried out here, but also the necessary memory is reserved for intermediate results, gradients and moments of the training and the results of the training are displayed
  train_AIfES_model();
  
  // Now the ANN is prepared and ready for the classification of the three objects
  Serial.println("Training finished!");
  Serial.println("Ready for classification!");
  
}

void loop() {
    float ax, ay, az;
    /*
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(ax, ay, az);
    }
    */
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    ism330dhcx.getEvent(&accel, &gyro, &temp);

    ax = accel.acceleration.x;
    ay = accel.acceleration.y;
    az = accel.acceleration.z;

    ax = constrain(ax - baseline[0], -TRUNCATE, TRUNCATE);
    ay = constrain(ay - baseline[1], -TRUNCATE, TRUNCATE);
    az = constrain(az - baseline[2], -TRUNCATE, TRUNCATE);

    if (!motionDetected(ax, ay, az)) {
    delay(10);
    return;
    }
    
    recordIMU();
    classification();

    delay(500);
    

}

bool motionDetected(float ax, float ay, float az) {
    return (abs(ax) + abs(ay) + abs(az)) > ACCEL_THRESHOLD;
}


void recordIMU() {
    float ax, ay, az;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        if (IMU.accelerationAvailable()) {
          IMU.readAcceleration(ax, ay, az);
  
        }

        ax = constrain(ax - baseline[0], -TRUNCATE, TRUNCATE);
        ay = constrain(ay - baseline[1], -TRUNCATE, TRUNCATE);
        az = constrain(az - baseline[2], -TRUNCATE, TRUNCATE);

        features[i * NUM_AXES + 0] = ax;
        features[i * NUM_AXES + 1] = ay;
        features[i * NUM_AXES + 2] = az;

        delay(INTERVAL);
    }
}

void printAndSafeFeatures() {
    const uint16_t numFeatures = sizeof(features) / sizeof(float);
     
    for (int i = 0; i < numFeatures; i++) {
        Serial.print(features[i]);
        training_data[training_data_counter][i] = features[i];
        
        if(i == numFeatures - 1){
          labels[training_data_counter][0] = labels_print[0];
          labels[training_data_counter][1] = labels_print[1];
          labels[training_data_counter][2] = labels_print[2];

          Serial.print('\t');
          Serial.print(labels_print[0]);
          Serial.print('\t');
          Serial.print(labels_print[1]);
          Serial.print('\t');
          Serial.print(labels_print[2]);
          Serial.print('\n');
              
        }
        else{
          Serial.print('\t');
        }
    }
}

void calibrate() {
    float ax, ay, az;

    for (int i = 0; i < 10; i++) {
        sensors_event_t accel;
        sensors_event_t gyro;
        sensors_event_t temp;
        ism330dhcx.getEvent(&accel, &gyro, &temp);

        ax = accel.acceleration.x;
        ay = accel.acceleration.y;
        az = accel.acceleration.z;
        delay(100);
    }

    baseline[0] = ax;
    baseline[1] = ay;
    baseline[2] = az;
}

void classification(){
  // ----------------------------------------- Definition of tensors --------------------------
  // Tensor for the input RGB
  // Those values are the input of the ANN
  uint16_t input_shape[] = {1, INPUTS};                          // Definition of the shape of the tensor, here: {1 (i.e. 1 sample), 3 (i.e. the sample contains 3 RGB values)}
  aitensor_t input_tensor = AITENSOR_2D_F32(input_shape, features);                 // Macro for the simple creation of a float32 tensor. Also usable in the normal AIfES version

  // Tensor for the output with 3 classes
  // Output values of the ANN are saved here
  float output_data[OUTPUTS];                                     // Array for storage of the output data, for each object/class one output is created
  uint16_t output_shape[] = {1, OUTPUTS};                         // Definition of the shape of the tensor, here: {1 (i.e. 1 sample), 3 (i.e. the sample contains predictions for 3 classes/objects)}
  aitensor_t output_tensor = AITENSOR_2D_F32(output_shape, output_data);              // Macro for the simple creation of a float32 tensor. Also usable in the normal AIfES version  

  // ----------------------------------------- Run the AIfES model to detect the object --------------------------
  // Run the inference with the trained AIfES model
  int8_t error = 0;
  error = AIFES_E_inference_fnn_f32(&input_tensor,&FNN,&output_tensor);

  error_handling_inference(error);

  // ----------------------------------------- Output of results --------------------------
  Serial.print (F("Gesture 1: "));
  Serial.print (output_data[0] * 100); // Probability in percent for class 0
  Serial.print (F("%"));
  Serial.println(F(""));
  Serial.print (F("Gesture 2: "));
  Serial.print (output_data[1] * 100); // Probability in percent for class 1
  Serial.print (F("%"));
  Serial.println(F(""));
  Serial.print (F("Gesture 3: "));
  Serial.print (output_data[2] * 100); // Probability in percent for class 2
  Serial.print (F("%"));
  Serial.println(F(""));
  Serial.println(F(""));
  
}
