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

/* SPEAKER CONTROL RELAYS */
#define SPEAKER_CONTROL_PIN_1 53
#define SPEAKER_CONTROL_PIN_2 51
#define SPEAKER_CONTROL_PIN_3 49
#define SPEAKER_CONTROL_PIN_4 47
#define SPEAKER_CONTROL_PIN_5 35
#define SPEAKER_CONTROL_PIN_6 33
#define SPEAKER_CONTROL_PIN_7 31
#define SPEAKER_CONTROL_PIN_8 29

#define CLOUDS_PER_SPEAKER 2

/*****************
* BUTTONS SETUP
*****************/

#define PRIVATE_BUTTON_PIN 36

#define PUBLIC_BUTTON_PIN 34
#define PUBLIC_BUTTON_POWER_PIN 32
#define PUBLIC_BUTTON_LIGHT_PIN 22

//For RGB Button
#define PUBLIC_BUTTON_RED_PIN 44
#define PUBLIC_BUTTON_GREEN_PIN 45
#define PUBLIC_BUTTON_BLUE_PIN 46


/*****************
* LIGHTS SETUP
*****************/

#define COLOR_ORDER GRB
//BGR
#define CHIPSET WS2811
//APA102

#define BRIGHTNESS 200
#define FRAMES_PER_SECOND 60

#define NUM_CLOUDS 6
#define LEDS_PER_CLOUD 12
#define LEDS_PER_BOLT 9
#define CLOUDS_PIN 37
#define BOLTS_PIN 39
#define NUM_CLOUD_LEDS LEDS_PER_CLOUD * NUM_CLOUDS
#define NUM_BOLT_LEDS LEDS_PER_BOLT * NUM_CLOUDS

#define NUM_MINI_CLOUDS 8
#define LEDS_PER_MINI_CLOUD 3
#define MINI_CLOUDS_PIN 42
#define NUM_MINI_CLOUD_LEDS NUM_MINI_CLOUDS * LEDS_PER_MINI_CLOUD

//predefined colors
#define TRAIL_LIGHTING_COLOR CRGB::DimGray

//For fire animations
#define NUM_FLAME_PALETTES 5
#define COOLING  55
#define SPARKING 120

struct CRGB cloudLeds[NUM_CLOUD_LEDS];
struct CRGB boltLeds[NUM_BOLT_LEDS];
struct CRGB miniCloudLeds[NUM_MINI_CLOUD_LEDS];

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

int stormCountdown = 300;
int trickCountdown = 180;
int currentStorm = 5;
int currentTrick = 0;
int currentTrickFrame = 0;
int inputStatus = 0;
int trickButtonExhausted = 0;
bool stormInProgress = false;
bool trickInProgress = false;
bool animationBeginning = false;
int stormClock;

CRGBPalette16 flamePalettes[NUM_FLAME_PALETTES]; 
CRGB flashColors[NUM_FLAME_PALETTES];
int numBoltSounds[9] = {3, 3, 3, 3, 2, 2, 3, 3, 2};

#define NUM_RAINBOW_COLORS 6
uint32_t rainbowColors[NUM_RAINBOW_COLORS] = { CRGB::Red, CRGB::OrangeRed, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Purple };

CRGBPalette16 currentRainbowPalette;
TBlendType    currentRainbowBlending;

int currentFlamePalette = 0;

