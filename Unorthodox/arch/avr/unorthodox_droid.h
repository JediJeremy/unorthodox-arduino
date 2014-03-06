
/*
  nano operating system for the Arduino (Leonardo)  (c) Jeremy Lee 2013
 */

#ifndef UNORTHODOX_DROID_H
#define UNORTHODOX_DROID_H



/*
 * A classic issue is that buttons and switches need to be 'debounced' due to the noisy nature
 * of metals making contact, at the microsecond scale.
 */
class Debounce {
protected:
	bool state;
	bool transit;
	long time;
	word delay;
public:
	Debounce(word delay) {
		delay = delay;
		state = false;
		transit = false;
	}
	bool update(bool b, long t) {
		if(transit) {
			// transit
			if(state!=b) {
				// still changed
				if( t>time ) { //  have we waited long enough?
					state = !state;
					transit = false;
				}
			} else {
				// lost it again.
				transit = false;
			}
		} else {
			// stable
			if(state!=b) {
				transit = true; // start transit
				time = t + delay; // compute timeout
			}
		}
		// return the debounced button state
		return state;
	}
};


struct JoystickAxis {
	int inertia;
	int center; 
	int deadzone;
	long delta;
	int sample;
	JoystickAxis() : inertia(0), deadzone(8), sample(0) { }
};

/*


class Motor {
public:
	// complete list of limit types, in case a motor implements some.
	static const byte POSITION_MIN_LIMIT = 1;
	static const byte POSITION_MAX_LIMIT = 2;
	static const byte SPEED_MIN_LIMIT = 3;
	static const byte SPEED_MAX_LIMIT = 4;
	static const byte ACCEL_MIN_LIMIT = 5;
	static const byte ACCEL_MAX_LIMIT = 6;
	static const byte IMPULSE_MIN_LIMIT = 7;
	static const byte IMPULSE_MAX_LIMIT = 8;
	static const byte SNAP_MIN_LIMIT = 9;
	static const byte SNAP_MAX_LIMIT = 10;
	static const byte POWER_MIN_LIMIT = 11;
	static const byte POWER_MAX_LIMIT = 12;
	static const byte PWM_PERIOD_MIN_LIMIT = 13;
	static const byte PWM_PERIOD_MAX_LIMIT = 14;
	static const byte PWM_PULSE_MIN_LIMIT = 15;
	static const byte PWM_PULSE_MAX_LIMIT = 16;
	static const byte PWM_GAP_MIN_LIMIT = 17;
	static const byte PWM_GAP_MAX_LIMIT = 18;
	// methods that all motors must implement
	virtual void start(); // prepare, arm, energise, and start the motor running (if it has a minimum speed). If the motor was idled, the return it to it's previous operational state.
	virtual void idle();  // put the motor in a minimum-energy safe (for the motor) state from which it is quick to start again. If it has a minimum speed, it will stay running.
	virtual void stop();  // if running, crash-stop the motor (with brakes if possible) and then de-energise or disarm to a safe (for the user) state. A timeout before the motor is allowed to start again wouldn't be unusual.
	virtual void speed(); // read or adjust the motor's speed to a new value.
	virtual void speed(int s, int t); // read or adjust the motor's speed to a new value.
	virtual void rate();  // read or adjust the rate at which the motor's speed can be updated by code.
	virtual void rate(int r);  // read or adjust the rate at which the motor's speed can be updated by code.
	virtual void limit(byte type); // read or adjust a motor limit
	virtual void limit(byte type, int val); // read or adjust a motor limit
};

class EncoderMotor {
	virtual void position();
};

class PositionMotor : public EncoderMotor {
	virtual void move_to();
	virtual void move_by();
};

class DCMotor: public Motor {
};

class StepperMotor : public PositionMotor {
};

class ServoMotor : public PositionMotor {
};

class PhasedMotor : public PositionMotor {
	
};
	
 */

/*
	Adafruit Motor shield drivers
 */

class AdaMotorM1 {
	
};


/*
	The base device class for dealing with the adafruit motor shield. Mostly this just
	sends a direction byte to the shift register - all the actual "motor control" depends
	on application. (Mostly using PWM, or the Motivator)
 */
