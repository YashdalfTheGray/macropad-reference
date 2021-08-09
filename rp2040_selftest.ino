#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Wire.h>
#include "digikey.h"

uint8_t allpins[] = {1,2,3,4,5,6,7,8,9,10,11,12};

// Create the neopixel strip with the built in definitions NUM_NEOPIXEL and PIN_NEOPIXEL
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_NEOPIXEL, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Create the OLED display
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &SPI1, OLED_DC, OLED_RST, OLED_CS);

// Create the rotary encoder
RotaryEncoder encoder(PIN_ROTA, PIN_ROTB, RotaryEncoder::LatchMode::FOUR3);
void checkPosition() {  encoder.tick(); } // just call tick() to check the state.
// our encoder position state
int encoder_pos = 0;

uint8_t j = 0;
bool i2c_found[128] = {false};

bool RUN_TEST = false;

void setup() {
  Serial.begin(115200);
  //while (!Serial) { yield(); delay(10); }     // wait till serial port is opened
  delay(100);  // RP2040 delay is not a bad idea

  Serial.println("Adafruit Macropad with RP2040");
  
  // start pixels!
  pixels.begin();
  pixels.setBrightness(40);
  pixels.show(); // Initialize all pixels to 'off'

  // Start OLED
  display.begin(0, true); // we dont use the i2c address but we will reset!
  display.display();
  display.setTextWrap(false);
  display.setTextColor(SH110X_WHITE, SH110X_BLACK); // white text, black background

  // set rotary encoder inputs and interrupts
  pinMode(PIN_ROTA, INPUT_PULLUP);
  pinMode(PIN_ROTB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTA), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTB), checkPosition, CHANGE);  

  pinMode(PIN_SWITCH, INPUT_PULLUP);
  for (int i=0; i<10; i++) {
    if (testpins(2, 4)) {
      RUN_TEST = true;
      break;
    }
  }
  if (RUN_TEST) {
    display.setTextSize(2);
  } else {
    // set all mechanical keys to inputs
    for (uint8_t i=0; i<=12; i++) {
      pinMode(i, INPUT_PULLUP);
    }
    // We will use I2C for scanning the Stemma QT port
    Wire.begin();
    display.setTextSize(1);
    delay(1000);
    display.clearDisplay();
    display.fillScreen(1);
    display.drawBitmap(0, 0, digikey, 128, 64, 0);
    display.display();
    delay(1000);
  }
}