void setup() {
  
  Serial.begin(9600);
  Serial.write("setup\n");

  //SOUND SETUP
  pinMode(SPEAKER_CONTROL_PIN_1, OUTPUT);
  pinMode(SPEAKER_CONTROL_PIN_2, OUTPUT);
  pinMode(SPEAKER_CONTROL_PIN_3, OUTPUT);
  pinMode(SPEAKER_CONTROL_PIN_4, OUTPUT);
  pinMode(SPEAKER_CONTROL_PIN_5, OUTPUT);
  pinMode(SPEAKER_CONTROL_PIN_6, OUTPUT);
  pinMode(SPEAKER_CONTROL_PIN_7, OUTPUT);
  pinMode(SPEAKER_CONTROL_PIN_8, OUTPUT);

  allSpeakersOff();
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
  //printDirectory(SD.open("/"), 0);
  
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(10,10);

  // Timer interrupts are not suggested, better to use DREQ interrupt!
  //musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int

  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  
  // Play one file, don't return until complete
  allSpeakersOn();
  musicPlayer.playFullFile("startup.mp3");

  //BUTTON SETUP
  pinMode(PUBLIC_BUTTON_PIN, INPUT);
  pinMode(PRIVATE_BUTTON_PIN, INPUT);
  pinMode(PUBLIC_BUTTON_RED_PIN, OUTPUT);
  pinMode(PUBLIC_BUTTON_GREEN_PIN, OUTPUT);
  pinMode(PUBLIC_BUTTON_BLUE_PIN, OUTPUT);
  pinMode(PUBLIC_BUTTON_POWER_PIN, OUTPUT);
  pinMode(PUBLIC_BUTTON_LIGHT_PIN, OUTPUT);
  digitalWrite(PUBLIC_BUTTON_POWER_PIN, HIGH);
  digitalWrite(PUBLIC_BUTTON_LIGHT_PIN, LOW);

  
  //LED SETUP

  delay(1000); // sanity delay
  LEDS.addLeds<WS2811, CLOUDS_PIN, COLOR_ORDER>(cloudLeds, NUM_CLOUD_LEDS);
  LEDS.addLeds<WS2811, BOLTS_PIN, COLOR_ORDER>(boltLeds, NUM_BOLT_LEDS);
  LEDS.addLeds<WS2811, MINI_CLOUDS_PIN, COLOR_ORDER>(miniCloudLeds, NUM_MINI_CLOUD_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  loadPalettes();

  startupDebug();
  ledTestStrip();

}

void startupDebug() {
 Serial.print("NUM_CLOUDS: "); 
 Serial.println(NUM_CLOUDS);
 Serial.print("NUM_CLOUD_LEDS: "); 
 Serial.println(NUM_CLOUD_LEDS); 
 Serial.print("NUM_BOLT_LEDS: "); 
 Serial.println(NUM_BOLT_LEDS);
 Serial.print("NUM_MINI_CLOUD_LEDS: "); 
 Serial.println(NUM_MINI_CLOUD_LEDS);
}

void loadPalettes(){
  // These are other ways to set up the color palette for the 'fire'.
  // First, a gradient from black to red to yellow to white -- similar to HeatColors_p
  flamePalettes[0] = CRGBPalette16( CRGB::DarkRed, CRGB::Red, CRGB::Yellow, CRGB::WhiteSmoke);
  flashColors[0] = CRGB::Blue;
  // Second, this palette is like the heat colors, but blue/aqua instead of red/yellow
  flamePalettes[1] = CRGBPalette16( CRGB::DarkBlue, CRGB::Blue, CRGB::Aqua,  CRGB::WhiteSmoke);
  flashColors[1] = CRGB::WhiteSmoke;
  // Third, here's a simpler, three-step gradient, from black to red to white
  flamePalettes[2] = CRGBPalette16( CRGB::DarkRed, CRGB::Red, CRGB::WhiteSmoke);
  flashColors[2] = CRGB::WhiteSmoke; 
  //Green
  flamePalettes[3] = CRGBPalette16( CRGB::DarkGreen, CRGB::Green, CRGB::WhiteSmoke);
  flashColors[3] = CRGB::DeepPink;  
  flamePalettes[4] = CRGBPalette16( CRGB::Purple, CRGB::Amethyst, CRGB::WhiteSmoke);
  flashColors[4] = CRGB::Orange;  
  
}

void debugLoop() {
  Serial.println(stormCountdown);
  Serial.println(stormClock);
  Serial.println(trickButtonExhausted);
  Serial.println(trickCountdown);  
  Serial.println(stormInProgress);
  Serial.println(trickInProgress);
}

void debugLeds() {
  for(int i = 0; i < NUM_CLOUD_LEDS; i++) {
   Serial.print(cloudLeds[i]); 
  }
  for(int i = 0; i < NUM_BOLT_LEDS; i++) {
   Serial.print(boltLeds[i]); 
  }
  for(int i = 0; i < NUM_MINI_CLOUD_LEDS; i++) {
   Serial.print(miniCloudLeds[i]); 
  }
}

void ledTestStrip(){
 for(int i = 1; i < NUM_CLOUD_LEDS; i++) {
  cloudLeds[i-1] = CRGB::Black;
  cloudLeds[i] = CRGB::WhiteSmoke;
  FastLED.show();
  delay(30);
 } 
 for(int i = 1; i < NUM_BOLT_LEDS; i++) {
  boltLeds[i-1] = CRGB::Black;
  boltLeds[i] = CRGB::WhiteSmoke;
  FastLED.show();
  delay(30);
 } 
 for(int i = 1; i < NUM_MINI_CLOUD_LEDS; i++) {
  boltLeds[i-1] = CRGB::Black;
  boltLeds[i] = CRGB::WhiteSmoke;
  FastLED.show();
  delay(30);
 } 
}

void loop() {
 
  //debugLoop();
  //debugLeds();
  
  if(stormInProgress || trickInProgress) {
    random16_add_entropy( random());
    animate(); 
  } else {

    inputStatus = 0;
    inputStatus = tick();
    if(trickButtonExhausted <= 0) {
      trickButtonExhausted = 0;
      activateButton();
    }
    
    if(inputStatus == 1 && trickButtonExhausted <= 0) { // Trick button pressed
      freeTrick();
      Serial.println("Free Trick!");
    } else if(inputStatus == 2) {
      demoStorm();
      Serial.println("Demo Storm!");
    }
  
    if(stormCountdown <= 0) {
      nextStorm();
    }
  
    if(trickCountdown <= 0) {
     nextTrick();
    }
    //setButtonColor(255,0,0); //for RGB Button
  }  
}

int tick() {
  //Update button color or animate it or something if using RGB
  stormCountdown--;
  trickCountdown--;
  trickButtonExhausted--;
  int input = 0;
  trailLighting();
  if(trickButtonExhausted > 0) {
   deactivateButton(); 
  }
  FastLED.show();
  for(int i = 0; i < 10; i++) {
    input = checkForInput();

    if(input == 1 && trickButtonExhausted <= 0) {
     return input; 
    }
    
    if(input == 2) {
     return input; 
    }
    delay(100);
  }
  Serial.println("tick");
  return 0;
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
    trickButtonExhausted = 60;
    stormCountdown += 90; 
    deactivateButton();
}

void setButtonColor(int red, int green, int blue) {
  analogWrite(PUBLIC_BUTTON_RED_PIN, red);
  analogWrite(PUBLIC_BUTTON_GREEN_PIN, green);
  analogWrite(PUBLIC_BUTTON_BLUE_PIN, blue);

}

void activateButton() {
  digitalWrite(PUBLIC_BUTTON_LIGHT_PIN, HIGH);  
}

void deactivateButton() {
  digitalWrite(PUBLIC_BUTTON_LIGHT_PIN, LOW);  
}

void demoStorm() {
   stormCountdown = 0; 
}

void nextTrick() {
    trickCountdown = random16(180, 300);
    stormCountdown += 10;
    currentTrick++;
    trickInProgress = true;
    animationBeginning = true;
    currentTrickFrame = 0;
    allSpeakersOn();
    Serial.print("Next Trick! ");
    Serial.println(currentTrick);
}

void nextStorm() {
   stormCountdown = random16(300,600); //only storm every 10-15 minutes
   trickCountdown = 30;
   currentStorm++;
   stormInProgress = true;
   animationBeginning = true;
   stormClock = 1200; // random16(1200,1800); //Storm for 20-30 seconds (1200 - 1800 frames)
   Serial.println("Next Storm! ");
   Serial.println(currentStorm);
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
     case 6: rainbowStorm();
             break;             
     default: rainbowTrick1();
              currentStorm = 0;
              stormClock = 0;
              stormInProgress = false;
              //TODO: We looped. Do something special here?
    }
  }
  
  if(trickInProgress) {
    switch(currentTrick) {
     case 1: rainbowTrick1();
             break;
     case 2: iceTrick();
             break;
     case 3: fizzleTrick(20);
             break;
     case 4: rainbowTrick2();
             break;     
     case 5: greenTrick();
             break;
     case 6: fizzleTrick(40);
             break;
     case 7: rainbowTrick3();
             break;
     case 8: sparkleColorsTrick();
             break;
     case 9: purpleTrick();
             break;
     case 10: fizzleTrick(60);
              break;
     case 11: sparkleTrick();
              break;
     case 12: fizzleTrick(20);
              break;
     case 13: sparkleColorsPersistentTrick();
              break;
     case 14: flameTrick();
              break;
     default: fizzleTrick(60);
              currentTrick = 0;
    }    
    currentTrickFrame++;
  }
  
  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND); 
}