class AdaMotorDevice {
protected:
	static const int PWM_0A = 6;
	static const int PWM_0B = 5;
	static const int PWM_1A = 9;
	static const int PWM_1B = 10;
	static const int PWM_2A = 11;
	static const int PWM_2B = 3;
	static const int DIR_EN = 7;
	static const int DIR_SDA = 8;
	static const int DIR_CLK = 4;
	static const int DIR_LATCH = 12;
	// latch bit mappings
	static const int LATCH_1A = 2;
	static const int LATCH_1B = 3;
	static const int LATCH_2A = 1;
	static const int LATCH_2B = 4;
	static const int LATCH_3A = 0;
	static const int LATCH_3B = 6;
	static const int LATCH_4A = 5;
	static const int LATCH_4B = 7;
	//
	byte _motor_forward[4];
public:
	// constructor
	AdaMotorDevice() {
		// since we have so many to do..
		int out[] = { LOW, HIGH, LOW, LOW, HIGH, HIGH, LOW, LOW, LOW, HIGH };
		for(int i=3; i<13; i++) {
			pinMode(i, OUTPUT); digitalWrite(i,out[i-3]);
		}
		// all motors start forward.
		memset(_motor_forward, 0x01, 4);
		update_direction();
	}
	// destructor
	~AdaMotorDevice() {
		
	}
	void forward(byte motor) {
		if(motor<4) _motor_forward[motor] = 1;
	}
	void reverse(byte motor) {
		if(motor<4) _motor_forward[motor] = 0;		
	}
	void exec() {
		update_direction();
	}
private:
	// update the motor shield with the latest direction information
	void update_direction() {
		// rebuild the direction byte
		byte d = 0;
		d |= _motor_forward[0] ? (1<<LATCH_1A) : (1<<LATCH_1B);
		d |= _motor_forward[1] ? (1<<LATCH_2A) : (1<<LATCH_2B);
		d |= _motor_forward[2] ? (1<<LATCH_3A) : (1<<LATCH_3B);
		d |= _motor_forward[3] ? (1<<LATCH_4A) : (1<<LATCH_4B);
		// send the update
		send_direction(d);
	}
	
	// send direction byte to motorshield shift register
	void send_direction(byte dir) {
		// enable the shift register
		// digitalWrite(DIR_EN, HIGH);
		// release the latch before transfer
		digitalWrite(DIR_LATCH, LOW);
		// and start clocking out the bits
		shiftOut(DIR_SDA, DIR_CLK, MSBFIRST, dir);
		// and set the latch
		digitalWrite(DIR_LATCH, HIGH);		
		// this should always be low, but take a moment to be sure
		digitalWrite(DIR_EN, LOW);
	}
};


class PingDevice {
	
};


class ControlDevice {
	virtual void data_rate();
	virtual void recieve();
	virtual void transmit();
};

class IRControlDevice : public ControlDevice {
	virtual void frequency();
};

class RFControlDevice : public ControlDevice {
	virtual void frequency();
	
};



/*
	Digital nervous system for small robots.

	Instancing the object will allocate a fair whack of memory (>1K) for the high-speed signal arrays and
	mipmaps. Extending the on_event methods allows droid-specific functions to be attached to the base object.
 */
class BaseDroid {
public:
	static const int SIGNALS = 128;
	static const int CODES = 64;
	static const int OnWrite = 1;
	static const int OnChange = 2;
	static const int OnTimeout = 4;
	TokenFS fs;
	int  signal_value[SIGNALS];
	byte signal_timer[SIGNALS];
	word signal_value_mipmap[9]; 
	word signal_timer_mipmap[9];
	// word signal_time;    
	static const byte signal_delay = 100; // milliseconds 
protected:
	unsigned long last_time;
	// word token_index[SIGNALS+CODES]; // the token index - for the TokenFS to fill.
	// word _debug_signal;
	// word _debug_code;    
public:
	BaseDroid(Page * storage, word size) : fs(storage,size, SIGNALS+CODES) {
		// Create a TokenFS "filesystem" from the entire 1K EEPROM.
		// fs.start();
		// clear the start time
		last_time = 0;
		// clear_mipmaps();
		memset(signal_value_mipmap,0,36); // assume both are contiguous
	}
	/*
	 *	Notify sub-classes of updates to a signal.
	 *	@param sig : signal id  
	 *	@param v : signal value
	 *	@param mode : state mode (OnWrite | OnChange | OnTimeout);  
	 */
	virtual void signal_state(word sig, int v, int mode) = 0;
	
