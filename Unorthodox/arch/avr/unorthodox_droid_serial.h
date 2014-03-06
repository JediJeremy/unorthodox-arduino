
/*
  nano operating system for the Arduino (Leonardo)  (c) Jeremy Lee 2013
 */

#ifndef UNORTHODOX_DROID_SERIAL_H
#define UNORTHODOX_DROID_SERIAL_H

/*
 * SerialDroid is for cases where we communicate with the user via a serial interface.
 */

class SerialDroid : public BaseDroid {
public:
  SerialDroid(Page * storage, word size) : BaseDroid(storage, size) { }
  void serial_signal_symbol(Stream * tty, byte sig) {
    tty->print('S'); tty->print(sig); tty->print(' '); 
  }
  void serial_code_symbol(Stream * tty, byte code) {
    tty->print('{'); tty->print(code,HEX); tty->print('}'); 
  }
  void serial_signals_list(Stream * tty) {
    // list all the signals to the tty
    for(byte sig=0; sig<SIGNALS; sig++) {
      serial_signal_symbol(tty, sig);
      // look up the signal's memorypage
      Page * page = fs.token_page(sig);
      if(page==0) {
        // empty symbol
        tty->print("-");
      } else {
        // and get it's token page size
        byte size = fs.token_size(sig);
        // go through the byte stream, first pass - look for initial codes
        for(byte i=0; i<size; i++) {
          byte b = page->read_byte(i);
          // is it the right kind?
          if((b&0x80)==0x00) {
            // dump the code token
            serial_code_symbol(tty, b);
          }
        }
        bool first = true;
        // go through the byte stream, second pass - look for delayed codes
        for(byte i=0; i<size; i++) {
          byte b = page->read_byte(i);
          // is it the right kind?
          if((b&0x80)==0x80) {
            // dump the code token
            if(first) {
              tty->print("...");
              first = false;
            }
            serial_code_symbol(tty, b&0x7F);
          }
        }
        // free objects
        delete page;
      }
      tty->print("\n");
    }
  }
  
  void serial_codes_list(Stream * tty) {
    // list all the signals to the tty
    for(byte code=0; code<CODES; code++) {
      serial_code_list(tty, code);
      tty->print("\n");
    }
  }
  
  void serial_code_list(Stream * tty, byte code) {
    serial_code_symbol(tty, code);
    tty->print("\n");
    // look up the codes' memorypage
    Page * page = fs.token_page(code | 0x80);
    if(page==0) {
      // empty symbol
      tty->print("-");
    } else {
      // and get it's token page size
      byte size = fs.token_size(code | 0x80);
      if(size==0) {
       // empty symbol
       tty->print("--");
      } else {
        // go through the byte stream, decode instructions
        byte op = 0;
        char ac_1 = ' ';
        char ac_2 = ' ';
        char op_1 = ' ';
        char op_2 = ' ';
        byte modea = 0;
        bool moded = false;
        byte modem = 0;
        for(byte i=0; i<size; i++) {
          byte b = page->read_byte(i);
          // first bit
          if(b>=128) {
            if(b>=192) {
              // SKIP opcode
              switch((b & 0x30)>>4) {
                case 0: op_1 = '='; break; // =0
                case 1: op_1 = '!'; break; // !0
                case 2: op_1 = '<'; break; // <0
                case 3: op_1 = '>'; break; // >0
              }
              tty->print(op_1); tty->print("0 ... "); tty->print(b&0x0F);
            } else {
              // MODE opcode
              byte modea = (b & 0x30)>>4;
              bool moded = (b & 0x08);
              byte modem = (b & 0x07);
              // access mode brackets
              ac_1 = (modea==3) ? ']' : ' ';
              ac_2 = (modea==2) ? '[' : ' ';
              // math mode operator
              switch(modem) {
                case 0: op_1 = ':'; break;
                case 1: op_1 = '+'; break;
                case 2: op_1 = '-'; break;
                case 3: op_1 = 'x'; break;
                case 4: op_1 = '/'; break;
                case 5: op_1 = '&'; break;
                case 6: op_1 = '#'; break;
                case 7: op_1 = '^'; break;
              }
              // reverse or normal?
              if(moded) {
                op_2 = op_1; op_1 = '=';
              } else {
                op_2 = '=';
              }
            }
          } else {
            // CONST opcode
            // output the opcode characters
            tty->print(ac_1);
            tty->print(op_1); tty->print(op_2);
            tty->print(ac_2);
            // dump the rhs token
            if(modea==0) {
              // literal
              tty->print(b);
            } else {
              // symbol
              serial_signal_symbol(tty, b);
            }
          }
          
        }
      }
      // free objects
      delete page;
    }
    tty->print("\n");
  }
  
};



#endif