/*************************
 * Storms and modes
 *************************/
 
 //TRAIL LIGHTING MODE
 
void trailLighting() {
 musicPlayer.stopPlaying();
 allSpeakersOff();
 for(int i = 0; i < NUM_CLOUDS; i++) {
  setBoltColor(i, CRGB::Black);
  setCloudColor(i, TRAIL_LIGHTING_COLOR);
 }
}
 
//STORM DEFINITIONS

void stormIntro() {
  Serial.println("stormIntro");
  //Dim the leds over 5 seconds
  for (int i = 0; i < 100; i++) {
     dimClouds(8, 50);
  }
  delay(2000);
  
  allSpeakersOn();
  musicPlayer.stopPlaying();
  musicPlayer.startPlayingFile("intro.mp3");
  //flicker leds for 9 seconds;
  for (int i = 0; i < 70; i++) {
     flickerClouds(50);
     FastLED.show();
     delay(90);
  }
  
  switch(currentStorm) {
     case 1: prideStormIntro();
             break;
     case 2: fireStormIntro();
             break;
     case 6: rainbowStormIntro();
             break;  
  } 
}

void prideStorm() {
  static int currentColor = 0;

  currentRainbowPalette = RainbowColors_p;
  currentRainbowBlending = LINEARBLEND;
  
  //playBackgroundSound("backwav3.mp3");
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */
  fillCloudsFromPaletteColors(startIndex);
  if(randomBolt(1, rainbowColors[currentColor]) == 1) {
    currentColor++;
    if(currentColor >= sizeof(rainbowColors) / sizeof(rainbowColors[0])) {
      currentColor = 0;
    }
  }
}

