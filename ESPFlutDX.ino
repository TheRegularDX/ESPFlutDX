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

const size_t bufferSize = 32768; 
char pixelflutBuffer[bufferSize];
size_t bufferIndex = 0;

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
  
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  for (int16_t i = 0; i < w * h; i++) {
    uint16_t color = bitmap[i];
    
    uint8_t r = (color >> 8) & 0xF8;
    uint8_t g = (color >> 3) & 0xFC;
    uint8_t b = (color << 3) & 0xF8;
    
    char colorHex[7];
    sprintf(colorHex, "%02X%02X%02X", r, g, b);

    int len = snprintf(pixelflutBuffer + bufferIndex, bufferSize - bufferIndex, "PX %d %d %s\n", x + (i % w), y + (i / w), colorHex);
    
    if (bufferIndex + len >= bufferSize) {
      client.write((const uint8_t*)pixelflutBuffer, bufferIndex);
      bufferIndex = 0; 
    }
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
  TJpgDec.setJpgScale(1);  
  TJpgDec.setCallback(tft_output);
  
  uint32_t t = millis();

  if (TJpgDec.drawSdJpg(0, 0,  jpegFile) != 0) {
    Serial.println("Failed to decode JPEG");
  }

  if (bufferIndex > 0) {
    client.write((const uint8_t*)pixelflutBuffer, bufferIndex);
  }
  
  t = millis() - t;
  Serial.print("Done! took ");
  Serial.print(t); 
  Serial.println(" ms to complete");
  client.stop();
}

void loop() {

}
