// ===== ESP32 + FreeRTOS + U8g2 (SH1106 128x64 I2C) =====
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include<SPI.h>
#include<math.h>
#include<SD.h>
#include<TinyGPS++.h>
#include<HardwareSerial.h>


#include <freertos/semphr.h>

SemaphoreHandle_t sdMutex;

HardwareSerial GPS(2);
TinyGPSPlus gps;


#include <Adafruit_NeoPixel.h>

// === NeoPixel Setup ===
#define LED_PIN   2      // Pin where NeoPixel is connected
Adafruit_NeoPixel strip(24, LED_PIN, NEO_GRB + NEO_KHZ800);



#define SD_CS 5




// I2C pins on ESP32 (default): SDA=21, SCL=22
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,              // rotation
  U8X8_PIN_NONE,        // reset (not used on most boards)
  /* clock=*/ 22,       // SCL
  /* data=*/ 21         // SDA
);

struct RideData {
  double odo;
  double distance;
  double time_r;
  double avg_speed;
  double max_speed;
  double speed;
  double battery_percentage;
  int    indicator;     // -1 left, 0 off, 1 right
  String location;
  
  int hour;             //actual hour throgh gps
  int minute;              //actual min thorugh gps

  int hlMode;
  int rlMode;           //mode of lights
  bool rl;
  bool hlh;             //headligght high beam
  bool hll;             //low beam


  bool ride;            //true when on ride
  int rhour;            //ride hours;
  int rmin;             //ride mins;
  bool rpause;          //ride pause (truue means pause)

};

struct Buttons {
  bool button1;
  bool button2;
  bool button3;
  bool button4;
  bool button5;
};


struct Screen
{
  int screenMode;
  int s2Mode;
  int s3Mode;
};


RideData rideData;
Buttons  buttons;
Screen screen;


TaskHandle_t displayTaskHandle = nullptr;
TaskHandle_t readingTaskHandle = nullptr;
TaskHandle_t neoPixelTaskHandle = nullptr;
TaskHandle_t buttonsTaskHandle = nullptr;
TaskHandle_t locationTaskHandle = nullptr;
TaskHandle_t memTaskHandle = nullptr;

// int blinkCounter = 0;
int blinkLightCounter = 0;
int indCounter = 0;



