
/*
  nano operating system for the Arduino (Leonardo)  (c) Jeremy Lee 2013
*/

#ifndef UNORTHODOX_DEVICE_H
#define UNORTHODOX_DEVICE_H

class Device {
  public:
    virtual void start() { }
    virtual void stop() {  }
    virtual void sleep() {  }
    virtual void wake() {  }
    virtual int error() { return 0; }
};

// if the SPI library is inclided, then add some device types
#ifdef _SPI_H_INCLUDED

/*
  SPIDevice
*/
class SPIDevice : public Device {
  protected:
    int chip_select;
  public:
    // constructor
    SPIDevice() {
      // set up the slave select line
      chip_select = 0;
    }
  protected:
  
    void transfer_word(word * w) { 
      word v = *w;
      digitalWrite(chip_select, HIGH); // should have been this way to start with...
      delayMicroseconds(10); // not sure how long since the last one, so wait a moment
      digitalWrite(chip_select, LOW); // select the chip, let it know data is coming.
      delayMicroseconds(4); // give it a moment to prepare
      word r = ( SPI.transfer((v&0xFF00)>>8)<<8 ) | ( SPI.transfer(v&0x00FF) ); // transfer the data
      digitalWrite(chip_select, HIGH); // and de-select the chip
      *w = r; // swap the buffer word with it's transfer result
    }

    void transfer_words(word * w, int count) {
      for(int i=0; i<count; i++) transfer_word(&w[i]);
    }

    void transfer_bytes(byte * b, int count) {
      digitalWrite(chip_select, HIGH); // should have been this way to start with...
      delayMicroseconds(10); // not sure how long since the last one, so wait a moment
      digitalWrite(chip_select, LOW); // select the chip, let it know data is coming.
      delayMicroseconds(4); // give it a moment to prepare
      while(count-->0) { *b = SPI.transfer(*b); b++; } // transfer the data
      digitalWrite(chip_select, HIGH); // and de-select the chip
    }
    
};

#endif

#endif