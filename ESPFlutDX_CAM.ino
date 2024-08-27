#include "esp_camera.h"          
#include "soc/soc.h"          
#include "soc/rtc_cntl_reg.h"  
#include "driver/rtc_io.h"    
#include "WiFi.h"
#include "FS.h"                
#include "SD_MMC.h" 
#include <TJpg_Decoder.h>


#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5

#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13


#define SD_DATA 40
#define SD_CLK 39
#define SD_CMD 38


camera_config_t config;

const char* ssid = "....";
const char* password = "....";
const char* server = "....";
int port = ....;

const size_t bufferSize = 65536;
char pixelflutBuffer[bufferSize]; 
size_t bufferIndex = 0;

WiFiClient client;

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
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());

  if(!client.connect(server, port)){
      Serial.println("Connection to host failed");
      delay(1000);
      return;
  }
  Serial.println("Connected to pixelflut server successfully");

  SD_MMC.setPins(SD_CLK, SD_CMD, SD_DATA);

  if (!SD_MMC.begin("/sdcard", true)) {  
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("SD card initialized via SDMMC.");
  
  uint8_t cardType = SD_MMC.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }
  
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_96X96;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera initialized");
  
  camera_fb_t * fb = NULL;
  // Take Picture with Camera
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.print("Image captured");

  fs::FS &fs = SD_MMC;

  File file = fs.open("/capture.jpg", FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file in writing mode");
  } 
  else {
    file.write(fb->buf, fb->len); 
    Serial.println("Image saved to SD card");
  }
  file.close();
  esp_camera_fb_return(fb);

  delay(4000);
  
  File jpegFile = SD_MMC.open("/capture.jpg");

  TJpgDec.setJpgScale(1);  
  TJpgDec.setCallback(tft_output);
  

  if (TJpgDec.drawSdJpg(0, 0,  jpegFile) != 0) {
    Serial.println("Failed to decode JPEG");
  }

  if (bufferIndex > 0) {
    client.write((const uint8_t*)pixelflutBuffer, bufferIndex);
  }

  client.stop();
  Serial.println("done?");
}

void loop() {
  
}