void displayTask(void *pv) {
  char buf[20];


  bool rl = 0;
  bool hl = 0;

  while(1) 
  {
    double odo   = rideData.odo;
    double dist  = rideData.distance;
    double tsec  = rideData.time_r;
    double avg   = rideData.avg_speed;
    double max_speed = rideData.max_speed;
    double spd   = rideData.speed;
    double batt  = rideData.battery_percentage;
    int    scr   = screen.screenMode;
    int    ind   = rideData.indicator;
    String loc = rideData.location;


    if (scr == 1) 
    {
      u8g2.clearBuffer();


      u8g2.drawHLine(0, 10, 112);
      u8g2.drawHLine(0, 54, 112);
      u8g2.drawVLine(112, 0, 64);

      u8g2.setFont(u8g2_font_6x12_tf);


      static uint8_t offset = 0;
      uint8_t displayWidth = 8;   // characters that fit on display
      String gap = "   ";         // 3-space gap
      String scrollStr = loc + gap; // append gap

      uint8_t len = scrollStr.length();

      String display;
      if (len <= displayWidth) 
      {
        display = scrollStr;
      }
      else
      {
          display = "";
          for (uint8_t i = 0; i < displayWidth; i++) {
          display += scrollStr[(offset + i) % len];
        }
      }

      u8g2.drawStr(2, 7, display.c_str());

      offset = (offset + 1) % len;




      // char timeStr[9];  
      sprintf(buf, "%02d:%02d", rideData.hour, rideData.minute);  
      u8g2.drawStr(110-u8g2.getStrWidth(buf), 8, buf);



      u8g2.setFont(u8g2_font_fur25_tf);




      sprintf(buf, "%2.1f ", spd);

      u8g2.drawStr(1, 45, buf);


      u8g2.setFont(u8g2_font_6x12_tf);      
      u8g2.drawStr(6 + u8g2.getStrWidth(buf)*2.5, 40, "kmph");



      sprintf(buf, "ODO %.2f KM", odo);
      u8g2.drawStr(2, 64, buf);


      u8g2.drawFrame(115, 1, 12, 8);  // battery outline
      u8g2.drawBox(127, 3, 1, 4);     // battery tip
      int bw = map(batt, 0, 100, 0, 12);
      u8g2.drawBox(115, 1, bw, 8);





      u8g2.drawCircle(122, 56, 5, U8G2_DRAW_ALL);  // gear outline
      u8g2.drawCircle(122, 56, 2, U8G2_DRAW_ALL);

      if(ind == 0)
      {
        if(rideData.hlh)
        {
          u8g2.drawBox(116, 19, 9, 1);
          u8g2.drawBox(116, 22, 9, 1);
          u8g2.drawBox(116, 25, 9, 1);
        }

        if(rideData.hll)
        {
          u8g2.drawLine(116, 19, 124, 21);
          u8g2.drawLine(116, 22, 124, 24);
          u8g2.drawLine(116, 25, 124, 27);
        }
        
        if(rideData.rl)
        {
          u8g2.drawCircle(122, 40, 5, U8G2_DRAW_ALL);  // outer ring
          u8g2.drawDisc(122, 40, 2, U8G2_DRAW_ALL);    // inner filled circle
        }
      }






      // blinkCounter++;
      if (indCounter == 0) {
        if (ind == -1) {
          // right
          u8g2.drawTriangle(124, 20, 124, 35, 119, 28);
        } else if (ind == 1) {
          // left (same location, pointing left)
          u8g2.drawTriangle(119, 20, 119, 35, 124, 28);
        }
      }
      u8g2.sendBuffer();
    }

    else if(scr == 2)
    {
      u8g2.clearBuffer();


      u8g2.drawHLine(0, 10, 112);
      u8g2.drawHLine(0, 54, 112);
      u8g2.drawHLine(60, 32, 52);
      u8g2.drawVLine(112, 0, 64);
      u8g2.drawVLine(60, 10,44);


      u8g2.setFont(u8g2_font_6x12_tf);

      static uint8_t offset = 0;
      uint8_t displayWidth = 8;   // characters that fit on display
      String gap = "   ";         // 3-space gap
      String scrollStr = loc + gap; // append gap

      uint8_t len = scrollStr.length();

      String display;
      if (len <= displayWidth) 
      {
        display = scrollStr;
      }
      else
      {
          display = "";
          for (uint8_t i = 0; i < displayWidth; i++) {
          display += scrollStr[(offset + i) % len];
        }
      }

      u8g2.drawStr(2, 7, display.c_str());

      offset = (offset + 1) % len;




      
      if(blinkLightCounter % 40 < 20 && blinkLightCounter % 40 >= 0)
      { 
        if(rideData.rpause == 0)
        {
          sprintf(buf, "%02d:%02d", rideData.rhour, rideData.rmin);  
        }

        else
        {
          sprintf(buf, "paused");
        }
      }
      
      else sprintf(buf, "%02d:%02d", rideData.hour, rideData.minute);
      u8g2.drawStr(110-u8g2.getStrWidth(buf), 8, buf);



      u8g2.setFont(u8g2_font_fur25_tf);


      if (spd >= 10) sprintf(buf, " %2.0f ", spd);
      
      else sprintf(buf, "%2.1f", spd);
      

      u8g2.drawStr(1, 45, buf);

      u8g2.setFont(u8g2_font_6x12_tf);

      if(blinkLightCounter % 40 < 20 && blinkLightCounter % 40 >= 0) sprintf(buf, "dis %.1f KM", rideData.distance);
      else sprintf(buf, "odo %.1f KM", rideData.odo);
      u8g2.drawStr(2, 64, buf);


      u8g2.drawFrame(115, 1, 12, 8);  // battery outline
      u8g2.drawBox(127, 3, 1, 4);     // battery tip
      int bw = map(batt, 0, 100, 0, 12);
      u8g2.drawBox(115, 1, bw, 8);


      u8g2.drawCircle(122, 56, 5, U8G2_DRAW_ALL);  // gear outline
      u8g2.drawCircle(122, 56, 2, U8G2_DRAW_ALL);

      if(ind == 0)
      {
        if(rideData.hlh)
        {
          u8g2.drawBox(116, 19, 9, 1);
          u8g2.drawBox(116, 22, 9, 1);
          u8g2.drawBox(116, 25, 9, 1);
        }

        if(rideData.hll)
        {
          u8g2.drawLine(116, 19, 124, 21);
          u8g2.drawLine(116, 22, 124, 24);
          u8g2.drawLine(116, 25, 124, 27);
        }
        
        if(rideData.rl)
        {
          u8g2.drawCircle(122, 40, 5, U8G2_DRAW_ALL);  
          u8g2.drawDisc(122, 40, 2, U8G2_DRAW_ALL);    
        }
      }


      if (indCounter == 0) {
        if (ind == -1) {
          u8g2.drawTriangle(124, 20, 124, 35, 119, 28);
        } else if (ind == 1) {
          u8g2.drawTriangle(119, 20, 119, 35, 124, 28);
        }
      }

      u8g2.setFont(u8g2_font_7x14_tf);


      if (max_speed >= 10) sprintf(buf, "^ %2.0f ", max_speed);
      else sprintf(buf, "^ %2.1f ", max_speed);
      u8g2.drawStr(66,28, buf);


      if (avg >= 10) sprintf(buf, "~ %2.0f ", avg);
      else sprintf(buf, "~ %2.1f ", avg);
      u8g2.drawStr(66,50, buf);
      u8g2.sendBuffer();
    }

    else if (scr == 3) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x12_tr);

      u8g2.drawHLine(0, 32, 128);
      u8g2.drawVLine(64, 0, 64);
      u8g2.drawHLine(0, 0, 128);
      u8g2.drawVLine(0, 0, 64);
      u8g2.drawHLine(0, 63, 128);
      u8g2.drawVLine(127, 0, 64);



      u8g2.drawDisc(28, 48, 8, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_LOWER_LEFT); // half-disc left

      u8g2.drawBox(32, 40, 2, 17);  // vertical bar


      switch(screen.s3Mode)
      {
        case 0:
          break;

        case 1:
          u8g2.drawFrame(1, 1, 63, 31);    // outer
          u8g2.drawFrame(2, 2, 61, 29);    // inner
          break;

        case 2:
          u8g2.drawFrame(65, 1, 62, 31);   // outer
          u8g2.drawFrame(66, 2, 60, 29);   // inner
          break;

        case 3:
          u8g2.drawFrame(1, 33, 63, 30);   // outer
          u8g2.drawFrame(2, 34, 61, 28);   // inner
          break;

        case 4:
          u8g2.drawFrame(65, 33, 62, 30);  // outer
          u8g2.drawFrame(66, 34, 60, 28);  // inner
          break;
      }



      if(rideData.hlh)
      {
        u8g2.drawBox(36, 40, 6, 2);  // top beam
        u8g2.drawBox(36, 47, 6, 2);  // middle beam
        u8g2.drawBox(36, 55, 6, 2);
      }

      if(rideData.hll)
      {
        u8g2.drawLine(36, 40, 40, 43);
        u8g2.drawLine(36, 41, 40, 44);
        u8g2.drawLine(36, 47, 40, 50);
        u8g2.drawLine(36, 48, 40, 51);
        u8g2.drawLine(36, 55, 40, 58);
        u8g2.drawLine(36, 56, 40, 59);
      }


      if(rideData.ride == 0)                                      //not on ride, so play pause wont be available, and the option will be to start new ride
      {
        sprintf(buf, "new ride");  
        u8g2.setFont(u8g2_font_6x12_tf);
        u8g2.drawStr(7, 15, buf);



      }


      if(rideData.ride == 1)                                      
      {
        sprintf(buf, "end ride");  
        u8g2.setFont(u8g2_font_6x12_tf);
        u8g2.drawStr(7, 15, buf);

        if(rideData.rpause == 0)
        {
          u8g2.drawBox(90, 8, 2, 16);
          u8g2.drawBox(100, 8, 2, 16);
        }

        if(rideData.rpause == 1)
        {
          u8g2.drawTriangle(92, 8, 92, 24, 102, 16);
        }

      }


      u8g2.drawCircle(96, 48, 8, U8G2_DRAW_ALL);  // outer ring
      u8g2.drawCircle(96, 48, 9, U8G2_DRAW_ALL);  // outer ring
      

      if (rideData.rl) {
        u8g2.drawDisc(96, 48, 4, U8G2_DRAW_ALL);    // inner filled circle
      }


      u8g2.sendBuffer();

    }

    vTaskDelay(10 / portTICK_PERIOD_MS);  // ~20 FPS
  }
}

