#include <Arduino.h>

#include <FastLED.h>

#include <data.h>

#define NEOPIXEL_PIN 13
#define NUM_LEDS 300
#define RGB_ORDER RGB
#define FRAMES_PER_SECOND 120

// CRGB leds[NUM_LEDS];
CRGBArray<NUM_LEDS> leds;

#define NUM_SPARKS NUM_LEDS / 2 // max number (could be NUM_LEDS / 2)
float sparkPos[NUM_SPARKS];
float sparkVel[NUM_SPARKS];
float sparkCol[NUM_SPARKS];
float flarePos;

float gravity = -.008; // m/s/s

void handle_ir_packet(IrDataPacket packet);
void flare();
void explodeLoop();


void setup()
{
  Serial.begin(115200);
  delay(3000);

  Serial.println("starting.....");

  FastLED.addLeds<WS2812, NEOPIXEL_PIN, RGB_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  leds.fill_rainbow(HUE_RED, 1);
  FastLED.show();

  delay(1000);
  FastLED.clear(true);

  Data.init();

}

void loop()
{

  // light effect when receiving blaster shot
  handle_ir_packet(Data.readIr());


  // // send up flare
  // flare();

  // // explode
  // explodeLoop();

  // Serial.printf("count: %" PRIu32 "\n", Data.getCount());
  // uint8_t state = digitalRead(11);
  // Serial.printf("state: %d\n", state);

  delay(100);

}


void handle_ir_packet(IrDataPacket packet)
{
    // Serial.printf("handle_ir_packet packet: %u, p.raw: %" PRIu32 ", p.channel: %" PRIu8 ", p.team: %" PRIu8 ", p.action: %" PRIu8 "\n", 
    // packet, packet.get_raw(), packet.get_channel(), packet.get_team(), packet.get_action());
    
    if (packet.get_raw() != 0 && packet.get_action() == eActionDamage)
    {
        CRGB color = CRGB::Black;

        switch (packet.get_team())
        {
        case eTeamRex:
            color = CRGB::Red;
            Serial.println("Team Rex.");
            break;

        case eTeamBuzz:
            color = CRGB::Blue;
            Serial.println("Team Buzz.");
            break;

        case eTeamGiggle:
            color = CRGB::Green;
            Serial.println("Team Giggle.");
            break;

        case eTeamYellow:
            color = CRGB::Yellow;
            Serial.println("Team Yellow.");
            break;

        case eTeamMagenta:
            color = CRGB::Magenta;
            Serial.println("Team Magenta.");
            break;

        case eTeamCyan:
            color = CRGB::Cyan;
            Serial.println("Team Cyan.");
            break;

        case eTeamWhite:
            color = CRGB::White;
            Serial.println("Team White.");
            break;

        default:
            color = CRGB::Black;
            break;
        }

        leds = color;
        while (leds[0].r != 0 || leds[0].g != 0 || leds[0].b != 0)
        {
            FastLED.show();
            delay(1000/FRAMES_PER_SECOND);
            leds.fadeToBlackBy(7);
        }
        Serial.printf("r: %u, g: %u, b: %u\n", leds[0].r, leds[0].g, leds[0].b);
        delay(100);
        Data.readIr(); // clear buffer
        FastLED.clear(true);
    }
}

/*
 * Send up a flare
 *
 */
void flare()
{

  flarePos = 0;
  float flareVel = float(random16(180, 195)) / 100; // trial and error to get reasonable range
  float brightness = 1;

  // initialize launch sparks
  for (int i = 0; i < 5; i++)
  {
    sparkPos[i] = 0;
    sparkVel[i] = (float(random8()) / 255) * (flareVel / 5); // random around 20% of flare velocity
    sparkCol[i] = sparkVel[i] * 1000;
    sparkCol[i] = constrain(sparkCol[i], 0, 255);
  }
  // launch
  // FastLED.clear();
  while (flareVel >= -.2)
  {
    // sparks
    for (int i = 0; i < 5; i++)
    {
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
    flarePos = constrain(flarePos, 0, NUM_LEDS - 1);
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

void explodeLoop()
{
  int nSparks = flarePos / 2; // works out to look about right

  // initialize sparks
  for (int i = 0; i < nSparks; i++)
  {
    sparkPos[i] = flarePos;
    sparkVel[i] = (float(random16(0, 20000)) / 10000.0) - 1.0; // from -1 to 1
    sparkCol[i] = abs(sparkVel[i]) * 500;                      // set colors before scaling velocity to keep them bright
    sparkCol[i] = constrain(sparkCol[i], 0, 255);
    sparkVel[i] *= flarePos / NUM_LEDS; // proportional to height
  }
  sparkCol[0] = 255; // this will be our known spark
  float dying_gravity = gravity;
  float c1 = 120;
  float c2 = 50;

  while (sparkCol[0] > c2 / 128)
  { // as long as our known spark is lit, work with all the sparks
    delay(0);
    FastLED.clear();

    for (int i = 0; i < nSparks; i++)
    {
      sparkPos[i] += sparkVel[i];
      sparkPos[i] = constrain(sparkPos[i], 0, NUM_LEDS - 1);
      sparkVel[i] += dying_gravity;
      sparkCol[i] *= .975;
      sparkCol[i] = constrain(sparkCol[i], 0, 255); // red cross dissolve

      if (sparkCol[i] > c1)
      { // fade white to yellow
        leds[int(sparkPos[i])] = CRGB(255, 255, (255 * (sparkCol[i] - c1)) / (255 - c1));
      }
      else if (sparkCol[i] < c2)
      { // fade from red to black
        leds[int(sparkPos[i])] = CRGB((255 * sparkCol[i]) / c2, 0, 0);
      }
      else
      { // fade from yellow to red
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