#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBoldOblique12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_SSD1306.h>
#include <elapsedMillis.h>
#include <EasyNeoPixels.h>
#include <INA3221.h>

// Set I2C address to 0x40
INA3221 ina3221(INA3221_ADDR40_GND);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const bool waitForCOMPort = false;
const int led_pin = 16;
float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float power_mW = 0;

int counter_LED = 0;  //cycles through 0 -> 1 -> 2 ->0
bool LED_On = true;
int position_Welcome_Screen = 1;  //sequential cycles 0 -> 1 -> 2 -> 1 -> 0
bool welcome_screen_going_forward = true;
int counter_Welcome_Screen = 1;
unsigned long lastWelcomeScreenAtTimeMillis = 0;

int row_start = -3;
int row_height = 16;

void setup() {
  Serial.begin(115200);
  setupEasyNeoPixels(led_pin, 1);
  int cpu_speed = rp2040.f_cpu();
  if(waitForCOMPort) {
    while(!Serial) {
      writeEasyNeoPixel(0, 255, 255, 255);
      delay(500);
      writeEasyNeoPixel(0, 0, 0, 0);
      delay(500);
    };
  }
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  Serial.println(F("SSD1306 allocation success"));
  // Clear the buffer
  display.clearDisplay();
  delay(100);

  welcomeScreen();

  //initialize current voltage power sensor
  ina3221.begin(&Wire);
  ina3221.reset();
  ina3221.setShuntRes(20, 20, 20);
  ina3221.setFilterRes(0, 0, 0);
  unsigned int manu_id = ina3221.getReg(INA3221_REG_MANUF_ID);
  unsigned int die_id = ina3221.getDieID();

  Serial.print("INA3221_REG_MANUF_ID : ");
  Serial.println(manu_id);
  Serial.print("die_id : ");
  Serial.println(die_id);

  delay(1000);

  // MeasurementPageSetup();
}

void welcomeScreen() {
  display.setFont(&FreeSansBoldOblique12pt7b);
  display.clearDisplay();
  //Welcome Screen!
  display.setTextSize(1);             // Draw 2X-scale text
  display.setCursor(position_Welcome_Screen * 12,15);             // Start at top-left corner
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.print("INA3221");

  display.setCursor((3 - position_Welcome_Screen) * 12,37);             // Start at top-left corner
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.print("Power");

  display.setCursor(position_Welcome_Screen * 12,61);             // Start at top-left corner
  display.setTextColor(SSD1306_WHITE);
  display.print("Monitor!");
  display.display();
}

void MeasurementPageSetup() {
  // Clear the buffer
  display.clearDisplay();
  delay(100);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(40, row_start + row_height);
  display.print("V");
  display.setCursor(80, row_start + row_height);
  display.print("I-mA");
  int channel = 1;
  display.setCursor(0, row_start + row_height * (channel + 1));
  display.print(channel);
  channel++;
  display.setCursor(0, row_start + row_height * (channel + 1));
  display.print(channel);
  channel++;
  display.setCursor(0, row_start + row_height * (channel + 1));
  display.print(channel);
}

void ChangeLedColor()
{
  int minColorLevel = 0, maxColorLevel = 15;
  int i = random(minColorLevel, maxColorLevel);
  int j = random(minColorLevel, maxColorLevel);
  int k = random(minColorLevel, maxColorLevel);
  //Green Red Blue
  writeEasyNeoPixel(0, i, j, k);
}

// current displayed values
float measured_V[3] = {0,0,0};
float measured_I[3] = {0,0,0};
float displayed_V[3] = {0,0,0};
float displayed_I[3] = {0,0,0};

const int kLetterSize = 10;
int ITextSize(float val) {
  int size = 0;
  if(val < 0) {
    size = 6;
    val = -val;
  }
  if(val < 10)
    return  size + 0*kLetterSize;
  else if(val < 100)
    return  size + 1*kLetterSize;
  else if(val < 1000)
    return  size + 2*kLetterSize;
  else
    return  size + 3*kLetterSize;
}
int VTextSize(float val) {
  if(val < 10)
    return  0;
  else
    return  kLetterSize;
}

void printVI(int channel_i) {
  // display
  const int V_pos = 30, I_pos = 110;
  // clear last values
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(V_pos - VTextSize(displayed_V[channel_i]), row_start + row_height * (channel_i + 2));
  display.print(displayed_V[channel_i],2);
  display.setCursor(I_pos - ITextSize(displayed_I[channel_i]), row_start + row_height * (channel_i + 2));
  display.print(displayed_I[channel_i], 0);
  // display measured values
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(V_pos - VTextSize(measured_V[channel_i]), row_start + row_height * (channel_i + 2));
  display.print(measured_V[channel_i],2);
  display.setCursor(I_pos - ITextSize(measured_I[channel_i]), row_start + row_height * (channel_i + 2));
  display.print(measured_I[channel_i], 0);
  display.display();
  // record measured values
  displayed_V[channel_i] = measured_V[channel_i];
  displayed_I[channel_i] = measured_I[channel_i];
}

