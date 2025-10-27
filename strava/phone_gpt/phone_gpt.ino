// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <WiFiClientSecure.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <FS.h>
// #include <SD.h>

#include <SD.h>

const char* ssid = "iPhone";        // replace with your WiFi
const char* password = "12345678";

const char* client_id = "179574";
const char* client_secret = "cb9fbad5f26cc9638dc2c6dc31f883534a0ff0f3";
String refresh_token = "e74df5b338d5865c79162eb61c4b9b1a53198777";

String access_token = "68ff9464d4647020a366fdea79bdf155302ccb21";

// void getAccessToken() {
//   HTTPClient http;
//   String token_url = "https://www.strava.com/oauth/token?client_id=" + String(client_id) +
//                      "&client_secret=" + String(client_secret) +
//                      "&grant_type=refresh_token&refresh_token=" + refresh_token;

//   http.begin(token_url);
//   int code = http.POST("");
//   if (code != 200) {
//     Serial.print("Failed to get token: ");
//     Serial.println(code);
//     http.end();
//     return;
//   }

//   String resp = http.getString();
//   http.end();

//   int i = resp.indexOf("\"access_token\":\"");
//   if (i == -1) {
//     Serial.println("Access token not found!");
//     return;
//   }
//   i += 16;
//   int j = resp.indexOf("\"", i);
//   access_token = resp.substring(i, j);
//   Serial.print("Access token: ");
//   Serial.println(access_token);
}



const char* gpxPath = "/gpxRides/dummy.gpx";        // your file containing only <trkpt> tags

void setup() 
{
  Serial.begin(9600);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Connected.");

  if (!SD.begin()) {
    Serial.println("❌ SD card initialization failed!");
    return;
  }

    // getAccessToken();


  File gpxFile = SD.open(gpxPath);
  if (!gpxFile) {
    Serial.println("❌ Failed to open GPX file!");
    return;
  }

  // Prepare Strava API upload
   
    String boundary = "ESP32Boundary123";
    String contentType = "multipart/form-data; boundary=" + boundary;

    HTTPClient http;
    http.begin("https://www.strava.com/api/v3/uploads");
    http.addHeader("Authorization", "Bearer " + access_token);
    http.addHeader("Content-Type", contentType);

    // Calculate content length (optional, but some servers require it)
    size_t contentLength = 0;
    String header = "--" + boundary + "\r\n" +
                    "Content-Disposition: form-data; name=\"file\"; filename=\"" + String(gpxPath) + "\"\r\n" +
                    "Content-Type: application/gpx+xml\r\n\r\n";
    String dataTypePart = "\r\n--" + boundary + "\r\n" +
                          "Content-Disposition: form-data; name=\"data_type\"\r\n\r\n" +
                          "gpx\r\n";
    String footer = "--" + boundary + "--\r\n";
    contentLength = header.length() + gpxFile.size() + dataTypePart.length() + footer.length();

    // Start the POST request with chunked transfer encoding
    http.beginRequest();
    http.sendHeader("Content-Length", contentLength);

    // Send the multipart header
    http.write(header.c_str(), header.length());

    // Stream the file content in chunks
    const size_t bufferSize = 1024;
    uint8_t buffer[bufferSize];
    while (gpxFile.available()) {
        size_t bytesRead = gpxFile.read(buffer, bufferSize);
        http.write(buffer, bytesRead);
    }
    gpxFile.close();

    // Send the data_type part and closing boundary
    http.write(dataTypePart.c_str(), dataTypePart.length());
    http.write(footer.c_str(), footer.length());

    // End the request and get the response
    int httpResponseCode = http.endRequest();
    Serial.print("Upload response code: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode > 0) {
        Serial.println(http.getString());
    } else {
        Serial.println("Error on sending POST");
    }

    http.end();
}

void loop() {}