#include <SoftwareSerial.h>
#include <SPIFFS.h>

SoftwareSerial mySerial(16, 17); // RX on pin 16, TX on pin 17
#define FILE_REQUEST_CODE 'R'
#define FILE_CHUNK_SIZE 1024

void setup()
{
  Serial.begin(115200);
  mySerial.begin(9600);

  if (!SPIFFS.begin(true))
  {
    Serial.println("Error: Failed to mount SPIFFS");
    while (1)
    {
      delay(1000);
    }
  }
}

void loop()
{
  // Wait for a request from the master
  while (!mySerial.available())
  {
    delay(100);
  }

  // Read the request from the master
  String requestCommand = mySerial.readStringUntil('\n');
  Serial.println("Request from Master: " + requestCommand);

  String filename = requestCommand.substring(4);

  if (requestCommand.startsWith("REQ "))
  {
    Serial.println("File request received for: " + filename);
    mySerial.println("STR" + filename);

  // Wait for acknowledgment from the master
    while (!mySerial.available()) 
    {
      delay(100);
    }

    // Read the acknowledgment from the master
    String ackCommand = mySerial.readStringUntil('\n');
    if (ackCommand.startsWith("OK")) {
      Serial.println("Acknowledgment received from master. Sending file.");
      sendFile(filename);
    } else {
      Serial.println("Unexpected acknowledgment received from master. Aborting.");
    }
  }
}

void sendFile(String filename)
{
  filename.trim();
  Serial.println("Sending file: " + filename);
  File file = SPIFFS.open("/" + filename, "r");

  if (!file)
  {
    Serial.println("Error opening file for reading");
    return;
  }  

  // Send the file content in chunks
  while (file.available())
  {
    char chunk[FILE_CHUNK_SIZE];
    int bytesRead = file.readBytes(chunk, FILE_CHUNK_SIZE);
    mySerial.write(chunk, bytesRead);
    delay(5);
  }
  
  mySerial.println();
  file.close();
  Serial.println("File sent");
}
