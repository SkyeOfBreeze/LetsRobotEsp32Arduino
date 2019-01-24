# LetsRobotEsp32Arduino
What works:

Bluetooth, Motor Control, Gyro, Wifi OTA

Wifi is not needed, and is disabled by default.

## How it works ##

Either the LetsRobot Raspberry Pi code [https://github.com/letsRobot/letsrobot](https://github.com/letsRobot/letsrobot) or the Android code [https://github.com/btelman96/letsrobot-android](https://github.com/btelman96/letsrobot-android) will have a mode that sends via Serial, sending commands to the arduino such as f,b,l,r,stop, or any custom commands. The exact raw string it would send for 'f' for example, is 'f/r/n'.

This converts the text into commands for the motors, using specifically a L298N driver, but can use L293D.

This sample is built different from [https://github.com/letsRobot/LetsRobot-Arduino](https://github.com/letsRobot/LetsRobot-Arduino) , but works the same when receiving controls