void buttonsTask(void* pv)                                    //defines button functions
{
  buttons.button1 = 1;
  buttons.button2 = 1;
  buttons.button3 = 1;
  buttons.button4 = 1;
  buttons.button5 = 1;


  while(1)
  {
    if(!buttons.button1 || !buttons.button2 || !buttons.button3 || !buttons.button4 || !buttons.button5 == 1)
    {

      vTaskDelay(30 / portTICK_PERIOD_MS);                                             //wait to see if two buttons are pressed

      
      if(buttons.button1 == 0 && buttons.button3 == 1)
      {
        if(screen.screenMode == 1 || screen.screenMode == 2)
        {
          if(rideData.indicator == -1 || rideData.indicator == 0) rideData.indicator = abs(rideData.indicator) -1;     //control indicators on screen 1 and 2
          if(rideData.indicator == 1) rideData.indicator = -1;
        }


        if(screen.screenMode == 3)
        {
          screen.s3Mode += 4; screen.s3Mode %= 5;
        }
      }

      else if(buttons.button3 == 0 && buttons.button1 == 1)
      {
        if(screen.screenMode == 1 || screen.screenMode == 2)
        {
          if(rideData.indicator == 1 || rideData.indicator ==0) rideData.indicator = -rideData.indicator + 1;       //control indicators on screen 1 and 2
          if(rideData.indicator == -1) rideData.indicator = 1;
        }                       

        if(screen.screenMode == 3)
        {
          screen.s3Mode += 1; screen.s3Mode %= 5;
        }
      }


      else if(buttons.button2 == 0)
      {
        if(screen.screenMode == 1 || screen.screenMode == 2)
        {
          // int prevMode = rideData.hlMode;
          // rideData.hlMode = 1;
          digitalWrite(33, LOW);

          vTaskDelay(100/ portTICK_PERIOD_MS);
          digitalWrite(33,HIGH);
          // rideData.hlMode = prevMode;

        }

        if(screen.screenMode == 3) 
        {
          if(screen.s3Mode == 3) rideData.hlMode = (rideData.hlMode + 1)%5;
          if(screen.s3Mode == 4 && !rideData.indicator )  rideData.rlMode = (rideData.rlMode + 1)%4;

          if(screen.s3Mode == 1)
          {

            if(rideData.ride == 0){
              rideData.ride = 1;
              rideData.distance = 0;
              rideData.avg_speed = 0;
              rideData.max_speed = 0;
              rideData.rhour = 0;
              rideData.rmin = 0;
              rideData.time_r = 0;
              rideData.rpause = 0;
            }
            else if(rideData.ride == 1)
            {
              rideData.ride = 0;
            }
          }

          if(screen.s3Mode == 2 && rideData.ride == 1)
          {
            rideData.rpause = !rideData.rpause;
          }
        }

      }

      else if(buttons.button1 == 0 && buttons.button3 == 0)
      {
        if(screen.screenMode == 1 || screen.screenMode == 2) screen.screenMode = 3;             //settings pressed in screen 1 so go to screen 3
        
        else if(screen.screenMode == 3)
        {
          if(rideData.ride) screen.screenMode = 2;                  //if on ride go to screen 2
          else screen.screenMode = 1;                               //if not, go to main screen
        }
      }

      vTaskDelay(70 / portTICK_PERIOD_MS);  // if a button is pressed, then wait for sometime before taking another input

    }

    buttons.button1 = 1;
    buttons.button2 = 1;
    buttons.button3 = 1;
    buttons.button4 = 1;
    buttons.button5 = 1;

    vTaskDelay(10 / portTICK_PERIOD_MS);  // keep checking for button presses
  }
}