void loop() {
  if (RUN_TEST) {
    return runTest();
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("* Adafruit Macropad *");
  
  encoder.tick();          // check the encoder
  int newPos = encoder.getPosition();
  if (encoder_pos != newPos) {
    Serial.print("Encoder:");
    Serial.print(newPos);
    Serial.print(" Direction:");
    Serial.println((int)(encoder.getDirection()));
    encoder_pos = newPos;
  }
  display.setCursor(0, 8);
  display.print("Rotary encoder: ");
  display.print(encoder_pos);

  // Scanning takes a while so we don't do it all the time
  if ((j & 0x3F) == 0) {
    Serial.println("Scanning I2C: ");
    Serial.print("Found I2C address 0x");
    for (uint8_t address = 0; address <= 0x7F; address++) {
      Wire.beginTransmission(address);
      i2c_found[address] = (Wire.endTransmission () == 0);
      if (i2c_found[address]) {
        Serial.print("0x");
        Serial.print(address, HEX);
        Serial.print(", ");
      }
    }
    Serial.println();
  }
  
  display.setCursor(0, 16);
  display.print("I2C Scan: ");
  for (uint8_t address=0; address <= 0x7F; address++) {
    if (!i2c_found[address]) continue;
    display.print("0x");
    display.print(address, HEX);
    display.print(" ");
  }
  
  // check encoder press
  display.setCursor(0, 24);
  if (!digitalRead(PIN_SWITCH)) {
    Serial.println("Encoder button");
    display.print("Encoder pressed ");
    pixels.setBrightness(255);     // bright!
  } else {
    pixels.setBrightness(80);
  }

  for(int i=0; i< pixels.numPixels(); i++) {
    pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
  }
  
  for (int i=1; i<=12; i++) {
    if (!digitalRead(i)) { // switch pressed!
      Serial.print("Switch "); Serial.println(i);
      pixels.setPixelColor(i-1, 0xFFFFFF);  // make white
      // move the text into a 3x4 grid
      display.setCursor(((i-1) % 3)*48, 32 + ((i-1)/3)*8);
      display.print("KEY");
      display.print(i);
    }
  }

  // show neopixels, incredment swirl
  pixels.show();
  j++;

  // display oled
  display.display();
}

bool switchesOK = false, encoderOK = false, pushOK= false;
void runTest(void) {
  delay(100);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Macropad!");
  display.display();
  
  display.setCursor(0, 16);
  display.print("Switches ");
  if (! switchesOK) {
    display.display();
    if (! testpins(1, 3)) return;
    if (! testpins(2, 4)) return;
    if (! testpins(5, 7)) return;
    if (! testpins(6, 8)) return;
    if (! testpins(9, 11)) return;
    if (! testpins(10, 12)) return;
    switchesOK = true;
  }
  display.print("OK");
  display.display();

  display.setCursor(0, 32);
  display.print("Turn plz ");
  if (! encoderOK) {
    display.display();
    encoder.tick();          // check the encoder
    int newPos = encoder.getPosition();
    if (encoder_pos != newPos) {
      Serial.print("Encoder:");
      Serial.print(newPos);
      Serial.print(" Direction:");
      Serial.println((int)(encoder.getDirection()));
      encoder_pos = newPos;
    }
    int max_neopixel = min(12, abs(encoder_pos));
    for(int i=0; i< max_neopixel; i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    if (max_neopixel != 12) {
      return; // more to go!
    }
    encoderOK = true;
  }
  display.print("OK");
  display.display();

  display.setCursor(0, 48);
  display.print("Push plz ");
  if (! pushOK) {
    display.display();
    if (digitalRead(PIN_SWITCH)) return;
    
    Serial.println("Encoder button");
    pinMode(PIN_SPEAKER_ENABLE, OUTPUT);
    digitalWrite(PIN_SPEAKER_ENABLE, HIGH);
    tone(PIN_SPEAKER, 988, 100);  // tone1 - B5
    delay(100);
    tone(PIN_SPEAKER, 1319, 200); // tone2 - E6
    delay(200);
    digitalWrite(PIN_SPEAKER_ENABLE, LOW);
    pushOK = true;
  }
  display.print("OK");
  display.display();

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("**********");
  display.println(" MACROPAD ");
  display.println("TEST PASS!");
  display.println("**********");
  display.display();
  Serial.println("TEST OK!");
  while (1) {
    for(int i=0; i<12; i++) {
      pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    j++;
    delay(10);
    yield();
  }
}


boolean testpins(uint32_t a, uint32_t b) {

  Serial.print("\tTesting ");
  Serial.print(a, DEC);
  Serial.print(" & ");
  Serial.println(b, DEC);
  

  // set both to inputs
  // turn on 'a' pullup
  pinMode(a, INPUT_PULLUP);
  pinMode(b, INPUT_PULLUP);
  delay(3);
  pinMode(b, INPUT);
  
  // verify neither are grounded
  if (! digitalRead(a) || ! digitalRead(b)) {
    Serial.println("Ground test 1 fail");
    return false;
  }
  
  
  // make a an output, b with a pullup
  pinMode(b, INPUT_PULLUP);
  pinMode(a, OUTPUT);
  digitalWrite(a, LOW);
  delay(3);

  int br = digitalRead(b);
  
  // make sure b is low
  if (br) {
    Serial.print(F("Low test fail on "));
    if (br) Serial.println(b, DEC);
    return false;
  }

  // make b an output, a with a pullup
  pinMode(a, INPUT_PULLUP);
  pinMode(b, OUTPUT);
  digitalWrite(b, LOW);
  delay(3);
  
  int ar = digitalRead(a);
  
  // make sure b is low
  if (ar) {
    Serial.print(F("Low test fail on "));
    if (ar) Serial.println(a, DEC);
    return false;
  }
 
  // a is an input, b is an output
  pinMode(a, INPUT);
  pinMode(b, OUTPUT);
  digitalWrite(b, HIGH);
  // verify a is not grounded
  delay(3);
  if (! digitalRead(a)) {
    Serial.println(F("Ground2 test fail"));
    return false;
  }
  
  // make sure no pins are shorted to pin a or b
  for (uint8_t i=0; i<sizeof(allpins); i++)  {
    pinMode(allpins[i], INPUT_PULLUP);
  }

  pinMode(a, OUTPUT);
  digitalWrite(a, LOW);
  pinMode(b, OUTPUT);
  digitalWrite(b, LOW);
  delay(3);
  for (uint8_t i=0; i<sizeof(allpins); i++) {
    if ((allpins[i] == a) || (allpins[i] == b)) {
      continue;
    }
 
    //Serial.print("Pin #"); Serial.print(allpins[i]);
    //Serial.print(" -> ");
    //Serial.println(digitalRead(allpins[i]));
    if (! digitalRead(allpins[i])) {
      Serial.print(allpins[i]);
      Serial.println(F(" is shorted?"));
      while(1);
      return false;
    }
  }
  pinMode(a, INPUT);
  pinMode(b, INPUT);

  return true;
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  uint32_t c = 0;
  if (WheelPos < 85) {
     c = (255 - WheelPos*3) & 0xFF;
     c <<= 8;
     c |= (WheelPos * 3) & 0xFF;
     c <<= 8;
  } else if (WheelPos < 170) {
     WheelPos -= 85;
     c <<= 8;
     c |= (255 - WheelPos*3) & 0xFF;
     c <<= 8;
     c |= (WheelPos * 3) & 0xFF;
  } else {
     WheelPos -= 170;
     c = (WheelPos * 3) & 0xFF;
     c <<= 16;
     c |= (255 - WheelPos*3) & 0xFF;
  }
  return c;
}