	/*
	 * Perform droid update pass
	 */
	void exec(int dt) {
		byte i,j;
		// process each signal_value flag
		word mip = signal_value_mipmap[8];
		// are there any signals that fired?
		if(mip!=0) {
			// outer set
			byte sig = 0;
			for(byte i=0; i<8; i++) { // outer loop
				if(mip & (1<<i)) {
					// there are things to do
					for(j=0; j<16; j++) { // inner loop
						if(signal_value_mipmap[i] & (1<<j)) {
							// Serial.print("\n sig:"); Serial.print(sig);
							// signal to process. flip it off.
							set_mipmap(sig,false, &signal_value_mipmap[0], &signal_value_mipmap[8]);
							// execute the symbol onchange codes
							signal_exec(sig,false);
						}
						sig++;
					}
				} else {
					// we skipped a whole block
					sig+=16;
				}
			}
		}
		// did any measurable time pass?
		if(dt) {
			// our counters are one byte max, so we can downcovert that.
			byte dtb = (dt>255) ? 255 : dt;
			// process each signal_timer flag
			word mip = signal_timer_mipmap[8];
			// are there any signals that fired?
			if(mip!=0) {
				// outer set
				byte sig = 0;
				for(i=0; i<8; i++) { // outer loop
					if(mip & (1<<i)) {
						// there are things to do
						for(j=0; j<16; j++) { // inner loop
							if(signal_timer_mipmap[i] & (1<<j)) {
								// has the signal timeout expired?
								if(signal_timer[sig] <= dtb) {
									// signal reached timeout. flip it off.
									set_mipmap(sig,false, &signal_timer_mipmap[0], &signal_timer_mipmap[8]);
									// execute the symbol ontimer codes
									signal_exec(sig,true);
									// notify subclass listeners that the signal timeout has occured
									signal_state(sig,signal_value[sig], OnTimeout);
									// all done
									signal_timer[sig] = 0;
								} else {
									// decrement remaining time
									signal_timer[sig] -= dtb;
								}
							}
							sig++;
						}
					} else {
						// we skipped a whole block
						sig+=16;
					}
				}
			}
		} 		
		
	}
	
	/*
	 * Set a droid signal to the given value. This may cause an entire chain of droid events during the next exec(),
	 * but will notify the signal_state listener immediately. 
	 */
	void set_signal(byte sig, int v) {
		int mode = OnWrite;
		// has it changed?
		if(signal_value[sig] != v) {
			mode = OnWrite | OnChange;
			signal_value[sig] = v;
			signal_timer[sig] = signal_delay;
			// set mipmap flags
			set_mipmap(sig, true, &signal_value_mipmap[0], &signal_value_mipmap[8]);
			set_mipmap(sig, true, &signal_timer_mipmap[0], &signal_timer_mipmap[8]);
		}
		// notify the sub-class
		signal_state(sig,v,mode);
	}


	/*
	 * Execute all available codes (either immediate or timer group) attached the signal.
	 */
	void signal_exec(byte sig, bool timers) {
		byte token = sig & 0x7F;
		// Serial.print("signal "); Serial.print(sig); Serial.print(","); Serial.print(timers); Serial.print("\n");
		// timers mask
		byte tflag = timers ? 0x80 : 0x00;
		// and get it's token page size
		byte size = fs.token_size(token);
		if(size==0) return;
		// look up the signal's memorypage
		Page * page = fs.token_page(token);
		// go through the byte stream
		for(byte i=0; i<size; i++) {
			byte b = page->read_byte(i);
			// is it the right kind?
			if((b&0x80)==tflag) code_exec(b,sig);
		}
		// free objects
		delete page;
	}

	/*
	 * Execute a code definition by id (as stored in the filesystem) as caused by the nominated signal.
	 */
	void code_exec(byte code, byte sig) {
		byte token = (code & 0x3F) | 0x80;
		// Serial.print(" code:"); Serial.print(code);
		// and get it's token page size
		byte size = fs.token_size(token);
		if(size==0) return;
		// look up the code memorypage
		Page * page = fs.token_page(token);
		// go through the byte stream
		op_mode = 0;
		op_skip = 0;
		// code_stream(page,size,sig);
		code_stream(page,size,0);
		// free objects
		delete page;
	}
	
	/*
	 * code stream state
	 */
	byte op_mode;
	byte op_skip;

	/*
	 * Execute a stream of code instructions
	 */
	int code_stream(Page * page, byte size, int acc) {
		for(byte i=0; i<size; i++) {
			byte b = page->read_byte(i);
			// first bit determines if is an opcode or operand
			if(b & 0x80) {
				// second bit determines if the opcode is a skip
				if(b & 0x40) {
					// SKIP opcode
					if(op_skip==0) { 
						// accumulator test
						bool test = (b & 0x20) ? (
							(b & 0x10) ? (acc>0) : (acc<0)
						) : (
							(b & 0x10) ? (acc!=0) : (acc==0)
						);
						if(test)  op_skip = b & 0x0F; // start skipping
						// Serial.print(' '); Serial.print(b, HEX); Serial.print(' '); Serial.print(b, acc); Serial.print(test?'t':'f'); 
					} else {
						// we are already skipping
						op_skip--;
					}
				} else {
					// MODE opcode
					op_mode = b;
				}
			} else {
				// CONST opcode
				if(op_skip==0) { 
					// get the current source and destination values
					// accumulator access
					int lhs = (op_mode & 0x20) ? signal_value[acc&0x7F] : acc; 
					// operand access
					int rhs = (op_mode & 0x10) ? signal_value[b&0x7F] : b; 
					int src, dst;
					if(op_mode & 0x08) {
						src = lhs; dst = rhs;
					} else {
						src = rhs; dst = lhs;
					}
					// apply the math operator
					if(op_mode & 0x04) {
						if(op_mode & 0x02) {
							if(op_mode & 0x01) { dst ^= src; } else { dst |= src; }
						} else {
							if(op_mode & 0x01) { dst &= src; } else { dst /= src; }
						}
					} else {
						if(op_mode & 0x02) {
							if(op_mode & 0x01) { dst *= src; } else { dst -= src; }
						} else {
							if(op_mode & 0x01) { dst += src; } else { dst  = src; }
						}
					}
					// now send the value to it's correct place
					int sig = -1;
					if(op_mode & 0x08) {
						// send to rhs operand. indirect?
						if(op_mode & 0x10) { sig = b & 0x7F; } else { } 
					} else {
						// send to lhs accumulator. indirect?
						if(op_mode & 0x20) { sig = acc & 0x7F; } else { acc = dst; } 						
					}
					// update signal value
					if(sig!=-1) set_signal(sig, dst);
				} else {
					// we are skipping
					op_skip--;
				}
			}
		}
		// return accumulator value
		return acc;
	}
	