void neoPixelTask(void* pv)              //headlights are also added here although not neopixel task, but similar funcitonality
{
  // int indCounter = 0;

  strip.clear();
  while(1)
  {
    switch(rideData.rlMode * (int)!rideData.indicator)
    {
      case 0:
        rideData.rl = 0;
        strip.clear();
        strip.show();
        break;
      case 1:
        rideData.rl = 1;
        for(int i = 0;i < 16;i++) strip.setPixelColor(i, strip.Color(249 * (int)rideData.rl, 0, 0));
        strip.show();
        break;
      case 2:
        rideData.rl = ((blinkLightCounter % 3) == 0) ? !rideData.rl : rideData.rl;
        // blinkLightCounter+=1; blinkLightCounter %= (100000007);
        for(int i = 0;i < 16;i++) strip.setPixelColor(i, strip.Color(249 * (int)rideData.rl, 0, 0));
        strip.show();
        break;        
      case 3:

        // blinkLightCounter = blinkLightCounter - blinkLightCounter % 17
        
        rideData.rl = ( 
            (blinkLightCounter % 9) == 0 || 
            ((blinkLightCounter + 2) % 9) == 0 || 
            ((blinkLightCounter + 4) % 9) == 0 ||
            ((blinkLightCounter + 6) % 9) == 0 
          ) ? !rideData.rl : rideData.rl;

        // blinkLightCounter+=1; blinkLightCounter %= (100000007);
        for(int i = 0;i < 16;i++) strip.setPixelColor(i, strip.Color(249 * (int)rideData.rl, 0, 0));
        strip.show();
        break; 
    }

    // Serial.println(rideData.hlMode);
    switch(rideData.hlMode)
    {
      case 0:
        rideData.hlh = 0;
        rideData.hll = 0;
        digitalWrite(33, !rideData.hlh);
        digitalWrite(13, !rideData.hll);
        break;
      case 1:
        rideData.hlh = 1;
        rideData.hll = 0;
        digitalWrite(33, !rideData.hlh);
        digitalWrite(13, !rideData.hll);
        break;
      case 2:
        rideData.hlh = 0;
        rideData.hll = 1;
        digitalWrite(33, !rideData.hlh);
        digitalWrite(13, !rideData.hll);
        break;
      case 3:
        rideData.hlh = 1;
        rideData.hll = 1;
        digitalWrite(33, !rideData.hlh);
        digitalWrite(13, !rideData.hll);
        break;

      case 4:
        rideData.hlh = ((blinkLightCounter % 3) == 0)? !rideData.hlh : rideData.hlh;
        rideData.hll = 0;

        digitalWrite(33, !rideData.hlh);
        digitalWrite(13, !rideData.hll);

        // blinkLightCounter+=1; blinkLightCounter %= (1000000007);
        break;
 
      //   vTaskDelay(10 / portTICK_PERIOD_MS);
      //   break;        
      // case 5:
      //   rideData.hl = (((blinkLightCounter % 50) == 0) || (((blinkLightCounter + 10) % 50) == 0))? !rideData.rl : rideData.rl;
      //   blinkLightCounter+=1; blinkLightCounter %= (1000000007);
      //   vTaskDelay(10 / portTICK_PERIOD_MS);
      //   break; 
    }


    if (rideData.indicator == -1) 
    {  
      if(indCounter == 0) strip.clear();

      for(int i = 0;i < indCounter;i++)
      {
        strip.setPixelColor(i,strip.Color(50,0,0));
        strip.setPixelColor(8+i, strip.Color(50 ,0,0));
        // strip.setPixelColor(16+i, strip.Color(50 ,0,0));

      }
      strip.setPixelColor(indCounter,strip.Color(249,96,0));
      strip.setPixelColor(8+indCounter, strip.Color(249 ,96,0));
      // strip.setPixelColor(16+indCounter, strip.Color(249 ,96,0));

      strip.show();
      indCounter++; indCounter %=7;
 
    } 
    else if (rideData.indicator == 1) 
    {  
      if(indCounter == 0) strip.clear();
      for(int i = 0;i <= indCounter;i++)
      {
        strip.setPixelColor((7-i), strip.Color(50,0,0)); 
        strip.setPixelColor((15-i), strip.Color(50,0,0));
        // strip.setPixelColor((23-i), strip.Color(50,0,0));

      }
      strip.setPixelColor((7-indCounter), strip.Color(249,96,0)); 
      strip.setPixelColor((15-indCounter), strip.Color(249,96,0));
      // strip.setPixelColor((23-indCounter), strip.Color(249,96,0));


      strip.show();    
      indCounter++; indCounter %=7;
    } 


    // if (rideData.indicator == -1)   // LEFT INDICATOR
    // {
    //   if(indCounter == 7) strip.clear();

    //   // Background dim red
    //   for(int i = 0; i < 24; i++) {
    //     strip.setPixelColor(i, strip.Color(50, 0, 0));
    //   }

    //   int headCol = indCounter;   // current head col
    //   strip.setPixelColor(headCol +8  -1 , strip.Color(249, 96, 0));

    //   // Tail (5 middle row LEDs behind head, going rightward)
    //   for(int t = 0; t < 4; t++) {
    //     int col = headCol + t;
    //     if (col < 8) {
    //       strip.setPixelColor(8 + col, strip.Color(249, 96, 0));  // middle row
    //     }
    //   }

    //   // Arrow tip (head, top & bottom at same col)
    //   if (headCol >= 0 && headCol < 8) {
    //     strip.setPixelColor(headCol, strip.Color(249, 96, 0));       // top row
    //     strip.setPixelColor(16 + headCol, strip.Color(249, 96, 0));  // bottom row
    //   }

    //   strip.show();
    //   indCounter--;
    //   if (indCounter < 0) indCounter = 7;  // loop back
    // }

    // if (rideData.indicator == 1)   // RIGHT INDICATOR
    // {
    //   if(indCounter == 0) strip.clear();

    //   // Background dim red
    //   for(int i = 0; i < 24; i++) {
    //     strip.setPixelColor(i, strip.Color(50, 0, 0));
    //   }

    //   int headCol = indCounter;        // current head column (0..7)
    //   strip.setPixelColor(headCol + 8 +1 , strip.Color(249, 96, 0));


    //   // Tail (5 middle row LEDs behind head, if inside matrix)
    //   for(int t = 0; t < 4; t++) {
    //     int col = headCol - t;
    //     if (col >= 0) {
    //       strip.setPixelColor(8 + col, strip.Color(249, 96, 0));  // middle row
    //     }
    //   }

    //   // Arrow tip (head, top & bottom at same col)
    //   if (headCol >= 0 && headCol < 8) {
    //     strip.setPixelColor(headCol, strip.Color(249, 96, 0));       // top row
    //     strip.setPixelColor(16 + headCol, strip.Color(249, 96, 0));  // bottom row
    //   }

    //   strip.show();
    //   indCounter++;
    //   if (indCounter >= 8) indCounter = 0;  // loop back
    // }


    blinkLightCounter+=1; blinkLightCounter %= (100000007);

    vTaskDelay(200 / portTICK_PERIOD_MS);  // ~20 FPS
    strip.clear();
    digitalWrite(13, HIGH);
    digitalWrite(33, HIGH);

  }
}
 

