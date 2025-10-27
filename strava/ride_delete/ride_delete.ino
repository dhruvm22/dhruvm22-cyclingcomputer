#include <SD.h>

#define SD_CS 5  // your SD card chip select pin

void listFiles(const char *dirname) {
  File root = SD.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  Serial.printf("Listing files in %s:\n", dirname);
  File file = root.openNextFile();
  while (file) {
    Serial.println(file.name());
    file = root.openNextFile();
  }
  root.close();
}

void deleteAllFiles(const char *dirname) {
  File root = SD.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  Serial.printf("Deleting files in %s...\n", dirname);
  File file = root.openNextFile();
  while (file) {
    String filename = file.name();
    file.close();  // must close before deleting

    // prepend the directory path if not already included
    String fullpath = String(dirname) + "/" + filename;

    if (SD.remove(fullpath)) {
      Serial.printf("Deleted: %s\n", fullpath.c_str());
    } else {
      Serial.printf("Failed to delete: %s\n", fullpath.c_str());
    }

    file = root.openNextFile();
  }
  root.close();
}


void listGPXFiles() {
    if (!SD.begin()) {
        Serial.println("SD card initialization failed!");
        return;
    }

    File root = SD.open("/gpxRides");
    if (!root) {
        Serial.println("Failed to open /gpxRides directory!");
        return;
    }

    if (!root.isDirectory()) {
        Serial.println("/gpxRides is not a directory!");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            Serial.print("File: ");
            Serial.println(file.name());

            // Print file contents line by line
            while (file.available()) {
                String line = file.readStringUntil('\n');
                Serial.println(line);
            }
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
}

void setup() {
  Serial.begin(9600);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed!");
    while (1);
  }

  // if(SD.mkdir("/uppdatelogs")) Serial.println("made directory");


  // SD.rmdir("/uppdatelogs");
  // SD.mkdir("/updateLogs");
  delay(2000);
  // const char* folder = "/";
  Serial.println("SD init successful!");

  // listGPXFiles();

  // // SD.rmdir("/gpx_rides");

  // SD.remove("/gpxRides/dummy.gpx");
  // SD.remove("/gpxRides/ride_2025-10-07_19_50_08Z");

  // listGPXFiles();

  // listFiles("/gpxRides");
  deleteAllFiles("/gpxRides");
  // listFiles("/gpxRides");
}

void loop() {}
