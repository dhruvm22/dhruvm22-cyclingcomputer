#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <SD.h>

const char* ssid = "iPhone";        // replace with your WiFi
const char* password = "12345678";

const char* client_id = "179574";
const char* client_secret = "cb9fbad5f26cc9638dc2c6dc31f883534a0ff0f3";
String refresh_token = "e74df5b338d5865c79162eb61c4b9b1a53198777";

String access_token = "";

void getAccessToken() {
  HTTPClient http;
  String token_url = "https://www.strava.com/oauth/token?client_id=" + String(client_id) +
                     "&client_secret=" + String(client_secret) +
                     "&grant_type=refresh_token&refresh_token=" + refresh_token;

  http.begin(token_url);
  int code = http.POST("");
  if (code != 200) {
    Serial.print("Failed to get token: ");
    Serial.println(code);
    http.end();
    return;
  }

  String resp = http.getString();
  http.end();

  int i = resp.indexOf("\"access_token\":\"");
  if (i == -1) {
    Serial.println("Access token not found!");
    return;
  }
  i += 16;
  int j = resp.indexOf("\"", i);
  access_token = resp.substring(i, j);
  Serial.print("Access token: ");
  Serial.println(access_token);
}

void createDummyGPX() {
  if (!SD.exists("/gpxRrides")) {
    SD.mkdir("/gpxRides");
  }

  File gpxFile = SD.open("/gpxRides/dummy.gpx", FILE_WRITE);
  if (!gpxFile) {
    Serial.println("Failed to create GPX file");
    return;
  }

    String gpxData = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    gpxData += "<gpx version=\"1.1\" creator=\"ESP32\" xmlns=\"http://www.topografix.com/GPX/1/1\">\n";
    gpxData += "<trk><name>ESP32 Test Ride</name><trkseg>\n";
    gpxData += "<trkpt lat=\"26.5123\" lon=\"80.2329\"><time>2025-10-08T07:00:00Z</time></trkpt>\n";
    gpxData += "<trkpt lat=\"26.5130\" lon=\"80.2335\"><time>2025-10-08T07:01:00Z</time></trkpt>\n";
    gpxData += "<trkpt lat=\"26.5140\" lon=\"80.2340\"><time>2025-10-08T07:02:00Z</time></trkpt>\n";
    gpxData += "</trkseg></trk>\n</gpx>\n";

  // Add a few points around IIT Kanpur
  gpxFile.print(gpxData) ;
  gpxFile.close();
  Serial.println("Dummy GPX file created!");
}


void uploadGPX(const char* gpxFilePath) {
    if (!SD.exists(gpxFilePath)) {
        Serial.println("GPX file does not exist!");
        return;
    }

    File gpxFile = SD.open(gpxFilePath);
    if (!gpxFile) {
        Serial.println("Failed to open GPX file!");
        return;
    }

    String gpxData = "";
    while (gpxFile.available()) {
        gpxData += gpxFile.readStringUntil('\n') + "\n";
    }
    gpxFile.close();

    String boundary = "ESP32Boundary123";

    // Build multipart body
    String body = "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"" + String(gpxFilePath) + "\"\r\n";
    body += "Content-Type: application/gpx+xml\r\n\r\n";
    body += gpxData + "\r\n";

    // Add data_type field (required by Strava)
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"data_type\"\r\n\r\n";
    body += "gpx\r\n";

    // Closing boundary
    body += "--" + boundary + "--\r\n";

    HTTPClient http;
    http.begin("https://www.strava.com/api/v3/uploads");
    http.addHeader("Authorization", "Bearer " + access_token);
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    int httpResponseCode = http.POST(body);

    Serial.print("Upload response code: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());

    http.end();
}

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

  // 1. Retrieve access token
  getAccessToken();

  if (access_token.length() == 0) {
    Serial.println("No access token, cannot upload");
    return;
  }

  // 2. Create dummy GPX file
  createDummyGPX();

  // 3. Upload GPX line-by-line
  // uploadGPX("/gpxRides/dummy.gpx");
}

void loop() {
  // Nothing
}
