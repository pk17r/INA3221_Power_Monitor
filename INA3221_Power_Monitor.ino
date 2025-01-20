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
bool displaySet = false;
float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float power_mW = 0;

int counter_LED = 0;  //cycles through 0 -> 1 -> 2 ->0
bool LED_On = true;
int position_Welcome_Screen = 1;  //sequential cycles 0 -> 1 -> 2 -> 1 -> 0
bool welcome_screen_going_forward = true;
int counter_Welcome_Screen = 0;
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
  displaySet = true;
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

  MeasurementPageSetup();
}

void welcomeScreen()
{
  display.setFont(&FreeSansBoldOblique12pt7b);
  display.clearDisplay();
  //Welcome Screen!
  display.setTextSize(1);             // Draw 2X-scale text
  display.setCursor(position_Welcome_Screen * 12,18);             // Start at top-left corner
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.print("INA3221");

  display.setCursor((3 - position_Welcome_Screen) * 12,39);             // Start at top-left corner
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.print("Power");

  display.setCursor(position_Welcome_Screen * 12,62);             // Start at top-left corner
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
float displayed_V[3] = {0,0,0};
float displayed_I[3] = {0,0,0};

const int kLetterSize = 10;
int ITextSize(float val) {
  if(val < 10)
    return  3*kLetterSize;
  else if(val < 100)
    return  2*kLetterSize;
  else if(val < 1000)
    return  1*kLetterSize;
  else
    return  0*kLetterSize;
}
int VTextSize(float val) {
  if(val < 10)
    return  kLetterSize;
  else
    return  0;
}

void printVI(int channel, float V, float I) {
  // serial
  // Serial.print("CH");
  // Serial.print(channel);
  // Serial.print("   V ");
  // Serial.print(V,2);
  // Serial.print("V   I ");
  // Serial.print(I);
  // Serial.println("mA");
  // display
  const int V_pos = 20, I_pos = 80;
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(V_pos + VTextSize(displayed_V[channel - 1]), row_start + row_height * (channel + 1));
  display.print(displayed_V[channel - 1],2);
  display.setCursor(I_pos + ITextSize(displayed_I[channel - 1]), row_start + row_height * (channel + 1));
  display.print(displayed_I[channel - 1], 0);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(V_pos + VTextSize(V), row_start + row_height * (channel + 1));
  display.print(V,2);
  display.setCursor(I_pos + ITextSize(I), row_start + row_height * (channel + 1));
  display.print(I, 0);
  display.display();
  displayed_V[channel - 1] = V;
  displayed_I[channel - 1] = I;
}

bool display_star = false;
elapsedMillis elapsed_mil = 0;

void loop() {
  int channel = 0;
  printVI(1, ina3221.getVoltage(INA3221_CH1), ina3221.getCurrent(INA3221_CH1) * 1000);
  printVI(2, ina3221.getVoltage(INA3221_CH2), ina3221.getCurrent(INA3221_CH2) * 1000);
  printVI(3, ina3221.getVoltage(INA3221_CH3), ina3221.getCurrent(INA3221_CH3) * 1000);
  if(elapsed_mil > 1000) {
    if(display_star)
      display.setTextColor(SSD1306_BLACK);
    else
      display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, row_height);
    display.print('*');
    display_star = !display_star;
    elapsed_mil = 0;
  }
  // delay(250);
  // display.clearDisplay();
}