	void set_mipmap(word i, bool bit, word * level1, word * level2) {
		// convert the index into word and bit index.
		word iw = i >> 4;
		word ib = i & 0x0F;
		// and the mipmap word and bit from that
		word mw = iw >> 4;
		word mb = iw & 0x0F;
		// flip the bit
		if(bit) { // on
			level1[iw] |= (1<<ib);
			level2[mw] |= (1<<mb);
		} else { // off
			level1[iw] &= ~(1<<ib);
			if(level1[iw]==0) {
				level2[mw] &= ~(1<<mb);
			}
		}
	}

};



/*
 	 Use the Beeps!
 	 
 	 This is a simple "sound effects module" intended to drive a small piezoelectric speaker
 	 directly from a pin on the Arduino. Effects are all frequency-modulation, since varying
 	 amplitude is not really an option. 
 	 
 	 tone() is used to generate the signal on the given pin, and so all the usual rules about
 	 pin and timer mappings for your particular arduino will apply. millis() is also used to
 	 check time.
 	 
 	 The module has a small 'ring' buffer (usually 16 entries long) which queues all waiting
 	 'sound fragments', so you don't have to wait around. The next fragment is automatically
 	 played once the previous fragment ends.
 	 
 	 The vocalize() methods can therefore take a single word, an array of words, or a memory
 	 page object.
 	 
 	 The module can also be asked to 'index' a stream of words which encodes a small
 	 (up to 255 entries) table of 'vocalizer strings', and then to play back an index entry.
 	 The Beeps object must allocate and maintain an index table of the given size, so smaller
 	 is better in this case. 
 	 
 	 This stream index uses the fact that a MSB byte of zero is not a valid vocalizer token, and
 	 so can be used to encode a sentinel token which indicates the start of the next entry. The
 	 'zero' token is reserved as an end-of-stream marker, and therefore the index cannot contain
 	 an entry for that key.

     An example of declaring and using stream indexes:
     
		PROGMEM prog_uint16_t vocal_stream[] = {
		  1,  0x3B15, 0x264B, 0x385D, 0x3291, 0x2832, 0x1AB8,  
		  2,  0x139A, 0x3DA4, 0x1915, 0x21EB, 0x2822, 0x36D4, 
		  3,  0x1D65, 0x27FC, 0x2726, 0x221E, 0x3B91,  
		  4,  0x239B, 0x2389, 0x2664, 
		  5,  0x24F1, 0x2023, 0x3950,  
		  0 // end of list
		};	
		NearProgramPage vocals((prog_uchar *)vocal_stream);
		
		Beeps beeps(3);
		beeps.index_decode(&vocals, 8); // make room for eight entries
		beeps.index_vocalize(&vocals, 2); // play entry 2
		
	Note that a memory page representing the 'real' storage of the streams must be passed both
	to the index 'decode' and 'play' methods. This object does NOT need to be persistent between
	calls, so long as it can be recreated. Therefore it is not stored internally to the object,
	and must be passed each time. (This creates opportunities to move the stream between
	storage mediums.)
     
 */
class DroidBeeps {
private:
	static const byte BUFFER_SIZE = 16;
	static const byte BUFFER_MASK = 0x0F;
	
	// two nice-looking fibonacci primes for the random number generator
	static const word PRIME_A = 1597;
	static const word PRIME_B = 28657;
	// operation codes
	static const byte STOP    = 0;
	static const byte SILENCE = 1;
	static const byte FLAT    = 2;
	static const byte SWEEP   = 3;
	static const byte WARBLE  = 4;
	// the circular buffer
	bool _idle;
	word _buffer[BUFFER_SIZE+2];
	byte _head;
	byte _tail;
	// current sound fragment
	int _sound_elapsed;
	int _sound_time;
	int _sound_f[2];
	byte _sound_op;
	word _random_a;
	word _random_b;
	// vocalizer index
	// output hardware
	int _sound_pin;

public:
	DroidBeeps(int pin) {
		_sound_pin = pin;
		_idle = true;
		_head = 0;
		_tail = 0;
		_sound_op = STOP;
	}
	
