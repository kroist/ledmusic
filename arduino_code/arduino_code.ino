#include <Adafruit_NeoPixel.h>

#define PIN 6
#define NUM_LEDS 90

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_RBG + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.setBrightness(255);

  
  strip.clear();
  strip.show();
  
}

void setColor(int i) {
  if (i < NUM_LEDS / 3) {
    strip.setPixelColor(i, 0xff0000);
  }
  else 
  if (i < NUM_LEDS * 2 / 3) {
    strip.setPixelColor(i, 0x00ff00);
  }
  else {
    strip.setPixelColor(i, 0x0000ff);
  }
  strip.show();
}

int BRIGHTNESS = 1;

int pls = 1;

int br_bass = 100, br_mid = 100, br_treble = 100;

void loop() {

  /*while((Serial.available() < 3)) {
    strip.clear();
    Serial.println("KEK");
    Serial.println(Serial.available());
    Serial.print(br_bass);
    Serial.print(" ");
    Serial.print(br_mid);
    Serial.print(" ");
    Serial.println(br_treble);
    for (int i = 0; i < NUM_LEDS/3; i++)
      strip.setPixelColor(i, 0, 0, br_bass);
    for (int i = NUM_LEDS/3; i < NUM_LEDS/3 * 2; i++)
      strip.setPixelColor(i, 0, br_mid, 0);
    for (int i = NUM_LEDS/3 * 2; i < NUM_LEDS; i++)
      strip.setPixelColor(i, br_treble, 0, 0);
    strip.show();
  }*/
  //br_bass = Serial.read()-'0';
  //br_mid = Serial.read()-'0';
  //br_treble = Serial.read()-'0';
  if (Serial.available() == 3){
    byte k = Serial.read();
    byte kk = Serial.read();
    byte kkk = Serial.read();
    //byte kk = k;
    //byte kkk = kkk;
    int num = k;
    int numm = kk;
    int nummm = kkk;
    for (int i = 0; i < NUM_LEDS/3; i++) {
      strip.setPixelColor(i, 0, 0, num);
    }
    for (int i = NUM_LEDS/3; i < NUM_LEDS/3*2; i++) {
      strip.setPixelColor(i, 0, numm, 0);
    }
    for (int i = NUM_LEDS/3*2; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, nummm, 0, 0);
    }
    strip.show();
    delay(5);

  }
  //else {
    
  //}
  byte a[3];

}
