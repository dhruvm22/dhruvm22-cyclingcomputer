#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <FS.h>
#include <SD.h>

void uploadGpxToStrava(const char* gpxFilePath, const String& access_token) {
    File gpxFile = SD.open(gpxFilePath, "r");
    if (!gpxFile) {
        Serial.println("Failed to open GPX file");
        return;
    }

    String boundary = "ESP32Boundary123";
    String contentType = "multipart/form-data; boundary=" + boundary;

    // Multipart header
    String header;
    header.reserve(200); // Reserve memory to avoid fragmentation
    header = "--" + boundary + "\r\n";
    header += "Content-Disposition: form-data; name=\"file\"; filename=\"" +     String(gpxFilePath) + "\"\r\n";
    header += "Content-Type: application/gpx+xml\r\n\r\n";

    // GPX header
    String gpxHeader;
    gpxHeader.reserve(300);
    gpxHeader = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
    gpxHeader += "<gpx version=\"1.1\" creator=\"ESP32\" xmlns=\"http://www.topografix.com/GPX/1/1\" ";
    gpxHeader += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
    gpxHeader += "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\r\n";
    gpxHeader += "<trk>\r\n";
    gpxHeader += "<trkseg>\r\n";

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


    String activityTypePart;
    activityTypePart.reserve(100);
    activityTypePart = "\r\n--" + boundary + "\r\n";
    activityTypePart += "Content-Disposition: form-data; name=\"activity_type\"\r\n\r\n";
    activityTypePart += "Ride\r\n";

    // Multipart footer
    String footer = "--" + boundary + "--\r\n";

    size_t totalSize = header.length() + gpxHeader.length() + gpxFile.size() + gpxFooter.length() + dataTypePart.length() + activityTypePart.length() + footer.length();

    WiFiClientSecure client;
    client.setInsecure(); // For testing; use SSL certificate in production
    if (!client.connect("www.strava.com", 443)) {
        Serial.println("Failed to connect to Strava");
        gpxFile.close();
        return;
    }

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
        return;
    }

    // Stream multipart header
    if (client.print(header) == 0) {
        Serial.println("Failed to send multipart header");
        client.stop();
        gpxFile.close();
        return;
    }

    // Stream GPX header
    if (client.print(gpxHeader) == 0) {
        Serial.println("Failed to send GPX header");
        client.stop();
        gpxFile.close();
        return;
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
            return;
        }
    }
    gpxFile.close();

    // Stream GPX footer
    if (client.print(gpxFooter) == 0) {
        Serial.println("Failed to send GPX footer");
        client.stop();
        return;
    }

    // Stream data_type part
    if (client.print(dataTypePart) == 0) {
        Serial.println("Failed to send data_type part");
        client.stop();
        return;
    }

    if (client.print(activityTypePart) == 0) {
        Serial.println("Failed to send activity_type part");
        client.stop();
        return;
    }

    // Stream multipart footer
    if (client.print(footer) == 0) {
        Serial.println("Failed to send multipart footer");
        client.stop();
        return;
    }

    // Read response
    String response;
    response.reserve(512);
    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 10000) {
        if (client.available()) {
            response += client.readStringUntil('\n');
            if (response.indexOf("\r\n\r\n") >= 0) break; // End of headers
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
    if (httpResponseCode == 201) {
        int bodyStart = response.indexOf("\r\n\r\n") + 4;
        Serial.println(response.substring(bodyStart));
    } else {
        Serial.print("Error on sending POST: ");
        if (httpResponseCode == 0) Serial.println("No response or connection failed");
        else Serial.println(response);
    }

    client.stop();
}





const char* ssid = "iPhone";        // replace with your WiFi
const char* password = "12345678";

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  if (!SD.begin()) {
    Serial.println("SD card init failed!");
    return;
  }
    // Example usage
    const char* gpxFilePath = "/gpxRides/ride_2025-10-07_19_50_08Z.gpx";
    String access_token = "68ff9464d4647020a366fdea79bdf155302ccb21";
    uploadGpxToStrava(gpxFilePath, access_token);
}

void loop() {
    // Your loop code here
}