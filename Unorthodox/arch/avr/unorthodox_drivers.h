
/*
  nano operating system for the Arduino (Leonardo)  (c) Jeremy Lee 2013
*/

#ifndef UNORTHODOX_DRIVERS_H
#define UNORTHODOX_DRIVERS_H

//    FILE: dht11.cpp
// VERSION: 0.5
// PURPOSE: DHT11 Temperature & Humidity Sensor library for Arduino
// LICENSE: GPL v3 (http://www.gnu.org/licenses/gpl.html)
//
// DATASHEET: http://www.micro4you.com/files/sensor/DHT11.pdf
//
// HISTORY:
// George Hadjikyriacou - Original version (??)
// Mod by SimKard - Version 0.2 (24/11/2010)
// Mod by Rob Tillaart - Version 0.3 (28/03/2011)
// + added comments
// + removed all non DHT11 specific code
// + added references
// Mod by Rob Tillaart - Version 0.4 (17/03/2012)
// + added 1.0 support
// Mod by Rob Tillaart - Version 0.4.1 (19/05/2012)
// + added error codes
// Mod by Jeremy Lee - Version 0.5 (30/10/2013)
// + refactored as device class
class DHT11 : public Device {

  public:
    static const int DHTLIB_OK = 0;
    static const int DHTLIB_ERROR_CHECKSUM = -1;
    static const int DHTLIB_ERROR_TIMEOUT  = -2;
    static const int DHTLIB_OFFLINE  = -3;
    // public properties
    int data_pin;
    int power_pin;
    int humidity;
    int temperature;
    int state;
    // constructor
    DHT11(int data, int power) {
      this->data_pin = data;
      this->power_pin = power;
      state = DHTLIB_OFFLINE;
      humidity = 0;
      temperature = 0;
    }
    void start() {
      // pull the data pin low
      digitalWrite(data_pin, LOW); pinMode(data_pin, OUTPUT); 
      // and optionally apply power
      if(power_pin>=0) {
        pinMode(power_pin, OUTPUT); digitalWrite(power_pin, HIGH);
      }
    }
    void stop() {
      // remove optional power
      if(power_pin>=0) {
        digitalWrite(power_pin, LOW); pinMode(power_pin, INPUT);
      }
      // release the input pin back to high-impedance state
      digitalWrite(data_pin, LOW); pinMode(data_pin, INPUT);
    }
    // 
    bool sample() {
      delay(100);
      // BUFFER TO RECEIVE
      uint8_t bits[5] = {0,0,0,0,0};
      uint8_t cnt = 7;
      uint8_t idx = 0;   
      // REQUEST SAMPLE
      pinMode(data_pin, OUTPUT);
      digitalWrite(data_pin, LOW);
      delay(18);
      digitalWrite(data_pin, HIGH);
      delayMicroseconds(40);
      pinMode(data_pin, INPUT);
      // ACKNOWLEDGE or TIMEOUT
      unsigned int loopCnt = 10000;
      while(digitalRead(data_pin) == LOW) {
        if (loopCnt-- == 0) { state = DHTLIB_ERROR_TIMEOUT; return false; }
      }
      loopCnt = 10000;
      while(digitalRead(data_pin) == HIGH) {
        if (loopCnt-- == 0) { state = DHTLIB_ERROR_TIMEOUT; return false; }
      }
      // READ OUTPUT - 40 BITS => 5 BYTES or TIMEOUT
      for (int i=0; i<40; i++) {
        loopCnt = 10000;
        while(digitalRead(data_pin) == LOW) {
          if (loopCnt-- == 0) { state = DHTLIB_ERROR_TIMEOUT; return false; }
        }
        unsigned long t = micros();
        loopCnt = 10000;
        while(digitalRead(data_pin) == HIGH) {
          if (loopCnt-- == 0) { state = DHTLIB_ERROR_TIMEOUT; return false; }
        }
        if ((micros() - t) > 40) bits[idx] |= (1 << cnt);
        if (cnt == 0) {  // next byte?
          cnt = 7;    // restart at MSB
          idx++;      // next byte!
        }
        else cnt--;
      }
      // WRITE TO RIGHT VARS
      // as bits[1] and bits[3] are allways zero they are omitted in formulas.
      humidity    = bits[0]; 
      temperature = bits[2]; 
      uint8_t sum = bits[0] + bits[2];  
      if (bits[4] != sum) { state = DHTLIB_ERROR_CHECKSUM; return false; }
      state = DHTLIB_OK;
      return true;
    }
};

#endif