#define hal_ir2
#define hal_compass
#define hal_ranger
#define hal_motors

#define hal_spi
#define hal_raster
// #define hal_gfx
#define hal_beeps

#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>

#ifdef hal_gfx
// include libraries
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
// instance object
Adafruit_ST7735 screen(10, A0, 4); // (cs, dc, rst);
#endif

#include <unorthodox.h>

#ifdef hal_gfx
#include <unorthodox_droid_gfx.h>
#endif

#ifdef hal_compass
// compass device
HMC5883L compass;
// 'hard iron' calibration offsets
#define HARDIRON_X  0
#define HARDIRON_Y  0
#define HARDIRON_Z  0

#endif

#ifdef hal_raster
#include <unorthodox_droid_raster.h>
// create a local variant of the ST7735 SPI Screen Device Driver
// which connects to the needed pins
class ST7735_Local : public ST7735_SPI_PortDown {
public:
  ST7735_Local() : ST7735_SPI_PortDown() {}
  void chip_mode(byte mode) {
    if(mode & ChipSelect) { PORTB &= ~(1<<6); } else { PORTB |= (1<<6);  } // D10 = PB6
    if(mode & ChipCommand) { PORTF &= ~(1<<7); } else { PORTF |= (1<<7); } // A0  = PF7
  }
  void chip_reset() { }
};

// instance the screen driver
ST7735_Local screen;

#endif


#ifdef hal_ir2
// IR hardware setup
const byte ir_rx_pin  = 0;
const volatile byte * ir_rx_port = &PIND;
const byte ir_rx_bit  = (1<<2);
// IR pulse reciever state
volatile word ir_ring[64];
volatile byte ir_ring_head = 0;
volatile byte ir_ring_tail = 63;
volatile byte ir_ring_overflow = 0;
const    byte ir_ring_mask = 63;
volatile unsigned long ir_last_time = 0;

// External Interrupt 2 (Pin 0) Interrupt handler
ISR(INT2_vect) {
  // sample the time
  unsigned long time = micros();
  word delta = min(time - ir_last_time,0xFFFF);
  ir_last_time = time;
  // flip the LSB to the pin state
  if(PIND & 0x04) {
    delta = delta | 1; 
  } else {
    delta = delta & ~1; 
  }
  // add to the buffer
  byte i = ir_ring_head;
  ir_ring[i] = delta;
  // increment the head
  i = (i+1) & ir_ring_mask;
  ir_ring_head = i;
  // check for overflow
  if(i==ir_ring_tail) ir_ring_overflow = 1;
}

// IR decoder state
byte ir_state = 0;
word ir_code = 0;
long ir_bits;
byte ir_bit_count;
#endif

#ifdef hal_ranger
// ultrasonic distance ranger state
volatile byte ranger_state = 0;
volatile unsigned long ranger_time[2];
// External Interrupt 3 (Pin 1) Interrupt handler
ISR(INT3_vect) {
  // are we in a valid state?
  if(ranger_state<2) {
    // sample the time
    ranger_time[ranger_state++] = micros();
  }
}

void ranger_pulse() {
  // regardless of previous state, pull the pin high and then force it be output.
  PORTD |= (1<<3);
  DDRD  |= (1<<3);
  // wait a moment
  delayMicroseconds(20);
  // while holding it high, reset the interrupt state
  ranger_state = 0;
  // force it low, which will trigger the first interrupt and begin the ranging
  PORTD &=~(1<<3);
  // now release the pin (to high-Z) so that the echo pulse can be read
  DDRD  &=~(1<<3);
}
#endif

#ifdef hal_motors
volatile byte laser_state;
// motor state - used by interrupt handlers
volatile byte motor_state;
// state flags
static const byte MotorFlagLeftPwm  = 1;
static const byte MotorFlagLeftDir  = 2;
static const byte MotorFlagRightPwm  = 16;
static const byte MotorFlagRightDir = 32;

// pin4 - PD4
#define Motor1Port   PORTD
#define Motor1Dir    DDRD
const byte Motor1Bit = 1<<4;
// Pin5 - PC6
#define Motor2Port   PORTC
#define Motor2Dir    DDRC
const byte Motor2Bit = 1<<6;
// Pin6 - PD7
#define Motor3Port   PORTD
#define Motor3Dir    DDRD
const byte Motor3Bit = 1<<7;
// Pin7 - PE6
#define Motor4Port   PORTE
#define Motor4Dir    DDRE
const byte Motor4Bit = 1<<6;

