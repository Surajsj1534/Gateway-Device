#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <ESPmDNS.h>
#include <SoftwareSerial.h>

const char *ssid = "Rdx";
const char *password = "Hotspot!!!";

AsyncWebServer server(80);
File uploadFile;
static const size_t bufferSize = 1024;
static uint8_t buffer[bufferSize];
SoftwareSerial mySerial(16, 17); // RX on pin 16, TX on pin 17
#define FILE_REQUEST_CODE 'R'
#define FILE_CHUNK_SIZE 1024

void setup()
{
  Serial.begin(115200);
  mySerial.begin(9600);

  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("fortress")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  if (!SPIFFS.begin()) {

    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // Serve HTML pages
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    servePage(request, "/web/login.html");
  });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    servePage(request, "/web/index.html");
  });

  server.on("/download.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    servePage(request, "/web/download.html");
  });

  server.on("/upload.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    servePage(request, "/web/upload.html");
  });

  server.on("/request.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    servePage(request, "/web/request.html");
  });

  server.on("/delete.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    servePage(request, "/web/delete.html");
  });

  server.on("/directory.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    servePage(request, "/web/directory.html");
  });

  server.on("/login.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    servePage(request, "/web/login.html");
  });

  // Handler for the /fetchFiles endpoint that is used in directory page
  server.on("/fetchFiles", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    // Open the specific folder (files)
    File folder = SPIFFS.open("/files");
    printSPIFFSDirectory();
    if (!folder) {
      request->send(500, "text/plain", "Failed to open the folder");
      return;
    }

    // Create a JSON array to store file information
    StaticJsonDocument<4096> serverResponse; // Use StaticJsonDocument instead of DynamicJsonDocument
    JsonArray fileArray = serverResponse.createNestedArray("files");

    // Read each file in the folder
    File file = folder.openNextFile();
    while (file) {
      // Encode the file name to prevent JSON parsing issues
      String encodedName = file.name();
      encodedName.replace("\"", "\\\"");

      // Create a JSON object for each file
      JsonObject fileObject = fileArray.createNestedObject();
      fileObject["name"] = encodedName;
      fileObject["size"] = file.size();

      file = folder.openNextFile();
    }

    // Send the JSON response
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(serverResponse, *response);
    request->send(response);
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    downloadFileOnWeb(request);

  });
  server.on("/file", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    // Get the filename from the query parameter
    String filename = request->arg("filename");

    // Serve the file content locally if it exists; otherwise, forward the request to the slave ESP32
    serveFileOrForwardToSlave(request, filename);
  });

  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String filename = request->arg("filename");
    Serial.println(filename.length());
    File dataFile = SPIFFS.open("/files/" + filename, "r"); // Now read data from SPIFFS Card
    if (dataFile)
    {
      if (SPIFFS.remove("/files/" + filename))
      {

        request->send(200, "text/plain", "File Deleted: " + filename);
      }
      else
      {

        request->send(404, "text/plain", "File Not Found: " + filename);
      }
    }
    else
      Serial.println(F("file not present 22"));
  });

  // Handle API requests to upload files that is used in upload page
  server.on("/upload.html", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SPIFFS, "/upload.html");
  });

  server.on(
    "/fupload", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    request->send(200);
  },
  handleUpload);

  server.begin();
  // Print the URL of the web server
  Serial.print("HTTP server started at http://");
  Serial.println(WiFi.localIP());
}

void servePage(AsyncWebServerRequest *request, String path)
{
  // Open the file
  File file = SPIFFS.open(path, "r");
  // Check if the file exists
  if (file)
  {
    // Send the file as the response
    request->send(200, "text/html", file.readString());
    // Close the file
    file.close();
  }
  else
  {
    // Send a 404 response
    request->send(404, "text/plain", "File not found");
  }
}

void downloadFileOnWeb(AsyncWebServerRequest *request)
{
  // Get the file path from the URL query string
  String filePath = "/files/" + request->arg("filepath");

  // Open the file for reading
  File file = SPIFFS.open(filePath, "r");

  // If the file exists
  if (file)
  {
    // Send the file as a response
    request->send(SPIFFS, filePath, "application/octet-stream");
    // Close the file
    file.close();
  }
  // If the file doesn't exist
  else
  {
    // Send a 404 response
    request->send(404, "text/plain", "File Not Found: " + filePath);
  }
}

// Define the custom function to handle file uploads
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  File f;
  if (!index)
  {
    Serial.println("Received file:");
    Serial.println(filename);
    f = SPIFFS.open("/files/" + filename, FILE_WRITE); // Save the file to the "files" folder
  }

  if (!f)
  {
    f = SPIFFS.open("/files/" + filename, FILE_APPEND); // Append to the file in the "files" folder
  }

  if (!f)
  {
    Serial.println("Failed to create file");
    return;
  }

  for (size_t i = 0; i < len; i++)
  {
    f.write(data[i]);
  }

  if (final)
  {
    int bytes = f.size();

    String fsize = "";
    if (bytes < 1024)
      fsize = String(bytes) + " B";
    else if (bytes < (1024 * 1024))
      fsize = String(bytes / 1024.0, 3) + " KB";
    else if (bytes < (1024 * 1024 * 1024))
      fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
    else
      fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";

    Serial.println((String)filename + " uploaded of size " + fsize);

    request->send(200, "text/plain", "File uploaded successfully");

    request->redirect("/directory.html");
  }
  f.close();
}

