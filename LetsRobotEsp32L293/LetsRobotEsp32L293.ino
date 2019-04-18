//for enabling serial output
#define DEBUG

///-- GYRO DRIFT COMPENSATION --///
/*
 * To use Gyro drift compensation, you must connect an MPU_6050 or better to I2C
 * This does not utilize the interrupt, so only SDA and SCL need to be hooked up
 * 
 * Required Libraries: 
 * - MPU6050_tockn.h
 * - PID_v1.h - PID values can be adjusted in its definition, but may really only need P
 */
//enable this to get gyro compensation features
#define GYRO

//enable this if the imu is upside down
//#define GYRO_UPSIDE_DOWN 


///-- OTA --///
/*
 * To Use OTA, you must have a file named wifi_pass.h, and add this code. 
 * This is done for version control safety. If you want this file versioned, remove it from the .gitignore file
 String wifiAPList[][2] = {
  {"SSID","PASS"} //First AP
  ,{"SSID","PASS"} //optional second AP
  };
*/
//uncomment to enable OTA support
//#define OTA 

#if defined(GYRO)
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <PID_v1.h>
#endif
#include "l293d.h"
#if defined(OTA)
#include "wifi_pass.h" //error here? Check the ///-- OTA --/// section at top
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ArduinoOTA.h>
WiFiMulti wifiMulti;     // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
#endif

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
int zangle = 0;
double curangle = 0;
double output;
double target;
#if defined(GYRO)
MPU6050 mpu6050(Wire);
//Specify the links and initial tuning parameters
PID myPID(&curangle, &output, &target,2,0,0, DIRECT); //2,5,1
bool enableTargetControl = false;

void disableGyro(){
  myPID.SetMode(MANUAL);
    enableTargetControl = false;
    output = 0;
    //Doing nothing or turning
}
#endif

int pos = 0;    // variable to store the servo position
//Servo myservo;  // create servo object to control a servo

// Motor A
MotorConfig leftMotor(16, 17, 4, 0);

// Motor B
MotorConfig rightMotor(18, 19, 5, 1);


void setup() {
  // put your setup code here, to run once:
  #if defined(DEBUG)
  Serial.begin(115200);
  #endif
  #if defined(OTA)
  int rowCount = sizeof wifiAPList / sizeof wifiAPList[0];
  for(int i = 0; i < rowCount; i++){ //error here? Check the ///-- OTA --/// section at top
    wifiMulti.addAP(wifiAPList[i][0].c_str(), wifiAPList[i][1].c_str());
  }
#if defined(DEBUG)
  Serial.println("Connecting ...");
#endif
  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(250);
    #if defined(DEBUG)
    Serial.print('.');
    #endif
  }
  #if defined(DEBUG)
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer
  #endif
  
  ArduinoOTA.setHostname("LetsRobot");
  ArduinoOTA.setPassword("esp8266");

#if defined(DEBUG)
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  #endif
  ArduinoOTA.begin();
  #if defined(DEBUG)
  Serial.println("OTA ready");
  #endif
  #endif
  SerialBT.begin("LetsRobot"); //Bluetooth device name
  #if defined(GYRO)
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
  myPID.SetOutputLimits(-50, 50);
  myPID.SetMode(MANUAL);
  leftMotor.setup();
  rightMotor.setup();
  #endif
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

const unsigned int MAX_INPUT = 20;
char input_line [MAX_INPUT];
unsigned int input_pos = 0;

bool comp(char search[]) {
  return strcmp(search, input_line) == 0;
}