double haversine(double lat1, double lon1, double lat2, double lon2)         //calculate distance between two coordinates on earth
{
  const double R = 6371000.0; // Earth radius in meters
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);
  lat1 = radians(lat1);
  lat2 = radians(lat2);

  double a = sin(dLat/2)*sin(dLat/2) + cos(lat1)*cos(lat2)*sin(dLon/2)*sin(dLon/2);
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

void readingTask(void *pv)                                                       //reads button presses and speed sensor, and also calculates speed
{
  // seed values
  // rideData.odo = 5700;                     //will be taken from mem
  // rideData.distance = 0;                   //will be 0 if ride mode is off, ow taken form memory card
  // //rideData.time_r = 0;                   //time since the ride started
  // rideData.avg_speed = 0; 
  // rideData.speed = 0;

  // rideData.battery_percentage = 78;
  // screen.screenMode = 1;
  // rideData.indicator = 0;

  // rideData.rlMode = 0;
  // rideData.hlMode = 0;


  // rideData.rhour = 0;                   //from ride
  // rideData.rmin = 0;                   //from ride
  // rideData.rpause = 0;                  //from ride

  double tyreCircumference = 2.136;  // in meters
  unsigned long timeold = 0;             //previous time at which sensor was triggerred
  unsigned long now = 0;
  bool mFlag = 1;                     //defined to ensure that sensor detecting multiple times for one tyre pass is avoided


  bool autoPause = 0;
  int tyreCount = 0;

  while (1) {

    int tyrePass = digitalRead(14);

    now = millis();  

    if (tyrePass == LOW && mFlag == 0) {
      if(now - timeold > 100)
      {
        double newSpeed = (tyreCircumference / ((now - timeold)/1e3)) * 3.6;

        if(newSpeed < rideData.speed || newSpeed - rideData.speed < 10)
        {
          rideData.speed = newSpeed;
          if(autoPause == 1)
          {
            autoPause = 0;
          } 

        }
        // rideData.speed = (tyreCircumference / ((now - timeold)/1e3)) * 3.6;
        rideData.odo += tyreCircumference / 1000.0;      
        if(rideData.ride == 1 && rideData.rpause == 0) rideData.distance += tyreCircumference / 1000.0; 
        timeold = now;
        mFlag = 1;
      }
    }

    if (tyrePass == HIGH)
      mFlag = 0;

    if ((now - timeold) > 5000) {
      rideData.speed = 0;

      if(rideData.ride == 1 && autoPause == 0)
      {
        autoPause = 1;
      }

    }

    // if(now-timeold > 60000)
    // {
    //   esp_deep_sleep_start();
    // }




    if(rideData.ride == 1 && rideData.rpause == 0 && autoPause == 0) rideData.time_r += 0.01*0.2;


    

    if (rideData.ride == 1 && rideData.time_r > 0 && rideData.rpause == 0 && autoPause ==  0) 
    {
      rideData.max_speed = max(rideData.speed, rideData.max_speed);
      rideData.rhour = rideData.time_r / 3600;
      rideData.rmin = ((int)rideData.time_r % 3600) / 60;
      rideData.avg_speed = (rideData.time_r > 0) ? (rideData.distance / (rideData.time_r / 3600.0)) : rideData.avg_speed;
    }

    // while (Serial.available()) {
    //   String line = Serial.readStringUntil('\n');

    //   line.trim();


    //   if (line.startsWith("button")) {
    //     int b = line.substring(6).toInt();
        
    //     switch(b)
    //     {
    //       case 1 : buttons.button1 = 0; break;
    //       case 2 : buttons.button2 = 0; break;
    //       case 3 : buttons.button3 = 0; break;
    //       case 4 : buttons.button4 = 0; break;
    //       case 5 : buttons.button5 = 0; break;
    //     }
    //   }
    //   if (line.startsWith("ind")) {
    //     int val = line.substring(3).toInt();
    //     if (val < -1) val = -1;
    //     if (val > 1)  val = 1;
    //     rideData.indicator = val;
    //   } else if (line.startsWith("scr")) {
    //     int s = line.substring(3).toInt();
    //     screen.screenMode = s;
    //     if(s == 1) rideData.ride = 0;
    //     else if(s == 2) rideData.ride = 1;

    //   } else if (line.startsWith("spd")) {
    //     double s = line.substring(3).toDouble();
    //     rideData.speed = s;
    //     rideData.max_speed = max(rideData.max_speed, rideData.speed);
    //   } else if (line.startsWith("batt")) {
    //     int b = line.substring(4).toInt();
    //     rideData.battery_percentage = constrain(b, 0, 100);
    //   } else if (line.startsWith("hdg")) {
    //     int h = line.substring(3).toInt();
    //     // rideData.heading = (h % 360 + 360) % 360;
    //   }

    //   else if(line.startsWith("hdl"))
    //   {
    //     rideData.hlMode = (rideData.hlMode+1)%5;
    //   }
    // }

    buttons.button1 = digitalRead(27);
    buttons.button2 = digitalRead(26);
    buttons.button3 = digitalRead(25);

    vTaskDelay(2 / portTICK_PERIOD_MS);                                             //wait to see if two buttons are pressed

  }
}

