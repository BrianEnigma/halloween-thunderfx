#include <Adafruit_NeoPixel.h>
#include <WaveHC.h>
#include <WaveUtil.h>

#define LED_PIN A5
#define LENGTH (30 * 4)
#define BUTTON_PIN A3

#define BUTTON_TOLERANCE 10
#define BUTTON_FOOT 14
#define BUTTON_CUSTOM 427
#define BUTTON_UP 625
#define BUTTON_DOWN 823
#define BUTTON_MODE 225

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LENGTH, LED_PIN, NEO_GRB + NEO_KHZ800);

SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the volumes root directory
FatReader file;
WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

unsigned char currentMode = 0; // what's being displayed
uint32_t customColor = 0x1E1E1E; // dim gray

void error (const char *s) 
{
    // do nothing
} 

void setup() {
    Serial.begin(9600);
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    digitalWrite(BUTTON_PIN, HIGH); // pullup

    //  if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
    if (!card.init()) {         //play with 8 MHz spi (default faster!)  
        error("Card init. failed!");  // Something went wrong, lets print out why
    }
    // enable optimize read - some cards may timeout. Disable if you're having problems
    card.partialBlockRead(true);
    // Now we will look for a FAT partition!
    uint8_t part;
    for (part = 0; part < 5; part++) {   // we have up to 5 slots to look in
        if (vol.init(card, part)) 
            break;                           // we found one, lets bail
    }
    if (part == 5) {                     // if we ended up not finding one  :(
        error("No valid FAT partition!");  // Something went wrong, lets print out why
    }
    // Try to open the root directory
    if (!root.openRoot(vol)) {
        error("Can't open root dir!");      // Something went wrong,
    }
}

void play(unsigned char fileNumber /*one-based*/)
{
    char filename[] = "1.wav";
    
    // Already playing...
    if (wave.isplaying)
        return;
    
    filename[0] = fileNumber + '0';
    if (!file.open(root, filename)) {      // open the file in the directory
        error("file.open failed");          // something went wrong
    }
    if (!wave.create(file)) {            // Figure out, is it a WAV proper?
        error("Not a valid WAV");     // ok skip it
    } else {
        wave.play();                       // make some noise!
        delay(100);
    }
}

int getButton()
{
    int val = analogRead(BUTTON_PIN);
    if (val >= BUTTON_FOOT - BUTTON_TOLERANCE && val <= BUTTON_FOOT + BUTTON_TOLERANCE)
        return BUTTON_FOOT;
    if (val >= BUTTON_CUSTOM - BUTTON_TOLERANCE && val <= BUTTON_CUSTOM + BUTTON_TOLERANCE)
        return BUTTON_CUSTOM;
    if (val >= BUTTON_UP - BUTTON_TOLERANCE && val <= BUTTON_UP + BUTTON_TOLERANCE)
        return BUTTON_UP;
    if (val >= BUTTON_DOWN - BUTTON_TOLERANCE && val <= BUTTON_DOWN + BUTTON_TOLERANCE)
        return BUTTON_DOWN;
    if (val >= BUTTON_MODE - BUTTON_TOLERANCE && val <= BUTTON_MODE + BUTTON_TOLERANCE)
        return BUTTON_MODE;
    return 0;
}

void setAllColor(uint32_t c)
{
    int pos;
    for (pos = 0; pos < LENGTH; pos++)
      strip.setPixelColor(pos, strip.Color(c >> 16, (c >> 8) & 0xFF, c & 0xFF));
    strip.show();
}

//unsigned char lightningNumber = 0;
void lightning()
{
    int count;
    int pos;
    int randVal;
    for (pos = 0; pos < LENGTH; pos++)
      strip.setPixelColor(pos, strip.Color(255, 255, 255));
    strip.show();
    play(1); //lightningNumber + 1);
    //lightningNumber = (lightningNumber + 1) % 3;
    for (pos = 0; pos < LENGTH; pos++)
        strip.setPixelColor(pos, strip.Color(128, 128, 128));
    strip.show();
    delay(300);
    for (pos = 0; pos < LENGTH; pos++)
      strip.setPixelColor(pos, strip.Color(0, 0, 0));
    strip.show();
}

void customColorLoop()
{
    int buttonValue;
    unsigned char octet = 16;
    uint32_t selectedColorValue;
    uint32_t octetAdjustValue;
    
    setAllColor(0);
    // Flash current selection
    setAllColor(0x0000FFUL << octet);
    delay(300);
    setAllColor(customColor);
    while(1)
    {
        buttonValue = getButton();
        if (buttonValue != 0)
        {
            selectedColorValue = customColor;
            selectedColorValue >>= octet;
            selectedColorValue &= 0xFFUL;
            octetAdjustValue = 0x0A;
            octetAdjustValue <<= octet;
            //Serial.println(selectedColorValue);
            //Serial.println(octetAdjustValue, HEX);
            switch(buttonValue)
            {
                case BUTTON_CUSTOM:
                    if (0 == octet)
                        octet = 16;
                    else
                        octet -= 8;
                    // Flash current selection
                    setAllColor(0x0000FFUL << octet);
                    delay(300);
                    setAllColor(customColor);
                    break;
                case BUTTON_UP:
                    if (selectedColorValue < 240)
                        customColor += octetAdjustValue;
                    setAllColor(customColor);
                    break;
                case BUTTON_DOWN:
                    if (selectedColorValue >= 10)
                        customColor -= octetAdjustValue;
                    setAllColor(customColor);
                    break;
                default:
                    setAllColor(0);
                    return;
            }
            Serial.println(customColor, HEX);
        }
        delay(100);
    }
}

