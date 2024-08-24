#include "WiFi.h"
#include <SD_MMC.h>
#include <TJpg_Decoder.h>


const char* ssid = "....";
const char* password = ".....";
const char* server = "....";
int port = ....;

int x = 0;
int y = 0;

#define SD_DATA 40
#define SD_CLK 39
#define SD_CMD 38

const char* image = "/test.jpg";

WiFiClient client;

const size_t bufferSize = 32768; // 32KB buffer, adjust depending on memory available
char pixelflutBuffer[bufferSize];
size_t bufferIndex = 0;

//bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void initSDCard() {
  SD_MMC.setPins(SD_CLK, SD_CMD, SD_DATA);
  if (!SD_MMC.begin("/sdcard", true)) {  
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("SD card initialized via SDMMC.");
  listDir(SD_MMC, "/", 0);
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  for (int16_t i = 0; i < w * h; i++) {
    uint16_t color = bitmap[i];
    
    // Convert RGB565 to RGB888
    uint8_t r = (color >> 8) & 0xF8;
    uint8_t g = (color >> 3) & 0xFC;
    uint8_t b = (color << 3) & 0xF8;
    
    // Format the color as a hex string
    char colorHex[7];
    sprintf(colorHex, "%02X%02X%02X", r, g, b);

    // Send the Pixelflut command using println
    //client.print("PX ");
    //client.print(x + (i % w));  // X positionaha
    //client.print(" ");
    //client.print(y + (i / w));  // Y position
    //client.print(" ");
    //client.println(colorHex);   // Color
    int len = snprintf(pixelflutBuffer + bufferIndex, bufferSize - bufferIndex, "PX %d %d %s\n", x + (i % w), y + (i / w), colorHex);
    
    // Check if buffer has enough space
    if (bufferIndex + len >= bufferSize) {
      // Send the current buffer content
      client.write((const uint8_t*)pixelflutBuffer, bufferIndex);
      bufferIndex = 0;  // Reset buffer index
    }

    // Append the command to the buffer
    bufferIndex += len;
  }
  
  
  return true;
}

void setup() {
  Serial.begin(115200);
  initSDCard();
  initWiFi();
  
  if(!client.connect(server, port)){
      Serial.println("Connection to host failed");
      delay(1000);
      return;
  }
  Serial.println("Connected to pixelflut server successfully");

  
  
  File jpegFile = SD_MMC.open(image);
  TJpgDec.setJpgScale(1);  // No scaling, full size
  TJpgDec.setCallback(tft_output);
  
  uint32_t t = millis();

  //TJpgDec.drawSdJpg(0, 0, image);
  if (TJpgDec.drawSdJpg(400, 0,  jpegFile) != 0) {
    Serial.println("Failed to decode JPEG");
  }


  if (bufferIndex > 0) {
    client.write((const uint8_t*)pixelflutBuffer, bufferIndex);
  }
  //client.print(commands);
  t = millis() - t;
  Serial.print("Done! took ");
  Serial.print(t); 
  Serial.println(" ms to complete");
  client.stop();
}

void loop() {

}