void locationTask(void* pv)
{

  rideData.hour = 69;                   //from gps
  rideData.minute = 69;                 //from gps
  // rideData.location = "location not available";

  uint32_t lastClosestPos = 0;
  const uint32_t WINDOW_BYTES = 2000*45;

  // if(!SD.begin(SD_CS, SPI))
  // {
  //   rideData.location = "SD Card Error";
  // }

  // File file = SD.open("/p7_sorted.csv");

  double minDist = 1e15;
  while(1)
  {
    double lat, lng;
    while(GPS.available())
    {
      // Serial.println(GPS.read());
      // Serial.println(gps.encode(GPS.read()));
      // Serial.write(GPS.read());
      gps.encode(GPS.read());
      
      if(gps.time.isValid()) 
      {
        int hour = gps.time.hour();
        int minute = gps.time.minute();


        

        minute += 30;
        if (minute >= 60) {
          minute -= 60;
          hour++;
        }
        hour += 5;
        
        hour = hour % 12;
        if(hour == 0) rideData.hour = 12;
        else rideData.hour = hour;
        rideData.minute = minute;
      }
    

      // minDist = haversine(26.508008, 80.227252, lat, lng);

      if(rideData.speed > 0 && gps.location.isUpdated()){

        if(xSemaphoreTake(sdMutex, portMAX_DELAY)) {  // lock SD
          


          Serial.println("gps got mutex");
          File file = SD.open("/p7_sorted.csv");

          if(file){

            file.seek(0);                  // to skip 
            file.readStringUntil('\n');    // headers

            String minName = "";
            int k = 0;


            lat = gps.location.lat();
            lng = gps.location.lng();


            uint32_t startPos = (lastClosestPos == 0) ? file.position() : max((uint32_t)file.position(), lastClosestPos - WINDOW_BYTES/2);
            uint32_t endPos   = (lastClosestPos == 0) ? 0xFFFFFFFF : lastClosestPos + WINDOW_BYTES/2;


            file.seek(startPos);
                

            minDist = 1e9;
            while(file.available() && file.position() <= endPos)
            {
              String line = file.readStringUntil('\n');
              line.trim();
              if(line.length() == 0) continue;

              int first = line.indexOf(',');
              int second = line.indexOf(',', first+1);
              if(first < 0 || second < 0) continue;

              long latf = line.substring(0, first).toInt();
              long lngf = line.substring(first+1, second).toInt();
              // String name = line.substring(second+1);

              double latD = latf / 1e6;
              double lngD = lngf / 1e6;
              
              String location;

              double d = haversine(lat, lng, latD, lngD);
              if(d < minDist)
              {
                minDist = d;
                minName = line.substring(second+1);
                // Serial.println(minName); 
                lastClosestPos = file.position() - line.length() - 1;
              }

              if(d < 30) break;

              
              if(k++ % 100 == 0) vTaskDelay(1 / portTICK_PERIOD_MS); // yield to watchdog

            }

            rideData.location = minName;
          }
          else
          {
            rideData.location = "SD Read Error";
          }

          xSemaphoreGive(sdMutex); // unlock SD


        }


      }

    }
    vTaskDelay(10 / portTICK_PERIOD_MS);  // ~20 FPS

  }
}

