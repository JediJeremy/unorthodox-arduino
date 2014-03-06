#include <unorthodox.h>


unsigned long debug_count = -1;
unsigned long stop_count = -1;


// create a 1024 byte buffer
byte storage[1024];
// wrap it in a virtual page
MemoryPage store(storage);
// create tokenfs storage
TokenFS fs(&store,1024,192);

// remember token state
byte token_state[256];
const int check_tokens = 192;

int block_size = 0;
int block_mask = 7;

void setup() {
  // initialize serial port
  Serial.begin(9600);
  delay(10000);
  // leonardo - wait for connection
  while(!Serial) { }
  memset(token_state,0,256); // clear the token state
  // add a selection of records to the store
  Serial.print("\n creating blocks..."); 
  for(int i=0; i<check_tokens; i++) {
    writeblock(i);
    if(!checkblocks(false)) checkblocks(true);
  }
  // dump the record store in it's initial state  
  checkblocks(true);
}

bool more = true;
int next = 0;
int dot_count = 0;
int line_count = 200;
unsigned long loop_count = 0;

void loop() {
  if(more) {
    // add the block again
    bool wr = writeblock(next++);
    if(next>=check_tokens) next = 0;
    more = checkblocks(loop_count >= debug_count);
    if(loop_count < debug_count) {
      if(dot_count==0) {
        if(line_count==0) {
          line_count = 200;
          checkblocks(true); // debug
        }
        line_count--;
        Serial.print("\n"); Serial.print(loop_count);
        dot_count=50;
      }
      dot_count--;
      if(more) {
        Serial.print(wr?'.':'/');
        if(dot_count==0) {
          Serial.print(" used:"); Serial.print(fs.used);
          Serial.print(" updates:"); Serial.print(fs.updates);
        }
      } else {
        Serial.print('!');
        // debug the broken state
        checkblocks(true);
      }
    }
    loop_count++;
    if(loop_count>=stop_count) more = false;
  }
}


bool writeblock(byte token) {
  byte block[16];
  // the length will be based on the last three digits.
  // int len = (token & 7) + 1;
  // the length will be the next in the sequence
  // int len = block_size + 1;
  // block_size = (block_size+1) & block_mask;
  // block size is random
  int len = random(6) + 1;
  // fill the block with the token byte
  memset(block,token,len);
  // write it to the filesystem
  MemoryPage page(block);
  if(loop_count < debug_count) {
    // Serial.print('W');  Serial.print(token); // Serial.print(' ');
  } else {
    Serial.print("\n Writing block ");  Serial.print(token); // Serial.print(' ');
  }
  if( fs.token_write(token, &page, len) ) {
      // record if the block is empty or not
      token_state[token] = len - 1;
      return true;
  } else {
     // Serial.print("\n Write Failed ");
     token_state[token] = 0;
     return false;
  }
}


bool checkblocks(bool debug) {
  bool result = true;
  // mark all tokens as not found yet
  byte token_count[256]; memset(token_count,0,256);
  // for(int i=0; i<256; i++) token_state[i] |= 0x01;
  if(debug) Serial.print("\n ============================== ");
  // second pass: load all the entires in journal order from the head to tail.
  fs.pass_reset();
  while(fs.i_more) {
    fs.pass_next();
    if(fs.i_valid) {
        if(debug) {
          Serial.print("\n block:"); Serial.print(fs.i_block-1); Serial.print(":"); Serial.print(fs.i_size+5);
          Serial.print("       "); 
        }
        for(int i=0; i<fs.i_size; i++) {
          byte b = fs.page->read_byte(fs.i_block+i);
          if(debug) Serial.print(b);
          if(i!=0) {
            // mark off this token as found
            token_count[b]++;
          }
          if(debug) Serial.print((i==0)?'.':' ');
        }
    }
  }
  // go through the tokens we were tracking
  for(int i=0; i<check_tokens; i++) {
    if( (token_count[i]==0) && (token_state[i]!=0)) {
      if(debug) {
        Serial.print("\n missing:"); Serial.print(i);
      }
      result = false;
    } else {
      // check that the expected length is still correct
      if(token_state[i] != fs.token_size(i)) {
        if(debug) {
          Serial.print("\n incorrect length:"); Serial.print(i);
          Serial.print(" put:"); Serial.print(token_state[i]);
          Serial.print(" got:"); Serial.print(fs.token_size(i));
        }
        result = false;
      }
    }
  }
  if(debug) {
    Serial.print("\n head:"); Serial.print(fs.head); 
    Serial.print(" tail:"); Serial.print(fs.tail); 
    Serial.print(" next:"); Serial.print(fs.next);
    Serial.print(" used:"); Serial.print(fs.used);
    Serial.print(" updates:"); Serial.print(fs.updates);
  }
  return result;
}