	// string version
	void vocalize(word * utter, byte count) {
		word i=0;
		while(i<count) {
			vocalize(utter[i++]);
		}
	}
	
	// memorypage version
	void vocalize(Page * src, word index, byte count) {
		word i=0;
		while(i<count) {
			word p = index+i*2; i++;
			vocalize(src->read_word(p));
		}
	}
	
	void vocalize(word w) {
		// Serial.print("\n voc "); Serial.print(w); 
		word next = (_head + 1) & BUFFER_MASK;
		if(next==_tail) {
			// no more room.
			return;
		} else {
			// next word			
			_buffer[_head] = w;
			_head  = next;
			_idle = false;
		}
	}
	
	void exec(int dt) {
		// are we idle?
		if(_idle) return;
		// what frequency do we need to output?
		word sound = 0;
		bool silence = false;
		// did we have an operation in progress? (if not, try to start the next one)
		bool inc_buffer = true;
		if(_sound_op) {
			// how much time has elapsed since the start of the sound fragment?
			_sound_elapsed += dt;
			if(_sound_elapsed <= _sound_time) {
				// we are within 256 miliseconds of the start time
				// have we incremented in time?
				if(dt) {
					if(_sound_op == SWEEP) {
						// set the frequency to the proportional value based on our fragment time
						int remain = _sound_time - _sound_elapsed;
						long d = (long)_sound_elapsed * (long)_sound_f[1]  + (long)remain * (long)_sound_f[0]; 
						d /= _sound_time;
						sound = d;
					} else if(_sound_op == WARBLE) {
						// depending how much time has elapsed, 'gate' between the two frequencies
						sound = _sound_f[ (16 * _sound_elapsed / _sound_time) & 1 ];
					}
				}
				inc_buffer = false;
			}
		}
		if(inc_buffer) {
			// anything else in the buffer?
			if(_head==_tail) {
				// all done
				_idle = true;
				_sound_op = STOP;
				silence = true;
				// notify the idle method
			} else {
				_idle = false;
				// get the next token
				word w = _buffer[_tail++];
				_tail = _tail & BUFFER_MASK;
				// decode the utterance
				byte utter_op   = (w>>12) & 0x0f;
				byte utter_time = (w>>8)  & 0x0f;
				// convert everything into 'real' units and commit the operation
				_sound_time = (utter_time+1) * 15; // 15 milliseconds per unit time, making longest possible 240ms
				_sound_elapsed = 0;
				// sound frequency generator for both parameters
				for(int i=0; i<2; i++) {
					// get the parameter value
					byte n = (i==0) ? ((w>>4) & 0x0f) : (w & 0x0f);
					// frequencies come from a hardcoded range
					static const word f1 = 800; // 1277; /// MIDDLE_C/2/1024;
					static const word f2 = 800+2048; // 5109; // MIDDLE_C*2/1024;
					// interpolated by the parameter
					word f = f1 + (15-n) * (f2-f1)/16;
					// add a few random hertz either way (to prevent it being boring)					
					f += ((byte)_random_a ^ (byte)_random_b) & 0x7F; // random(0,128)
					_random_a += PRIME_A; _random_b += PRIME_B; // shuffle random bits
					// store the result
					_sound_f[i] = f;
				}
				_sound_op = utter_op;				
				// initialize the next sound op
				switch(_sound_op) {
				case SILENCE:
					silence = true;
					break;
				case FLAT:
				case SWEEP:
				case WARBLE:
					sound = _sound_f[0];
					break;
				default:
					// corrupt!?
					silence = true;
					_idle = true;
					_sound_op = 0;
				}
			}
		}
		if(sound) {
			pinMode(_sound_pin,OUTPUT);
			// tone(_sound_pin,sound);
			// set the PWM frequency of Timer1 to this value
			OCR1AH = sound >> 8;
			OCR1AL = sound;
		}
		if(silence) {
			// noTone(_sound_pin);
			pinMode(_sound_pin,INPUT);
		}
	}
	
	void vocalize_index(Page * stream, Page * index, byte voc) {
		word w[2];
		index->read(voc*2-2, w, 4); // read four bytes into the array		
		// vocalize the stream
		vocalize(stream, w[0]*2, w[1]-w[0]);
	}
	
	word random_fragment() {
		word w = 0;
		w |= (random(3)+1) << 12;
		w |= random(15) << 8;
		w |= random(255);
		return w;
	}
	
};

