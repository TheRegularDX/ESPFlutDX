#include "WiFi.h"
#include <SD_MMC.h>
#include <TJpg_Decoder.h>


const char* ssid = "....";
const char* password = "....";
const char* server = "....";
int port = ....;


#define SD_DATA 40
#define SD_CLK 39
#define SD_CMD 38


const char* image = "/test.jpg";


WiFiClient client;


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
    
    uint8_t r = (color >> 8) & 0xF8;
    uint8_t g = (color >> 3) & 0xFC;
    uint8_t b = (color << 3) & 0xF8;
    
    char colorHex[7];
    sprintf(colorHex, "%02X%02X%02X", r, g, b);

    client.print("PX ");
    client.print(x + (i % w));  
    client.print(" ");
    client.print(y + (i / w));  
    client.print(" ");
    client.println(colorHex);   
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

  if (TJpgDec.drawSdJpg(400, 0,  jpegFile) != 0) {
    Serial.println("Failed to decode JPEG");
  }

  t = millis() - t;
  Serial.print("Done! took ");
  Serial.print(t); 
  Serial.println(" ms to complete");
  client.stop();
}

void loop() {

}
