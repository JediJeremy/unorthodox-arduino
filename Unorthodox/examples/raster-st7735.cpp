// #include <Wire.h>
// #include <EEPROM.h>
#include <SPI.h>
#include <unorthodox.h>

#define hal_raster
#define hal_gfx

#ifdef hal_gfx
// include libraries
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
// instance object
Adafruit_ST7735 lcd = Adafruit_ST7735(10, A0, 4); // (cs, dc, rst);

#endif

#ifdef hal_raster

class ST7735_Local : public ST7735_SPI {
public:
  ST7735_Local() : ST7735_SPI() {}
  void chip_mode(byte mode) {
    // Serial.print("\nmode "); Serial.print(mode);
    if(mode & ChipSelect) { PORTB &= ~(1<<6); } else { PORTB |= (1<<6);  } // D10 = PB6
    if(mode & ChipCommand) { PORTF &= ~(1<<7); } else { PORTF |= (1<<7); } // A0 = D18 = PF7
    // digitalWrite(10, (mode & ChipSelect) ? LOW : HIGH );
    // digitalWrite(A0, (mode & ChipCommand) ? LOW : HIGH );
    // Serial.print((mode & ChipSelect) ? "S" : "s");
    // Serial.print((mode & ChipCommand) ? "C" : "c");
  }
  void chip_reset() { 
     Serial.print("\nreset\n");
  }
};

// ILI9325C_Leo Raster;
ST7735_Local Raster;
RasterDraw16 Draw(&Raster);

// include the standard font
NearProgramPage Sysfont(font_5x8_128);
RasterFont16 Font(&Raster, &Sysfont);
#endif


/*
// HMC5883L Sensor;
MPU6050 Sensor;
int cx = 0;
int cy = 0;
word scrolly = 0;
word col = 0;
*/
bool do_raster = false;
bool do_gfx = false;


void setup() {
  //  Wire.begin();
  while(!Serial) { }
  SPI.begin();
#ifdef hal_raster
  // initialize output ports
  pinMode(10,OUTPUT); digitalWrite(10,HIGH);
  pinMode(A0,OUTPUT); digitalWrite(A0,HIGH);
  do_raster = true;
#endif
#ifdef hal_gfx
  do_gfx = true;
#endif
#if defined(hal_raster) && defined(hal_gfx)
  do_gfx = false; // don't do it after all
#endif
  // Sensor.start();
}

byte char_start = 0;
byte char_limit = 128;
byte char_index;

byte loop_state = 0; // start in initial state

int line_start = 0;
int line_limit = 160;
int line_index;

word color_index = 0;


long timer_origin;

void timer_start() {
  timer_origin = millis();
}

void timer_stop(char * s) {
  long delta = millis() - timer_origin;
  Serial.print(do_raster?"raster ":"gfx "); 
  Serial.print(s); 
  Serial.print(" ("); Serial.print(delta); Serial.print(")\n");   
}

void loop() {
  if(loop_state==0) {
    // restart the display
    timer_start();
#ifdef hal_raster
    if(do_raster) Raster.start();
#endif
#ifdef hal_gfx
    if(do_gfx) lcd.initR(INITR_BLACKTAB);
#endif
    timer_stop("init");
    delay(1000);
    // clear screen
    timer_start();
#ifdef hal_raster
    if(do_raster) Draw.rect_fill(0,0,128,160,0x0000);
#endif
#ifdef hal_gfx
    if(do_gfx) lcd.fillScreen(ST7735_BLACK);
#endif
    timer_stop("cls");
    delay(1000);
    // reinitialize to line 0, start scan
    loop_state = 1;
    line_index = line_start;
    timer_start();
  } else if(loop_state==1) {
    // draw next line of pixels
    for(int i=0; i<128; i++) {
#ifdef hal_raster
      if(do_raster) Draw.rect_fill(i,line_index,1,1, color_index++);
#endif
#ifdef hal_gfx
      if(do_gfx) lcd.drawPixel(i,line_index, color_index++);
#endif
    }
    // increment, and possibly go to next state
    if(++line_index >= line_limit) loop_state = 2;
  } else if(loop_state==2) {
    timer_stop("pixels");
    // reinitialize to line 0, start scan
    loop_state = 3;
    line_index = line_start;
    char_index = char_start;
    delay(1000);
    timer_start();
  } else if(loop_state==3) {
    // draw next line of characters
    for(int i=0; i<22; i++) {
#ifdef hal_raster
      if(do_raster) Font.char_fill(i*6,line_index,char_index, 0xFFFF, color_index++);
#endif
#ifdef hal_gfx
      if(do_gfx) lcd.drawChar(i*6,line_index,char_index, 0xFFFF, color_index++, 1);
#endif
      if(++char_index >= char_limit) char_index = char_start;
    }
    // increment, and possibly go to next state
    line_index += 8;
    if(line_index >= line_limit) loop_state = 4;
  } else if(loop_state==4) {
    timer_stop("font fill");
    delay(4000);
    loop_state = 5;
    line_index = line_start;
    char_index = char_start;
    timer_start();
  } else if(loop_state==5) {
    // draw next line of characters
    for(int i=0; i<22; i++) {
#ifdef hal_raster
      if(do_raster) Font.char_outline(i*6,line_index,char_index, color_index++);
#endif
#ifdef hal_gfx
      if(do_gfx) lcd.drawChar(i*6,line_index,char_index, color_index, color_index++, 1);
#endif
      if(++char_index >= char_limit) char_index = char_start;
    }
    // increment, and possibly go to next state
    line_index += 8;
    if(line_index >= line_limit) loop_state = 6;
  } else if(loop_state==6) {
    timer_stop("font outline");
    delay(4000);
    // draw a rectangle
#ifdef hal_raster
    if(do_raster) Draw.rect_outline(10,10,100,100,0);
#endif
#ifdef hal_gfx
    if(do_gfx) lcd.drawRect(10,10,100,100,0);
#endif
    delay(10000);
    loop_state = 7;
  } else {
#if defined(hal_raster) && defined(hal_gfx)
    // flip which one we're doing
    do_raster = !do_raster; 
    do_gfx = !do_gfx; 
#endif
    // reset
    delay(1000);
    loop_state = 0;
  }
}

  /*
void loop() {
  int vector[6];
  // draw latest line
  Draw.rect_fill(0,scrolly,240,1, 0 );
  if(Sensor.get_ident()) {

    Sensor.temp_vector(vector);
    draw_entry(vector[0], Raster.color(128,0,0) );
    
    Sensor.gyro_vector(vector);
    draw_entry(vector[0], Raster.color(0,128,128) );
    draw_entry(vector[1], Raster.color(128,128,0) );
    draw_entry(vector[2], Raster.color(128,0,128) );

    Sensor.accel_vector(vector);
    draw_entry(vector[0], Raster.color(0,0,255) );
    draw_entry(vector[1], Raster.color(0,255,0) );
    draw_entry(vector[2], Raster.color(255,0,0) );
    /*
    Serial.print(vector[0]); Serial.print(",");
    Serial.print(vector[1]); Serial.print(",");
    Serial.print(vector[2]); Serial.print("\n");
    *
  } else {
    Serial.print("no ident\n");
  } 
  // scroll up one
  scrolly = ++scrolly % 320;
  Raster.scroll(scrolly);
  // delay(100);
}

void draw_entry(int a, word col) {
  // int x = constrain( map(a,-16536,16535, 0,239), 0,239);
  int x = constrain( map(a,-16536,16535, 0,127), 0,127);
  Draw.rect_fill(x,scrolly,1,1, col );
}
*/