void prideStormIntro() {
  //TODO: Play Enchant_target_slow.mp3
  for(int i = 0; i < NUM_CLOUDS; i++) {
    setBoltColor(i, CRGB::Black);
    setCloudColor(i, rainbowColors[i % NUM_RAINBOW_COLORS]);
    //delay(1000);
  }
  //delay(3000);
}

void fireStorm() {
  //playBackgroundSound("backfire.mp3");
  currentFlamePalette = 0;
  flameClouds(); 
  randomBolt(1, flashColors[currentFlamePalette]); 
  //Use the Doom/Lina crackling flames and something firey for the bolts
}
 
void fireStormIntro() {
  
}

void rainbowStorm() {
  //TODO: Pulsate brightness?
  randomRainbowBolt(2);
}

void rainbowStormIntro() {
  for (int i = 0; i < NUM_CLOUDS; i++) {
   //TODO: Play blink dagger sound
   setCloudColor(i, rainbowColors[i]); 
   delay(1000);
  }
}

void iceStorm() {
  //playBackgroundSound("backwind.mp3");
  currentFlamePalette = 1;
  flameClouds(); 
  randomBolt(1, flashColors[currentFlamePalette]); 
  //Use the CM sound at the beginning
}

void iceStormIntro() {
  
}

void greenStorm() {
  //playBackgroundSound("backwavy.mp3");
  currentFlamePalette = 3;
  flameClouds(); 
  randomBolt(1, flashColors[currentFlamePalette]); 
}

void purpleStorm() {
  //playBackgroundSound("backwav3.mp3");
  currentFlamePalette = 4;
  flameClouds(); 
  randomBolt(1, flashColors[currentFlamePalette]); 
}

/////////////////////
//TRICK DEFINITIONS//
/////////////////////

void trickIntro() {
  fadeCloudsToBlack();
}

void trickConclusion() {
  fadeCloudsToBlack();
  fadeToTrailLighting(); 
}

void rainbowTrick1() {
  musicPlayer.stopPlaying();
  musicPlayer.startPlayingFile("rune1.mp3");
  for(int i = 0; i < NUM_CLOUDS; i++){
    setCloudColor(i, rainbowColors[i]);
    FastLED.show();
    delay(500);
  }
  for(int i = 0; i < NUM_MINI_CLOUDS; i++){
    setCloudColor(i, rainbowColors[i]);
    setMiniCloudColor(i, rainbowColors[i]);
    FastLED.show();
    delay(500);
  } 
  delay(5000);
  
  trickConclusion();
  trickInProgress = false;
}

