#include <FastLED.h>
#include <arduinoFFT.h>
#define LED_PIN  4
#define COLOR_ORDER GRB
#define CHIPSET     WS2811
#define BRIGHTNESS 64

#define SAMPLES         128          // Must be a power of 2
#define SAMPLING_FREQ   40000         // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define AMPLITUDE       1000          // Depending on your audio source level, you may need to alter this value. Can be used as a 'sensitivity' control.
#define AUDIO_IN_PIN    35            // Signal in on this pin
#define LONG_PRESS_MS   200           // Number of ms to count as a long press
#define MAX_MILLIAMPS   2000          // Careful with the amount of power here if running off USB port
#define NUM_BANDS       6            // To change this, you will need to change the bunch of if statements describing the mapping from bins to bands
#define NOISE           1500           // Used as a crude noise filter, values below this are ignored


// Params for width and height
const uint8_t kMatrixWidth = 5;
const uint8_t kMatrixHeight = 6;

#define NUM_LEDS       (kMatrixWidth * kMatrixHeight)     // Total number of LEDs
#define BAR_WIDTH      (kMatrixWidth  / (NUM_BANDS - 1))  // If width >= 8 light 1 LED width per bar, >= 16 light 2 LEDs width bar etc
#define TOP            (kMatrixWidth - 0)                // Don't allow the bars to go offscreen

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;

unsigned int sampling_period_us;
byte peak[] = {0, 0, 0, 0, 0, 0};         // The length of these arrays must be >= NUM_BANDS
int oldBarHeights[] = {0, 0, 0, 0, 0, 0};
int bandValues[] = {0, 0, 0, 0, 0, 0};
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime;
arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);

CRGB leds[NUM_LEDS];
DEFINE_GRADIENT_PALETTE( purple_gp ) {
  0,   0, 212, 255,   //blue
  255, 179,   0, 255
}; //purple
DEFINE_GRADIENT_PALETTE( outrun_gp ) {
  0, 141,   0, 100,   //purple
  127, 255, 192,   0,   //yellow
  255,   0,   5, 255
};  //blue
DEFINE_GRADIENT_PALETTE( greenblue_gp ) {
  0,   0, 255,  60,   //green
  64,   0, 236, 255,   //cyan
  128,   0,   5, 255,   //blue
  192,   0, 236, 255,   //cyan
  255,   0, 255,  60
}; //green
DEFINE_GRADIENT_PALETTE( redyellow_gp ) {
  0,   200, 200,  200,   //white
  64,   255, 218,    0,   //yellow
  128,   231,   0,    0,   //red
  192,   255, 218,    0,   //yellow
  255,   200, 200,  200
}; //white
CRGBPalette16 purplePal = purple_gp;
CRGBPalette16 outrunPal = outrun_gp;
CRGBPalette16 greenbluePal = greenblue_gp;
CRGBPalette16 heatPal = redyellow_gp;
uint8_t colorTimer = 0;
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;


  if ( y & 0x01) {
    // Odd rows run backwards
    uint8_t reverseX = (kMatrixWidth - 1) - x;
    i = (y * kMatrixWidth) + reverseX;
  } else {
    // Even rows run forwards
    i = (y * kMatrixWidth) + x;
  }


  return i;
}
void loop()
{
  // Reset bandValues[]
  for (int i = 0; i < NUM_BANDS; i++) {
    bandValues[i] = 0;
  }
  for (int i = 0; i < SAMPLES; i++) {
    newTime = micros();
    vReal[i] = analogRead(AUDIO_IN_PIN); // A conversion takes about 9.7uS on an ESP32
    Serial.println(vReal[i]);
    vImag[i] = 0;
    while ((micros() - newTime) < sampling_period_us) {
      /* chill */
    }
  }

  FFT.DCRemoval();
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();

  for (int i = 2; i < (SAMPLES / 2); i++) {
    // Don't use sample 0 and only first SAMPLES/2 are usable. Each array element represents a frequency bin and its value the amplitude.

    //    if (vReal[i] > NOISE) {                    // Add a crude noise filter

    //8 bands, 12kHz top band
    if (i <= 3 )           bandValues[0]  += (int)vReal[i];
    if (i > 3   && i <= 6  ) bandValues[1]  += (int)vReal[i];
    if (i > 6   && i <= 13 ) bandValues[2]  += (int)vReal[i];
    if (i > 13  && i <= 27 ) bandValues[3]  += (int)vReal[i];
    if (i > 27  && i <= 55 ) bandValues[4]  += (int)vReal[i];
    if (i > 55  && i <= 112) bandValues[5]  += (int)vReal[i];
    //        if (i>112 && i<=229) bandValues[6]  += (int)vReal[i];
    //        if (i>229          ) bandValues[7]  += (int)vReal[i];

    //16 bands, 12kHz top band
    //      if (i <= 2 )           bandValues[0]  += (int)vReal[i];
    //      if (i > 2   && i <= 3  ) bandValues[1]  += (int)vReal[i];
    //      if (i > 3   && i <= 5  ) bandValues[2]  += (int)vReal[i];
    //      if (i > 5   && i <= 7  ) bandValues[3]  += (int)vReal[i];
    //      if (i > 7   && i <= 9  ) bandValues[4]  += (int)vReal[i];
    //      if (i > 9   && i <= 13 ) bandValues[5]  += (int)vReal[i];
    //      if (i > 13  && i <= 18 ) bandValues[6]  += (int)vReal[i];
    //      if (i > 18  && i <= 25 ) bandValues[7]  += (int)vReal[i];
    //      if (i > 25  && i <= 36 ) bandValues[8]  += (int)vReal[i];
    //      if (i > 36  && i <= 50 ) bandValues[9]  += (int)vReal[i];
    //      if (i > 50  && i <= 69 ) bandValues[10] += (int)vReal[i];
    //      if (i > 69  && i <= 97 ) bandValues[11] += (int)vReal[i];
    //      if (i > 97  && i <= 135) bandValues[12] += (int)vReal[i];
    //      if (i > 135 && i <= 189) bandValues[13] += (int)vReal[i];
    //      if (i > 189 && i <= 264) bandValues[14] += (int)vReal[i];
    //      if (i > 264          ) bandValues[15] += (int)vReal[i];
    //    }
  }

  // Process the FFT data into bar heights
  for (byte band = 0; band < NUM_BANDS; band++) {

    // Scale the bars for the display
    int barHeight = bandValues[band] / AMPLITUDE;
    if (barHeight > TOP) barHeight = TOP;

    // Small amount of averaging between frames
    barHeight = ((oldBarHeights[band] * 1) + barHeight) / 2;

    // Move peak up
    if (barHeight > peak[band]) {
      peak[band] = min(TOP, barHeight);
    }

    // Draw bars

    rainbowBars(band, barHeight);

    //purpleBars(band, barHeight);
    //
    //    centerBars(band, barHeight);
    //
    //    changingBars(band, barHeight);
    //
    //    waterfall(band);



    whitePeak(band);
    //outrunPeak(band);

    // Save oldBarHeights for averaging later
    oldBarHeights[band] = barHeight;
  }

  // Decay peak
  EVERY_N_MILLISECONDS(60) {
    for (byte band = 0; band < NUM_BANDS; band++)
      if (peak[band] > 0) peak[band] -= 1;
    colorTimer++;
  }

  // Used in some of the patterns
  EVERY_N_MILLISECONDS(10) {
    colorTimer++;
  }

  FastLED.show();


}




