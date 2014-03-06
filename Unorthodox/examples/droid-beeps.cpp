// #define hal_ir
// #define hal_raster
// #define hal_tft
// #define hal_compass
#define hal_beeps

#include <EEPROM.h>
#include <Wire.h>

#ifdef hal_raster
ILI9325C_Leo Raster;
RasterDraw16 Draw(&Raster);
#endif

#ifdef hal_tft
// include libraries
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
// instance object
Adafruit_ST7735 lcd = Adafruit_ST7735(10, A0, 4); // (cs, dc, rst);
#endif

#include <unorthodox.h>

#ifdef hal_compass
HMC5883L compass;
#endif

#ifdef hal_beeps

// use the beeps!
DroidBeeps beeps(9);
PROGMEM prog_uint16_t vocal_stream[] = {
  0x3B15, 0x264B, 0x385D, 0x3291, 0x2832, 0x1AB8,  // I am a droid!
  0x139A, 0x3DA4, 0x1915, 0x21EB, 0x2822, 0x36D4,  // beep boop 1
  0x1D65, 0x27FC, 0x2726, 0x221E, 0x3B91,   // beep boop 2
  0x239B, 0x2389, 0x2664,  // all is good
  0x24F1, 0x2023, 0x3950,  // whazzup
  0x35E1, 0x3C08, 0x2ACE, 0x2B83, 0x34FE, 0x12FE,  // whistle 1
  0x3144, 0x2D33, 0x3D24,  // sad bot
  0x35B7, 0x2AC9, 0x1956, 0x2134, 0x390C, 0x3AC2, 0x386E,  // that's so cool!
  0x2217, 0x2489, 0x142B, 0x1540, 0x34E8, 0x3526, 0x2088,  // neutral beeps 
  0x1755, 0x1D7B, 0x3876, 0x2A32, 0x2502, 0x34D0,  // I'm so there!
  0x26DF, 0x38D3, 0x395C, 0x14E5, 0x25EA, 0x3ABA,  // what's up R2?
  0x2AFC, 0x2A99, 0x3519, 0x113A,  // lots of stuff, R2
  0x2EAC, 0x3640, 0x2AEC, 0x172F, 0x26EB, 0x3834,  // i have a query
  0x3D63, 0x33D6,  // huh!?
  0x39FC, 0x2421, 0x244E, 0x3136, 0x2309, 0x232A,  // something just happened
  0x3720, 0x1845, 0x2425, 0x35E1, 0x329A, 0x304B, // swear 3
  0x2E1A, 0x3C50, 0x249A,  // searching
  0x3470, 0x1378, 0x3E18,  // watch out!
  0x385B, 0x3E7F, 0x3E18, 0x10B5, 0x1DC0, 0x312A, 0x36F6,  // all done, what's next?
  0x1130, 0x36B4, 0x133D, 0x3D6C, 0x3208, 0x25B0,  // are you here?
  0x3586, 0x3A74, 0x2C0F, 0x3EE8,  // trick or treat
  0x2182, 0x116C, 0x1729, 0x2D45, 0x32F5, 0x268E, 0x17A7,  // I think it worked.
  0x2B25, 0x3171, 0x100E, 0x170A, 0x1DB3,  // coo-ee!
  0x1609, 0x27CA, 0x3924, 0x1020, 0x2059,  // I'm sorry!
  0x32D0, 0x1B6E, 0x178E,  // bump!
  0x1DB1, 0x21B6, 0x11C4, 0x357E, 0x1D8F,  // double bump
  0x1448, 0x395A, 0x324F, 0x27E1, 0x301E, 0x268E,  // I am done!
  0x3684, 0x1227, 0x3DDC, 0x3A25, 0x2DDD, 0x2D15, 0x3D99,  // more complicated than I thought
  0x2899, 0x34F0, 0x17CD, 0x2C24, 0x30DE, 0x129E, 0x35F2,  // why would you do that?
  0x153D, 0x1C7B, 0x3976, 0x296F,  // hi there.
  0x3BE8, 0x26A7, 0x368F, 0x11B4,  // don't like!
  0x10AE, 0x3E31, 0x32F2, 0x34B2, 0x13CF, 0x20DE,  // happy chirp
  0x3884, 0x2CED, 0x2A72,  // that's confirmed.
  0x39F0, 0x24CF, 0x12C9, 0x24CF, 0x1BA7, 0x138A,  // swear 2
  0x15EB, 0x2C91, 0x3D74, 0x2D3F, 0x1694, 0x2997, 0x3605,  // I tried it, It didn't work.
  0x13CB, 0x3BF7, 0x34E9, 0x2673, 0x35C4, 0x3E2A, 0x1108,  // leave me alone!
  0x151D, 0x1023, 0x1340, 0x2370, 0x1CC5, 0x394D, 0x38D3,  // are you okay?
  0x3C4B, 0x3E3A,  // oh no!
  0x38D6, 0x22AE, 0x1360, 0x3B4F, 0x3679, 0x2689, 0x21E6,  // i don't know
  0x3E04, 0x174C, 0x2814, 0x31D1, 0x1CF1, 0x231E, 0x1AD4,  // everything checks out
  0x319D, 0x364E, 0x285B, 0x1477, 0x3DCC, 0x2E6B,  // that didn't quite work
  0x23F1, 0x2E47, 0x3A94, 0x2CB6, 0x109F, 0x1DEE,  // on my way
  0x3460, 0x37A6, 0x3031, 0x2EF5, 0x380D, 0x3646,  // sie la vie
  0x2861, 0x3588, 0x223C, 0x1A0B, 0x319A, 0x3C52, 0x101E,  // let me explain?
  0x3004, 0x21EA, 0x371F, 0x187F, 0x18A9,  // ouch
  0x10D9, 0x2000, 0x241D, 0x17D4, 0x36C2, 0x17A0, 0x34E6,  // please repeat.
  0x1127, 0x36A2, 0x26F0, 0x1D9F, 0x2452, 0x2D78,  // ready to go!
  0x2355, 0x2E67, 0x21D9, 0x244E, 0x39AB, 0x1080,  // this is hard!
  0x25C7, 0x1E1D, 0x35A5, 0x3CB1, 0x10AB,  // which way...
  0x32C1, 0x2A3B, 0x22BB, 0x310C,  // swear word 1
  0x38EF, 0x2874, 0x29A2, 0x3C3F, 0x2DAA,  // I don't like it here
  0x38A0, 0x2A4A, 0x3ADB, 0x3D35, 0x1558, 0x12F4, 0x3C0F,  // screw this, I'mn going home.
  0x26CC, 0x2D01, 0x381C, 0x16BB, 0x196A, 0x3359,  // come here, kitteh!
  0x20FD, 0x1501, 0x3578, 0x3CC9, 0x35E9, 0x3853,  // something just happened
  0x3976, 0x29AE, 0x32A9, 0x2B48, 0x374E, 0x3E52,  // are you OK?
  0x28F8, 0x2688, 0x3DFA, 0x2CFC, 0x3750, 0x1319,  // neutral curious
  0x1083, 0x2666, 0x2670, 0x35B4,  // positive query
  0x3A99, 0x3267, 0x26DB, 0x3320, 0x36CD,  // that's not good.
  0x39AA, 0x385C, 0x3D59, 0x30D2, 0x37A7, 0x2E9E, 0x22D2,  // "singing a short song.."
  0x2D67, 0x126C, 0x3E73, 0x3D26, 0x3EE2,  // what about me!
  0x3072, 0x1840, 0x20DF, 0x111B, 0x3015, 0x1621, 0x34B1,  // just a sec!
  0x31B8, 0x1806, 0x37C2, 0x2EB9, 0x26B8, 0x2DEB,  // coo-ee 2
  0x3776, 0x2552, 0x1466, 0x3D7B, 0x3AE9, 0x3619, 0x11D4,  // it's over here!
};
PROGMEM prog_uint16_t vocal_index[] = {
// sequence lengths:
//  6,6, 5, 3, 3, 6, 3, 7, 7, 6, 6, 4, 6, 2, 6, 6, 3, 3, 7, 6, 4, 7, 5, 5, 3, 5, 6, 7,
    0,6,12,17,20,23,29,36,43,49,55,59,65,67,33,39,42,45,52,58,62,69,74,79,82,87,93,100,
//    7,  4,  4,  6,  3,  6,  7,  7,  7,  2,  7,  7,  6,  6,  6,  7,  5,  7,  6,  6,  5,  4,
    107,111,115,121,124,130,137,144,151,153,160,167,173,179,185,192,197,204,210,216,221,225,
//    5,  7,  6,  6,  6,  6,  4,  5,  7,  5,  7,  6,  7
    230,237,243,249,255,261,265,270,277,282,289,295,302
};