void rainbowTrick2() {
  musicPlayer.stopPlaying();
  musicPlayer.startPlayingFile("rune2.mp3");
  for(int i = 0; i < NUM_CLOUDS; i++){
    setCloudColor(i - 1, TRAIL_LIGHTING_COLOR);
    setCloudColor(i, rainbowColors[i]);
    FastLED.show();
    switch(i) {
      case 0: delay(200);
              break;
      case 1: delay(200);
              break;
      case 2: delay(200);
              break;
      case 3: delay(400);
              break;
      case 4: delay(400);
              break;
      case 5: delay(200);
              break;
          
    }
  }
  
  for(int i = 0; i < NUM_MINI_CLOUDS; i++){
    setMiniCloudColor(i - 1, TRAIL_LIGHTING_COLOR);
    setMiniCloudColor(i, rainbowColors[i]);
    FastLED.show();
    switch(i) {
      case 0: delay(100);
              break;
      case 1: delay(100);
              break;
      case 2: delay(100);
              break;
      case 3: delay(100);
              break;
      case 4: delay(100);
              break;
      case 5: delay(100);
              break;
    }
  }
  
  setCloudColor(NUM_CLOUDS -1, TRAIL_LIGHTING_COLOR);
  trickInProgress = false;

}

void rainbowTrick3() {
  musicPlayer.stopPlaying();
  musicPlayer.startPlayingFile("rune3.mp3");
  for(int i = 0; i < NUM_CLOUDS * 2; i++){
    setCloudColor(random(0,NUM_CLOUDS), rainbowColors[i % NUM_RAINBOW_COLORS]);
    setMiniCloudColor(random(0,NUM_MINI_CLOUDS), rainbowColors[i % NUM_RAINBOW_COLORS]);
    FastLED.show();
    delay(250);
  }
  delay(2000);
  trickConclusion();
  trickInProgress = false;

}  

void flameTrick() {
  currentFlamePalette = 0;
  flameClouds();  
  playBackgroundSound("backfire.mp3");
  if(currentTrickFrame >= FRAMES_PER_SECOND * 10) {
    trickConclusion();
    trickInProgress = false;
  }
}

void iceTrick() {
  currentFlamePalette = 1;
  flameClouds();  
  playBackgroundSound("ticestor.mp3");
  if(currentTrickFrame >= FRAMES_PER_SECOND * 12) {
      trickInProgress = false;
      trickConclusion();
  }
}

void greenTrick() {
  currentFlamePalette = 3;
  flameClouds();  
  playBackgroundSound("backtele.mp3");
  if(currentTrickFrame >= FRAMES_PER_SECOND * 10) {
      trickInProgress = false;
      trickConclusion();
  }
}

void purpleTrick() {
  currentFlamePalette = 4;
  flameClouds();  
  playBackgroundSound("backele5.mp3");
  if(currentTrickFrame >= FRAMES_PER_SECOND * 10) {
      trickInProgress = false;
      trickConclusion();
  }
}

void sparkleTrick() {
  playBackgroundSound("tsparkle.mp3");
  flickerClouds(25);
  if(currentTrickFrame >= FRAMES_PER_SECOND * 5) {
      trickInProgress = false;
      trickConclusion();
  }
}

void sparkleColorsTrick() {
  playBackgroundSound("tflower2.mp3");
  flickerCloudsColors(5);
  if(currentTrickFrame >= FRAMES_PER_SECOND * 5) {
      trickInProgress = false;
      trickConclusion();
  }
}

void sparkleColorsPersistentTrick() {
  playBackgroundSound("tflower1.mp3");
  flickerCloudsColorsPersistent(25);
  if(currentTrickFrame >= FRAMES_PER_SECOND * 4) {
      trickInProgress = false;
      trickConclusion();
  }
}

