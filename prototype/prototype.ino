// ===== ESP32 + FreeRTOS + U8g2 (SH1106 128x64 I2C) =====
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include<SPI.h>
#include<math.h>
#include<SD.h>
#include<TinyGPS++.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_system.h>
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
  double at_max_speed;
  double speed;
  double battery_percentage;
  double battery_voltage;
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
  String start_time;
  // rideData.
  double r_startlat;
  // double r_endlat;
  double r_startlng;
  // double r_endlng;
  String r_location;
  double r_maxDist;


  int day;
  int month;
  int year;
  String utf_time;

  double lattitude;
  double longitude;
  // float elevation;
  bool isGPS;




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
TaskHandle_t stravaTaskHandle = nullptr;
TaskHandle_t gpxLoggingTaskHandle = nullptr;

// int blinkCounter = 0;
int blinkLightCounter = 0;
int indCounter = 0;



void displayTask(void *pv) 
{
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

      int hour = rideData.hour % 12;

      if(hour == 0) hour = 12;

      sprintf(buf, "%02d:%02d", hour, rideData.minute);  
      u8g2.drawStr(110-u8g2.getStrWidth(buf), 8, buf);

      if(rideData.isGPS && blinkLightCounter % 6 < 1 && blinkLightCounter % 6 >= 0)
      {
        int cx = 103-u8g2.getStrWidth(buf), cy = 2, r = 2; 
        u8g2.drawCircle(cx, cy, r, U8G2_DRAW_ALL);
        u8g2.drawTriangle(cx-r, cy+1, cx+r, cy+1, cx, cy+6);
        u8g2.setDrawColor(0);  
        u8g2.drawDisc(cx, cy, r / 2, U8G2_DRAW_ALL);
        u8g2.setDrawColor(1);  
      }



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
      if (indCounter % 4== 0) {
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





      int hour = rideData.hour % 12;
      if(hour == 0) hour = 12;

      
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


      else sprintf(buf, "%02d:%02d", hour, rideData.minute);
      u8g2.drawStr(110-u8g2.getStrWidth(buf), 8, buf);

      
      if(rideData.isGPS && blinkLightCounter % 6 < 1 && blinkLightCounter % 6 >= 0)
      {
        int cx = 103-u8g2.getStrWidth(buf), cy = 2, r = 2; 
        u8g2.drawCircle(cx, cy, r, U8G2_DRAW_ALL);
        u8g2.drawTriangle(cx-r, cy+1, cx+r, cy+1, cx, cy+6);
        u8g2.setDrawColor(0);  
        u8g2.drawDisc(cx, cy, r / 2, U8G2_DRAW_ALL);
        u8g2.setDrawColor(1);  
      }



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






      if (indCounter % 4 == 0) {
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
        u8g2.drawStr(7, 20, buf);




        if (rideData.at_max_speed >= 10) sprintf(buf, "^^%2.0f ", rideData.at_max_speed);
        else sprintf(buf, "^^%2.1f ", rideData.at_max_speed);
        u8g2.setFont(u8g2_font_10x20_tf);

        u8g2.drawStr(70,24, buf);






      }


      if(rideData.ride == 1)                                      
      {
        sprintf(buf, "end ride");  
        u8g2.setFont(u8g2_font_6x12_tf);
        u8g2.drawStr(7, 20, buf);

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



  while(1)
  {
    if(!buttons.button1 || !buttons.button2 || !buttons.button3)
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
          int prevMode = rideData.hlMode;
          // rideData.hlMode = 1;
          digitalWrite(33, 1);

          vTaskDelay(70/ portTICK_PERIOD_MS);
          digitalWrite(33,0);
          // rideData.hlMode = prevMode;

        }

        if(screen.screenMode == 3) 
        {
          if(screen.s3Mode == 3) rideData.hlMode++;
          if(screen.s3Mode == 4 && !rideData.indicator )  rideData.rlMode++;

          if(screen.s3Mode == 1)
          {

            if(rideData.ride == 0){

              // rideData.start_time = rideData.utf_time;
              rideData.ride = 1;
              rideData.distance = 0;
              rideData.avg_speed = 0;
              rideData.max_speed = 0;
              rideData.rhour = 0;
              rideData.rmin = 0;
              rideData.time_r = 0;
              rideData.rpause = 0;

              if(rideData.isGPS == 0)
              {
                rideData.location = "logging off!";
              }

            }
            else if(rideData.ride == 1)
            {


              rideData.ride = 0;
              rideData.distance = 0;
              rideData.avg_speed = 0;
              rideData.max_speed = 0;
              rideData.time_r = 0;
              rideData.rhour = 0;
              rideData.rmin = 0;
              rideData.start_time = "";


              
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

      vTaskDelay(90 / portTICK_PERIOD_MS);  // if a button is pressed, then wait for sometime before taking another input

    }

    buttons.button1 = 1;
    buttons.button2 = 1;
    buttons.button3 = 1;


    vTaskDelay(10 / portTICK_PERIOD_MS);  // keep checking for button presses
  }
}


void stravaTask(void* pv)
{
    const char* client_id = "179574";
    const char* client_secret = "cb9fbad5f26cc9638dc2c6dc31f883534a0ff0f3";
    const char* refresh_token = "e74df5b338d5865c79162eb61c4b9b1a53198777";

    int retries = 0;

    while(WiFi.status() != WL_CONNECTED && retries < 5)
    {
      delay(1000);
      retries++;
    }


    if (WiFi.status() != WL_CONNECTED) 
    {
        Serial.println("\nWiFi connection failed, ending task to save energy");

        WiFi.disconnect(true);  // disconnect from AP
        WiFi.mode(WIFI_OFF);    // turn off WiFi to save power

        xTaskCreatePinnedToCore(
        locationTask,
        "LocationTask",
        4096,
        nullptr,
        1,
        &locationTaskHandle,
        1
        );
        // return;
        vTaskDelete(NULL);  // safely delete this task
    } 
    else 
    {
        Serial.println("\nWiFi connected!");
        rideData.location = "WiFi connected!";
    }


    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    // HTTPClient http;
    String token_url = "https://www.strava.com/oauth/token?client_id=" + String(client_id) +
                      "&client_secret=" + String(client_secret) +
                      "&grant_type=refresh_token&refresh_token=" + refresh_token;

    http.begin(client, token_url);
    int httpResponseCode = http.POST("");

    if (httpResponseCode != 200) 
    {
      rideData.location = "Failed to get token";
      http.end();
      WiFi.disconnect(true);  // disconnect from AP
      WiFi.mode(WIFI_OFF);
      // return;
        xTaskCreatePinnedToCore(
        locationTask,
        "LocationTask",
        4096,
        nullptr,
        1,
        &locationTaskHandle,
        1
        );
      vTaskDelete(NULL);
    }

    String response = http.getString();
    Serial.println("Token Response:");
    Serial.println(response);
    http.end();

    int i = response.indexOf("\"access_token\":\"");

    if (i == -1) {
      rideData.location = "Access token not found!";
      http.end();
      WiFi.disconnect(true);  // disconnect from AP
      WiFi.mode(WIFI_OFF);


        xTaskCreatePinnedToCore(
        locationTask,
        "LocationTask",
        4096,
        nullptr,
        1,
        &locationTaskHandle,
        1
        );

      vTaskDelete(NULL);
      // return;
    }

    i += 16;
    int j = response.indexOf("\"", i);
    String access_token = response.substring(i, j);
    Serial.println(access_token);

    client.stop();                 // Force close previous SSL session
    delay(200); 

    // WiFiClientSecure client;

    if (xSemaphoreTake(sdMutex, portMAX_DELAY)) 
    {
        File root = SD.open("/gpxRides");
        File gpxFile = root.openNextFile();

        while (gpxFile) {
            if (!gpxFile.isDirectory()) 
            {
                String boundary = "ESP32Boundary123";
                String contentType = "multipart/form-data; boundary=" + boundary;

                // Multipart header
                String header;
                header.reserve(200);
                header = "--" + boundary + "\r\n";
                header += "Content-Disposition: form-data; name=\"file\"; filename=\"" + (String)gpxFile.name() + "\"\r\n";
                header += "Content-Type: application/gpx+xml\r\n\r\n";

                Serial.println((String)gpxFile.name());

                // GPX header
                String gpxHeader;
                gpxHeader.reserve(300);
                gpxHeader = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
                gpxHeader += "<gpx version=\"1.1\" creator=\"ESP32\" xmlns=\"http://www.topografix.com/GPX/1/1\" ";
                gpxHeader += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
                gpxHeader += "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\r\n";
                gpxHeader += "<trk><name>ESP32 ride!!!</name><trkseg>\r\n";

                // GPX footer
                String gpxFooter;
                gpxFooter.reserve(50);
                gpxFooter = "</trkseg>\r\n";
                gpxFooter += "</trk>\r\n";
                gpxFooter += "</gpx>\r\n";

                // Data type part
                String dataTypePart;
                dataTypePart.reserve(100);
                dataTypePart = "\r\n--" + boundary + "\r\n";
                dataTypePart += "Content-Disposition: form-data; name=\"data_type\"\r\n\r\n";
                dataTypePart += "gpx\r\n";


                // Activity type part
                String activityTypePart;
                activityTypePart.reserve(100);
                activityTypePart = "\r\n--" + boundary + "\r\n";
                activityTypePart += "Content-Disposition: form-data; name=\"activity_type\"\r\n\r\n";
                activityTypePart += "Ride\r\n";

                // Multipart footer
                String footer;
                footer.reserve(50);
                footer = "--" + boundary + "--\r\n";

                size_t totalSize = header.length() + gpxHeader.length() + gpxFile.size() + gpxFooter.length() + 
                                   dataTypePart.length() + activityTypePart.length() + footer.length();

                client.setInsecure(); // For testing; use SSL certificate in production
                delay(1000);
                if (!client.connect("www.strava.com", 443)) {
                    Serial.println("Failed to connect to Strava");
                    gpxFile.close();
                    gpxFile = root.openNextFile();
                    continue;
                }
                // Serial.printf("Free heap before connect: %u bytes\n", ESP.getFreeHeap());

                // Send HTTP request headers
                String request;
                request.reserve(300);
                request = "POST /api/v3/uploads HTTP/1.1\r\n";
                request += "Host: www.strava.com\r\n";
                request += "Authorization: Bearer " + access_token + "\r\n";
                request += "Content-Type: " + contentType + "\r\n";
                request += "Content-Length: " + String(totalSize) + "\r\n";
                request += "Connection: close\r\n\r\n";
                if (client.print(request) == 0) {
                    Serial.println("Failed to send request headers");
                    client.stop();
                    gpxFile.close();
                    gpxFile = root.openNextFile();
                    continue;
                }

                // Stream multipart header
                if (client.print(header) == 0) {
                    Serial.println("Failed to send multipart header");
                    client.stop();
                    gpxFile.close();
                    gpxFile = root.openNextFile();
                    continue;
                }

                // Stream GPX header
                if (client.print(gpxHeader) == 0) {
                    Serial.println("Failed to send GPX header");
                    client.stop();
                    gpxFile.close();
                    gpxFile = root.openNextFile();
                    continue;
                }

                // Stream file in chunks




                
                const size_t bufferSize = 512;
                uint8_t buffer[bufferSize];


                while (gpxFile.available()) {
                    size_t bytesRead = gpxFile.read(buffer, bufferSize);
                    if (client.write(buffer, bytesRead) == 0) {
                        Serial.println("Failed to send file chunk");
                        client.stop();
                        gpxFile.close();
                        gpxFile = root.openNextFile();
                        continue;
                    }
                }







                String fullPath = "/gpxRides"; 
                fullPath += "/";
                fullPath += (String)gpxFile.name();


                gpxFile.close();

                // Stream GPX footer
                if (client.print(gpxFooter) == 0) {
                    Serial.println("Failed to send GPX footer");
                    client.stop();
                    gpxFile = root.openNextFile();
                    continue;
                }

                // Stream data_type part
                if (client.print(dataTypePart) == 0) {
                    Serial.println("Failed to send data_type part");
                    client.stop();
                    gpxFile = root.openNextFile();
                    continue;
                }

                // Stream activity_type part
                if (client.print(activityTypePart) == 0) {
                    Serial.println("Failed to send activity_type part");
                    client.stop();
                    gpxFile = root.openNextFile();
                    continue;
                }

                // Stream multipart footer
                if (client.print(footer) == 0) {
                    Serial.println("Failed to send multipart footer");
                    client.stop();
                    gpxFile = root.openNextFile();
                    continue;
                }

                // Read response
                String response;
                response.reserve(512);
                unsigned long timeout = millis();

                while (client.connected() && millis() - timeout < 10000) {
                    if (client.available()) {
                        response += client.readStringUntil('\n');
                        if (response.indexOf("\r\n\r\n") >= 0) break;
                    }
                }
                while (client.available()) {
                    response += client.readString();
                }

              

                int httpResponseCode = 0;
                if (response.startsWith("HTTP/1.1 ")) {
                    httpResponseCode = response.substring(9, 12).toInt();
                }

                Serial.print("Upload response code: ");
                Serial.println(httpResponseCode);

                String uploadId = "";
                if (httpResponseCode == 201) {
                    int bodyStart = response.indexOf("\r\n\r\n") + 4;
                    String body = response.substring(bodyStart);
                    Serial.println(body);
                    Serial.println("GPX uploaded successfully!");
                    // rideData.location = "ride uploaded to Strava!";
                    Serial.print("full path: "); Serial.println(fullPath);

                    // Extract upload ID from response
                    int idStart = body.indexOf("\"id\":") + 5;
                    int idEnd = body.indexOf(",", idStart);
                    uploadId = body.substring(idStart, idEnd);
                    uploadId.trim();
                    Serial.print("Upload ID: ");
                    Serial.println(uploadId);

                    // Poll Strava for upload status
                    bool processedSuccessfully = false;
                    delay(2000); 

                    String statusUrl = "https://www.strava.com/api/v3/uploads/" + uploadId;
                    http.begin(client, statusUrl);
                    http.addHeader("Authorization", "Bearer " + access_token);

                    int statusCode = http.GET();
                    if (statusCode > 0) {
                        String statusResponse = http.getString();
                        Serial.println("Upload Status Response:");
                        Serial.println(statusResponse);

                        if (statusResponse.indexOf("Your activity is ready") >= 0) {
                            Serial.println("Ride processed successfully!");
                            rideData.location = "ride uploaded to Strava!!";
                            processedSuccessfully = true;
                        }

                        else  
                        {
                            Serial.println("Ride upload failed! See error message above.");
                            rideData.location = "error processing activity";
                            processedSuccessfully = false;
                        }


                    } 
                    
                    else {
                        Serial.print("GET failed, error: ");
                        Serial.println(statusCode);
                    }

                    http.end();  // clean up before next retry
                    
                    if (processedSuccessfully) {
                        if (SD.remove(fullPath)) Serial.println("GPX file removed successfully after processing.");
                        else Serial.println("Could not remove GPX file.");
                    } else {
                        Serial.println("GPX file kept for retry.");
                    }

                } else {
                    Serial.print("Error on sending POST: ");
                    if (httpResponseCode == 0) Serial.println("No response or connection failed");
                    else Serial.println(response);
                    Serial.println("GPX upload failed, keeping file for retry.");
                    rideData.location = "ride upload failed!";
                }

                client.stop();
                gpxFile = root.openNextFile();
            }
        }

        root.close();
    }


  xSemaphoreGive(sdMutex); // unlock SD

  WiFi.disconnect(true);  // disconnect from AP
  WiFi.mode(WIFI_OFF);    // turn off WiFi to save power
  Serial.println("WiFi turned off");

  vTaskDelay(5000 / portTICK_PERIOD_MS);

  xTaskCreatePinnedToCore(
    locationTask,
    "LocationTask",
    4096,
    nullptr,
    1,
    &locationTaskHandle,
    1
  );

  vTaskDelete(NULL);  // <- THIS IS REQUIRED

}


void gpxLoggingTask(void* pvParameters)
{


    String safeTime = rideData.start_time;
    if(safeTime == "") safeTime = rideData.utf_time;                     //if not empty, that means from previous ride
    rideData.start_time = safeTime;

    String timeOld = safeTime;
    String timeNew = safeTime; 

    safeTime.replace(":", "_");
    safeTime.replace("T", "_");

    String filename = "/gpxRides/ride_" + safeTime + ".gpx";

    Serial.println(filename);

    File rideFile;
    if (xSemaphoreTake(sdMutex, portMAX_DELAY)) {
        if(SD.exists(filename)) 
        {
          rideFile = SD.open(filename, FILE_APPEND);
          Serial.println("appending");
        }
        else 
        {
          rideFile = SD.open(filename, FILE_WRITE);
          Serial.println("new file created");
        }
        xSemaphoreGive(sdMutex);
    } 

    // 3. Logging loop
    while (1)
    {
        // Example: check if ride is active
        if (rideData.ride == 0) break; // you should update this flag elsewhere

        // Acquire SD mutex to log data
        if (xSemaphoreTake(sdMutex, portMAX_DELAY) && rideData.isGPS)
        {
            timeNew = rideData.utf_time;
            if(timeOld != timeNew)
            {
              // Serial.println("logger got mutex");

              char latStr[12];
              char lonStr[12];
              dtostrf(rideData.lattitude, 0, 6, latStr);  
              dtostrf(rideData.longitude, 0, 6, lonStr);

              if (strlen(latStr) == 9 && strlen(lonStr) == 9 && timeNew.length() == 20) {
                  rideFile.print("<trkpt lat=\"");
                  rideFile.print(latStr);
                  rideFile.print("\" lon=\"");
                  rideFile.print(lonStr);
                  rideFile.print("\"><time>");
                  rideFile.print(timeNew);
                  rideFile.println("</time></trkpt>");
                  rideFile.flush();
                  timeOld = timeNew;
              } else {
                  Serial.println("Skipping malformed GPS point");
              }


            }

        }
        xSemaphoreGive(sdMutex);


        vTaskDelay(1050 / portTICK_PERIOD_MS); // log at 0.2 Hz (every 5s) //mutex takes care that it doesnt log too fast so no need to add it 
    }



    // 4. Close GPX properly
    if (xSemaphoreTake(sdMutex, portMAX_DELAY))
    {

        Serial.println("logger got mutex to end task");
        // rideFile.println("</trkseg></trk></gpx>"); added in upload
        rideFile.close();
        xSemaphoreGive(sdMutex);
    }

    Serial.println("Ride logging finished.");
    rideData.location = "ride logging finished!";

    // 5. Delete this task
    gpxLoggingTaskHandle = NULL;
    vTaskDelete(NULL);

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
        if((blinkLightCounter % 4) == 0 || ((blinkLightCounter - 2) % 4 == 0)) rideData.rl = 1;
        else rideData.rl = 0;
        for(int i = 0;i < 16;i++) strip.setPixelColor(i, strip.Color(249 * (int)rideData.rl, 0, 0));
        strip.show();
        break;    

      case 4:        
        if((blinkLightCounter % 10) == 0 || ((blinkLightCounter - 2) % 10 == 0)) rideData.rl = 1;
        else rideData.rl = 0;
        for(int i = 0;i < 16;i++) strip.setPixelColor(i, strip.Color(249 * (int)rideData.rl, 0, 0));
        strip.show();
        break; 

      

      default:
        rideData.rlMode %= 5;
        break;
    }


    if(rideData.battery_voltage < 9.5 && rideData.hlMode != 0)
    {
      rideData.hlMode = 0;
      rideData.location = "low battery :(";
    }

    switch(rideData.hlMode)
    {
      case 0:
        rideData.hlh = 0;
        rideData.hll = 0;
        break;
      case 1:
        rideData.hlh = 1;
        rideData.hll = 0;
        break;
      case 2:
        rideData.hlh = 0;
        rideData.hll = 1;
        break;
      case 3:
        rideData.hlh = 1;
        rideData.hll = 1;
        break;

      case 4:
        if((blinkLightCounter % 10) == 0 || ((blinkLightCounter - 2) % 10 == 0)) rideData.hlh = 1;
        else rideData.hlh = 0;
        rideData.hll = 0;
        break;

      case 5:

        rideData.hll = 1;
        if(blinkLightCounter % 6 == 0) rideData.hlh = 1;
        else rideData.hlh = 0;

      default:

        rideData.hlMode %= 6;

    }

    digitalWrite(33, rideData.hlh);
    digitalWrite(13, rideData.hll);


    if (rideData.indicator == -1) 
    {  
      if(indCounter % 4 == 0) strip.clear();

      for(int i = 0;i <= indCounter;i++)
      {
        strip.setPixelColor(i,strip.Color(50,0,0));
        strip.setPixelColor(8+i, strip.Color(50 ,0,0));
        // strip.setPixelColor(16+i, strip.Color(50 ,0,0));

      }
      strip.setPixelColor(indCounter,strip.Color(249,96,0));
      strip.setPixelColor(8+indCounter, strip.Color(249 ,96,0));
      // strip.setPixelColor(16+indCounter, strip.Color(249 ,96,0));

      strip.show();
      indCounter++; indCounter %=8;
 
    } 
    else if (rideData.indicator == 1) 
    {  
      if(indCounter % 4 == 0) strip.clear();
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
      indCounter++; indCounter %=8;
    } 

    blinkLightCounter+=1; blinkLightCounter %= (100000007);

    vTaskDelay(200 / portTICK_PERIOD_MS);  // ~20 FPS
    strip.clear();
    // digitalWrite(33, 0);
    // digitalWrite(13, 0);

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

        if(newSpeed < rideData.speed || newSpeed - rideData.speed < 30)
        {
          rideData.speed = newSpeed;

          rideData.at_max_speed = max(rideData.speed, rideData.at_max_speed);
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




    if(rideData.ride == 1 && rideData.rpause == 0 && autoPause == 0) rideData.time_r += 0.01*0.2;


    

    if (rideData.ride == 1 && rideData.time_r > 0 && rideData.rpause == 0 && autoPause ==  0) 
    {
      rideData.max_speed = max(rideData.speed, rideData.max_speed);
      rideData.rhour = rideData.time_r / 3600;
      rideData.rmin = ((int)rideData.time_r % 3600) / 60;
      rideData.avg_speed = (rideData.time_r > 0) ? (rideData.distance / (rideData.time_r / 3600.0)) : rideData.avg_speed;
    }

    

    if(GPS.available())
    {
      gps.encode(GPS.read());
      
      if(gps.time.isValid() && gps.date.isValid()) 
      {
        int hour = gps.time.hour();
        int minute = gps.time.minute();
        int second = gps.time.second();

        if(gps.date.year() != 2000)
        {
          rideData.day = gps.date.day();
          rideData.month = gps.date.month();
          rideData.year = gps.date.year();
        }

        

        if (rideData.year >= 2025 &&
            rideData.month >= 1 && rideData.month <= 12 &&
            rideData.day >= 1 && rideData.day <= 31 &&
            hour >= 0 && hour <= 23 &&
            minute >= 0 && minute <= 59 &&
            second >= 0 && second <= 59)
        {
            rideData.utf_time = String(rideData.year) + "-" +
                      (rideData.month < 10 ? "0" : "") + String(rideData.month) + "-" +
                      (rideData.day < 10 ? "0" : "") + String(rideData.day) + "T" +
                      (hour < 10 ? "0" : "") + String(hour) + ":" +
                      (minute < 10 ? "0" : "") + String(minute) + ":" +
                      (second < 10 ? "0" : "") + String(second) + "Z";

            minute += 30;
            if (minute >= 60) {
              minute -= 60;
              hour++;
            }
            hour += 5;

            rideData.hour = hour;
            rideData.minute = minute;
        }




      }



      // if(gps.altitude.isUpdated() && gps.altitude.isValid())
      // {
      //   rideData.elevation = gps.altitude.meters();
      // }

      
      if(gps.location.isUpdated() && gps.location.isValid())
      {
        rideData.lattitude = gps.location.lat();
        rideData.longitude = gps.location.lng();
      }

      if(gps.location.isValid() && gps.time.isValid() && rideData.utf_time != "")
      {
        rideData.isGPS = 1;
        
        if(rideData.ride == 1 && gpxLoggingTaskHandle == NULL)
        {

          Serial.println("logging ride");
          rideData.location = "logging ride!!";
          xTaskCreatePinnedToCore(
          gpxLoggingTask,
          "GPX Logging Task",
          4096,
          nullptr,
          1,
          &gpxLoggingTaskHandle,
          1
          );
        }

      }

      else if(gps.location.age() < 4000)
      {
        rideData.isGPS = 0;
      }

    }






    buttons.button1 = digitalRead(27);
    buttons.button2 = digitalRead(26);
    buttons.button3 = digitalRead(25);




    // Serial.println(rideData.battery_percentage);

    vTaskDelay(2 / portTICK_PERIOD_MS);                                             //wait to see if two buttons are pressed

  }
}

void locationTask(void* pv)
{


  rideData.location = "location not available";

  Serial.println("location task active");

  uint32_t lastClosestPos = 0;
  const uint32_t WINDOW_BYTES = 4000*45;


  double minDist = 1e15;


  while(1)
  {

      if(xSemaphoreTake(sdMutex, portMAX_DELAY) && rideData.isGPS) {  // lock SD





        int raw = analogRead(35);             // Read ADC (0–4095)
        float vAdc = (raw / 4095.0) * 3.3 * 1.067;            // Convert to rideData.battery_voltage at ADC pin (3.3V reference)
        rideData.battery_voltage = vAdc * (30000.0 + 7500.0) / 7500.0;           // Undo divider → actual battery voltage



        const float vEmpty = 9.0;
        const float vFull  = 12.5;
        // Serial.println(voltage);
        if (rideData.battery_voltage <= vEmpty) rideData.battery_percentage = 0.0;
        else if (rideData.battery_voltage >= vFull)  rideData.battery_percentage = 100.0;
        else
        {
          float x = (rideData.battery_voltage - vEmpty) / (vFull - vEmpty);
          rideData.battery_percentage = (1 - pow(1 - x, 1.5)) * 100.0;
        }
        

        // Serial.println("gps got mutex");
        File file = SD.open("/p7_sorted.csv");

        if(file){

          file.seek(0);                  // to skip 
          file.readStringUntil('\n');    // headers

          String minName = "";





          uint32_t startPos = (lastClosestPos == 0) ? file.position() : max((uint32_t)file.position(), lastClosestPos - WINDOW_BYTES/2);
          uint32_t endPos   = (lastClosestPos == 0) ? 0xFFFFFFFF : lastClosestPos + WINDOW_BYTES/2;


          file.seek(startPos);
              

          minDist = 1e9;

          int lines_read = 0;

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

            double d = haversine(rideData.lattitude, rideData.longitude, latD, lngD);
            if(d < minDist)
            {

              if(rideData.ride && rideData.r_startlat == 0)
              {
                rideData.r_startlat = latD;
                rideData.r_startlng = lngD;
              } 
              minDist = d;
              minName = line.substring(second+1);
              // Serial.println(minName); 
              lastClosestPos = file.position() - line.length() - 1;
            }


            if(d < 30) break;

            
            if(lines_read++ % 100 == 0) vTaskDelay(1 / portTICK_PERIOD_MS); // yield to watchdog


            lines_read++;

            if(lines_read >= 500) 
            { 
              size_t curr_pos = file.position();
              
              file.close();
              
              xSemaphoreGive(sdMutex);
              
              vTaskDelay(1 / portTICK_PERIOD_MS); 
              
              lines_read = 0; 


              if(xSemaphoreTake(sdMutex, portMAX_DELAY)) 
              { 
                file = SD.open("/p7_sorted.csv"); 
                file.seek(curr_pos); 
                // Serial.println("gps got mutex after line read");
              } 
            
            }
            

          }

          rideData.location = minName;
        }
        else
        {
          rideData.location = "SD Read Error";
        }



      }

        xSemaphoreGive(sdMutex); // unlock SD
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // ~20 FPS

  }
}

void memTask(void *pv)
{

  delay(5000);

  bool writeToA = 1;
  while(1)
  {
   
    if (xSemaphoreTake(sdMutex, portMAX_DELAY))
    {
      String target = writeToA ? "/updateLogs/latestA.csv" : "/updateLogs/latestB.csv";
      String backup = writeToA ? "/updateLogs/latestB.csv" : "/updateLogs/latestA.csv";

      // Serial.println("mem task got mtuex");


      File f = SD.open(target, FILE_WRITE);
      if (f)
      {
        f.seek(0);
        f.println(rideData.odo);
        f.println(rideData.distance);
        f.println(rideData.time_r);
        f.println(rideData.avg_speed);
        f.println(rideData.max_speed);
        f.println(rideData.at_max_speed);
        
        f.println(rideData.ride);
        f.println(rideData.day);
        f.println(rideData.month);
        f.println(rideData.year);
        f.println(rideData.rpause);
        f.println(rideData.start_time);
        // f.println(rideData.elevation);
        f.flush();
        f.close();

        // verify file closed successfully
        File check = SD.open(target, FILE_READ);
        if (check && check.size() > 0) {
          // Serial.println("Write verified ✅");
          writeToA = !writeToA;  // toggle after a verified write
        } 
        
        else {
          Serial.println("Write failed ❌, keeping previous backup");
          rideData.location = "SD Write Error!!";
        }

        check.close();
      }


      else {
        Serial.println("SD open failed ❌");
      }

      xSemaphoreGive(sdMutex);

    }


    vTaskDelay(1000 / portTICK_PERIOD_MS);                                             //wait to see if two buttons are pressed
  }
}





void setup() {
  Serial.begin(9600);
  // I2C init (optional: set speed)
  int sd = SD.begin(SD_CS, SPI);

  if(sd)
  {
    esp_reset_reason_t reason = esp_reset_reason();


    if (!SD.exists("/crashLogs")) SD.mkdir("/crashLogs");

    File f = SD.open("/crashLogs/restart_log.txt", FILE_APPEND);
    if (f) {
      f.print("Restart reason code: ");
      f.println((int)reason);   // prints the numeric reset reason
      f.close();
      Serial.print("restart reason ");
      Serial.println((int)reason);
    }
  }


  WiFi.mode(WIFI_STA);

  // 2. Set WiFi power to minimum needed (max is 78 for full power, lower reduces current draw)
  // WiFi.setTxPower(WIFI_POWER_19_5dBm); // options: WIFI_POWER_19_5dBm, 14, 8, 0 dBm



  WiFi.begin("iPhone", "12345678");  // start connecting
  Serial.println("Connecting to WiFi...");

  // if(sd) stravaTask();


 




  Wire.begin(21, 22);
  Wire.setClock(50000);
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
  digitalWrite(33, 0);
  digitalWrite(13, 0);



  screen.screenMode = 1;
  // rideData.location = "location not available (init)";               // empty location string

  File f = SD.open("/updateLogs/latestA.csv");

  File f_temp = SD.open("/updateLogs/latestB.csv");


  // SD.remove("/latestlogs.csv");
  // SD.rmdir("/updatelogs");

  if (f && f.size() > 0) 
  {


    rideData.odo       = f.readStringUntil('\n').toDouble();
    rideData.distance  = f.readStringUntil('\n').toDouble();
    rideData.time_r    = f.readStringUntil('\n').toDouble();
    rideData.avg_speed = f.readStringUntil('\n').toDouble();
    rideData.max_speed = f.readStringUntil('\n').toDouble();
    rideData.at_max_speed = f.readStringUntil('\n').toDouble();

    rideData.ride      = f.readStringUntil('\n').toInt();
    rideData.day = f.readStringUntil('\n').toInt();
    rideData.month = f.readStringUntil('\n').toInt();
    rideData.year = f.readStringUntil('\n').toInt();
    rideData.rpause    = f.readStringUntil('\n').toInt();
    rideData.start_time = f.readStringUntil('\n'); rideData.start_time.trim();
    // rideData.elevation = f.readStringUntil('\n').toFloat();

    rideData.location = "Data Initialised!";




    f.close();

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


  else if(f_temp && f_temp.size() > 0)
  {


    rideData.odo       = f_temp.readStringUntil('\n').toDouble();
    rideData.distance  = f_temp.readStringUntil('\n').toDouble();
    rideData.time_r    = f_temp.readStringUntil('\n').toDouble();
    rideData.avg_speed = f_temp.readStringUntil('\n').toDouble();
    rideData.max_speed = f_temp.readStringUntil('\n').toDouble();
    rideData.at_max_speed = f_temp.readStringUntil('\n').toDouble();

    rideData.ride      = f_temp.readStringUntil('\n').toInt();
    rideData.day = f.readStringUntil('\n').toInt();
    rideData.month = f.readStringUntil('\n').toInt();
    rideData.year = f.readStringUntil('\n').toInt();
    rideData.rpause    = f_temp.readStringUntil('\n').toInt();
    rideData.start_time = f.readStringUntil('\n');  rideData.start_time.trim();
    // rideData.elevation = f.readStringUntil('\n').toFloat();



    rideData.location = "Data Initialised (temp)!";


    f_temp.close();

    xTaskCreatePinnedToCore(
    memTask,
    "MemTask",
    4096,
    nullptr,
    1,
    &memTaskHandle,
    0
  );

    // SD.remove("/latestlogstmp.csv");

  }
  


  else
  {
    rideData.location = "Data Initialization Failed";               // empty location string

    // SD.mkdir("/uppdatelogs");

    rideData.odo = 6230;                    // total odometer, 0 on first run
    rideData.distance = 0;               // distance for current ride, 0
    rideData.time_r = 0;                 // ride time, 0
    rideData.avg_speed = 0;              // average speed, 0
    rideData.max_speed = 0;              // max speed, 0
    rideData.at_max_speed = 41;


    // rideData.speed = 0;                  // current speed, 0
    // rideData.battery_percentage = 61;   // assume full battery on first run
    // rideData.indicator = 0;              // indicator off
    // rideData.location = "location not available (setup)";               // empty location string
    // rideData.hour = 69;
    // rideData.minute = 69;

    // rideData.hlMode = 0;                 // headlight mode
    // rideData.rlMode = 0;                 // rear light mode
    // rideData.rl = false;                 // rear light off
    // rideData.hlh = false;                // headlight high beam off
    // rideData.hll = false;                // headlight low beam off

    rideData.ride = 0;               // not riding yet
    rideData.day = 14;
    rideData.month = 10;
    rideData.year = 2025;

    rideData.rhour = 0;                  // ride hours
    rideData.rmin = 0;                   // ride minutes
    rideData.rpause = false;             // ride not paused
    rideData.start_time = "";
    // rideData.elevation = 141;
    
  }



  // delay(2000);




  if(rideData.ride) screen.screenMode = 2;


    // rideData.odo = 6000;                    // total odometer, 0 on first run
    // rideData.distance = 0;               // distance for current ride, 0
    // rideData.time_r = 0;                 // ride time, 0
    // rideData.avg_speed = 0;              // average speed, 0
    // rideData.max_speed = 0;              // max speed, 0

    rideData.speed = 0;                  // current speed, 0


    int raw = analogRead(35);             // Read ADC (0–4095)
    float vAdc = (raw / 4095.0) * 3.3 * 1.067;            // Convert to voltage at ADC pin (3.3V reference)
    rideData.battery_voltage = vAdc * (30000.0 + 7500.0) / 7500.0;           // Undo divider → actual battery voltage



    const float vEmpty = 9.0;
    const float vFull  = 12.5;
    if (rideData.battery_voltage <= vEmpty) rideData.battery_percentage = 0.0;
    else if (rideData.battery_voltage >= vFull)  rideData.battery_percentage = 100.0;
    else
    {
      float x = (rideData.battery_voltage - vEmpty) / (vFull - vEmpty);
      rideData.battery_percentage = (1 - pow(1 - x, 1.5)) * 100.0;
    }


    rideData.indicator = 0;              // indicator off
    // rideData.location = "location not available (init)";               // empty location string
    rideData.hour = 00;
    rideData.minute = 00;

    rideData.hlMode = 0;                 // headlight mode
    rideData.rlMode = 0;                 // rear light mode
    rideData.rl = false;                 // rear light of
    rideData.hlh = false;                // headlight high beam off
    rideData.hll = false;                // headlight low beam off

    // rideData.ride = false;               // not riding yet
    // rideData.rhour = 0;                  // ride hours
    // rideData.rmin = 0;                   // ride minutes
    // rideData.rpause = false;             // ride not paused

    rideData.lattitude = 0;
    rideData.longitude = 0;

    rideData.isGPS = 0;



  u8g2.begin();
  // u8g2.setDisplayRotation(U8G2_R2);


  Serial.print("ridedata start_time "); Serial.println(rideData.start_time);




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

  // xTaskCreatePinnedToCore(
  //   locationTask,
  //   "LocationTask",
  //   4096,
  //   nullptr,
  //   1,
  //   &locationTaskHandle,
  //   1
  // );


  xTaskCreatePinnedToCore(
    buttonsTask,
    "ButtonsTask",
    4096,
    nullptr,
    1,
    &buttonsTaskHandle,
    0
  );

  // xTaskCreatePinnedToCore(
  //   memTask,
  //   "MemTask",
  //   4096,
  //   nullptr,
  //   1,
  //   &memTaskHandle,
  //   0
  // );


  if(rideData.ride == 0)                                  //only turn on if not on ride, otherwise will send partial file;
  {
    xTaskCreatePinnedToCore(
      stravaTask,
      "StravaTask",
      16384,
      nullptr,
      1,
      &stravaTaskHandle,
      1
    );
  }
  else
  {
    WiFi.disconnect(true);  // turn off wifi if on ride
    WiFi.mode(WIFI_OFF);
    xTaskCreatePinnedToCore(
      locationTask,
      "LocationTask",
      4096,
      nullptr,
      1,
      &locationTaskHandle,
      1
    );

  }

}

void loop() {}