bool display_star = false;
elapsedMillis elapsed_mil = 0, zero_values_elapsed_mil = 0;

void loop() {
  for (int channel_i = 0; channel_i < 3; channel_i++) {
    measured_V[channel_i] = ina3221.getVoltage(static_cast<ina3221_ch_t>(channel_i));
    measured_I[channel_i] = ina3221.getCurrent(static_cast<ina3221_ch_t>(channel_i)) * 1000;
    // serial
    // Serial.print("CH");
    // Serial.print(channel_i + 1);
    // Serial.print("   V ");
    // Serial.print(measured_V[channel_i],2);
    // Serial.print("V   I ");
    // Serial.print(measured_I[channel_i]);
    // Serial.println("mA");

    if(channel_i == 0) {
      current_mA = abs(measured_I[channel_i]);
      busvoltage = measured_V[channel_i];
    }
    else {
      if(abs(measured_I[channel_i]) > current_mA)
        current_mA = abs(measured_I[channel_i]);
      if(measured_V[channel_i] > busvoltage)
        busvoltage = measured_V[channel_i];
    }
  }
  // Serial.print("busvoltage ");
  // Serial.print(busvoltage,2);
  // Serial.print("V   current_mA ");
  // Serial.println(current_mA);

  // if nothing is happening then show Welcome screen as screensaver
  if(busvoltage > 0.1 || current_mA > 0)
  {
    zero_values_elapsed_mil = 0; 
  }

  if(zero_values_elapsed_mil > 10000)   // show screensaver
  {
    if(counter_Welcome_Screen == 0) {
      display.clearDisplay(); // Clear display buffer
      elapsed_mil = 0;
      // display.dim(true);
      //dim color changes
      ChangeLedColor();
      counter_LED = 0;
      LED_On = true;
      // display.invertDisplay(false);
      welcomeScreen();
    }

    if(elapsed_mil > 1000) {
      //dim color changes
      ChangeLedColor();
      counter_LED = 0;
      LED_On = true;
      elapsed_mil = 0;
    }

    if(millis() - lastWelcomeScreenAtTimeMillis > 2000)
    {
      lastWelcomeScreenAtTimeMillis = millis();
      
      if(position_Welcome_Screen == 0)
        welcome_screen_going_forward = true;
      else if(position_Welcome_Screen == 2)
        welcome_screen_going_forward = false;
      
      if(welcome_screen_going_forward)
        position_Welcome_Screen++;
      else
        position_Welcome_Screen--;
      
      if(counter_Welcome_Screen < 7)
      {
        counter_Welcome_Screen++;
        welcomeScreen();
      }
      else
      {
        counter_Welcome_Screen = 1;
        display.clearDisplay();
        display.setCursor(0,37);             // Start at top-left corner
        display.setTextColor(SSD1306_WHITE);        // Draw white text
        display.print("  INA3221");
        display.display();
      }
    }
  }
  else    // show measurements
  {
    // first time
    if(counter_Welcome_Screen != 0) {
      // Clear display
      MeasurementPageSetup();
      counter_Welcome_Screen = 0;
      elapsed_mil = 0;
      display_star = false;
      // display.dim(false);
      counter_LED = 0;
      LED_On = true;
    }

    for (int channel_i = 0; channel_i < 3; channel_i++)
      printVI(channel_i);

    
    // if(counter_LED == 0) {
    //   if(LED_On)
    //   {
    //     int green = min(max(round((1000-current_mA)/1000*current_mA/2),0),255);
    //     int red = min(max(round((current_mA-100)/1000*current_mA/2),0),255);
    //     writeEasyNeoPixel(0, green, red, 0);    //Green Red Blue
    //   }
    //   else
    //     writeEasyNeoPixel(0, 0, 0, 0);
    //   counter_LED++;
    //   if(counter_LED > 2) {
    //     counter_LED = 0;
    //     LED_On = !LED_On;
    //   }
    // }

    if(elapsed_mil > 1000) {
      if(display_star) {
        writeEasyNeoPixel(0, 0, 0, 0);
        // display.setTextColor(SSD1306_BLACK);
      }
      else {
        //blink LED with Amp equivalent green to red color
        int green = min(max(round((1000-current_mA)/1000*current_mA/2),0),255);
        int red = min(max(round((current_mA-100)/1000*current_mA/2),0),255);
        writeEasyNeoPixel(0, green, red, 0);    //Green Red Blue
        // display.setTextColor(SSD1306_WHITE);
      }
      // display.setCursor(10, row_height);
      // display.print("*");
      display_star = !display_star;
      elapsed_mil = 0;
    }
  }

  // delay(1000);
  // display.clearDisplay();
}