// Timer4 Overflow Interrupt handler
ISR(TIMER4_OVF_vect) {
  // left motor pwm
  byte f = motor_state;
  // ensure the pins are in the right state as we pass 'go'.
  if(f & MotorFlagLeftPwm) {
    if(f & MotorFlagLeftDir) { Motor2Port |= Motor2Bit; } else { Motor1Port |= Motor1Bit; }
  }
  // right motor pwm
  if(f & MotorFlagRightPwm) {
    Motor3Port &= ~Motor3Bit; Motor4Port &= ~Motor4Bit;
  }
  // laser pwm on
  if(laser_state) PORTB |= (1<<4);
}

ISR(TIMER4_COMPA_vect) {
  // turn off motor pins
  Motor1Port &= ~Motor1Bit; Motor2Port &= ~Motor2Bit;
}

ISR(TIMER4_COMPB_vect) {
  // turn on motor pins
  if(motor_state & MotorFlagRightDir) { Motor4Port |= Motor4Bit; } else { Motor3Port |= Motor3Bit; }
}

ISR(TIMER4_COMPD_vect) {
  // laser pwm off
  PORTB &=~ (1<<4); 
}

void motor_speed(int left, int right) {
  // compute pwm values, and check if pwm is needed at all.
  byte oca = min(abs(left),255);
  byte ocb = min(abs(right),255);
  bool pwm_left = (oca>0) && (oca<255);
  bool pwm_right = (ocb>0) && (ocb<255);
  // set timer values
  OCR4A = oca;
  OCR4B = 255 - ocb;
  // update motor interrupt flags
  motor_state = ((left<0)    ? (MotorFlagLeftDir)  : 0)
              | ((right<0)   ? (MotorFlagRightDir) : 0)
              | ((pwm_left)  ? (MotorFlagLeftPwm)  : 0)
              | ((pwm_right) ? (MotorFlagRightPwm) : 0);
  // update masked interrupts
  TIMSK4 = (pwm_left  ? (1<<OCIE4A) : 0)
         | (pwm_right ? (1<<OCIE4B) : 0)
         | (laser_state ? (1<<OCIE4D) : 0)
         | (1<<TOIE4);
  // if not pwm, then apply the hard values
  if(!pwm_left) {
    if(oca==255) { // full on
      if(left<0) { 
        Motor2Port |= Motor2Bit;  Motor1Port &= ~Motor1Bit;
      } else { 
        Motor1Port |= Motor1Bit; Motor2Port &= ~Motor2Bit;
      }
    } else { // off
      Motor1Port &= ~Motor1Bit; Motor2Port &= ~Motor2Bit;
    }    
  }
  if(!pwm_right) {
    if(ocb==255) { // full on
      if(right<0) { 
        Motor4Port |= Motor4Bit; Motor3Port &= ~Motor3Bit; 
      } else { 
        Motor3Port |= Motor3Bit; Motor4Port &= ~Motor4Bit;
      }
    } else { // off
      Motor3Port &= ~Motor3Bit; Motor4Port &= ~Motor4Bit;
    }
  }
}

#endif


