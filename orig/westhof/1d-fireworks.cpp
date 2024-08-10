/*  1D Fireworks by Daniel Westhof https://www.Daniel-Westhof.de
 *  Based on original code by Carl Rosendahl: https://www.anirama.com/1000leds/1d-fireworks/
 *  
 *  What I changed:
 *  Added an LED to mime a fuse.
 *  Added a button to "light" the fuse.
 *  
 *  The code is configured to run on a D1-Mini Board and a WS2812B LED-strip with 300 LEDs.
 *  If you want to use more or less LEDs, you might want to to change the gravity value as well.
 */
 
#include <FastLED.h>
 
#define NUM_LEDS 300
#define DATA_PIN 5
#define BUTTON_PIN 13
#define LED_PIN 15
#define NUM_SPARKS NUM_LEDS/2 // max number (could be NUM_LEDS / 2)
 
CRGB leds[NUM_LEDS]; // sets up block of memory
 
float sparkPos[NUM_SPARKS];
float sparkVel[NUM_SPARKS];
float sparkCol[NUM_SPARKS];
float flarePos;
 
float gravity = -.008; // m/s/s
 
void setup() {
  Serial.begin(115200);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
}
 
void loop() {  
 
 
  digitalWrite(LED_PIN, HIGH);
  Serial.println("Wait");
  while (digitalRead(BUTTON_PIN) == HIGH) {
  delay(100);
  }
 
  for (int i = 30; i > 0; i--) {
    digitalWrite(LED_PIN, LOW);
    delay(50);
    digitalWrite(LED_PIN, HIGH);
    delay(50);
    Serial.println("zzzzzz");
  }
 
  Serial.println("BOOM");
  digitalWrite(LED_PIN, LOW);
  
  // send up flare
  flare();
 
  // explode
  explodeLoop();
}
 
 
/*
 * Send up a flare
 *
 */
void flare() {
 
  flarePos = 0;
  float flareVel = float(random16(180, 195)) / 100; // trial and error to get reasonable range
  float brightness = 1;
 
  // initialize launch sparks
  for (int i = 0; i < 5; i++) {
    sparkPos[i] = 0; sparkVel[i] = (float(random8()) / 255) * (flareVel / 5); // random around 20% of flare velocity
  sparkCol[i] = sparkVel[i] * 1000; sparkCol[i] = constrain(sparkCol[i], 0, 255);
  }
  // launch
// FastLED.clear();
  while (flareVel >= -.2) {
    // sparks
    for (int i = 0; i < 5; i++) {
      sparkPos[i] += sparkVel[i];
      sparkPos[i] = constrain(sparkPos[i], 0, 120);
      sparkVel[i] += gravity;
      sparkCol[i] += -.8;
      sparkCol[i] = constrain(sparkCol[i], 0, 255);
      leds[int(sparkPos[i])] = HeatColor(sparkCol[i]);
      leds[int(sparkPos[i])] %= 50; // reduce brightness to 50/255
    }
   
    // flare
    leds[int(flarePos)] = CHSV(0, 0, int(brightness * 255));
    FastLED.show();
    delay(5);
    FastLED.clear();
    flarePos += flareVel;
    flarePos = constrain(flarePos, 0, NUM_LEDS-1);
    flareVel += gravity;
    brightness *= .98;
  }
}
 
/*
 * Explode!
 *
 * Explosion happens where the flare ended.
 * Size is proportional to the height.
 */
 
 
void explodeLoop() {
  int nSparks = flarePos / 2; // works out to look about right
   
  // initialize sparks
  for (int i = 0; i < nSparks; i++) {
    sparkPos[i] = flarePos; sparkVel[i] = (float(random16(0, 20000)) / 10000.0) - 1.0; // from -1 to 1
    sparkCol[i] = abs(sparkVel[i]) * 500; // set colors before scaling velocity to keep them bright
    sparkCol[i] = constrain(sparkCol[i], 0, 255);
    sparkVel[i] *= flarePos / NUM_LEDS; // proportional to height
  }
  sparkCol[0] = 255; // this will be our known spark
  float dying_gravity = gravity;
  float c1 = 120;
  float c2 = 50;
  
  
  while(sparkCol[0] > c2/128) { // as long as our known spark is lit, work with all the sparks
    delay(0);
    FastLED.clear();
    
    for (int i = 0; i < nSparks; i++) {
      sparkPos[i] += sparkVel[i];
      sparkPos[i] = constrain(sparkPos[i], 0, NUM_LEDS-1);
      sparkVel[i] += dying_gravity;
      sparkCol[i] *= .975;
      sparkCol[i] = constrain(sparkCol[i], 0, 255); // red cross dissolve
      
      if(sparkCol[i] > c1) { // fade white to yellow
        leds[int(sparkPos[i])] = CRGB(255, 255, (255 * (sparkCol[i] - c1)) / (255 - c1));
      }
      else if (sparkCol[i] < c2) { // fade from red to black
        leds[int(sparkPos[i])] = CRGB((255 * sparkCol[i]) / c2, 0, 0);
      }
      else { // fade from yellow to red
        leds[int(sparkPos[i])] = CRGB(255, (255 * (sparkCol[i] - c2)) / (c1 - c2), 0);
      }
 
      
    }
    dying_gravity *= .99; // as sparks burn out they fall slower
    FastLED.show();
    
  }
 
  delay(5);
  FastLED.clear();
  FastLED.show();
}