/*
	The Motivator classes are designed to drive sets of DC Motors, LEDs, Inductive and resistive loads,
	but NOT steppers or servos. (Yet. It does create a form of PWM, but at a significantly slower speed 
	that is incompatible. The "modulation" is all wrong.)
	
	The primary difference is that the Motivator is concerned with trying to stabilize the total
	draw on the power supply caused by multiple 'units' being switched on at once.
	
	Consider the usual timeline of events if motors are connected up to four PWM outputs on the Arduino:
	
	M1_=======___=======___=======___=======__
	M2_===_______===_______===_______===______
	M3_=========_=========_=========_=========
	M4_=_________=_________=_________=________
	
	Note how at the beginning of each period all the motors are switched on, creating the greatest
	possible instantaneous drain on the power supply. Then each motor is gradually turned off.
	
	Even if we manage to get the "phase correct PWM" going, those peaks are still there, but at least
	with a nice ramp up as well as down:

	M1__=======___=======___=======___=======_
	M2____===_______===_______===_______===___
	M3_=========_=========_=========_=========
	M4_____=_________=_________=_________=____
	
	What the motivator does is apply a "global cap" to the amount of power that each power source
	can provide simultaneously to a "pool" of units, and allocates the limited power based on which 
	units are furthest behind in their needs.
	
	We could tell the motivator to limit itself to "2 motors worth of current", (in arbitrary units)
	and it would shape the pulse train so that there was never more than that many turned on at once.
	It would try to put the "on" times of some motors within the "off" times of others:
	
	M1__=======___=======___=======___=======__
	M2_________===_______===_______===_______==
	M3_=========_=========_=========_=========_
	M4__________=_________=_________=_________=
	
	
	So long as the "total power" requested by all the motors is within the source maximum, all motors
	will get the power they wanted. If limits are hit then power will be shared, and if the outage
	duration is brief enough the deficit will be made up within the next few pulses.
	
	If set correctly this will help prevent momentary brown-outs in the power system, or other effects 
	of extreme transients caused by big loads from DC motors. "Power loss" will still occur when too many
	motors are turned on at once, but this is digitally enforced. That way our microcontroller doesn't 
	crash along with the motors.

	Actually, the Motivator will interleave the pulses much more chaotically; it "dithers" in
	a non-periodic way to maintain lowest errors on average. (Also called "spread spectrum")

	M1__=_==_==_====_===_=_====_===_=_===_===_
	M2___=__=__=____=___=__=___=___=_=___=___=
	M3_=========_==========_=======_=========_
	M4__________=_________=________=_________=
	
	This "dithering" is the main reason why the Motivator can't currently drive steppers or servos, but is 
	great for DC loads! 
	
	There are several other metrics which can be used to tune the motivator for real-world systems
	where all your motors are actually different. The "profile" numbers set how much of the power
	pool each unit draws from the power source when it is on. That way a bunch of little motors
	can be turned on, or one big motor, (but perhaps not both) up to the available pool size.
	
	Motors with a lower winding resistance can draw more supply power than others. Some may be better
	mechanically balanced, and use energy efficiently. Some may have large internal losses (or damage)
	that should be compensated for.
	
	The "profile_source" value describes how much 'energy' the unit _gains_ per pulse. The "profile_drain"
	value respectively defines how much 'energy' the power supply _loses_. In an idea world, this
	correspondence is 1:1. But if one motor is known to consume more current, and for less result, then
	the Motivator can compensate for that - so long as it is told.
	
	Therefore, correct 'modelling' of your power system and loads will bring great happiness. If you
	fail to know the true nature of your motors, you will only incur bad fortune. 
	
	Given these physical parameters, units are tuned on and off in order to keep their estimated "power level"
	as close to the desired quantity as possible. Note that the motivator currently has no way to sense any of this 
	(or self-tune) - the parameters merely exist for you to tweak.
	
	The same basic issues apply to other constant-average-current devices (like LEDs!) when
	hooked up directly to the Arduino. Sure, each pin can drive a 35mA ultra-bright LED just fine; but using 
	all pins together (16 or more, so >500mA!) far exceeds the maximum total rating of the chip.
	We don't really care if the LED is pulsed chaotically, so long as it's faster than persistence of vision
	and we don't try to run them all 100% at once. (And just in case our code goes funny, we'd like some safeguards...)
	
	One final function performed by the motivator is to smoothly 'tween' the desired power levels on each cycle, 
	since this would otherwise require yet another timer and is cheap to do.
	
	Motivator power units are arbitrary, but are stored as integers and for accuracy and should use a significant
	portion of the available (int) range. So in a four-motor system, assinging values of around 600 as each motors'
	source and drain metrics, and 2000 for the power source would create very "smooth" dithering patterns with the equivalent
	of hundreds of increments. If you liked, you could pretend they equated to "milliamps" or some fraction of a joule.
	
	The primary consideration is that the automatic tweening must move in constant integer increments each pulse, 
	therefore scaling your motors to "1000 power" and a flow rate of "1" means it will take 1 second (at the expected 1/ppms)
	to ramp over the full range at slowest possible acceleration. If you need much longer control times, then scale your
	numbers up to accomodate the finer resolution. The second consideration is that 16-bit integers are constrained to +=32k,
	and so the scaled 'source current' number needs to be less than that. (Usually only a problem if you have a dozen very
	slow devices on a huge supply.)
	
	Each source (and it's units) is treated as a completely seperate pool. Units should never be assigned
	to two source pools at once, but they can be omitted entirely. 
	

 */
