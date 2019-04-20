#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define LED_DRIVER_PIN 15
#define LED_DRIVER_COUNT 6
#define EYES_BLINK 1
#define EYES_INCREASE_BRIGHTNESS 2
#define EYES_DECREASE_BRIGHTNESS 3
#define EYES_SET_COLOR_SINGLE_PIXEL 4
#define EYES_SET_COLOR_ALL_PIXELS 5
#define EYES_SET_MASK 6
#define EYES_DO_LRN 7
#define EYES_DO_COP 8

volatile int update_eyes = 0;
volatile uint8_t pixel = 0;
volatile uint32_t pixel_color = 0xFFFFFF;
volatile uint8_t brightness = 10;
volatile uint32_t eye_color = 0xFFFFFF;
volatile int eye_command = 0;
volatile uint8_t current_mask = 1;
volatile uint8_t update_mask = 0;
TaskHandle_t Task2;

uint32_t ledColors[LED_DRIVER_COUNT];
uint32_t dim_array[LED_DRIVER_COUNT];

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_DRIVER_COUNT, LED_DRIVER_PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void SetupRGB();
void handleCommand(const char* inputString);
void handleLeds();
void colorWipe(uint32_t c, uint8_t wait);
void rainbow(uint8_t wait);
void rainbowCycle(uint8_t wait);
void theaterChase(uint32_t c, uint8_t wait);
void theaterChaseRainbow(uint8_t wait);
uint32_t Wheel(byte WheelPos);

void LedTask( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    handleLeds();
    delay(500);
  }
}



void SetupTask(){
  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    LedTask,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */
}

void SetupRGB(){
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code
  SetupTask();

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void refreshLeds(){
    //colorWipe(ledColors[0], 1);
    for (int i2 = 0; i2 < LED_DRIVER_COUNT; ++i2)
    {
        uint32_t red = (ledColors[i2]>>16) & 0xFF;
        red = red*brightness/255;
        uint32_t green = (ledColors[i2]>>8) & 0xFF;
        green = green*brightness/255;
        uint32_t blue = (ledColors[i2]) & 0xFF;
        blue = blue*brightness/255;
        uint32_t scaled_color = (red << 16)+(green << 8)+(blue);
        //dim_array[i2] = scaled_color;
        strip.setPixelColor(i2, scaled_color);
    }     
    strip.show();
}



void set_neopixel(uint8_t pixel_num, uint32_t color)
{
    if(pixel_num <LED_DRIVER_COUNT)
    {
        ledColors[pixel_num] = color;
    }
    refreshLeds();
}

void set_neopixel_group(uint32_t color)
{
    for (int i = 0; i < LED_DRIVER_COUNT; ++i)
    {
        ledColors[i] = color;
    }   
    refreshLeds();
}

void SetColorsDirect(int red, int green, int blue){
  red = red*brightness/255;
  green = green*brightness/255;
  blue = blue*brightness/255;
  uint32_t scaled_color = (red << 16)+(green << 8)+(blue);
  set_neopixel_group(scaled_color);
}

void increase_brightness()
{
    int16_t temp_brightness = brightness+10;
    if(temp_brightness>255)
    {
        temp_brightness = 255;
    } 
    brightness = temp_brightness;
    refreshLeds();
}  

void decrease_brightness()
{
    int16_t temp_brightness = brightness-10;
    if(temp_brightness<=1)
    {
        temp_brightness = 2;
    } 
    brightness = temp_brightness;
    refreshLeds();
}

void eyes_blink(){
 //TODO?
}

void eyes_cop(){
 //TODO? 
}

void handleCommand(const char* inputString){
    if (strcmp(inputString, "brightness_up") == 0)
    {
        update_eyes = EYES_INCREASE_BRIGHTNESS;
    }

    if (strcmp(inputString, "brightness_down") == 0)
    {
        update_eyes = EYES_DECREASE_BRIGHTNESS;
    }
    
    if (strcmp(inputString, "cop") == 0)
    {
        update_eyes = EYES_DO_COP;
    }

    if (strcmp(inputString, "blink") == 0)
    {
        update_eyes = EYES_BLINK;
    }

    if (strncmp(inputString, "led",3) == 0) 
    { 
        const char* pBeg = &inputString[0];
        char* pEnd;
        pixel = strtol(pBeg+4, &pEnd,10);
        pixel_color = strtol(pEnd, &pEnd,16);
        if ((pixel < LED_DRIVER_COUNT)&&(pixel_color<=0xFFFFFF))
        {
            update_eyes = EYES_SET_COLOR_SINGLE_PIXEL;
        }
    }     
    if (strncmp(inputString, "leds",4) == 0) 
    { 
        const char* pBeg = &inputString[0];
        char* pEnd;
        pixel_color = strtol(pBeg+5, &pEnd,16);
        if (pixel_color<=0xFFFFFF)
        {
            update_eyes = EYES_SET_COLOR_ALL_PIXELS;
        }
    }    
}

void handleLeds(){
  if (update_eyes > 0)
        {
            // I'm doing this local copy of the update_eyes variable so that I can clear update_eyes variable right away
            // this makes it so that if you get another eye command while it's still in the middle of doing the last one
            // it will do that new one after.  This only allows one queued up command, but that should be fine for most cases
            eye_command = update_eyes;
            update_eyes = 0;
            switch(eye_command)
            {
                case EYES_BLINK:
                    eyes_blink();
                    break;
                case EYES_INCREASE_BRIGHTNESS:
                    increase_brightness();
                    break;
                case EYES_DECREASE_BRIGHTNESS:
                    decrease_brightness();
                    break;
                case EYES_SET_COLOR_SINGLE_PIXEL:
                    set_neopixel(pixel, pixel_color);
                    break;
                case EYES_SET_COLOR_ALL_PIXELS:
                    set_neopixel_group(pixel_color);
                    break;
                case EYES_SET_MASK:
//                    current_mask = update_mask;
//                    refresh_eyes_masked(nmask[current_mask]);
                    break;
                case EYES_DO_COP:
                    eyes_cop();
                    break;
            }
        }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/**
 * // Some example procedures showing how to display to the pixels:
  colorWipe(strip.Color(255, 0, 0), 50); // Red
  colorWipe(strip.Color(0, 255, 0), 50); // Green
  colorWipe(strip.Color(0, 0, 255), 50); // Blue
//colorWipe(strip.Color(0, 0, 0, 255), 50); // White RGBW
  // Send a theater pixel chase in...
  theaterChase(strip.Color(127, 127, 127), 50); // White
  theaterChase(strip.Color(127, 0, 0), 50); // Red
  theaterChase(strip.Color(0, 0, 127), 50); // Blue

  rainbow(20);
  rainbowCycle(20);
  theaterChaseRainbow(50);
 */
