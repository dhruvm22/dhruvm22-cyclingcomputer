#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

TinyGPSPlus gps;

// Use UART2 (pins 16 = RX, 17 = TX)
HardwareSerial SerialGPS(2);

void setup() {
  Serial.begin(115200);              
  SerialGPS.begin(9600, SERIAL_8N1, 16, 17); 
  Serial.println("GPS Test Started");
}

void loop() {
  while (SerialGPS.available() > 0) {
    gps.encode(SerialGPS.read());
    
    if (gps.location.isUpdated()) {
      Serial.print("Lat: ");  Serial.println(gps.location.lat(), 6);
      Serial.print("Lng: ");  Serial.println(gps.location.lng(), 6);
    }

    if (gps.satellites.isUpdated()) {
      Serial.print("Satellites: "); Serial.println(gps.satellites.value());
    }

    if (gps.date.isUpdated() && gps.time.isUpdated()) {
      Serial.print("Date: ");
      Serial.print(gps.date.day()); Serial.print("/");
      Serial.print(gps.date.month()); Serial.print("/");
      Serial.println(gps.date.year());

      Serial.print("Time: ");
      Serial.print(gps.time.hour()); Serial.print(":");
      Serial.print(gps.time.minute()); Serial.print(":");
      Serial.println(gps.time.second());
    }

    // REMOVE this → Serial.println("---");
  }
}