class MotivatorUnit {
public:
	int power_require;	// current unit power requirement
	int power_level;	// current power level of unit
	int flow_target;	// animation target for power requirement flow
	int flow_rate;		// animation rate for power requirement flow
	int profile_source;	// how much source power is transferred per pulse
	int profile_drain;  // how much source power is used per pulse. ('loss' is the difference)
	byte is_on;			// is this unit currently switched on?
	virtual void switch_on();	// turn on - overload with real action in descendant class.
	virtual void switch_off();	// turn off
	MotivatorUnit(int source, int drain) {
		power_level = 0;
		profile_source = source;
		profile_drain = drain;
		// profile_leak = leak;
		power_require = 0;
		flow_target = 0;
		flow_rate = 0;
		is_on = 0x00;
	}
};

class MotivatorSource {
public:
	int power_limit;  // maximum single pulse power (max 'tank' size)
	int power_source; // supply rate of the power source (constant 'tank' refill rate)
	int power_spent;  // total power used in the last pulse.  (latest 'tank' drain rate)
	int power_level;  // power level remaining after last pulse (current 'tank' level)
	byte unit_first;  // first of subset of units which belong to this source
	byte unit_count;  // count of units
	MotivatorSource(int source, int max, byte unit, byte count) {
		power_limit = max;
		power_source = source;
		power_spent = 0;
		power_level = 0;
		unit_first = unit;
		unit_count = count;
	}
};

/*
	
 */
class Motivator {
public:
	MotivatorSource ** source;
	MotivatorUnit ** unit;
	// constructor
	Motivator(byte sources,byte units) {
		// create and clear the power tables
		source = new MotivatorSource*[sources+1];
		unit = new MotivatorUnit*[units+1];
		for(int i=0; i<=sources; i++) source[i] = 0;
		for(int i=0; i<=units; i++) unit[i] = 0;
	}
	~Motivator() {
		// delete our entries
		delete source;
		delete unit;
	}
	
	void pulse() {
		// for each source pool
		byte j=0; MotivatorSource * s;
		while(s = source[j++]) {
			byte first = s->unit_first;
			byte count = s->unit_count;
			// drain the required power levels of all units in this pool
			for(byte i=0; i<count; i++) {
				MotivatorUnit * u = unit[first+i];
				// subtract power requirements
				int level = u->power_level - u->power_require; 
				// requirement limit (may have been suddenly changed, so slowly drain if too much)
				if(level > u->power_require) { level--; }
				// negative limit of power supply.
				if(level < -s->power_limit) { level = -s->power_limit; }
				// store back
				u->power_level = level;
			}
			// increment the source with it's next pulse of available power
			int power = s->power_level + s->power_source;
			int pmax = s->power_limit;
			if(power > pmax) { power = pmax; } // limit to the max
			// seperately record how much power was allocated during this pulse
			int spent = 0;
			// keep a list of the units which will receive power this pulse
			byte pulse_unit[count]; memset(pulse_unit, 0x00, count);
			// while we still have power available, distribute it between our units
			byte i = first; 
			byte c = count;
			int ignore_val = 0; 
			int ignore_count = 0;
			bool more = power > 0;			
			while(more && (c>0)) {
				// find the neediest unit
				int n = find_neediest(first, count, ignore_val, ignore_count);
				if(n>=0) {
					// this one...
					MotivatorUnit * u = unit[n];
					int v = u->power_level;
					int d = u->power_require - v;
					// do we have enough power for this unit? (and is it needful?)
					int q = u->profile_drain;
					if((d>0) && (q<=power)) {
						// allocate the power from the source
						power -= q; spent += q; 
						// set the flag to pulse this unit
						pulse_unit[n] = 0xff;
						// any more to do for this source?
						more = --c > 0;
						// stop extra loops if remaining power is zero
						if(power==0) more = false;
					} else {
						// out of source power.
						more = false;
					}
					// are we doing more?
					if(more) {
						// update ignores for next loop
						if(ignore_val==v) {
							ignore_count++;
						} else {
							ignore_val = v;
							ignore_count = 1;
						}
					}
				} else {
					// no more.
					more = false;
				}
			}
			// now we know who got a power allocation, update the ports as simultaneously as possible.
			for(i=0; i<count; i++) {
				MotivatorUnit * u = unit[first+i];
				// what were the last and next pulse states? 
				byte last = u->is_on; byte next = pulse_unit[first+i];
				if(last!=next) {
					// we need to flip the port
					if(next) { u->switch_on(); } else { u->switch_off(); }
					// and store the result
					u->is_on = next;
				}
			}
			// update the power levels of all units in this pool again
			for(i=0; i<count; i++) {
				MotivatorUnit * u = unit[first+i];
				// update the power level, add source power if on
				if(u->is_on) { u->power_level += u->profile_source; } 
			}
			// store back some source metrics
			s->power_level = power;
			s->power_spent = spent;
		}
		// animate our power requirements with the flow metrics
		update_flows();
	}
	

