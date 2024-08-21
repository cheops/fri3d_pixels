#include <Arduino.h>

#include <FastLED.h>

#include <data.h>

#define NUM_LEDS 300 // 60 * 5m
// #define NUM_LEDS 432 // 144 * 3m
// #define NUM_LEDS 480 // 96 * 5m

#define NEOPIXEL_PIN 13  // SAO pin
// #define NEOPIXEL_PIN 2   // SAO leds pin on Octopus 2022
// #define NEOPIXEL_PIN 12  // SAO leds pin on Fox 2024

#define RGB_ORDER RGB
#define FRAMES_PER_SECOND 120

// CRGB leds[NUM_LEDS];
CRGBArray<NUM_LEDS> leds;

enum class State {
    color_palette,
    charge_loading,
    fireworks
};

static State current_state = State::color_palette;
static unsigned long start_millis_charge_loading;

#define NUM_SPARKS NUM_LEDS / 2 // max number (could be NUM_LEDS / 2)
float sparkPos[NUM_SPARKS];
float sparkVel[NUM_SPARKS];
float sparkCol[NUM_SPARKS];
float flarePos;

static const float gravity = -.008; // m/s/s

static const uint8_t charge_times = 5;
static const uint8_t charge_leds = NUM_LEDS / charge_times;
static uint16_t current_charge = 0;
static TeamColor current_charge_teamColor = TeamColor::eNoTeam;
static CRGB current_charge_color = CRGB::Black;

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;




void handle_ir_packet(IrDataPacket packet);
CRGB map_team_to_color(TeamColor team);
void flare();
void explodeLoop();
void FillLEDsFromPaletteColors( uint8_t colorIndex);
void ChangePalettePeriodically();
void SetupTotallyRandomPalette();
void SetupBlackAndWhiteStripedPalette();
void SetupPurpleAndGreenPalette();
void loop_color_palette();
void loop_charge_loading();
void loop_fireworks();


void setup()
{
    Serial.begin(115200);
    delay(3000);

    Serial.println("starting.....");

    FastLED.addLeds<WS2812, NEOPIXEL_PIN, RGB_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.clear(true);

    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;    

    Data.init();
    Serial.println("Ready running...");
}

void loop()
{

    // light effect when receiving blaster shot
    handle_ir_packet(Data.readIr());

    switch (current_state)
    {
    case State::color_palette:
        loop_color_palette();
        break;
    
    case State::charge_loading:
        loop_charge_loading();
        break;
    
    case State::fireworks:
        loop_fireworks();
        break;
    }

    delay(100);
}

void loop_color_palette()
{
    ChangePalettePeriodically();

    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */
    
    FillLEDsFromPaletteColors( startIndex);
    
    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
}

void loop_charge_loading()
{
    EVERY_N_MILLIS(250)
    {
        leds[current_charge] = CRGB::Black;
        FastLED.show();
        current_charge -= 1;
        if (current_charge <= 0)
        {
            current_state = State::color_palette;
        }
    }
}

void loop_fireworks()
{
    // send up flare
    flare();

    // explode
    explodeLoop();
    current_state = State::color_palette;
}

void handle_ir_packet(IrDataPacket packet)
{
    // Serial.printf("handle_ir_packet packet: %u, p.raw: %" PRIu32 ", p.channel: %" PRIu8 ", p.team: %" PRIu8 ", p.action: %" PRIu8 "\n",
    // packet, packet.get_raw(), packet.get_channel(), packet.get_team(), packet.get_action());

    if (packet.get_raw() != 0 && packet.get_action() == eActionDamage)
    {
        CRGB color = map_team_to_color((TeamColor)packet.get_team());
        leds = color;
        while (leds[0].r != 0 || leds[0].g != 0 || leds[0].b != 0)
        {
            FastLED.show();
            delay(1000 / FRAMES_PER_SECOND);
            leds.fadeToBlackBy(7);
        }
        Data.readIr(); // clear buffer
        FastLED.clear(true);

        
        if (current_charge <= 0)
        {
            current_charge += charge_leds;
            current_charge_teamColor = (TeamColor) packet.get_team();
            current_charge_color = color;
        }
        else if (current_charge > 0 && current_charge_teamColor == packet.get_team())
        {
            current_charge += charge_leds;
        }
        else if (current_charge > 0 && current_charge_teamColor != packet.get_team())
        {
            current_charge -= charge_leds;
        }
        
        for (uint16_t i = 0; i < current_charge; i++)
        {
            leds[i] = current_charge_color;
        }
        FastLED.show();

        if (current_charge > 0)
        {
            current_state = State::charge_loading;
            start_millis_charge_loading = millis();
        }
        else
        {
            current_state = State::color_palette;
        }

        if (current_charge >= charge_times * charge_leds)
        {
            current_charge = 0;
            current_state = State::fireworks;
        }
    }
}

CRGB map_team_to_color(TeamColor team)
{
    CRGB color = CRGB::Black;
    switch (team)
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
    return color;
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
        // delay(0);
        FastLED.clear(true);

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
    FastLED.clear(true);
}

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;
    
    for( int i = 0; i < NUM_LEDS; ++i) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}


// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
        if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
        if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
        if( secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
        if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
        if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
        if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
        if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
    }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; ++i) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};
