#include <FastLED.h>

#define COLOR_ORDER GRB
//BGR
#define CHIPSET WS2811
//APA102

#define BRIGHTNESS 25
#define FRAMES_PER_SECOND 60

#define NUM_CLOUDS 2
#define CLOUDS_PIN 12
#define BOLTS_PIN 11
#define LEDS_PER_CLOUD 12 //Clouds and bolts must have the same number of LEDs for this implementation
#define LEDS_PER_BOLT 12
#define LEDS_PER_STRIP LEDS_PER_CLOUD * NUM_CLOUDS

//For fire animations
#define NUM_FIRE_PALETTES 3
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

//storms is an array of Shows
//tricks is an array of Shows

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
int stormClock;

CRGBPalette16 firePalettes[3]; 
CRGB flashColors[3];

CRGB rainbowColors[6] = { CRGB::Red, CRGB::DarkOrange, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Purple };

CRGBPalette16 currentRainbowPalette;
TBlendType    currentRainbowBlending;

int currentFlamePalette = 0;

void setup() {
  Serial.begin(9600);
  Serial.write("setup\n");

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
  firePalettes[0] = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White);
  flashColors[0] = CRGB::Red;
  // Second, this palette is like the heat colors, but blue/aqua instead of red/yellow
  firePalettes[1] = CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);
  flashColors[1] = CRGB::White;
  // Third, here's a simpler, three-step gradient, from black to red to white
  firePalettes[2] = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::White);
  flashColors[2] = CRGB::White; 
}

void debugLoop() {
  Serial.println(stormCountdown);
  Serial.println(stormClock);
  Serial.println(stormInProgress);
}

void loop() {
 
  if(true){
    debugLoop();
  }
  
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
    //turn on button light
}

void nextStorm() {
   stormCountdown = 15; //random(300,600); //only storm every 5-10 minutes
   trickCountdown = 30;
   currentStorm++;
   stormInProgress = true;
   stormClock = 1800; // random(1200,1800); //Storm for 20-30 seconds (1200 - 1800 frames)
/*   if(currentStorm > storms.length) {
     currentStorm = 0;
   }*/
   //TODO: Dim clouds and make ominous rumbling
}
void animate() {
  if(stormInProgress) {
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
     default: currentStorm = 0;
              stormClock = 0;
              stormInProgress = false;
              //TODO: We looped. Do something special here?
    }
  }
  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND); 
}

void tick() {
  //Update button color or animate it or something
  trailLighting();
  FastLED.show();
  delay(1000);
  Serial.print("tick\n");
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
void prideStorm(){
  static int currentColor = 0;
  
  Serial.println("CurrentColor:");
  Serial.println(currentColor);

  currentRainbowPalette = RainbowColors_p;
  currentRainbowBlending = LINEARBLEND;
  
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */
  fillCloudsFromPaletteColors( startIndex);
  if(randomBolt(1, rainbowColors[currentColor]) == 1) {
    currentColor++;
    if(currentColor >= sizeof(rainbowColors) / sizeof(rainbowColors[0])) {
      currentColor = 0;
    }
  }
}

void fireStorm() {
  currentFlamePalette = 0;
  flameClouds(); 
  randomBolt(2, flashColors[currentFlamePalette]); 
  //Use the Doom/Lina crackling flames and something firey for the bolts
}
 
void iceStorm() {
  currentFlamePalette = 1;
  flameClouds(); 
  randomBolt(2, flashColors[currentFlamePalette]); 
  //Use the CM sound at the beginning
}
 
 //TRICK DEFINITIONS
 
 //UTILITY FUNCTIONS
 
int randomBolt(int boltsPerSecond, CRGB color) {
  static bool isAnimating;
  static int frame;
  static int bolt;
  
  if(isAnimating) {
    switch(frame) {
     case 0: bolt = random(0,NUM_CLOUDS);
             setBoltColor(bolt, color);
             break;
     case 5:  setBoltColor(bolt, CRGB::Black);
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
    Serial.print(randomNum);
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
      leds[0][j] = ColorFromPalette( firePalettes[currentFlamePalette], colorindex);
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