DroidBeeps beeps(9);
#ifdef hal_beeps
// use the beeps!
PROGMEM prog_uint16_t vocal_stream[] = {
  // 1-10
  0x3B15, 0x264B, 0x385D, 0x3291, 0x2832, 0x1AB8,  // I am a droid!
  0x139A, 0x3DA4, 0x1915, 0x21EB, 0x2822, 0x36D4,  // beep boop 1
  0x1D65, 0x27FC, 0x2726, 0x221E, 0x3B91,   // beep boop 2  **3
  0x239B, 0x2389, 0x2664,  // all is good
  0x24F1, 0x2023, 0x3950,  // whazzup
  0x35E1, 0x3C08, 0x2ACE, 0x2B83, 0x34FE, 0x12FE,  // whistle 1 **6
  0x3144, 0x2D33, 0x3D24,  // sad bot
  0x35B7, 0x2AC9, 0x1956, 0x2134, 0x390C, 0x3AC2, 0x386E,  // that's so cool!
  0x2217, 0x2489, 0x142B, 0x1540, 0x34E8, 0x3526, 0x2088,  // neutral beeps **9
  0x1755, 0x1D7B, 0x3876, 0x2A32, 0x2502, 0x34D0,  // I'm so there!
  // 11-20
  0x26DF, 0x38D3, 0x395C, 0x14E5, 0x25EA, 0x3ABA,  // what's up R2?
  0x2AFC, 0x2A99, 0x3519, 0x113A,  // lots of stuff, R2
  0x2EAC, 0x3640, 0x2AEC, 0x172F, 0x26EB, 0x3834,  // i have a query
  0x3D63, 0x33D6,  // huh!? **
  0x39FC, 0x2421, 0x244E, 0x3136, 0x2309, 0x232A,  // something just happened
  0x3720, 0x1845, 0x2425, 0x35E1, 0x329A, 0x304B, // swear 3
  0x2E1A, 0x3C50, 0x249A,  // searching
  0x3470, 0x1378, 0x3E18,  // watch out!
  0x385B, 0x3E7F, 0x3E18, 0x10B5, 0x1DC0, 0x312A, 0x36F6,  // all done, what's next?  **18
  0x1130, 0x36B4, 0x133D, 0x3D6C, 0x3208, 0x25B0,  // are you here?
  // 21-30
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
  // 31-40
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
  // 41-50
  0x319D, 0x364E, 0x285B, 0x1477, 0x3DCC, 0x2E6B,  // that didn't quite work
  0x23F1, 0x2E47, 0x3A94, 0x2CB6, 0x109F, 0x1DEE,  // on my way
  0x3460, 0x37A6, 0x3031, 0x2EF5, 0x380D, 0x3646,  // sie la vie
  0x2861, 0x3588, 0x223C, 0x1A0B, 0x319A, 0x3C52, 0x101E,  // let me explain?
  0x3004, 0x21EA, 0x371F, 0x187F, 0x18A9,  // ouch
  0x10D9, 0x2000, 0x241D, 0x17D4, 0x36C2, 0x17A0, 0x34E6,  // please repeat.
  0x1127, 0x36A2, 0x26F0, 0x1D9F, 0x2452, 0x2D78,  // ready to go!
  0x2355, 0x2E67, 0x21D9, 0x244E, 0x39AB, 0x1080,  // this is hard!
  0x25C7, 0x1E1D, 0x35A5, 0x3CB1, 0x10AB,  // which way
  0x32C1, 0x2A3B, 0x22BB, 0x310C,  // swear word 1
  // 51-60
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
  // 61-63
  0x3072, 0x1840, 0x20DF, 0x111B, 0x3015, 0x1621, 0x34B1,  // just a sec!
  0x31B8, 0x1806, 0x37C2, 0x2EB9, 0x26B8, 0x2DEB,  // coo-ee 2
  0x3776, 0x2552, 0x1466, 0x3D7B, 0x3AE9, 0x3619, 0x11D4,  // it's over here!
};

PROGMEM prog_uint8_t vocal_index[] = {
  6, 6, 5, 3, 3, 6, 3, 7, 7, 6, 
  6, 4, 6, 2, 6, 6, 3, 3, 7, 6, 
  4, 7, 5, 5, 3, 5, 6, 7, 7, 4,
  4, 6, 3, 6, 7, 7, 7, 2, 7, 7,
  6, 6, 6, 7, 5, 7, 6, 6, 5, 4,
  5, 7, 6, 6, 6, 6, 4, 5, 7, 5,
  7, 6, 7
};

NearProgramPage vocals((prog_uchar *)vocal_stream);
NearProgramPage vocali((prog_uchar *)vocal_index);


// Timer3 Overflow Interrupt handler
ISR(TIMER3_OVF_vect) {
  beeps.exec(1); // assume 1ms has passed.
}

#endif

/*
 * Build our droid out of mostly standard parts.
 */

