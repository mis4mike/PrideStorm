#include <FastLED.h>
// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

/***********************
* SOUND SETUP
***********************/

// define the pins used
//#define CLK 13       // SPI Clock, shared with SD card
//#define MISO 12      // Input data, from VS1053/SD card
//#define MOSI 11      // Output data, to VS1053/SD card
// Connect CLK, MISO and MOSI to hardware SPI pins. 
// See http://arduino.cc/en/Reference/SPI "Connections"

// These are the pins used for the breakout example
#define BREAKOUT_RESET  9      // VS1053 reset pin (output)
#define BREAKOUT_CS     10     // VS1053 chip select pin (output)
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output)
// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = 
  // create breakout-example object!
  //Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, CARDCS);
  // create shield-example object!
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

/*****************
* LIGHTS SETUP
*****************/

#define COLOR_ORDER GRB
//BGR
#define CHIPSET WS2811
//APA102

#define BRIGHTNESS 200
#define FRAMES_PER_SECOND 60

#define NUM_CLOUDS 2
#define CLOUDS_PIN 12
#define BOLTS_PIN 11
#define LEDS_PER_CLOUD 12 //Clouds and bolts must have the same number of LEDs for this implementation
#define LEDS_PER_BOLT 12
#define LEDS_PER_STRIP LEDS_PER_CLOUD * NUM_CLOUDS

//For fire animations
#define NUM_FLAME_PALETTES 5
#define COOLING  55
#define SPARKING 120

//CRGB clouds[NUM_CLOUDS * LEDS_PER_CLOUD];
//CRGB bolts[NUM_CLOUDS * LEDS_PER_BOLT];

struct CRGB leds[2][LEDS_PER_STRIP];

// Here are the button pins:
#define PUBLIC_BUTTON_PIN 18 //A4 //A4 / 18
#define PRIVATE_BUTTON_PIN 19 //A5 //A5 / 19

int mode = 1;
bool buttonDown = false;

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

int stormCountdown = 5;
int trickCountdown = 0;
int currentStorm = 0;
int currentTrick = 0;
int inputStatus = 0;
bool trickButtonExhausted = false;
bool stormInProgress = false;
bool trickInProgress = false;
bool animationBeginning = false;
int stormClock;

CRGBPalette16 flamePalettes[NUM_FLAME_PALETTES]; 
CRGB flashColors[NUM_FLAME_PALETTES];
int numBoltSounds[5] = {3, 1, 3, 2, 1};

uint32_t rainbowColors[6] = { CRGB::Red, CRGB::OrangeRed, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Purple };

CRGBPalette16 currentRainbowPalette;
TBlendType    currentRainbowBlending;

int currentFlamePalette = 0;

void setup() {
  
  Serial.begin(9600);
  Serial.write("setup\n");

  //SOUND SETUP

  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));
  
   if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }

  // list files
  printDirectory(SD.open("/"), 0);
  
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(20,20);

  // Timer interrupts are not suggested, better to use DREQ interrupt!
  //musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int

  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  
  // Play one file, don't return until complete
  //musicPlayer.playFullFile("startup.mp3");

  
  //LED SETUP

  delay(1000); // sanity delay
  LEDS.addLeds<WS2811, 12, COLOR_ORDER>(leds[0], LEDS_PER_STRIP);

  LEDS.addLeds<WS2811, 11, COLOR_ORDER>(leds[1], LEDS_PER_STRIP);

  FastLED.setBrightness( BRIGHTNESS );

  loadPalettes();

  // This first palette is the basic 'black body radiation' colors,
  // which run from black to red to bright yellow to white.

  //Set up button for pressing!
  pinMode(PUBLIC_BUTTON_PIN, INPUT);
  pinMode(PRIVATE_BUTTON_PIN, INPUT);
}

