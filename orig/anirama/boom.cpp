#include <FastLED.h>
#define NUM_LEDS 120
#define DATA_PIN 6
CRGB leds[NUM_LEDS]; // sets up block of memory

#define NUM_SPARKS 61 // max number (could be NUM_LEDS / 2);
float sparkPos[NUM_SPARKS];
float sparkVel[NUM_SPARKS];
float sparkCol[NUM_SPARKS];
float flarePos;

float gravity = -.004; // m/s/s

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
}

/*
 * Main Loop
 */
void loop() {  
  // send up flare
  flare();
  
  // explode
  explodeLoop();

  // wait before sending up another
  delay(random16(1000, 4000));
}


/*
 * Send up a flare
 * 
 */
void flare() {
  
  flarePos = 0;
  float flareVel = float(random16(50, 90)) / 100; // trial and error to get reasonable range
  float brightness = 1;

  // initialize launch sparks
  for (int i = 0; i < 5; i++) { 
    sparkPos[i] = 0;
    sparkVel[i] = (float(random8()) / 255) * (flareVel / 5);
    // random around 20% of flare velocity
    sparkCol[i] = sparkVel[i] * 1000;
    sparkCol[i] = constrain(sparkCol[i], 0, 255);
  } 
  // launch 
  FastLED.clear();
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
    FastLED.clear();
    flarePos += flareVel;
    flareVel += gravity;
    brightness *= .985;
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
    FastLED.clear();
    for (int i = 0; i < nSparks; i++) { 
      sparkPos[i] += sparkVel[i]; 
      sparkPos[i] = constrain(sparkPos[i], 0, NUM_LEDS); 
      sparkVel[i] += dying_gravity; 
      sparkCol[i] *= .99; 
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
    dying_gravity *= .995; // as sparks burn out they fall slower
    FastLED.show();
  }
  FastLED.clear();
  FastLED.show();
}