void fizzleTrick(int wait) {
  musicPlayer.startPlayingFile("tfizzle.mp3");
  trickConclusion();
  trickConclusion();
  setAllCloudColors(CRGB::Black);
  FastLED.show();
  delay(wait);
  FastLED.show();
  setAllCloudColors(TRAIL_LIGHTING_COLOR);
  FastLED.show();
  delay(wait);
  FastLED.show();
  setAllCloudColors(CRGB::Black);
  FastLED.show();
  delay(wait);
  FastLED.show();
  setAllCloudColors(TRAIL_LIGHTING_COLOR);
  FastLED.show();
  delay(wait);
  FastLED.show();
  setAllCloudColors(CRGB::Black);
  FastLED.show();
  delay(wait);
  FastLED.show();
  setAllCloudColors(TRAIL_LIGHTING_COLOR);
  FastLED.show();
  delay(wait);
  FastLED.show();
  trickConclusion();
  trickInProgress = false;
}
 //UTILITY FUNCTIONS
 
void playBackgroundSound(char* filename) {
  //return; //deactivateBackgroundSounds
  Serial.println(filename);
  allSpeakersOn();

  if (musicPlayer.stopped()) {
    musicPlayer.stopPlaying();
    musicPlayer.setVolume(45,45);
    musicPlayer.startPlayingFile(filename);
  }
}
 