#ifdef hal_gfx
class LocalDroid : public GFXDroid {
#endif
#ifdef hal_raster
class LocalDroid : public RasterDroid {
#endif
public:
#ifdef hal_gfx
  LocalDroid(Page * storage, word size, Adafruit_GFX * gfx) : GFXDroid(storage, size, gfx) {}
#endif
#ifdef hal_raster
  LocalDroid(Page * storage, word size, Raster16 * raster) : RasterDroid(storage, size, raster) {}
#endif
  void signal_state(word sig, int v, int mode) { 
    int sl, sr; 
    if(mode & OnWrite) {
      switch(sig) {
#ifdef hal_motors
        case 112: // left motor
        case 113: // right motor
          // map (-100,100) to (-256,256) for both axes
          sl = constrain(signal_value[112],-100,100)*41; sl /= 16;
          sr = constrain(signal_value[113],-100,100)*41; sr /= 16;
          motor_speed( sl, sr ); 
          break;
        case 115: // PWM laser
          sl = constrain(signal_value[115],0,99)*41; sl /= 16;
          if(sl==0) {
            // shut down pwm
            laser_state = 0;
          } else {
            // set pwm length and enable interrupt for off cycle
            OCR4D =  sl;
            TIMSK4 |= (1<<OCIE4D);
            // enable the pwm on cycle
            laser_state = 1;
          }
          break;
#endif
#ifdef hal_beeps
        case 127: // beep codes
          if((v>0) && (v<64)) {
            // preprogrammed vocalization stream
            word p = 0;
            // run through the index
            for(int i=0; i<v; i++) p += vocali.read_byte(i);
            p = p<<1;
            // vocalise the segment
            byte c = vocali.read_byte(v);
            for(int i=0; i<c; i++) {
              beeps.vocalize( vocals.read_word(p) );
              p+=2;
            }
          } else if(v>255) {
            // full vocalizer code
            beeps.vocalize((word)v);
          }
          break;
#endif
      }
    }
    // if it was a signal change, update the user interface
    if(mode & OnChange) { redraw_signal(sig); }
  }
 
};

// instance the droid from storage
EEPROMPage eeprom(0);
LocalDroid droid(&eeprom, 1024, &screen);

void setup() {  
  pinMode(A3, INPUT_PULLUP); // joystick button pullup A3
  pinMode(0, INPUT_PULLUP); // infrared RX + pullup
  for(int i=4; i<=7; i++) {
    pinMode(i, OUTPUT); digitalWrite(i,LOW); // motor control pins
  }
  pinMode(8,OUTPUT); // laser 
  // initialize serial port
  Serial.begin(9600);
  // leonardo - wait for connection
  // while(!Serial) { }
#ifdef hal_spi
  // start the spi bus
  SPI.begin();
#endif
#ifdef TwoWire_h
  // start the i2C bus
  Wire.begin();
#endif
#ifdef hal_gfx
  screen.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  screen.fillScreen(ST7735_BLACK);
#endif
#ifdef hal_raster
  pinMode(10, OUTPUT); digitalWrite(10,HIGH); // 
  pinMode(A0, OUTPUT); digitalWrite(A0,HIGH); // 
  screen.start();
#endif
#ifdef hal_ir
  // Start the IR Receiver
  ir_receiver.enableIRIn(); 
  // ir_decoder.UseExtnBuf(Buffer);
#endif
#ifdef hal_compass
  compass.start();
#endif
  // start the droid filesystem (now we have serial)
  droid.fs.start();
  // center the joystick with the current position
  droid.joystick[0].center = 1024 - analogRead(A1);
  droid.joystick[1].center = 1024 - analogRead(A2);
#ifdef hal_raster
  // clear the RHS edge of the display - line rendering will do the rest
  droid.Draw.rect_fill(126,0,2,160, 0x0000);
#endif
  // initialize timers and interrupts
  cli();           // disable all interrupts
  EICRA = 0;
#ifdef hal_ir2
  // enable Int2 (for Pin0)
  EICRA |= (0<<ISC21) | (1<<ISC20);             // trigger on either edge
  EIMSK |= (1<<INT2);                           // enable external interrupt mask
  EIFR  |= (1<<INTF2);                          // clear the interrupt flag
#endif
#ifdef hal_ranger
  // initialize timer1
  TCCR1A = (1<<COM1A0) | (1<<WGM11) | (1<<WGM10); // PWM, 16-bit Phase correct, routed to Pin 9
  TCCR1B = (1<<CS10)   | (1<<WGM13);              // clk/1 prescaler 
  // TIMSK1 |= (1<<TOIE1);                        // enable timer overflow interrupt
  // enable Int3 (for Pin1)
  EICRA |= (1<<ISC31) | (0<<ISC30);             // trigger on falling edge
  EIMSK |= (1<<INT3);                           // enable external interrupt mask
  EIFR  |= (1<<INTF3);                          // clear the interrupt flag
#endif
#ifdef hal_beeps
  // initialize timer3
  TCCR3A = (1<<WGM30);                          // PWM, 8-bit fast 
  TCCR3B = (1<<CS31) | (1<<CS30) | (1<<WGM32);  // clk/64 prescaler 
  TIMSK3 |= (1<<TOIE1);                         // enable timer overflow interrupt
#endif
#ifdef hal_motors
  // initialize timer4
  TCCR4A = (1<<PWM4A) | (1<<PWM4B);                        //  PWM A+B+D
  TCCR4B = (1<<CS43)  | (0<<CS42) | (1<<CS41) | (1<<CS40); // clk/1024 prescaler 
  TCCR4C = (1<<PWM4D);
  TCCR4D = (0<<WGM41) | (0<<WGM40);                        // Fast PWM
  TCCR4E = 0;
  //TIMSK4 = (0<<OCIE4A) | (0<<OCIE4B) | (0<<OCIE4D) (0<<TOIE4);   // no enabled masked interrupts
  OCR4C = 0xFF;
#endif
  // complete
  sei();             // enable all interrupts
  motor_speed(0,0);
  pinMode(0, INPUT_PULLUP);  // infrared RX + pullup
}

unsigned long last_time = 0;
unsigned long last_pulse = 0;

void loop() {
  unsigned long time = millis();
  // based on the current time and the previous time, work out the elapsed time, including edge cases
  unsigned long elapsed = ( time < last_time ) ? ( 0xffffffff - last_time + time ) : ( time - last_time );
  // downconvert that to a plain old integer, max out at 32k.
  int dt = (elapsed & 0xffff8000) ? 0x7fff : elapsed;
  // droid update
  droid.exec(dt);
#ifdef hal_ir
  ir_update();
#endif
  // read joystick
  int joystick[2];
  joystick[0] = 1024 - analogRead(A1); 
  joystick[1] = 1024 - analogRead(A2);
  bool button0 = (digitalRead(A3) == LOW); 
  droid.update(joystick,button0,time,dt);
#ifdef hal_compass
  // get the compass vector
  int v[3];
  compass.get_vector(v);
  // account for 'hard iron' effects.
  v[0] += HARDIRON_X;
  v[1] += HARDIRON_Y;
  v[2] += HARDIRON_Z;
  // convert mangetometer values to compass readings
  int heading = atan2_degrees(v[1],-v[0]);
  droid.set_signal(121, heading);  
  int heading_length = sqrt( ((long)v[0]*(long)v[0]) + ((long)v[2]*(long)v[2]) );
  // int inclination = atan2_degrees(heading_length,v[1]);
  // droid.set_signal(122, inclination);
  int strength = sqrt( ((long)v[0]*(long)v[0]) + ((long)v[1]*(long)v[1]) + ((long)v[2]*(long)v[2]) );
  droid.set_signal(123, strength);
  // and update the droid signals
  for(byte i=0; i<3; i++) droid.set_signal(124+i, v[i]);
#endif
#ifdef hal_ranger
  if(last_pulse + 100 < time) {
    // has the last one finished?
    if(ranger_state==2) {
      // compute the elapsed time
      unsigned long range = (ranger_time[1] - ranger_time[0]);
      // sanity check
      if((range>500) && (range < 80000)) {
        // subtract 500us to account for sensor delays
        range -= 500;
        // convert to (approximate) millimeters
        range = (range<<4)/93;
        // limit to 5 meters
        range = min(range, 5000);
        droid.set_signal(120, range);
      }
    }
    // start the next ranging pulse.
    ranger_pulse();
    // record the last time we did this.
    last_pulse = time;
  }
#endif
#ifdef hal_ir2
  // consume time events from the ir ring
  byte tail = (ir_ring_tail+1) & ir_ring_mask;
  if(tail==ir_ring_head) {
    // none available
  } else {
    // get the next entry in the sequence
    word delta = ir_ring[tail];
    byte debug = ir_state;
    ir_accept(delta);
    debug |= ir_state;
    // store the tail back
    ir_ring_tail = tail;
  }
#endif
  last_time = time;
}


#ifdef hal_ir2

void ir_accept(word w) {
  // if the LSB was off, this is a mark
  bool mark = (w & 1);
  // do what ditty 
  if(ir_state==0) {
    // waiting for header mark
      // did it look like a start mark?
      if(mark && (w>8500) && (w<9500)) {
        ir_state = 1;
        ir_bits = 0;
        ir_bit_count = 0;
      } else {
        // keep waiting.
      }
  } else if(ir_state==1) { // waiting for header space
      // did it look like a start space?
      if(!mark && (w>4000) && (w<4800)) {
        ir_state = 2;
      } else {
        // reject,
        ir_state = 0;
      }
    } else if(ir_state==2) { // waiting for data mark
      // did it look like a mark?
      if(mark && (w>400) && (w<800)) {
        ir_state = 3;
      } else {
        // reject
        ir_state = 0;
      }
    } else if(ir_state==3) { // waiting for data space
      if(!mark) {
        if((w>400) && (w<800)) {
          // logical one
          ir_bits = (ir_bits<<1) | 1;
          // try again
          ir_state = 2;
        } else if((w>1500) && (w<1900)) {
          // local zero
          ir_bits = (ir_bits<<1);
          // try again
          ir_state = 2;
        } else {
          // reject
          ir_state = 0;
        }
        if(ir_state==2) {
          if((++ir_bit_count)==32) {
            // use the last byte as the signal code.
            droid.set_signal(114, 0);
            droid.set_signal(114, (byte)ir_bits);
          };
        }
      } else {
        ir_state = 0;
      }
    }
  }

#endif

/*
  fast curve-fit integer atan2. Accurate to within 1 degree, at best.
  These routines internally use "fixed point" but arguments are integers,
  and the result is a 0..360 degree integer.
*/
int atan2_degrees(int x, int y) {
  if((x==0)&&(y==0)) return 0;
  word a = abs(x);
  word b = abs(y);
  // curve coefficients
  word ta;
  word tb;
  int  tc = 14667;
  // quadrant determination
  word q = 0;
  word r;
  if(b>a) { // abs(y) > abs(x)
    // top or bottom quadrant?
    if(y<0) { q = 90; } else { tc = -tc; q = 270; }
    // left or right of center?
    if(x<0) { } else { tc = -tc; }
    // scale down numerator to 8 bits
    word n = a; byte sn = 8;
    while(n>255) { n = n>>1; b = b>>1; sn--; }
    // tangent slope a/b as 16-bit fixed point
    r = (a << sn) / b;
  } else { // abs(x) >= abs(y)
    // left or right quadrant?
    if(x<0) { tc = -tc; q = 180; } else { q = 0; }
    // above or below center?
    if(y<0) { } else { tc = -tc; }
    // scale down numerator to 8 bits
    word n = b; byte sn = 8;
    while(n>255) { a = a>>1; n = n>>1; sn--; }
    // tangent slope b/a as 16-bit fixed point
    r = (b << sn) / a;
  }
  // we now have a 0..256 value that expresses the side ratio, and we want to turn it into decimal degrees.
  // compute: atan(x) = ( (0.97239 * x)  - (0.1915 * x^3) )                * (+/-)90 / (PI/4)
  // 16 bit fp        = ( (x*0xF9)>>8    - (((x*x)>>8)*x>>8)*0x31)>>8 )    * (+/-)114
  // compute (0.97239 * r) in 16-bit fixed point
  ta = r * 0xF9;
  // compute (0.1915 * x^3) in 16-bit fixed point
  if(r==0x0100) {
    // atan(1) special case when r==256 (since the multiplies would overflow)
    tb = 0x3100;
  } else {
    tb = ((((r*r)>>8)*r)>>8) * 0x31;
  }
  // combine 16-bit terms together
  long t = (long)(ta - tb);
  t = (t * (long)tc )>>16;
  // we now have a 16-bit fixed point angle in degrees from -45 to +45
  // downconvert to integer and add the quadrant
  int angle = (int)(t>>8) + q;
  if(angle<0) angle += 360; // make positive
  // return the result
  return angle;
}