NearProgramPage vocals((prog_uchar *)vocal_stream);
NearProgramPage vocali((prog_uchar *)vocal_index);

#endif


/*
 * Build our droid out of mostly standard parts.
 */

// class LocalDroid : public GFXDroid {
class LocalDroid : public BaseDroid {
public:
  // LocalDroid(Page * storage, word size, Adafruit_GFX * gfx) : GFXDroid(storage, size, gfx) {}
  LocalDroid(Page * storage, word size) : BaseDroid(storage, size) {}
  
  void signal_state(word sig, int v, int mode) { 
    if(mode & OnWrite) {
      //Serial.print(" w:"); Serial.print(sig); Serial.print(":"); Serial.print(v);
      switch(sig) {
        /* case 123: 
          if(v==-3) {
            // accept the command
            set_signal(123, 0);
            // do something else
            Serial.print("\n format!");
            // clear the entire eeprom
            for(int i=0; i<1024; i++) fs.page->write_byte(i, 0xFF);
            fs.head = 0;
            fs.tail = 0;
            fs.next = 0;
            fs.empty = true;
          }
          break; */

#ifdef hal_beeps
       case 127: // beep codes
          if((v>0) && (v<64)) {
            // preprogrammer vocalization stream
            beeps.vocalize_index(&vocals, &vocali, v);
          } else if(v>255) {
            // full vocalizer code
            beeps.vocalize((word)v);
          }
          break;
#endif
      }
    }
  }
  
 
};

