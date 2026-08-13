/********************************************************

  Retro PC Clk-Controller - Version 0.1.0
  by Beavers Electric Dam https://www.youtube.com/@BeaversElectricDam
  https://github.com/BeaversElectricDam/Clk-Controller

  MIT License

  Copyright (c) 2026 Beavers Electric Dam

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*********************************************************/

#include <Wire.h>                     
#include <si5351.h>                   //https://github.com/etherkit/Si5351Arduino
#include <Adafruit_SSD1306.h>         //https://github.com/adafruit/Adafruit_SSD1306
#include <Adafruit_GFX.h>             //https://github.com/adafruit/Adafruit-GFX-Library

//Variables
#define IF  0
#define FREQ_INIT 80000000            //Initial frequency, can be changed to anything betweeh 1 MHz up to 160 MHz
#define FREQ_UPPER_LIMIT 160000000    //160 MHz upper limit. Not all Si5351 modules can handle 160MHz. You can lower this value according to the performance of your specific Si5351 module
#define FREQ_LOWER_LIMIT 1000000      //1 MHz lower limit
#define XT_CAL_F   280000             //Si5351 calibration factor. You might need to change this value according to you Si5351 modules specifications. Default is 280000
#define TINYFSTEP 100000              //Tiny step
#define BIGFSTEP 1000000              //Big step

//Control Panel buttons
#define tinyfrequp    A2              //0.1 MHz Freq Up button
#define tinyfreqdown  A3              //0.1 MHz Freq Down button
#define bigfrequp     A1              //1.0 MHz Freq Up button
#define bigfreqdown   A0              //1.0 MHz Freq Down button

//Setup Display
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

//Declare the Si5351 module
Si5351 si5351;

long interfreq = IF;
long tfstep = TINYFSTEP;
long bfstep = BIGFSTEP;
long cal = XT_CAL_F;
unsigned long cur_freq = FREQ_INIT;
unsigned long freqold;
unsigned int period = 100;   //Update display every 100ms
unsigned long time_now = 0;  //millis display active
unsigned long long pll_freq = 90000000000ULL;

void setup() {
  //Serial.begin(9600);

  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.display();

  show_splash_screen();

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(tinyfrequp, INPUT_PULLUP);
  pinMode(tinyfreqdown, INPUT_PULLUP);
  pinMode(bigfrequp, INPUT_PULLUP);
  pinMode(bigfreqdown, INPUT_PULLUP);
  
  //Initialize the SI5351 module. You might need to change the capacitive load according to you Si5351 modules specifications. Default is 8PF
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, cal);
  
  //Module has 3 clock outputs (CLK) to select from
  //1 = Enable, 0 = Disable    
  si5351.output_enable(SI5351_CLK0, 1);              
  si5351.output_enable(SI5351_CLK1, 0);             
  si5351.output_enable(SI5351_CLK2, 0);

  //Here we can change the modules drive strength
  //Setting this to max (8MA) is in most cases not enough to drive a motherboard on its own without amplification
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_6MA);  //Output current 2MA, 4MA, 6MA or 8MA

  //Pin configuration
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  //Enable interrupts
  sei();

  show_layout();
  showfreq();

  //Serial.println("Setup Done!");
}

void set_frequency(short command) {
  if(command > 0)
  {
    if (command == 1) cur_freq = cur_freq + tfstep;
    if (command == 10) cur_freq = cur_freq + bfstep;
  }
  else{
    if (command == -1) cur_freq = cur_freq - tfstep;
    if (command == -10) cur_freq = cur_freq - bfstep;
  }

  if (cur_freq < FREQ_LOWER_LIMIT) cur_freq = FREQ_LOWER_LIMIT;
  if (cur_freq > FREQ_UPPER_LIMIT) cur_freq = FREQ_UPPER_LIMIT;
}

void loop() {
  if (freqold != cur_freq) {
    time_now = millis();
    si5351.set_freq_manual((cur_freq + (interfreq * 1000ULL)) * 100ULL, pll_freq, SI5351_CLK0);
    freqold = cur_freq;
  }

  if ((time_now + period) > millis()) {
    showfreq();
    show_layout();
  }

  if (digitalRead(tinyfrequp) == 0)
  {
    set_frequency(1);
    delay(200);
  }
  else if (digitalRead(tinyfreqdown) == 0)
  {
    set_frequency(-1);
    delay(200);
  }
  else if (digitalRead(bigfrequp) == 0)
  {
    set_frequency(10);
    delay(200);
  }
  else if (digitalRead(bigfreqdown) == 0)
  {
    set_frequency(-10);
    delay(200);
  }
}

void showfreq() {
  unsigned int m = cur_freq / 1000000;
  unsigned int k = (cur_freq % 1000000) / 100000;

  display.clearDisplay();
  display.setTextSize(4);

  char buffer[10] = "";
  display.setCursor(0, 1); 
  sprintf(buffer, "%3d", m);
  display.print(buffer);
  buffer[10] = "";

  display.setTextSize(2);
  display.setCursor(66, 16); 
  sprintf(buffer, ".%1d", k);
  display.print(buffer);

}

void show_layout() {
  display.setTextColor(WHITE);
  display.drawLine(0, 32, 127, 32, WHITE);
  display.setTextSize(2);
  display.setCursor(20, 38);
  display.print("ClkCtl");
  display.setTextSize(1);
  display.setCursor(80, 55);
  display.print("v 0.1.0");
  display.setCursor(92, 16); 
  display.print("MHz"); 
  display.display();

}

void show_splash_screen() {
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(20, 4);
  display.print("ClkCtl");
  display.setTextSize(1);
  display.setCursor(45, 25);
  display.print("Beavers");
  display.setCursor(30, 35);
  display.print("Electric Dam");
  display.setTextSize(1);
  display.setCursor(70, 55);
  display.print("v 0.1.0");
  display.display();
  delay(3000);
  display.clearDisplay();
}