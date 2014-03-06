#include <EEPROM.h>
#include <unorthodox.h>

#define hal_joystick
// #define hal_adamotor

class MotivatorM1 : public MotivatorUnit {
public: 
  MotivatorM1(int source, int drain) : MotivatorUnit(source, drain) {};
  void switch_on() { digitalWrite(7,HIGH); }
  void switch_off() { digitalWrite(7,LOW); }
};

class MotivatorM2 : public MotivatorUnit {
public: 
  MotivatorM2(int source, int drain) : MotivatorUnit(source, drain) {};
  void switch_on() { digitalWrite(6,HIGH); }
  void switch_off() { digitalWrite(6,LOW); }
};

class MotivatorM3 : public MotivatorUnit {
public: 
  MotivatorM3(int source, int drain) : MotivatorUnit(source, drain) {};
  void switch_on() { digitalWrite(5,HIGH); }
  void switch_off() { digitalWrite(5,LOW); }
};

class MotivatorM4 : public MotivatorUnit {
public: 
  MotivatorM4(int source, int drain) : MotivatorUnit(source, drain) {};
  void switch_on() { digitalWrite(4,HIGH); }
  void switch_off() { digitalWrite(4,LOW); }
};


// create a motivator
Motivator motive(1,4);
// and the units units
MotivatorM1 M1(1000,1000);
MotivatorM2 M2(1000,1000);
MotivatorM3 M3(1000,1000);
MotivatorM4 M4(1000,1000);
MotivatorSource Motors(2000,2000, 0, 4);

#ifdef hal_adamotor
long ada_next_time;
long ada_wait_time;
AdaMotorDevice AdaMotor;
#endif

long pulse_next_time;
long pulse_wait_time;

void setup() {
  for(int i=2; i<=9; i++) {
    pinMode(i,OUTPUT); digitalWrite(i,LOW);
  }
  // attach the motivator components
  motive.unit[0] = &M1;
  motive.unit[1] = &M2;
  motive.unit[2] = &M3;
  motive.unit[3] = &M4;
  motive.source[0] = &Motors; // source last, in case interrupt is running.
  //
#ifdef hal_adamotor
  AdaMotor.exec();
  ada_wait_time = 100;
  ada_next_time = millis() + ada_wait_time;
#endif
  // set up timers
  pulse_wait_time = 1;
  pulse_next_time = millis() + pulse_wait_time;
}

long next_joystick = 0;
void loop() {
#ifdef hal_joystick
  // joystick control
  word a0 = analogRead(A0);
  word a1 = analogRead(A1);
  int m1 = map(a0,562,1023, 0,1000); m1 = constrain(m1,0,1000);
  int m2 = map(a0,462,0,    0,1000); m2 = constrain(m2,0,1000);
  int m3 = map(a1,562,1023, 0,1000); m3 = constrain(m3,0,1000);
  int m4 = map(a1,462,0,    0,1000); m4 = constrain(m4,0,1000);
  M1.power_require = m1;
  M2.power_require = m2; 
  M3.power_require = m3; 
  M4.power_require = m4; 
  if(millis()>next_joystick) {
    Serial.print("\nm1: "); Serial.print(m1); 
    Serial.print("\nm2: "); Serial.print(m2); 
    Serial.print("\nm3: "); Serial.print(m3); 
    Serial.print("\nm4: "); Serial.print(m4);
    next_joystick = millis()+1000;
  }
#endif
  // serial input
  if(Serial.available() > 0) {
    byte b = Serial.read(); // read byte
    switch(b) {
      case 13: break;
      case ',': pulse_wait_time = 1000; break;
      case '.': pulse_wait_time = 1;  motive.stream_debug(&Serial); break;
      case '1': M1.flow_target = 1000; M1.flow_rate = 1; break;
      case '2': M2.flow_target = 1000; M2.flow_rate = 1;  break;
      case '3': M3.flow_target = 1000; M3.flow_rate = 1;  break;
      case '4': M4.flow_target = 1000; M4.flow_rate = 1;  break;
      case 'q': M1.power_require += 100; break;
      case 'w': M2.power_require += 100; break;
      case 'e': M3.power_require += 100; break;
      case 'r': M4.power_require += 100; break;
      case 'a': M1.power_require -= 100; break;
      case 's': M2.power_require -= 100; break;
      case 'd': M3.power_require -= 100; break;
      case 'f': M4.power_require -= 100; break;
      case 'z': M1.flow_target = 0; M1.flow_rate = 1; break;
      case 'x': M2.flow_target = 0; M2.flow_rate = 1;  break;
      case 'c': M3.flow_target = 0; M3.flow_rate = 1;  break;
      case 'v': M4.flow_target = 0; M4.flow_rate = 1;  break;
#ifdef hal_adamotor
      case 'Q': AdaMotor.reverse(0); AdaMotor.exec(); break;
      case 'A': AdaMotor.forward(0); AdaMotor.exec(); break;
      case 'W': AdaMotor.reverse(1); AdaMotor.exec(); break;
      case 'S': AdaMotor.forward(1); AdaMotor.exec(); break;
      case 'E': AdaMotor.reverse(2); AdaMotor.exec(); break;
      case 'D': AdaMotor.forward(2); AdaMotor.exec(); break;
      case 'R': AdaMotor.reverse(3); AdaMotor.exec(); break;
      case 'F': AdaMotor.forward(3); AdaMotor.exec(); break;
#endif
    }
  }
  long time = millis();
  if(time>=pulse_next_time) {
    motive.pulse();
    if(pulse_wait_time>500) {
      motive.stream_debug(&Serial);
    }
    pulse_next_time = time + pulse_wait_time;
  }
 #ifdef hal_adamotor
  if(time>=ada_next_time) {
    AdaMotor.exec();
    ada_next_time = time + ada_wait_time;
  } 
 #endif
}