// instance the droid from storage
EEPROMPage eeprom(0);
// LocalDroid droid(&eeprom, 1024, &lcd);
LocalDroid droid(&eeprom, 1024);

void setup() {
// start the spi bus
#ifdef hal_spi
  SPI.begin();
#endif
#ifdef hal_tft
  lcd.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  lcd.fillScreen(ST7735_BLACK);
#endif
  // initialize serial port
  Serial.begin(9600);
  // leonardo - wait for connection
  while(!Serial) { }
#ifdef TwoWire_h
  // start the i2C bus
  Wire.begin();
#endif
#ifdef hal_raster
  Raster.start();
#endif
#ifdef hal_compass
  compass.start();
#endif
  // start the droid filesystem (now we have serial)
  droid.fs.start();
  // center the joystick
#ifdef hal_tft
  droid.joystick[0].center = 1024 - analogRead(A1);
  droid.joystick[1].center = analogRead(A2);
#endif
  // droid.joystick_center[0] = 1024 - analogRead(A1);
  // droid.joystick_center[1] = analogRead(A2);
}


#ifdef hal_tft
int cursor_x = 0;
int cursor_y = 0;
#endif
void loop() {
  long t = millis();
  // put your main code here, to run repeatedly: 
  droid.exec(t);
#ifdef hal_beeps
  beeps.exec(t);
#endif
#ifdef hal_ir
  ir_update();
#endif
#ifdef hal_tft
  // read joystick
  int ana[2];
  ana[0] = 1024 - analogRead(A1); 
  ana[1] = analogRead(A2);
  int b0 = (digitalRead(A3) == LOW); 
  droid.update_joystick(ana,b0,t);
  droid.update_display(t);
#endif
#ifdef hal_compass
  int v[3];
  compass.get_vector(v);
  for(byte i=0; i<3; i++) droid.set_signal(124+i, v[i]);
  /* Serial.print(v[0]); Serial.print(','); 
  Serial.print(v[1]); Serial.print(','); 
  Serial.print(v[2]); Serial.print("\n"); 
  delay(100); */
#endif

}
