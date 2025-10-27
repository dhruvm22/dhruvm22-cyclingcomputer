// #include <Wire.h>
// #include <U8g2lib.h>

// // SH1106 128x64 I2C
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// int blinkCounter = 0; // for blinking indicator

// void setup() {
//   Serial.begin(9600);

//   u8g2.begin();
// }

// int ind()
// {
//   if (Serial.available()) {
//     String s = Serial.readStringUntil('\n'); // read line until Enter
//     s.trim(); // remove spaces/newlines
//     if (s == "1") return 1;
//     if (s == "-1") return -1;
//     if (s == "0") return 0;
//   }
//   return 2; // no new input
// }

// int prev = 0;
// void loop() 
// {
//  int indicator = ind();

//   if (indicator == 2) {
//     indicator = prev;         // keep last value if no new input
//   } else if (indicator == prev) {
//     indicator = 0;            // toggle off if same input comes again
//   }
//   prev = indicator;


//   casualRide(123.4, 45.6, 90.0, 78.0, indicator);

//   delay(200); // slower refresh for clearer blink
// }

// void casualRide(double distance, double speed, double heading, double battery_percentage, int indicator) {
//   char buf[20];
//   u8g2.clearBuffer();

//   // === Heading (Top Left) ===
//   u8g2.setFont(u8g2_font_5x8_tr);
//   sprintf(buf, "HDG:%03d", (int)heading);
//   u8g2.drawStr(0, 10, buf);

//   // === Battery Box (Top Right) ===
//   int batt_x = 110, batt_y = 0, batt_w = 18, batt_h = 8;
//   u8g2.drawFrame(batt_x, batt_y, batt_w, batt_h);      
//   u8g2.drawBox(batt_x + 2, batt_y + 2, (int)((batt_w - 4) * battery_percentage / 100.0), batt_h - 4);  
//   sprintf(buf, "%d%%", (int)battery_percentage);
//   u8g2.setFont(u8g2_font_5x8_tr);
//   u8g2.drawStr(batt_x - 20, batt_y + 6, buf);

//   // === Speed (Big, Center) ===
//   u8g2.setFont(u8g2_font_logisoso32_tr);
//   sprintf(buf, "%d", (int)speed);
//   u8g2.drawStr(30, 48, buf);

//   // === Odometer (Bottom Left) ===
//   u8g2.setFont(u8g2_font_6x12_tr);
//   sprintf(buf, "ODO:%d", (int)distance);
//   u8g2.drawStr(0, 62, buf);

//   // === Blinking Indicator Arrow ===
//   blinkCounter++;
//   if ((blinkCounter / 5) % 2 == 0) {   // blink toggle
//     if (indicator == 1) 
//     {
//       u8g2.drawTriangle(110, 50, 120, 55, 110, 60);
//     } 
//     else if (indicator == -1) 
//     { 
//       u8g2.drawTriangle(120, 50, 110, 55, 120, 60);
//     }

//   }

//   u8g2.sendBuffer();
// }

#include <Arduino.h>
#include <U8g2lib.h>

// Example: for 128x64 OLED using I2C (change if you have SPI or different controller)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Variables to display
String location = "Kanpur";
float speed = 88.0;         // km/h
float distance = 100.0;     // km
int battery = 37;           // %
bool leftIndicator = false;
bool rightIndicator = false;

void setup() {
  u8g2.begin();
}

void loop() {
  // Example toggling indicators
  static unsigned long lastBlink = 0;
  static bool blinkState = false;
  if (millis() - lastBlink > 500) {
    blinkState = !blinkState;
    lastBlink = millis();
  }

  // Draw screen
  u8g2.firstPage();
  do {
    drawDashboard(blinkState);
  } while (u8g2.nextPage());

  delay(50);
}

void drawDashboard(bool blinkState) {


  u8g2.drawHLine(0, 10, 112);
  u8g2.drawHLine(0, 54, 112);
  u8g2.drawVLine(112, 0, 64);



  u8g2.setFont(u8g2_font_6x12_tf);

  u8g2.drawStr(2, 7, location.c_str());

  char timeStr[9];  
  sprintf(timeStr, "%02d:%02d", 10, 15);  
  u8g2.drawStr(110-u8g2.getStrWidth(timeStr), 8, timeStr);


  // Center: Speed
  u8g2.setFont(u8g2_font_fur25_tf);  // Big font
  char spd[10];
  sprintf(spd, "%.1f", speed);
  u8g2.drawStr(1, 45, spd);

  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(5 + u8g2.getStrWidth(spd)*3, 40, "KM/Hr");

  // Bottom left: Distance
  char dist[20];
  sprintf(dist, "ODO %.1f KM", distance);
  u8g2.drawStr(2, 64, dist);

  // Top right: Battery box
  u8g2.drawFrame(115, 1, 12, 8);  // battery outline
  u8g2.drawBox(127, 3, 1, 4);     // battery tip
  int bw = map(battery, 0, 100, 0, 14);
  u8g2.drawBox(115, 1, bw, 8);

  // char battStr[10];
  // sprintf(battStr, "%d%%", battery);
  // u8g2.drawStr(90, 10, battStr);

  // Turn indicators
  if (1) {
    if (1) {
      u8g2.drawTriangle(120, 20, 120, 35, 115, 28);  // left arrow
    }
    if (rightIndicator) {
      u8g2.drawTriangle(95, 20, 95, 35, 110, 28); // right arrow
    }
  }

  // Gear/settings icon (just a small cog wheel shape)
  u8g2.drawCircle(122, 56, 5, U8G2_DRAW_ALL);  // gear outline
  u8g2.drawCircle(122, 56, 2, U8G2_DRAW_ALL);  // center
}