	void stream_debug(Stream * tty) {
		// for each source pool
		byte j=0; MotivatorSource * s;
		while(s = source[j++]) {
			// for each unit
			byte first = s->unit_first;
			byte count = s->unit_count;
			for(byte i=0; i<count; i++) {
				MotivatorUnit * u = unit[first + i];
				bool last = (i==(count-1));
				// line 1
				if(i==first) {
					// source line 1
					tty->print(" Source"); stream_padnumber(tty, j);
					stream_spaces(tty,7);
				} else {
					stream_spaces(tty,20);
				}
				// unit line 1
				tty->print(" Unit "); stream_padnumber(tty, first+i);
				tty->print("s:"); stream_padnumber(tty, u->profile_source); 
				tty->print("d:"); stream_padnumber(tty, u->profile_drain);
				// stream_padnumber(tty, u->profile_leak); tty->print("k"); 
				tty->print("\n"); 
				// line 2
				if(i==first) {
					// source line 2
					stream_bargraph(tty, 12, s->power_spent, s->power_limit);
					stream_padnumber(tty, s->power_spent);
				} else {
					stream_spaces(tty,20);
				}
				// unit line 2
				stream_bargraph(tty, 12, u->power_level, s->power_limit);
				stream_padnumber(tty, u->power_level);
				stream_bargraph(tty, 12, u->power_require, s->power_limit);
				stream_padnumber(tty, u->power_require);
				tty->print("\n"); 
			}
		}
	}
	
private:
	// return the index of the unit with the largest requirement-level disparity (in the range start,count) 
	// ignoring all requirements above a value (of previously set values) or that match and are already accounted for.
	int find_neediest(byte start, byte count, int ignore_val, int ignore_count) {
		byte i = start; MotivatorUnit * u;
		int n = -1; //int m = -32000;
		int m = 0;
		u = unit[i];
		while(u) {
			bool do_test = false;
			// ignore values we have already processed
			if( (ignore_count==0) || (u->power_level>ignore_val) ) {
				int v = u->power_level;
				if(v<m) { m = v; n = i; } // remember the smallest level.
			} else if(u->power_level==ignore_val) { 
				if(ignore_count>0) {
					// mark off value duplicates
					ignore_count--; 
				} else {
					// we have found the next maximum entry. return now.
					return i;
				}
			} 
			// loop for next unit, up to count times
			u = (--count==0) ? 0 : unit[++i];
		}
		// return the maximum entry index
		return n;
	}

	
	// update unit flows for next time
	void update_flows() {
		// for each unit
		byte i=0; MotivatorUnit * u;
		while(u = unit[i++]) {
			int r = u->flow_rate;
			if(r!=0) {
				// how far are we off the target?
				int t = u->flow_target;
				int d = u->power_require - t;
				if(abs(d)<r) {
					// within accelleration distance of target, so stop there.
					u->power_require = t;
					u->flow_rate = 0;
				} else if(d>0) {
					// way above, decrease.
					u->power_require -= r;
				} else if(d<0) {
					// way below, increase.
					u->power_require += r;
				}
			}
		}
	}

	void stream_spaces(Stream * tty, int n) {
		for(int i=0; i<n; i++) tty->print(' ');
	}
	void stream_padnumber(Stream * tty, int n) {
		int s = 0;
		if(n>=10000) s=1;
		else if(n>=1000) s=2;				
		else if(n>=100) s=3;
		else if(n>=10) s=4;
		else if(n>=0) s=5;
		else if(n>=-10) s=4;
		else if(n>=-100) s=3;
		else if(n>=-1000) s=2;
		else if(n>=-10000) s=1;
		tty->print(n);
		stream_spaces(tty, s);
	}
	void stream_bargraph(Stream * tty, int size, int v, int r) {
		tty->print('[');
		for(int i=0; i<size; i++) {
			char c = (i*r)<(size*v) ? '=' : '-';
			tty->print(c);
		}
		tty->print(']');
	}
};

#endif