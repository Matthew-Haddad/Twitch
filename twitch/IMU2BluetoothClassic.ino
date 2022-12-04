// Basic demo for accelerometer/gyro readings from Adafruit ISM330DHCX
#include <BluetoothSerial.h>
#include "BluetoothSerial.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_ISM330DHCX.h>

BluetoothSerial SerialBT;
// Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL);
byte BTData;

/* Check if Bluetooth configurations are enabled in the SDK */
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

int t = 0;

Adafruit_ISM330DHCX ism330dhcx;
void setup(void) {

  int t = 0;
    
  // pixels.begin();
  Serial.begin(115200);
  SerialBT.begin("Twitch Sensor - Heath");
  
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Wire1.setPins(SDA1, SCL1);
  if (!ism330dhcx.begin_I2C(0x6A, &Wire1)) {
    while (1) {
      delay(10);
    }
  }

  ism330dhcx.configInt1(false, false, true); // accelerometer DRDY on INT1
  ism330dhcx.configInt2(false, true, false); // gyro DRDY on INT2

}

void loop() {
  
  if (SerialBT.available()){

    BTData = SerialBT.read();
    Serial.write(BTData);
    
  }

  if (SerialBT.connected()){

    SerialBT.print("Time Step, Accelerometer: X-axis, Accelerometer: Y-axis, Accelerometer: Z-axis, Gyroscope: X-axis, Gyroscope: Y-axis, Gyroscope: Z-axis");
    SerialBT.println();
    // pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    // pixels.show();
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;

    while (SerialBT.connected()){
    ism330dhcx.getEvent(&accel, &gyro, &temp);
    SerialBT.print(t);
    SerialBT.print(",");
    SerialBT.print(accel.acceleration.x);
    SerialBT.print(",");
    SerialBT.print(accel.acceleration.y);
    SerialBT.print(",");
    SerialBT.print(accel.acceleration.z);
    SerialBT.print(",");
    SerialBT.print(gyro.gyro.x);
    SerialBT.print(",");
    SerialBT.print(gyro.gyro.y);
    SerialBT.print(",");
    SerialBT.print(gyro.gyro.z);
    SerialBT.println();
    delay(100);
    t++;
    }
  }

  if (!SerialBT.connected()){

    t = 0;  
    // pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    // pixels.show();
    
  }

}
