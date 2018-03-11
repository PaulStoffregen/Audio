// Advanced Microcontroller-based Audio Workshop
//
// http://www.pjrc.com/store/audio_tutorial_kit.html
// https://hackaday.io/project/8292-microcontroller-audio-workshop-had-supercon-2015
// 
// Part 3-3: Add a TFT Display

#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3


///////////////////////////////////
// copy the Design Tool code here
///////////////////////////////////





#define TFT_DC      20
#define TFT_CS      21
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI     7
#define TFT_SCLK    14
#define TFT_MISO    12
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);


// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// Use these with the Teensy 3.5 & 3.6 SD card
//#define SDCARD_CS_PIN    BUILTIN_SDCARD
//#define SDCARD_MOSI_PIN  11  // not actually used
//#define SDCARD_SCK_PIN   13  // not actually used

// Use these for the SD+Wiz820 or other adaptors
//#define SDCARD_CS_PIN    4
//#define SDCARD_MOSI_PIN  11
//#define SDCARD_SCK_PIN   13

void setup() {
  Serial.begin(9600);
  delay(500);
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setFont(Arial_24);
  //tft.setTextSize(3);
  tft.setCursor(40, 8);
  tft.println("Peak Meter");
  
  AudioMemory(10);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  delay(1000);
}

elapsedMillis msecs;

void loop() {
  if (playSdWav1.isPlaying() == false) {
    Serial.println("Start playing");
    //playSdWav1.play("SDTEST1.WAV");
    //playSdWav1.play("SDTEST2.WAV");
    playSdWav1.play("SDTEST3.WAV");
    //playSdWav1.play("SDTEST4.WAV");
    delay(10); // wait for library to parse WAV info
  }
  
  if (msecs > 15) {
    if (peak1.available() && peak2.available()) {
      msecs = 0;
      float leftNumber = peak1.read();
      float rightNumber = peak2.read();
      Serial.print(leftNumber);
      Serial.print(", ");
      Serial.print(rightNumber);
      Serial.println();

      // draw the verticle bars
      int height = leftNumber * 240;
      tft.fillRect(60, 280 - height, 40, height, ILI9341_GREEN);
      tft.fillRect(60, 280 - 240, 40, 240 - height, ILI9341_BLACK);
      height = rightNumber * 240;
      tft.fillRect(140, 280 - height, 40, height, ILI9341_GREEN);
      tft.fillRect(140, 280 - 240, 40, 240 - height, ILI9341_BLACK);
      // a smarter approach would redraw only the changed portion...

      // draw numbers underneath each bar
      tft.setFont(Arial_14);
      tft.fillRect(60, 284, 40, 16, ILI9341_BLACK);
      tft.setCursor(60, 284);
      tft.print(leftNumber);
      tft.fillRect(140, 284, 40, 16, ILI9341_BLACK);
      tft.setCursor(140, 284);
      tft.print(rightNumber);
    }
  }
}
