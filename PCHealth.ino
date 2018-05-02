#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <string.h>
#include <IRremote.h>

#define DHTPIN 3
#define DHTTYPE DHT11
#define RELAY 5
#define RGBPIN 4
#define NUMPIXELS 16
#define ERROR_UPTIME 0

LiquidCrystal_I2C lcd(0x3F,16,2);
DHT dht(DHTPIN, DHTTYPE);
IRrecv irrecv(2);
decode_results results;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, RGBPIN, NEO_GRB + NEO_KHZ800);

byte temp;
byte hum;
char inData[82];       
int PCdata[20];        
byte index = 0;
byte display_mode = 1;
String string_convert;
unsigned long timeout, uptime_timer;
boolean reDraw_flag = 1, updateDisplay_flag, timeOut_flag = 1, updateTemp_flag;
byte lines[] = {4, 5, 7, 6};
String perc;
unsigned long sec, mins, hrs;

byte temperature[8] =
{
  B00100,
  B01010,
  B01010,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110
};

byte humidity[8] =
{
  B00100,
  B00100,
  B01010,
  B01010,
  B10001,
  B10001,
  B01110,
};

byte right_empty[8] = 
{
  B11111,  
  B00001,  
  B00001,  
  B00001,  
  B00001,  
  B00001,  
  B00001,  
  B11111
};

byte left_empty[8] = 
{
  B11111,  
  B10000,  
  B10000,  
  B10000,  
  B10000,  
  B10000,  
  B10000,  
  B11111
};

byte center_empty[8] = 
{
  B11111, 
  B00000,  
  B00000, 
  B00000,  
  B00000,  
  B00000,  
  B00000,  
  B11111
};

byte row[8] = 
{
  B11111,  
  B11111,  
  B11111,  
  B11111,  
  B11111,  
  B11111,  
  B11111, 
  B11111
};

void setup()
{
  lcd.init();
  lcd.backlight();
  lcd.clear();           
  showLogo();           
  dht.begin();
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);
  irrecv.enableIRIn();
  strip.begin();
  strip.show();
  strip.show();
  Serial.begin(9600);
}

void loop()
{ 
  parsing();
  updateDisplay();
  getTemperature();
  IRcontroler();
  timeoutTick();                     
}

void setRGB(int R, int G, int B)
{
  for(int i=0;i<NUMPIXELS;i++)
  {   
    strip.setPixelColor(i, strip.Color(R,G,B)); 
    strip.show();
  }
}

void IRcontroler(){
  if ( irrecv.decode( &results )) {
    switch ( results.value ) {
    case 0xFFA857: // Button -        
        digitalWrite(RELAY, LOW);
        break;
    case 0xFFE01F: // Button +
        digitalWrite(RELAY, HIGH);        
        break;
    case 0xFF30CF: // Button 1 - WHITE
        setRGB(255, 255, 255);       
        break;
    case 0xFF18E7: // Button 2 - GREEN
        setRGB(0, 255, 0);        
        break;
    case 0xFF7A85: // Button 3 - BLUE
        setRGB(0, 0, 255);        
        break;
    case 0xFF10EF: // Button 4 - AQUA
        setRGB(0, 255, 255);        
        break;
    case 0xFF38C7: // Button 5 - PURP  
        setRGB(204, 0, 255);       
        break;
    case 0xFF5AA5: // Button 6 - RED 
        setRGB(255, 0, 0);       
        break;
    case 0xFF42BD: // Button 7 - ORANGE
        setRGB(255, 191, 0);        
        break;
    case 0xFF4AB5: // Button 8 - PINK
        setRGB(255, 0, 191);        
        break;
    case 0xFF52AD: // Button 9 - YELLOW
        setRGB(255, 255, 0);        
        break;    
    case 0xFF6897: // Button 0 - OFF
        setRGB(0, 0, 0);       
        break;
    case 0xFFA25D: // CH- -
        display_mode--;
        reDraw_flag = 1;
        if (display_mode < 1) display_mode = 4;
        break;
    case 0xFF629D: // CH - 
        break;
    case 0xFFE21D: // CH - +
        display_mode++;
        reDraw_flag = 1;
        if (display_mode > 4) display_mode = 1;      
        break;
    case 0xFF22DD: // VOL- noBacklight
        lcd.noBacklight();      
        break; 
    case 0xFF02FD: // VOL+ - backlight       
        lcd.backlight();
        break;
    case 0xFFC23D: // >|| -         
        break;
    case 0xFF906F: // EQ - 
        break;                                     
    }    
    irrecv.resume();
  }
}

void showLogo() {
  lcd.setCursor(4, 0);
  lcd.print("DENIS");
  lcd.setCursor(5, 1);
  lcd.print("STIFLER");
  delay(9000);
}

void parsing() {
  while (Serial.available() > 0) {
    char aChar = Serial.read();
    if (aChar != 'E') {
      inData[index] = aChar;
      index++;
      inData[index] = '\0';
    } else {
      char *p = inData;
      char *str;
      index = 0;
      String value = "";
      while ((str = strtok_r(p, ";", &p)) != NULL) {
        string_convert = str;
        PCdata[index] = string_convert.toInt();
        index++;
      }
      index = 0;
      updateDisplay_flag = 1;
      updateTemp_flag = 1;
    }
    if (!timeOut_flag) {                                
      if (ERROR_UPTIME) uptime_timer = millis();       
    }
    timeout = millis();
    timeOut_flag = 1;
  }
}

