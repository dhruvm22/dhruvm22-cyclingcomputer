// #include <SPI.h>
// #include <SD.h>

// #define CS_PIN 10  // Chip Select for SD card

// File dataFile;

// // Query location (microdegrees, i.e. lat*1e6, lon*1e6)
long targetLat = 26510460;  
long targetLon = 80231260;  
#include <SPI.h>
#include <SD.h>
#include <math.h>

#define SD_CS 10   // Chip Select pin for SD card

// --- Target coordinates (microdegrees -> divide by 1e6 to get decimal) ---
// long targetLat = 25070000; // example
// long targetLon = 81090000; // example

// --- Haversine Distance Function (meters) ---
double haversine(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0; // Earth radius in meters
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);
  lat1 = radians(lat1);
  lat2 = radians(lat2);

  double a = sin(dLat/2)*sin(dLat/2) + cos(lat1)*cos(lat2)*sin(dLon/2)*sin(dLon/2);
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

void setup() {
  Serial.begin(9600);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed!");
    return;
  }

  File file = SD.open("p7.csv");
  if (!file) {
    Serial.println("File open failed!");
    return;
  }

  double minDist = 1e15;
  String minName = "";

  // skip header
  file.readStringUntil('\n');

  int i = 0;
  while (file.available()) {

    if(i++ % 1000 == 0) Serial.println(i);
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    // Parse CSV line
    int firstComma = line.indexOf(',');
    int secondComma = line.indexOf(',', firstComma+1);

    if (firstComma < 0 || secondComma < 0) continue;

    long lat = line.substring(0, firstComma).toInt();
    long lon = line.substring(firstComma+1, secondComma).toInt();
    String name = line.substring(secondComma+1);

    // Convert to decimal degrees
    double latD = lat / 1e6;
    double lonD = lon / 1e6;
    double tgtLatD = targetLat / 1e6;
    double tgtLonD = targetLon / 1e6;

    // Compute distance
    double d = haversine(tgtLatD, tgtLonD, latD, lonD);

    if (d < minDist) {
      minDist = d;
      minName = name;
    }
  }

  file.close();

  // Print result
  Serial.print("Nearest: ");
  Serial.print(minName);
  Serial.print(" | Distance: ");
  Serial.print(minDist, 1);
  Serial.println(" m");
}

void loop() {
  // nothing
}