int timeout;
int speeds = 150;
DIR leftDir = OFF;
int leftSpeed = 0;
DIR rightDir = OFF;
int rightSpeed = 0;
void command(char commandStr[]) {
  bool didTimeout = false;
  if (comp("b")) {
    //going backwards
    #if defined(DEBUG)
    Serial.println("b!");
    #endif
    leftDir = BACKWARD;
    rightDir = BACKWARD;
  } else if (comp("f")) {
    //going forwards
    #if defined(DEBUG)
    Serial.println("f!");
    #endif
    leftDir = FORWARD;
    rightDir = FORWARD;
  } else if (comp("l")) {
    //going left
    #if defined(DEBUG)
    Serial.println("l!");
    #endif
    leftDir = BACKWARD;
    rightDir = FORWARD;
  } else if (comp("r")) {
    //going right
    leftDir = FORWARD;
    rightDir = BACKWARD;
  } 
  else if (comp("stop") && leftDir != rightDir) { //only do stop command on turning
    leftDir = OFF;
    rightDir = OFF;
  }
  else{
    didTimeout = true;
  }

  //check for timeout
  if(!didTimeout){
    timeout = 0;
  }
}

int forwardCount = 0;

void loop() {

  #if defined(OTA)
  ArduinoOTA.handle();
  #endif
  // put your main code here, to run repeatedly:
  if (SerialBT.available () > 0)
  {
    char inByte = SerialBT.read ();

    switch (inByte)
    {

      case '\n':   // end of text
        input_line [input_pos] = 0;  // terminating null byte

        // terminator reached! process input_line here ...
        #if defined(DEBUG)
        Serial.println (input_line);
        #endif
        command(input_line);
        // reset buffer for next time
        input_pos = 0;
        break;

      case '\r':   // discard carriage return
        break;

      default:
        // keep adding if not full ... allow for terminating null byte
        if (input_pos < (MAX_INPUT - 1))
          input_line [input_pos++] = inByte;
        break;

    }  // end of switch

  }  // end of incoming data

  #if defined(DEBUG)
  if (Serial.available () > 0)
  {
    char inByte = Serial.read ();

    switch (inByte)
    {

      case '\n':   // end of text
        input_line [input_pos] = 0;  // terminating null byte

        // terminator reached! process input_line here ...
        Serial.println (input_line);
        command(input_line);
        // reset buffer for next time
        input_pos = 0;
        break;

      case '\r':   // discard carriage return
        break;

      default:
        // keep adding if not full ... allow for terminating null byte
        if (input_pos < (MAX_INPUT - 1))
          input_line [input_pos++] = inByte;
        break;

    }  // end of switch

  }  // end of incoming data
  #endif
  
  int offset = 0;
  
  #if defined(GYRO)
  myPID.Compute();
  mpu6050.update();
  curangle = mpu6050.getGyroAngleZ();
  
  if(rightDir == leftDir && rightDir != OFF){
    //going forwards or backwards
    if(!enableTargetControl){
      target = curangle;
      enableTargetControl = true;
      myPID.SetMode(AUTOMATIC);
      Serial.println("BBBBB");
    }

    if(enableTargetControl){
      Serial.println("AAAA");
      offset = output;  
    }
  }
  else{
    Serial.println("CCCCCC");
    disableGyro();
  }
  if(leftDir == BACKWARD){
    offset = -offset;  
  }
  
  #if defined(GYRO_UPSIDE_DOWN)
    offset = -offset; //flip the offset
  #endif
  
  #if defined(DEBUG)
  Serial.print(offset);
  Serial.print(",");
  Serial.print(speeds);
  Serial.print(",");
  Serial.print(curangle);
  Serial.print(",");
  Serial.println(output);
  #endif

  #else
  offset = 0;
  #endif

  if(timeout > 100){ //safety timeout in case something external dies
    //TODO
    leftDir = rightDir = OFF;
    forwardCount = 0;
    #if defined(GYRO)
    disableGyro();
    #endif
    //Doing nothing or turning
    offset = 0;
  }
  int speedFinal = speeds;
  if(leftDir != rightDir){
    speedFinal = speeds/4;
  }

  int leftSpeed = speedFinal-offset;
  int rightSpeed = speedFinal+offset;
  leftMotor.setPower(leftSpeed);
  rightMotor.setPower(rightSpeed);
  leftMotor.setDirection(leftDir);
  rightMotor.setDirection(rightDir);
  leftMotor.update();
  rightMotor.update();
  //increase timeout counter here. If we get valid serial, it will get set to 0
  timeout++;
}