// void memTask(void *pv)
// {
//   const char *filePath = "/ridedata.csv";
//   while(1)
//   {
//     if(xSemaphoreTake(sdMutex, portMAX_DELAY))
//     {
//       Serial.println("mem task got mutex");

//       File f = SD.open(filePath, FILE_WRITE);
//       if (f) {

//         Serial.println("no error");

//         f.seek(0); // overwrite existing file
//         f.print(rideData.odo); f.print(",");
//         f.print(rideData.distance); f.print(",");
//         f.print(rideData.time_r); f.print(",");
//         f.print(rideData.avg_speed); f.print(",");
//         f.print(rideData.max_speed); f.print(",");
//         // f.print(rideData.speed); f.print(",");
//         // f.print(rideData.battery_percentage); f.print(",");
//         // f.print(rideData.indicator); f.print(",");
//         // f.print(rideData.location); f.print(",");
//         f.print(rideData.hour); f.print(",");
//         f.print(rideData.minute); f.print(",");
//         // f.print(rideData.hlMode); f.print(",");
//         // f.print(rideData.rlMode); f.print(",");
//         // f.print(rideData.rl); f.print(",");
//         // f.print(rideData.hlh); f.print(",");
//         // f.print(rideData.hll); f.print(",");
//         f.print(rideData.ride); f.print(",");
//         f.print(rideData.rhour); f.print(",");
//         f.print(rideData.rmin); f.print(",");
//         f.print(rideData.rpause);
//         f.println();
//         f.close();
//       }

//       else
//       {
//         rideData.location = "Some Error!";
//       }

//       xSemaphoreGive(sdMutex);

//     }

//       vTaskDelay(1000 / portTICK_PERIOD_MS);                                             //wait to see if two buttons are pressed
//   }
// }