void loadPalettes(){
  // These are other ways to set up the color palette for the 'fire'.
  // First, a gradient from black to red to yellow to white -- similar to HeatColors_p
  flamePalettes[0] = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White);
  flashColors[0] = CRGB::Red;
  // Second, this palette is like the heat colors, but blue/aqua instead of red/yellow
  flamePalettes[1] = CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);
  flashColors[1] = CRGB::White;
  // Third, here's a simpler, three-step gradient, from black to red to white
  flamePalettes[2] = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::White);
  flashColors[2] = CRGB::White; 
  //Green
  flamePalettes[3] = CRGBPalette16( CRGB::DarkGreen, CRGB::Green, CRGB::White);
  flashColors[3] = CRGB::Green;  
  flamePalettes[4] = CRGBPalette16( CRGB::Purple, CRGB::Amethyst, CRGB::White);
  flashColors[4] = CRGB::Amethyst;  
  
}

void debugLoop() {
  Serial.println(stormCountdown);
  Serial.println(stormClock);
  Serial.println(stormInProgress);
}

void loop() {
 
  //debugLoop();
  
  if(stormInProgress || trickInProgress) {
    random16_add_entropy( random());
    animate(); 
  } else {
    
    stormCountdown--;
    trickCountdown--;
    inputStatus = checkForInput();
  
    if(inputStatus == 1 && !trickButtonExhausted) { // Trick button pressed
      freeTrick();
    } else if(inputStatus == 2) {
      demoStorm();
    }
  
    if(stormCountdown <= 0) {
      nextStorm();
    }
  
    if(trickCountdown <= 0) {
      //nextTrick();
    }
  
    tick();
  }  

}

int checkForInput() {
 //check for a button press 
 if(digitalRead(PUBLIC_BUTTON_PIN)) {
  return 1; 
 }
 if(digitalRead(PRIVATE_BUTTON_PIN)) {
  return 2; 
 }

}

void freeTrick() {
    trickCountdown = 0;
    trickButtonExhausted = true;
    stormCountdown += 90; 
    //Turn off button light
}

void demoStorm() {
   stormCountdown = 0; 
}

void nextTrick() {
    trickCountdown = random(30, 60);
    stormCountdown += 10;
    currentTrick++;
 /*   if(currentTrick >= tricks.length) {
     currentTrick = 0; 
    } */
    trickButtonExhausted = false; 
    trickInProgress = true;
    animationBeginning = true;
    //turn on button light
}

void nextStorm() {
   stormCountdown = 5; //random(300,600); //only storm every 5-10 minutes
   trickCountdown = 30;
   currentStorm++;
   stormInProgress = true;
   animationBeginning = true;
   stormClock = 600; // random(1200,1800); //Storm for 20-30 seconds (1200 - 1800 frames)

   //TODO: Dim clouds and make ominous rumbling in a blocking function.
}
void animate() {
  
  if(stormInProgress) {
    if(animationBeginning) {
      stormIntro();
      animationBeginning = false;
    }    
    
    stormClock--;
    if(stormClock == 0) {
     stormInProgress = false; 
    }
    switch(currentStorm) {
     case 1: prideStorm();
             break;
     case 2: fireStorm();
             break;
     case 3: iceStorm();
             break;  
     case 4: greenStorm();
             break;  
     case 5: purpleStorm();
             break; 
     default: currentStorm = 0;
              stormClock = 0;
              stormInProgress = false;
              //TODO: We looped. Do something special here?
    }
  }
  
  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND); 
}

void stormIntro() {
  switch(currentStorm) {
     case 1: prideStormIntro();
             break;
     case 2: fireStormIntro();
             break;
     case 3: iceStormIntro();
             break;  
  } 
}

void tick() {
  //Update button color or animate it or something
  trailLighting();
  FastLED.show();
  delay(1000);
  Serial.println("tick");
}

/*************************
 * Below here is noodling that will need to get moved to the library
 *************************/
 
 //TRAIL LIGHTING MODE
 
void trailLighting() {
 for(int i = 0; i < NUM_CLOUDS; i++) {
  setBoltColor(i, CRGB::Black);
  setCloudColor(i, CRGB::AntiqueWhite);
 }
}
 