void setup() {

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
  Serial.begin(115200);
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQ));
  analogReadResolution(10);
}

void rainbowBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = 0; y < barHeight; y++) {
            leds[XY(y, x)] = CHSV((x / BAR_WIDTH) * (255 / NUM_BANDS), 255, 255);
      //leds[XY(x, y)] = CHSV(200, 255, 255);
    }
  }
//    int yStart = BAR_WIDTH * band;
    
}

void purpleBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = 0; y < barHeight; y++) {
      leds[XY(y, x)] = ColorFromPalette(purplePal, y * (255 / barHeight));
    }
  }
}

void changingBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = 0; y < barHeight; y++) {
      leds[XY(y, x)] = CHSV(y * (255 / kMatrixHeight) + colorTimer, 255, 255);
    }
  }
}

void centerBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (barHeight % 2 == 0) barHeight--;
    int yStart = ((kMatrixHeight - barHeight) / 2 );
    for (int y = yStart; y <= (yStart + barHeight); y++) {
      int colorIndex = constrain((y - yStart) * (255 / barHeight), 0, 255);
      leds[XY(y,x)] = ColorFromPalette(heatPal, colorIndex);
    }
  }
}

void whitePeak(int band) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = peak[band];
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    leds[XY(peakHeight,x )] = CRGB::White;
  }
}

void outrunPeak(int band) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = peak[band];
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    //leds[XY(x,peakHeight)] = CHSV(peakHeight * (255 / kMatrixHeight), 255, 255);
    leds[XY(x, peakHeight)] = ColorFromPalette(outrunPal, peakHeight * (255 / kMatrixHeight));
  }
}

void waterfall(int band) {
  int xStart = BAR_WIDTH * band;
  double highestBandValue = 60000;        // Set this to calibrate your waterfall

  // Draw bottom line
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    leds[XY(x, 0)] = CHSV(constrain(map(bandValues[band], 0, highestBandValue, 160, 0), 0, 160), 255, 255);
  }

  // Move screen up starting at 2nd row from top
  if (band == NUM_BANDS - 1) {
    for (int y = kMatrixHeight - 2; y >= 0; y--) {
      for (int x = 0; x < kMatrixWidth; x++) {
        leds[XY(x, y + 1)] = leds[XY(x, y)];
      }
    }
  }
}