int modeChasePosition = 0;
int modeChaseDirection = 1;
void modeChaseTwo()
{
    setAllColor(0);
    strip.setPixelColor(modeChasePosition, strip.Color(255, 0, 0));
    strip.setPixelColor(LENGTH - modeChasePosition - 1, strip.Color(0, 255, 0));
    strip.show();
    
    modeChasePosition += modeChaseDirection;
    if (modeChasePosition >= LENGTH)
    {
        modeChasePosition = LENGTH - 2;
        modeChaseDirection = -1;
    } else if (modeChasePosition < 0)
    {
        modeChasePosition = 1;
        modeChaseDirection = 1;
    }
    delay(40);
}

int redPos = 0;
int greenPos = LENGTH - 1;
int bluePos = LENGTH / 2;
int whitePos = LENGTH / 2;
int redDirection = 1;
int greenDirection = -1;
int blueDirection = 1;
int whiteDirection = -1;
void modeChaseFour()
{
    setAllColor(0);
    strip.setPixelColor(redPos, strip.Color(255, 0, 0));
    strip.setPixelColor(greenPos, strip.Color(0, 255, 0));
    strip.setPixelColor(bluePos, strip.Color(0, 0, 255));
    strip.setPixelColor(whitePos, strip.Color(255, 255, 255));
    strip.show();
  
    redPos += redDirection;
    greenPos += greenDirection;
    bluePos += blueDirection;
    whitePos += whiteDirection;
    if (0 == redPos) redDirection = 1;
    if (0 == greenPos) greenDirection = 1;
    if (0 == bluePos) blueDirection = 1;
    if (0 == whitePos) whiteDirection = 1;
    if (LENGTH - 1 == redPos) redDirection = -1;
    if (LENGTH - 1 == greenPos) greenDirection = -1;
    if (LENGTH - 1 == bluePos) blueDirection = -1;
    if (LENGTH - 1 == whitePos) whiteDirection = -1;
    delay(40);
}

int redBreatheValue = 10;
int redBreatheDirection = 1;
#define RED_BREATHE_MAX 70
#define RED_BREATHE_MIN 20
#define RED_BREATHE_STEP 1
#define RED_BREATHE_DELAY 70
void redBreathe(int shiftAmount)
{
    uint32_t color = redBreatheValue;
    color <<= shiftAmount;
    setAllColor(color);
    redBreatheValue += redBreatheDirection * RED_BREATHE_STEP;
    if (redBreatheValue > RED_BREATHE_MAX)
    {
        redBreatheValue = RED_BREATHE_MAX;
        redBreatheDirection = -1;
    } else if (redBreatheValue < RED_BREATHE_MIN)
    {
        redBreatheValue = RED_BREATHE_MIN;
        redBreatheDirection = 1;
    }
    //Serial.println(redBreatheValue);
    //Serial.println(redBreatheDirection);
    delay(RED_BREATHE_DELAY);
}

int redOrangeMode = 0;
int redOrangeCounter = 0;
#define REDORANGE_RED_VALUE 0x77
#define REDORANGE_GREEN_VALUE 0x28
#define REDORANGE_DELAY_COUNT 100
void redOrange()
{
    uint32_t thisColor = REDORANGE_RED_VALUE;
    int counter;
    thisColor <<= 16;
    switch (redOrangeMode)
    {
        case 0:
            setAllColor(thisColor);
            redOrangeCounter += 1;
            if (redOrangeCounter >= REDORANGE_DELAY_COUNT)
            {
                for (counter = 0; counter <= REDORANGE_GREEN_VALUE; counter++)
                {
                    thisColor = REDORANGE_RED_VALUE;
                    thisColor <<= 8;
                    thisColor += counter;
                    thisColor <<= 8;
                    setAllColor(thisColor);
                    delay(50);
                }
                redOrangeMode = 1;
                redOrangeCounter = 0;
            }
            break;
        case 1:
            thisColor = REDORANGE_RED_VALUE;
            thisColor <<= 8;
            thisColor += REDORANGE_GREEN_VALUE;
            thisColor <<= 8;
            setAllColor(thisColor);
            redOrangeCounter += 1;
            if (redOrangeCounter >= REDORANGE_DELAY_COUNT)
            {
                for (counter = REDORANGE_GREEN_VALUE; counter >= 0; counter--)
                {
                    thisColor = REDORANGE_RED_VALUE;
                    thisColor <<= 8;
                    thisColor += counter;
                    thisColor <<= 8;
                    setAllColor(thisColor);
                    delay(50);
                }
                redOrangeMode = 0;
                redOrangeCounter = 0;
            }
            break;
    }
    delay(100);
}

#define MODES 6
void loop()
{
    switch (currentMode)
    {
        case 0: redBreathe(16); break;
        case 1: redOrange(); break;
        case 2: redBreathe(8); break;
        case 3: redBreathe(0); break;
        case 4: modeChaseTwo(); break;
        case 5: modeChaseFour(); break;
        
    }
    switch(getButton())
    {
        case BUTTON_MODE: currentMode = (currentMode + 1) % MODES; delay(100); break;
        case BUTTON_FOOT: lightning(); break;
        case BUTTON_CUSTOM: customColorLoop(); break;
    }
}