//STORM DEFINITIONS
void prideStorm() {
  static int currentColor = 0;

  currentRainbowPalette = RainbowColors_p;
  currentRainbowBlending = LINEARBLEND;
  
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */
  fillCloudsFromPaletteColors( startIndex);
  if(randomBolt(1, rainbowColors[currentColor]) == 1) {
      Serial.println(rainbowColors[currentColor]);
    currentColor++;
    if(currentColor >= sizeof(rainbowColors) / sizeof(rainbowColors[0])) {
      currentColor = 0;
    }
  }
}
void prideStormIntro() {
  //TODO: Play Enchant_target_slow.mp3
  Serial.println("Pridestorm Intro");
  for(int i = 0; i < NUM_CLOUDS; i++) {
    Serial.println(i);
    Serial.println(CRGB::Red);
    Serial.println(rainbowColors[i % NUM_CLOUDS]);
    setBoltColor(i, CRGB::Black);
    setCloudColor(i, rainbowColors[i % NUM_CLOUDS]);
    //delay(1000);
  }
  //delay(3000);
}

void fireStorm() {
  currentFlamePalette = 0;
  flameClouds(); 
  randomBolt(2, flashColors[currentFlamePalette]); 
  //Use the Doom/Lina crackling flames and something firey for the bolts
}
 
void fireStormIntro() {
  
}

void iceStorm() {
  currentFlamePalette = 1;
  flameClouds(); 
  randomBolt(2, flashColors[currentFlamePalette]); 
  //Use the CM sound at the beginning
}

void iceStormIntro() {
  
}

void greenStorm() {
  currentFlamePalette = 3;
  flameClouds(); 
  randomBolt(2, flashColors[currentFlamePalette]); 
}

void purpleStorm() {
  currentFlamePalette = 3;
  flameClouds(); 
  randomBolt(2, flashColors[currentFlamePalette]); 
}
 //TRICK DEFINITIONS
 
 //UTILITY FUNCTIONS
 
int randomBolt(int boltsPerSecond, CRGB color) {
  static bool isAnimating;
  static int frame;
  static int bolt;
  String sound;
  char fileName[12];
  
  if(isAnimating) {
    switch(frame) {
     case 0: bolt = random(0,NUM_CLOUDS);
             setBoltColor(bolt, color);
             sound = "bolt" + String(currentStorm) + String("-") + String(random(0, numBoltSounds[currentStorm - 1]) + 1) + String(".mp3");
             sound.toCharArray(fileName, 12);
             Serial.println(fileName);
             musicPlayer.stopPlaying();
             musicPlayer.startPlayingFile(fileName);
             break;
     case 5:   setBoltColor(bolt, CRGB::Black);
               break;
     case 10:  setBoltColor(bolt, color);
               break;
     case 40:  setBoltColor(bolt, CRGB::Black);
               frame = 0;
               isAnimating = false;
               bolt = random(0,NUM_CLOUDS);
    }
    frame++;

    return 2;
  } else {
    int randomNum = random(0,FRAMES_PER_SECOND * boltsPerSecond);
    if(randomNum == 0) {
      frame = 0;
      isAnimating = true;
      return 1;
    } else {
      for(int i = 0; i < NUM_CLOUDS; i++){
        setBoltColor(bolt, CRGB::Black);
      }
      return 0;
    }
  }
}

void setBoltColor(int bolt, CRGB color) {
  int boltEnd = LEDS_PER_BOLT * (bolt + 1);
  for(int i = bolt * LEDS_PER_BOLT; i < boltEnd; i++) {
    leds[1][i] = color;
  }
}

void setCloudColor(int cloud, CRGB color) {
  int cloudEnd = LEDS_PER_CLOUD * (cloud + 1);
  for(int i = cloud * LEDS_PER_CLOUD; i < cloudEnd; i++) {
    leds[0][i] = color;
  }
}

void flameClouds()
{
  // Array of temperature readings at each simulation cell
  static byte heat[LEDS_PER_STRIP];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < LEDS_PER_STRIP; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / LEDS_PER_STRIP) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= LEDS_PER_STRIP - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < LEDS_PER_STRIP; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);
      leds[0][j] = ColorFromPalette( flamePalettes[currentFlamePalette], colorindex);
    }
 
}

void fillCloudsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;
    
    for( int i = 0; i < LEDS_PER_STRIP; i++) {
        leds[0][i] = ColorFromPalette( currentRainbowPalette, colorIndex, brightness, currentRainbowBlending);
        colorIndex += 3;
    }
}

/// File listing helper
void printDirectory(File dir, int numTabs) {
   while(true) {
     
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       //printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
   }
}