// Function to print the size of the saved file
void printFileSize(String filename)
{
  // Open the file in SPIFFS for reading with the requested filename
  File file = SPIFFS.open("/files/" + filename, "r");

  if (!file)
  {
    Serial.println("Error: Unable to open file for reading");
    return;
  }

  // Get the size of the file
  size_t fileSize = file.size();

  // Print the size of the file
  Serial.println("Size of the saved file (" + filename + "): " + fileSize + " bytes");

  // Close the file
  file.close();
}

bool saveDataToFile(String response, String filename)
{
  // Open the file in SPIFFS for writing with the requested filename
  File file = SPIFFS.open("/files/" + filename, "w");

  if (!file)
  {
    Serial.println("Error: Unable to open file for writing");
    return false;
  }

  // Skip the "STR" command
  while (mySerial.available() == 0)
  {
    delay(100);
  }

  // Read and write chunks of data until an empty line is received
  while (true)
  {
    char chunk[FILE_CHUNK_SIZE];
    int bytesRead = mySerial.readBytesUntil('\n', chunk, FILE_CHUNK_SIZE);

    // Check for the end of the file marker (empty line)
    if (bytesRead == 0)
    {
      break;
    }

    // Write the chunk to the file
    file.write(reinterpret_cast<const uint8_t *>(chunk), bytesRead);
  }

  file.close();
  return true;
}

void requestAndSendFile(AsyncWebServerRequest *request, String filename)
{
  // Send file request to the slave ESP32
  String requestCommand = "REQ " + filename;
  Serial.println("REQ ");
  Serial.println(filename.length());
  mySerial.println(requestCommand);
  Serial.println("Sent request command to slave ESP32: " + requestCommand);

  // Wait for response from the slave ESP32
  while (mySerial.available() == 0)
  {
    delay(100);
  }

  // Read the response from the slave ESP32
  String response = mySerial.readStringUntil('\n');
  Serial.println("Response from Slave: " + response);

  // Check if the response starts with "STR" indicating the start of file transfer
  if (response.startsWith("STR"))
  {
    // Send acknowledgment to the slave
    mySerial.println("OK");
    Serial.println("File transfer started");

    // Extract the filename from the response
    String receivedFilename = filename;

    // we can use trim function as well

    // Open the file for writing
    File file = SPIFFS.open("/files/" + receivedFilename, FILE_WRITE);

    if (!file)
    {
      Serial.println("Failed to create file");
      request->send(500, "text/plain", "Failed to create file on server");
      return;
    }

    // Start sending the HTTP response with the file content
    AsyncResponseStream *responseStream = request->beginResponseStream("application/octet-stream");

    // Read and write chunks of file data until an empty line is received
    while (true)
    {
      char chunk[FILE_CHUNK_SIZE];
      int bytesRead = mySerial.readBytes(chunk, FILE_CHUNK_SIZE);

      // Check for the end of the file marker (empty line)
      if (bytesRead == 0)
      {
        break;
      }

      // Write the chunk to the file
      file.write(reinterpret_cast<const uint8_t *>(chunk), bytesRead);

      // Write the chunk to the HTTP response stream
      responseStream->write(reinterpret_cast<const uint8_t *>(chunk), bytesRead);
    }

    // Close the file
    file.close();

    // Send the HTTP response
    request->send(responseStream);
    Serial.println("File content sent to client");
  }
  else
  {
    // Send a 404 response if file request is rejected
    request->send(404, "text/plain", "File request rejected");
    Serial.println("File request rejected");
  }
}

void serveFile(AsyncWebServerRequest *request, const String &filename)
{
  String filePath = "/files/" + filename;
  // Open the file
  File file = SPIFFS.open(filePath, "r");
  if (!file)
  {
    request->send(500, "text/plain", "Error: Failed to open file");
    Serial.println("Failed to open file: " + filePath);
    return;
  }

  // Read the file content
  String content = file.readString();
  file.close();

  // Check if the content is empty
  if (content.isEmpty())
  {
    request->send(500, "text/plain", "Error: File content is empty");
    Serial.println("File content is empty: " + filePath);
    return;
  }

  // Send the file content to the client
  request->send(200, "text/plain", content);
  Serial.println("Sent file content: " + filePath);
}

void serveFileOrForwardToSlave(AsyncWebServerRequest *request, const String &filename)
{
  String filePath = "/files/" + filename;
  // Check if the file exists locally
  if (SPIFFS.exists(filePath))
  {
    // Open the file
    File file = SPIFFS.open(filePath, "r");
    if (!file)
    {
      request->send(500, "text/plain", "Error: Failed to open file");
      return;
    }

    // Set the content type header
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, filePath);


    // Send the file as response
    request->send(response);
  }
  else
  {
    // Forward the request to the slave ESP32
    requestAndSendFile(request, filename);
  }
}

void printSPIFFSDirectory() {
  Serial.println("Attempting to print SPIFFS Directory...");

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  File root = SPIFFS.open("/files");
  if (!root || !root.isDirectory()) {
    Serial.println("/files directory not found");
    return;
  }

  Serial.println("Files in SPIFFS Directory:");
  File file = root.openNextFile();
  while (file) {
    Serial.print("  FILE: ");
    Serial.println("/files/" + String(file.name()));
    file = root.openNextFile();
  }
}


void loop()
{
  // Nothing to do here
}