void updateDisplay() {
  if (updateDisplay_flag) {
    if (reDraw_flag) {
      lcd.clear();
      switch (display_mode) {
        case 1: draw_labels_11();
          break;
        case 2: draw_labels_12();
          break;
        case 3: draw_labels_22();
          break;
        case 4: draw_temp_hum_labels();
          break;
      }
      reDraw_flag = 0;
    }
    switch (display_mode) {
      case 1: draw_stats_11();
        break;
      case 2: draw_stats_12();
        break;
      case 3: draw_stats_22();
        break;
      case 4: draw_stats_21(); 
        break;
    }
    updateDisplay_flag = 0;
  }
}

// Draw CPU / GPU
void draw_labels_11() {
  lcd.createChar(1, left_empty);
  lcd.createChar(2, center_empty);
  lcd.createChar(3, right_empty);
  lcd.createChar(4, row);
  lcd.setCursor(0, 0);
  lcd.print("CPU:");
  lcd.setCursor(0, 1);
  lcd.print("GPU:");
}

// Draw TEMP / HUM
void draw_temp_hum_labels() {
  lcd.setCursor(0, 0);                     
  lcd.print("TEMP:");
  lcd.setCursor(7, 0);
  lcd.print("HUM:");
}

// Draw GPUmem / RAMuse
void draw_labels_12() {
  lcd.createChar(1, left_empty);
  lcd.createChar(2, center_empty);
  lcd.createChar(3, right_empty);
  lcd.createChar(4, row);
  lcd.setCursor(0, 0);
  lcd.print("GPUmem:");
  lcd.setCursor(0, 1);
  lcd.print("RAMuse:");
}

// Draw Motherboard / HDD / Uptime
void draw_labels_22() {
  lcd.createChar(1, left_empty);
  lcd.createChar(2, center_empty);
  lcd.createChar(3, right_empty);
  lcd.createChar(4, row);
  lcd.setCursor(0, 0);
  lcd.print("M:");
  lcd.setCursor(7, 0);
  lcd.print("HM:");
  lcd.setCursor(0, 1);
  lcd.print("UPTIME:");
}

// Draw stats for CPU / GPU
void draw_stats_11() {
  lcd.setCursor(4, 0); lcd.print(PCdata[0]); lcd.write(223);
  lcd.setCursor(13, 0); lcd.print(PCdata[4]);
  if (PCdata[4] < 10) perc = "% ";
  else if (PCdata[4] < 100) perc = "%";
  else perc = "";  lcd.print(perc);
  lcd.setCursor(4, 1); lcd.print(PCdata[1]); lcd.write(223);
  lcd.setCursor(13, 1); lcd.print(PCdata[5]);
  if (PCdata[5] < 10) perc = "% ";
  else if (PCdata[5] < 100) perc = "%";
  else perc = "";  lcd.print(perc);

  for (int i = 0; i < 2; i++) {
    byte line = ceil(PCdata[lines[i]] / 16);
    lcd.setCursor(7, i);
    if (line == 0) lcd.write(1);
      else lcd.write(4);
    for (int n = 1; n < 5; n++) {
      if (n < line) lcd.write(4);
      if (n >= line) lcd.write(2);
    }
    if (line == 6) lcd.write(4);
      else lcd.write(3);
  }
}

// Draw stats for GPUmem / RAMuse
void draw_stats_12() {
  lcd.setCursor(13, 0); lcd.print(PCdata[7]);
  if (PCdata[7] < 10) perc = "% ";
  else if (PCdata[7] < 100) perc = "%";
  else perc = "";  lcd.print(perc);
  lcd.setCursor(13, 1); lcd.print(PCdata[6]);
  if (PCdata[6] < 10) perc = "% ";
  else if (PCdata[6] < 100) perc = "%";
  else perc = "";  lcd.print(perc);

  for (int i = 0; i < 2; i++) {
    byte line = ceil(PCdata[lines[i + 2]] / 16);
    lcd.setCursor(7, i);
    if (line == 0) lcd.write(1);
      else lcd.write(4);
    for (int n = 1; n < 5; n++) {
      if (n < line) lcd.write(4);
      if (n >= line) lcd.write(2);
    }
    if (line == 6) lcd.write(4);
      else lcd.write(3);
  }
}

// Draw Stats Motherboard / HDD / Uptime
void draw_stats_22() {
  lcd.setCursor(2, 0); lcd.print(PCdata[2]); lcd.write(223);
  lcd.setCursor(10, 0); lcd.print(PCdata[3]); lcd.write(223);

  lcd.setCursor(7, 1);
  sec = (long)(millis() - uptime_timer) / 1000;
  hrs = floor((sec / 3600));
  mins = floor(sec - (hrs * 3600)) / 60;
  sec = sec - (hrs * 3600 + mins * 60);
  if (hrs < 10) lcd.print(0);
  lcd.print(hrs);
  lcd.print(":");
  if (mins < 10) lcd.print(0);
  lcd.print(mins);
  lcd.print(":");
  if (sec < 10) lcd.print(0);
  lcd.print(sec);
}

void timeoutTick() {
  if ((millis() - timeout > 5000) && timeOut_flag) {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("CONNECTION");
    lcd.setCursor(5, 1);
    lcd.print("FAILED");
    lcd.setCursor(14, 0);
    lcd.print(dht.readTemperature());
    lcd.setCursor(14, 1);
    lcd.print(dht.readHumidity());
    timeOut_flag = 0;
    reDraw_flag = 1;
  }
}

void getTemperature() {
  if (updateTemp_flag) {
    temp = dht.readTemperature();
    hum = dht.readHumidity();
    updateTemp_flag = 0;
  }
}

// Draw Temperature/Humidity
void draw_stats_21() {
  lcd.setCursor(1, 1); lcd.print(temp); lcd.write(223);
  lcd.setCursor(8, 1); lcd.print(hum); lcd.write(223);
}