int randomBolt(int boltsPerSecond, CRGB color) {
  static bool isAnimating;
  static int frame;
  static int bolt;
  String sound;
  char fileName[12];
  
  if(isAnimating) {
    switch(frame) {
     case 0: bolt = random16(0,NUM_CLOUDS);
             setBoltColor(bolt, color);
             sound = "bolt" + String(currentStorm) + String("-") + String(random16(0, numBoltSounds[currentStorm - 1]) + 1) + String(".mp3");
             sound.toCharArray(fileName, 12);
             Serial.print("Bolt!");
             Serial.println(fileName);
             allSpeakersOff();
             speakerOn(int(bolt / CLOUDS_PER_SPEAKER));
             Serial.print(int(bolt / CLOUDS_PER_SPEAKER));
             musicPlayer.stopPlaying();
             musicPlayer.setVolume(5,5);
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

int randomRainbowBolt(int boltsPerSecond) {
  static bool isAnimating;
  static int frame;
  static int bolt;
  String sound;
  char fileName[12];
  
  if(isAnimating) {
    switch(frame) {
     case 0: bolt = random16(0,NUM_CLOUDS);
             setBoltColor(bolt, rainbowColors[bolt]);
             sound = "bolt" + String(currentStorm) + String("-") + String(random16(0, numBoltSounds[currentStorm - 1]) + 1) + String(".mp3");
             sound.toCharArray(fileName, 12);
             Serial.print("Bolt!");
             Serial.println(fileName);
             allSpeakersOff();             
             speakerOn(int(bolt / CLOUDS_PER_SPEAKER));
             Serial.print(int(bolt / CLOUDS_PER_SPEAKER));
             musicPlayer.stopPlaying();
             musicPlayer.setVolume(5,5);
             musicPlayer.startPlayingFile(fileName);             
             break;
     case 5:   setBoltColor(bolt, CRGB::Black);
               break;
     case 10:  setBoltColor(bolt, rainbowColors[bolt]);
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
    boltLeds[i] = color;
  }
}

void setCloudColor(int cloud, CRGB color) {
  if(cloud < 0 || cloud > NUM_CLOUDS) {
    return; 
  }
  int cloudEnd = LEDS_PER_CLOUD * (cloud + 1);
  for(int i = cloud * LEDS_PER_CLOUD; i < cloudEnd; i++) {
    cloudLeds[i] = color;
  }
}

void setMiniCloudColor(int cloud, CRGB color) {
  if(cloud < 0 || cloud > NUM_MINI_CLOUDS) {
    return; 
  }
  int cloudEnd = LEDS_PER_MINI_CLOUD * (cloud + 1);
  for(int i = cloud * LEDS_PER_MINI_CLOUD; i < cloudEnd; i++) {
    miniCloudLeds[i] = color;
  }
}

void setAllCloudColors(CRGB color){
 for(int i = 0; i < NUM_CLOUDS; i++) {
   setCloudColor(i, color);
 } 
  for(int i = 0; i < NUM_MINI_CLOUDS; i++) {
   setMiniCloudColor(i, color);
 } 
}

void fadeCloudsToBlack() {
  for(int i = 0; i < 100; i++){
    dimCloudsToBlack(16, 10);
  }
}

void fadeToTrailLighting() {
  setAllCloudColors(0x010101);
  // while(uint32_t(cloudLeds[0]) < TRAIL_LIGHTING_COLOR) { //This is the right way to do it
  for (int i = 0; i < 11; i++) {
    brightenClouds(16,15);
  }
}

void dimClouds(int factor, int wait) {
  for(int i = 0; i < NUM_CLOUD_LEDS; i++) {
    cloudLeds[i].fadeLightBy(factor);
  }
  for(int i = 0; i < NUM_MINI_CLOUD_LEDS; i++) {
    miniCloudLeds[i].fadeLightBy(factor);
  }
  FastLED.show();
  delay(wait);
}

void dimCloudsToBlack(int factor, int wait) {
  for(int i = 0; i < NUM_CLOUD_LEDS; i++) {
    cloudLeds[i].fadeToBlackBy(factor);
  }
  for(int i = 0; i < NUM_MINI_CLOUD_LEDS; i++) {
    miniCloudLeds[i].fadeToBlackBy(factor);
  }

  FastLED.show();
  delay(wait);
}

void brightenClouds(int factor, int wait) {
  for(int i = 0; i < NUM_CLOUD_LEDS; i++) {
    cloudLeds[i] += cloudLeds[i].fadeLightBy(factor);
  }
  for(int i = 0; i < NUM_MINI_CLOUD_LEDS; i++) {
    miniCloudLeds[i] += miniCloudLeds[i].fadeLightBy(factor);
  }  
  FastLED.show();
  delay(wait);
}

void flickerClouds(int probability) {
  for(int i = 0; i < NUM_CLOUD_LEDS; i++) {
    if(random(0,probability) == 0) {
      cloudLeds[i] = CRGB::WhiteSmoke; 
    } else {
      cloudLeds[i] = 0x202020;
    }
  }
  for(int i = 0; i < NUM_MINI_CLOUD_LEDS; i++) {
    if(random(0,probability) == 0) {
      miniCloudLeds[i] = CRGB::WhiteSmoke; 
    } else {
      miniCloudLeds[i] = 0x202020;
    }
  }
}

void flickerCloudsColors(int probability) {  
  for(int i = 0; i < NUM_CLOUD_LEDS; i++) {
    if(random(0,probability) == 0) {
      cloudLeds[i] = rainbowColors[random(0,5)]; 
    } else {
      cloudLeds[i] = 0x202020;
    }
  }
  for(int i = 0; i < NUM_MINI_CLOUD_LEDS; i++) {
    if(random(0,probability) == 0) {
      miniCloudLeds[i] = rainbowColors[random(0,5)]; 
    } else {
      miniCloudLeds[i] = 0x202020;
    }
  }  
}

void flickerCloudsColorsPersistent(int probability) {  
  for(int i = 0; i < NUM_CLOUD_LEDS; i++) {
    if(random(0,probability) == 0) {
      cloudLeds[i] = rainbowColors[random(0,5)]; 
    }
  }
  for(int i = 0; i < NUM_MINI_CLOUD_LEDS; i++) {
    if(random(0,probability) == 0) {
      miniCloudLeds[i] = rainbowColors[random(0,5)]; 
    }
  }  
}

void flameClouds()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_CLOUD_LEDS];
  static byte miniCloudHeat[NUM_MINI_CLOUD_LEDS];

  // Step 1.  Cool down every cell a little
    for(int i = 0; i < NUM_CLOUD_LEDS; i++) {
      heat[i] = qsub8(heat[i],  random8(0, ((COOLING * 10) / NUM_CLOUD_LEDS) + 2));
    }
    for(int i = 0; i < NUM_MINI_CLOUD_LEDS; i++) {
      miniCloudHeat[i] = qsub8(miniCloudHeat[i], random8(0, ((COOLING * 10) / NUM_MINI_CLOUD_LEDS) + 2));
    }    
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for(int k= NUM_CLOUD_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    for(int k= NUM_MINI_CLOUD_LEDS - 1; k >= 2; k--) {
      miniCloudHeat[k] = (miniCloudHeat[k - 1] + miniCloudHeat[k - 2] + miniCloudHeat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8(heat[y], random8(160,255) );
    }
    if( random8() < SPARKING ) {
      int y = random8(7);
      miniCloudHeat[y] = qadd8(miniCloudHeat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_CLOUD_LEDS; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8(heat[j], 240);
      cloudLeds[j] = ColorFromPalette(flamePalettes[currentFlamePalette], colorindex);
    }
    for( int j = 0; j < NUM_MINI_CLOUD_LEDS; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8(miniCloudHeat[j], 240);
      miniCloudLeds[j] = ColorFromPalette(flamePalettes[currentFlamePalette], colorindex);
    }    
 
}

void fillCloudsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;
    
    for( int i = 0; i < NUM_CLOUD_LEDS; i++) {
        cloudLeds[i] = ColorFromPalette( currentRainbowPalette, colorIndex, brightness, currentRainbowBlending);
        colorIndex += 3;
    }
    for( int i = 0; i < NUM_MINI_CLOUD_LEDS; i++) {
        miniCloudLeds[i] = ColorFromPalette( currentRainbowPalette, colorIndex, brightness, currentRainbowBlending);
        colorIndex += 3;
    }    
}

/********************
 * Speaker Controls
********************/

void allSpeakersOn() {
  digitalWrite(SPEAKER_CONTROL_PIN_1, LOW);
  digitalWrite(SPEAKER_CONTROL_PIN_2, LOW);
  digitalWrite(SPEAKER_CONTROL_PIN_3, LOW);
  digitalWrite(SPEAKER_CONTROL_PIN_4, LOW);
  digitalWrite(SPEAKER_CONTROL_PIN_5, LOW);
  digitalWrite(SPEAKER_CONTROL_PIN_6, LOW);
  digitalWrite(SPEAKER_CONTROL_PIN_7, LOW);
  digitalWrite(SPEAKER_CONTROL_PIN_8, LOW);
}

void allSpeakersOff() {
  digitalWrite(SPEAKER_CONTROL_PIN_1, HIGH);
  digitalWrite(SPEAKER_CONTROL_PIN_2, HIGH);
  digitalWrite(SPEAKER_CONTROL_PIN_3, HIGH);
  digitalWrite(SPEAKER_CONTROL_PIN_4, HIGH);
  digitalWrite(SPEAKER_CONTROL_PIN_5, HIGH);
  digitalWrite(SPEAKER_CONTROL_PIN_6, HIGH);
  digitalWrite(SPEAKER_CONTROL_PIN_7, HIGH);
  digitalWrite(SPEAKER_CONTROL_PIN_8, HIGH);
}

void speakerOn(int speaker) {
  Serial.print(speaker);
  switch(speaker) {
    case 0:   digitalWrite(SPEAKER_CONTROL_PIN_1, LOW);
              digitalWrite(SPEAKER_CONTROL_PIN_2, LOW);
              break;
    case 1:   digitalWrite(SPEAKER_CONTROL_PIN_3, LOW);
              digitalWrite(SPEAKER_CONTROL_PIN_4, LOW);
              break;
    case 2:   digitalWrite(SPEAKER_CONTROL_PIN_5, LOW);
              digitalWrite(SPEAKER_CONTROL_PIN_6, LOW);
              break;
    case 3:   digitalWrite(SPEAKER_CONTROL_PIN_7, LOW);
              digitalWrite(SPEAKER_CONTROL_PIN_8, LOW);
              break;
  }
}

void speakerOff(int speaker) {
  switch(speaker) {
    case 0:   digitalWrite(SPEAKER_CONTROL_PIN_1, HIGH);
              digitalWrite(SPEAKER_CONTROL_PIN_2, HIGH);
              break;
    case 1:   digitalWrite(SPEAKER_CONTROL_PIN_3, HIGH);
              digitalWrite(SPEAKER_CONTROL_PIN_4, HIGH);
              break;
    case 2:   digitalWrite(SPEAKER_CONTROL_PIN_5, HIGH);
              digitalWrite(SPEAKER_CONTROL_PIN_6, HIGH);
              break;
    case 3:   digitalWrite(SPEAKER_CONTROL_PIN_7, HIGH);
              digitalWrite(SPEAKER_CONTROL_PIN_8, HIGH);
              break;
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
