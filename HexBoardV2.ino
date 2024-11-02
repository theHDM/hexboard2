#include "src/includes.h"

#include "src/config.h"
#include "src/io.h"
#include "src/synth.h"

#include <Wire.h>               // this is necessary to deal with the pins and wires
//#include <LittleFS.h>
//#include <GEM_u8g2.h>           // library of code to create menu objects on the B&W display



//U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);
//File settingsFile;
byte_vec hex;

void setup() {
  #if (defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040))
  TinyUSB_Device_Init(0);  // Manual begin() is required on core without built-in support for TinyUSB such as mbed rp2040
  #endif
  //Wire.setSDA(OLED_sdaPin);
  //Wire.setSCL(OLED_sclPin);
  //u8g2.begin();                       // Menu and graphics setup
  //u8g2.setBusClock(1000000);          // Speed up display
  //u8g2.setContrast(63);   // Set contrast
  //u8g2.clearBuffer();
  //u8g2.setFont(u8g2_font_4x6_tf);
  // put your setup code here, to run once:
  //LittleFSConfig cfg;       // Configure file system defaults
  //cfg.setAutoFormat(true);  // Formats file system if it cannot be mounted.
  //LittleFS.setConfig(cfg);
  //LittleFS.begin();         // Mounts file system.
  //if (!LittleFS.begin()) {
  //  u8g2.drawStr(20,20,"FS not mounted");
  //} else {
  //  u8g2.drawStr(20,20,"FS mounted");
  //}
  //settingsFile = LittleFS.open("settings.dat","r+");
  //if (!settingsFile) {
  //  settingsFile = LittleFS.open("settings.dat","w+");
  //  u8g2.drawStr(20,30,"W+'d");      
  //} else {
  //  u8g2.drawStr(20,30,"R+'d");      
  //}
  //u8g2.sendBuffer();
  //settingsFile.printf("testing 123");
  //settingsFile.close
  synth.setup(sample_rate_in_Hz);

  hex.resize(colPins.size() << muxPins.size());
}

uint8_t c;
void loop() {
  if (pinGrid.readTo(hex)) {
    Serial.print("K");
    if (hex[0] = btn_press) {
      c = synth.tryNoteOn(default_hybrid_sound,440.0,96,0);
    }
    // interpret grid info
  }
  int8_t x = rotary.getTurnFromBuffer();
  uint64_t y = rotary.getClickFromBuffer();
  if (x) {
    synth.tryNoteOff(c);
  }

  uint v = synth.count_voices_playing();
  uint f = audioOut.free_bytes();
  Serial.print("v");
  Serial.println(v);
  Serial.print("f");
  Serial.println(f);

  if (f && v) {
    for (uint i = 0; i < f; i++) {
      audioOut.buffer(synth.writeNextSample());
    }
  }

}

void setup1() {
  pinGrid.setup(muxPins, colPins, mapGridToPixel); 
  rotary.setup(rotaryPinA, rotaryPinB, rotaryPinC);
  audioOut.setup(pwmPins, adcPins);
	start_processing_background_IO_tasks();
}

void loop1() {
}
