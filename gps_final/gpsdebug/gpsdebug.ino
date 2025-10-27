#include <TinyGPS++.h>
#include <HardwareSerial.h>

HardwareSerial GPS(2); // Use Serial2
TinyGPSPlus gps;

void setup() {
  Serial.begin(115200);                 // Serial monitor
  GPS.begin(9600, SERIAL_8N1, 16, 17); // RX2=16, TX2=17
  Serial.println("GPS Parser Started");
}

void loop() {
  // Feed GPS data into TinyGPS++
  while (GPS.available()) {
    char c = GPS.read();
    gps.encode(c);
  }

  // If a valid location is available
  if (gps.location.isUpdated()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);  // 6 decimal places
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
    
    if (gps.time.isValid()) {
      Serial.print("Time (UTC): ");
      if (gps.time.hour() < 10) Serial.print('0');
      Serial.print(gps.time.hour());
      Serial.print(':');
      if (gps.time.minute() < 10) Serial.print('0');
      Serial.print(gps.time.minute());
      Serial.print(':');
      if (gps.time.second() < 10) Serial.print('0');
      Serial.println(gps.time.second());
    } else {
      Serial.println("Time: Not valid yet");
    }

    Serial.println("---------------------");
  }
}
