//#include <LoudClouds.h>
#include <FastLED.h>

#define COLOR_ORDER GRB
//BGR
#define CHIPSET WS2811
//APA102
//

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

int stormCountdown = 300;
int trickCountdown = 0;
int currentStorm = 0;
int currentTrick = 0;
int inputStatus = 0;
bool trickButtonExhausted = false;

int testCount = 0;


CRGBPalette16 firePalettes[3]; 
CRGB flashColors[3];


int currentFirePalette = 0;

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
  testCount++;
  if(testCount >= 100){
    testCount = 0;
    Serial.print(LEDS_PER_STRIP);
    for(int i = 0; i < LEDS_PER_STRIP; i++) {
      Serial.print(i);
      Serial.print(":   ");
      Serial.print(leds[0][i]);
      Serial.print("     ");
      Serial.print(leds[1][i]);
      Serial.print("\n");
    }
  }
}

void loop() {
 
  if(false){
    debugLoop();
  }
  
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
    nextTrick();
  }
  
  tick();
  
  /* demo/test code below here! */
  
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy( random());

  FireClouds(); 
  randomBolt(2);
  
  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND);  
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
    //turn on button light
}

void nextStorm() {
   stormCountdown = random(300,600); //only storm every 5-10 minutes
   trickCountdown = 30;
   currentStorm++;
/*   if(currentStorm > storms.length) {
     currentStorm = 0;
   }*/
}
void tick() {
  //Update button color or animate it or something
  //delay(1000);
}

/*************************
 * Below here is noodling that will need to get moved to the library
 *************************/
void randomBolt(int boltsPerSecond) {
  static bool isAnimating;
  static int frame;
  static int bolt;
  
  if(isAnimating) {
    switch(frame) {
     case 0: bolt = random(0,NUM_CLOUDS);
             setBoltColor(bolt, flashColors[currentFirePalette]);
             Serial.println("RANDOMBOLT ANIMATION BEGINNGING");
             Serial.println(bolt);
             break;
     case 5:  setBoltColor(bolt, CRGB::Black);
               break;
     case 10:  setBoltColor(bolt, flashColors[currentFirePalette]);
               break;
     case 40:  setBoltColor(bolt, CRGB::Black);
               frame = 0;
               isAnimating = false;
               bolt = random(0,NUM_CLOUDS);
    }
    frame++;

  } else {
    int randomNum = random(0,FRAMES_PER_SECOND * boltsPerSecond);
    Serial.print(randomNum);
    if(randomNum == 0) {
      Serial.print("Bolt!\n");
      frame = 0;
      isAnimating = true;
      Serial.print("\n\n\nANIMATING\n\n");
    } else {
      for(int i = 0; i < NUM_CLOUDS; i++){
        setBoltColor(bolt, CRGB::Black);
      }
    }
  }
}

void setBoltColor(int bolt, CRGB color) {
  int boltEnd = LEDS_PER_BOLT * (bolt + 1);
  for(int i = bolt * LEDS_PER_BOLT; i < boltEnd; i++) {
    leds[1][i] = color;
  }
}

void FireClouds() //Combine with iceClouds obviously
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
      leds[0][j] = ColorFromPalette( firePalettes[currentFirePalette], colorindex);
    }
 
}

