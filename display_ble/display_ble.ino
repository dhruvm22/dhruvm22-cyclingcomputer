#include <mbed.h>
using namespace rtos;
using namespace std::chrono;

#include <Wire.h>
#include <U8g2lib.h>

// SH1106 128x64 I2C
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);


// Thread
Thread displayThread;

Thread readingThread;


// Shared struct
struct RideData {
  double odo;
  double distance;
  double time;
  double avg_speed;
  double speed;
  double heading;
  double battery_percentage;
  int displayInfo;  //basically tells what is on the screen right now, so that we can check what buttons take which action, also for ridemodes
  int indicator;
};
int blinkCounter = 0;

struct Buttons{
  bool button1;
  bool button2;
  bool button3;
  bool button4;
  bool button5;
};

RideData rideData;
Buttons buttons;

void displayTask() {
  while (true) {

    double odo = rideData.odo;
    double distance = rideData.distance;
    double time = rideData.time;
    double avg_speed = rideData.avg_speed;
    double speed = rideData.speed;
    double heading = rideData.heading;
    double battery_percentage = rideData.battery_percentage;
    int displayInfo = rideData.displayInfo;  
    int indicator = rideData.indicator;

    
    if(displayInfo == 1)
    {
      //display for ridemode 1;

      char buf[20];
      u8g2.clearBuffer();

      // === Heading (Top Left) ===
      u8g2.setFont(u8g2_font_5x8_tr);
      sprintf(buf, "HDG:%03d", (int)heading);
      u8g2.drawStr(0, 10, buf);

      // === Battery Box (Top Right) ===
      int batt_x = 110, batt_y = 0, batt_w = 18, batt_h = 8;
      u8g2.drawFrame(batt_x, batt_y, batt_w, batt_h);      
      u8g2.drawBox(batt_x + 2, batt_y + 2, (int)((batt_w - 4) * battery_percentage / 100.0), batt_h - 4);  
      sprintf(buf, "%d%%", (int)battery_percentage);
      u8g2.setFont(u8g2_font_5x8_tr);
      u8g2.drawStr(batt_x - 20, batt_y + 6, buf);

      // === Speed (Big, Center) ===
      u8g2.setFont(u8g2_font_logisoso32_tr);
      sprintf(buf, "%d", (int)speed);
      u8g2.drawStr(30, 48, buf);

      // === Odometer (Bottom Left) ===
      u8g2.setFont(u8g2_font_6x12_tr);
      sprintf(buf, "ODO:%d", (int)distance);
      u8g2.drawStr(0, 62, buf);

      // === Blinking Indicator Arrow ===
      blinkCounter++;
      if ((blinkCounter / 5) % 2 == 0) {   // blink toggle
        if (indicator == 1) 
        {
          u8g2.drawTriangle(110, 50, 120, 55, 110, 60);
        } 
        else if (indicator == -1) 
        { 
          u8g2.drawTriangle(120, 50, 110, 55, 120, 60);
        }

      }

      u8g2.sendBuffer();


    }

    else if(displayInfo == 2)
    {
      // display for ridemode 2;
    }

    ThisThread::sleep_for(50ms); // update ~5 Hz
  }
}

void readingTask()
{
  // readings of sensors and buttons, simulated by serial monitor for now;
  double odo = 5;  
  double distance;
  double time = 1;
  double avg_speed = 2;
  double speed = 10;
  double heading = 90;
  double battery_percentage = 10;
  int displayInfo = 1;
  int indicator = 0;

  bool button1 = 0;
  bool button2 = 0;
  bool button3 = 0;
  bool button4 = 0;
  bool button5 = 0;

  rideData.odo = odo;
  rideData.distance = distance;
  rideData.time = time;
  rideData.avg_speed = avg_speed;
  rideData.speed = speed;
  rideData.heading = heading;
  rideData.battery_percentage = battery_percentage;
  rideData.displayInfo = displayInfo;
  rideData.indicator = indicator;

  buttons.button1 = button1;
  buttons.button2 = button2;
  buttons.button3 = button3;
  buttons.button4 = button4;
  buttons.button5 = button5;
}



void setup() {

  Serial.begin(9600);

  u8g2.begin();
  displayThread.start(displayTask);
  readingThread.start(readingTask);
}

void loop() {


}