void memTask(void *pv)
{
  const char *filePath = "/ridedata.csv";
  while(1)
  {
    if(xSemaphoreTake(sdMutex, portMAX_DELAY))
    {
      SD.remove(filePath);  // truncate old content

      Serial.println("mem got mutex");

      File f = SD.open(filePath, FILE_WRITE);
      if (f) {
        f.print(rideData.odo); f.print(",");
        f.print(rideData.distance); f.print(",");
        f.print(rideData.time_r); f.print(",");
        f.print(rideData.avg_speed); f.print(",");
        f.print(rideData.max_speed); f.print(",");
        f.print(rideData.hour); f.print(",");
        f.print(rideData.minute); f.print(",");
        f.print(rideData.ride); f.print(",");
        f.print(rideData.rhour); f.print(",");
        f.print(rideData.rmin); f.print(",");
        f.print(rideData.rpause);
        f.println();
        f.close();
      }
      else
      {
        rideData.location = "SD Write Error!";
      }

      xSemaphoreGive(sdMutex);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}






void setup() {
  Serial.begin(9600);
  // I2C init (optional: set speed)
  SD.begin(SD_CS, SPI);

  Wire.begin(21, 22);
  Wire.setClock(400000);
  strip.begin();
  strip.show();
  GPS.begin(9600 , SERIAL_8N1, 16,17);

  pinMode(25, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_26, 0);
  pinMode(27, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(14), wheelISR, FALLING);

  // pinMode(14, INPUT);

  pinMode(33, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(33, HIGH);
  digitalWrite(13, HIGH);



  // screen.screenMode = 1;


  // const char *filePath = "/ridedata.bin";

  //   File f = SD.open(filePath, FILE_READ);
  //   if (!f) return;

  //   String line = f.readStringUntil('\n');
  //   f.close();

  //   int idx = 0;
  //   int lastComma = -1;
  //   int nextComma = 0;
  //   String tokens[21]; // total fields

  //   while (idx < 21 && lastComma < line.length()) {
  //     nextComma = line.indexOf(',', lastComma + 1);
  //     if (nextComma == -1) nextComma = line.length();
  //     tokens[idx++] = line.substring(lastComma + 1, nextComma);
  //     lastComma = nextComma;
  //   }

  //   if (idx == 21) { // all fields present
  //     rideData.odo = tokens[0].toDouble();
  //     rideData.distance = tokens[1].toDouble();
  //     rideData.time_r = tokens[2].toDouble();
  //     rideData.avg_speed = tokens[3].toDouble();
  //     rideData.max_speed = tokens[4].toDouble();
  //     rideData.speed = tokens[5].toDouble();
  //     rideData.battery_percentage = tokens[6].toDouble();
  //     rideData.indicator = tokens[7].toInt();
  //     rideData.location = tokens[8];
  //     rideData.hour = tokens[9].toInt();
  //     rideData.minute = tokens[10].toInt();
  //     rideData.hlMode = tokens[11].toInt();
  //     rideData.rlMode = tokens[12].toInt();
  //     rideData.rl = tokens[13].toInt();
  //     rideData.hlh = tokens[14].toInt();
  //     rideData.hll = tokens[15].toInt();
  //     rideData.ride = tokens[16].toInt();
  //     rideData.rhour = tokens[17].toInt();
  //     rideData.rmin = tokens[18].toInt();
  //     rideData.rpause = tokens[19].toInt();
  //   }
  // }

  // if(rideData.ride) screen.screenMode = 2;




  // rideData.odo = 6000;                     //will be taken from mem


  // rideData.distance = 0;                   //will be 0 if ride mode is off, ow taken form memory card
  // rideData.time_r = 0;       
  //             //time since the ride started
  // rideData.avg_speed = 0; 
  // rideData.speed = 0;
  // rideData.battery_percentage = 61;
  // rideData.indicator = 0;

  // rideData.rlMode = 0;
  // rideData.hlMode = 0;

  // // rideData.hlMode = 3;

  // rideData.ride = 0;
  // rideData.rhour = 0;                   //from ride
  // rideData.rmin = 0;                   //from ride
  // rideData.rpause = 0;  


    rideData.odo = 6000;                    // total odometer, 0 on first run
    rideData.distance = 0;               // distance for current ride, 0
    rideData.time_r = 0;                 // ride time, 0
    rideData.avg_speed = 0;              // average speed, 0
    rideData.max_speed = 0;              // max speed, 0
    rideData.speed = 0;                  // current speed, 0
    rideData.battery_percentage = 61;   // assume full battery on first run
    rideData.indicator = 0;              // indicator off
    rideData.location = "location not available (setup)";               // empty location string
    rideData.hour = 0;
    rideData.minute = 0;

    rideData.hlMode = 0;                 // headlight mode
    rideData.rlMode = 0;                 // rear light mode
    rideData.rl = false;                 // rear light off
    rideData.hlh = false;                // headlight high beam off
    rideData.hll = false;                // headlight low beam off

    rideData.ride = false;               // not riding yet
    rideData.rhour = 0;                  // ride hours
    rideData.rmin = 0;                   // ride minutes
    rideData.rpause = false;             // ride not paused





  screen.screenMode = 1;


  u8g2.begin();
  // u8g2.setDisplayRotation(U8G2_R2);

  sdMutex = xSemaphoreCreateMutex();



  // Create tasks pinned to cores (ESP32 has 0 & 1)
  // Display on core 1 (typically Arduino loop core)
  xTaskCreatePinnedToCore(
    displayTask,
    "DisplayTask",
    4096,        // stack
    nullptr,
    1,           // priority
    &displayTaskHandle,
    1            // core 1
  );

  // Readings on core 0
  xTaskCreatePinnedToCore(
    readingTask,
    "ReadingTask",
    4096,
    nullptr,
    1,
    &readingTaskHandle,
    0            // core 0
  );


  xTaskCreatePinnedToCore(
    neoPixelTask,
    "NeoPixelTask",
    4096,
    nullptr,
    1,
    &neoPixelTaskHandle,
    0
  );

  xTaskCreatePinnedToCore(
    locationTask,
    "LocationTask",
    4096,
    nullptr,
    1,
    &locationTaskHandle,
    1
  );


  xTaskCreatePinnedToCore(
    buttonsTask,
    "ButtonsTask",
    4096,
    nullptr,
    1,
    &buttonsTaskHandle,
    0
  );

  xTaskCreatePinnedToCore(
    memTask,
    "MemTask",
    4096,
    nullptr,
    1,
    &memTaskHandle,
    0
  );


}

void loop() {}
