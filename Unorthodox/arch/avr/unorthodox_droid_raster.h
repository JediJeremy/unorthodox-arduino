
/*
  nano operating system for the Arduino (Leonardo)  (c) Jeremy Lee 2013
 */

#ifndef UNORTHODOX_DROID_RASTER_H
#define UNORTHODOX_DROID_RASTER_H

/*
 * RasterDroid is for cases where we use our own Raster drivers
 */


struct RasterSpan {
	int row;
	int col;
	int width;
	word bg;
	word * fg;
	RasterDraw16 * draw;
	RasterFont16 * font;
};

static const int Pager_decimal_places[] = { 0,1,10,100,1000,10000 };


class RasterPager {
public:
	// action bitmasks
	static const int NoAction = 0; // 
	static const word MoveCursor   = 0x0001; // 
	static const word MovePage     = 0x0002; // 
	static const word RedrawScreen = 0x0010; // 
	static const word RedrawLine   = 0x0020; // 
	static const word RedrawCursor = 0x0080; // 
	static const word ViewStatus   = 0x0100; // 
	static const word ViewSignal   = 0x0200; // 
	static const word ViewSource   = 0x0400; // 
	static const word ViewCodes    = 0x0800; // 
	
	//
	virtual void draw_span(RasterSpan * s) = 0;
	
	void clear_span(RasterSpan * s) {
		s->draw->rect_fill(s->col*6, s->row*8, s->width*6, 8, s->bg);
	}
	
	// fragment metadata labels
	static const byte Span         = 0b00000000; // basic span metadata         n[0..63]
	static const byte Last         = 0b10000000; // last span flag              n[0..63]
	static const byte Style        = 0b01000000; // style metadata              n[0..32]
	static const byte Vector       = 0b00100000; // vectored style flag         n[0..32]
		
	// fragment instruction labels
	static const byte Dec          = 0b00000000; // decimal format vector         v[0..15]
	static const byte Hex          = 0b01000000; // hex format vector             v[0..15]
	static const byte Lit          = 0b10000000; // sequence of literal characters [1..64]
	static const byte Vec          = 0b11000000; // choice of literal characters   [1..64], v[0..15]
	static const byte Debug        = 0b10100000; // debug format
	

	
	void format_span(RasterSpan * s, prog_uint8_t * flash, int * vector) {
		if(flash==0) {
			// fill the span with background
			s->draw->rect_fill(s->col*6, s->row*8, s->width*6, 8, s->bg);
			return;
		}
		// skip through metadata stream to the instruction block
		prog_uint8_t * p = flash;
		bool more = true;
		while(more) {
			byte b = pgm_read_byte_near(p++);
			if(b & Last) more = false;
		}
		// start from column zero with initial style
		int  i = 0;
		byte style = 0;
		// read through metadata (again, properly) and instruction streams in parallel
		prog_uint8_t * pf = flash;
		bool within = false;
		more = true;
		while(more) {
			// decode span metadata
			byte bf;
			byte fl;
			bool meta = true;
			while(meta) {
				// read the next byte
				bf = pgm_read_byte_near(pf++);
				if(bf & 0x80) more = false;
				if(bf & 0x40) {
					// it was a style entry. decode and skip.
					style = bf & 0x0F;
					if(bf & 0x20) style = vector[style];
				} else {
					// found the next span length
					fl = bf & 0x3F;
					meta = false;
				}
			}
			// get the intial fragment instruction byte
			byte b = pgm_read_byte_near(p);
			// get the lower 6 bits
			byte bl = b & 0x3F;
			// decode the size of the span instruction
			byte bc = 1; 			// they are usually one byte long
			if(b & 0x80) {			// but literals contain a run of character bytes
				bc += bl; 			// so the lower bits encodes the string length
				if(b & 0x40) bc++;	// if vectored, then the instruction contains an extra byte
			}
			// compute the next instruction pointer
			prog_uint8_t * np = p + bc;
			// how many columns will this fragment consume?
			int ni = i + fl;
			// are we still looking for the first overlapping fragment?
			if(!within) {
				// will processing this fragment intrude into the layout span?
				if(ni > s->col) { within = true; }
			}
			// do we process the fragment or just skip to the next?
			if(within) {
				// what range are we painting?
				int cw = s->col + s->width;
				int rb = max( s->col, i );
				int re = min( cw, i+fl );
				int rc = re - rb;
				// how many initial characters did we skip?
				int ri = rb - i;
				// decode the instruction
				if(b & 0x80) { 
					// literal character content
					prog_uint8_t * tb; // text start pointer (first char)
					prog_uint8_t * te; // text end pointer (first after)
					int tc; // text length count
					// is this vectored?
					if(b & 0x40) { 
						byte vb = pgm_read_byte_near(p+1); // read the parameter byte
						tb = p + 2 + vector[vb & 0x0F]; // start at the third byte plus the vector offset
						tc = fl; // and run for one fragment length
					} else { // constant string
						tb = p + 1; // start at the second byte
						tc = bl; // and run for the entire literal
					}
					// where does it end?
					te = tb + tc; 
					// where does the span begin in the string?
					prog_uint8_t * ti = tb + ri;
					// render remaining characters
					while(rc > 0) {
						if(ti>=te) ti=te-1; // clamp to end
						char c = (char)pgm_read_byte_near(ti++); // get next character
						// g->write(c); // paint the character
						s->font->char_fill(rb*6, s->row*8, c, s->fg[style],s->bg);
						rc--; rb++;
					}
				} else { // number formatting
					// which right-aligned digit are we starting from?
					int digit = fl - (rb - i);
					// bits 2..3 contain the format operation
					// bits 4..7 contain the vector index
					byte vi = b & 0x0F;
					// decimal value
					int n = vector[bl];
					// cardinal value
					int cn = abs(n);
					// span digits
					char c;
					while(rc > 0) {
						c = ' ';							
						// what format?
						if(b & 0x40) {
							// hex digit
							if(digit<5) {
								byte d = ( ((word)n) >> ((digit-1)<<2)) & 0x0F;
								c = (d>9) ? ('A'-10+d) : ('0'+d);
							} else {
								c = '0';									
							}
						} else {
							// decimal digit
							if(digit==1) {
								// extract the final digit
								int d = cn % 10; 
								c = '0' + d;
							} if(digit<6) {
								// extract a leading digit
								int place = Pager_decimal_places[digit];
								int dn = cn / place; // truncate the number
								int d = dn % 10; // and extract the new last digit
								if(dn>0) {
									c = '0' + d; // 
								} else {
									if( (n<0) && ((cn/Pager_decimal_places[digit-1])>0 ) ) c = '-'; // 
								}
							} if(digit==6) {
								if(n<=-10000) c = '-';
							}
						}
						// paint the character
						// g->write(c);
						s->font->char_fill(rb*6, s->row*8, c, s->fg[style],s->bg);
						// next
						digit--; rc--; rb++;					
					}
				}
				// has processing this fragment filled the layout span?
				if(ni>=cw) {
					// then we don't need to do any more
					more = false;
				}
			}
			// increment stream pointer and column index for next fragment
			p = np;
			i = ni;
		}
		// is there remaining unfilled width?
		int r = (s->col + s->width) - i;
		if(r>0) {
			// fill the block with background
			s->draw->rect_fill(i*6, s->row*8, r*6, 8, s->bg);
		}
	}
	
	virtual word click(RasterSpan * s) = 0;
	virtual word cursor_increment(RasterSpan * s, int dx, int dy) = 0;
	virtual word page_increment(RasterSpan * s, int dx, int dy) = 0;

};

// title line
PROGMEM prog_uint8_t SignalPager_title_line[] = {
	// fragment metadata 
	RasterPager::Span | 6,
	RasterPager::Span | 9 | RasterPager::Last,
	// fragment instructions
	RasterPager::Lit | 1, ' ',
	RasterPager::Lit | 9, 'X','-','D','R','O','I','D',' ','1'
};

// line
PROGMEM prog_uint8_t SignalPager_signal_line[] = {
	// fragment metadata 
	RasterPager::Style | RasterPager::Vector | 5, RasterPager::Span | 2,
	RasterPager::Style | 6, RasterPager::Span | 4,
	RasterPager::Style | 3, RasterPager::Span | 2,
	RasterPager::Style | 0, RasterPager::Span | 7,
	RasterPager::Style | 3, RasterPager::Span | 5,
	RasterPager::Style | RasterPager::Vector | 3, RasterPager::Span | 1 | RasterPager::Last,
	// fragment instructions
	RasterPager::Lit | 2, '<',' ', 
	RasterPager::Dec | 0,
	RasterPager::Lit | 2, ' ','(', 
	RasterPager::Dec | 1,
	RasterPager::Lit | 3, ' ',')',' ',
	RasterPager::Lit | 1, '>'
};

// filesystem stats
PROGMEM prog_uint8_t SignalPager_stats_line[] = {
	// fragment metadata 
	RasterPager::Style | 2, RasterPager::Span | 2,
	RasterPager::Span | 3,
	RasterPager::Span | 1,
	RasterPager::Span | 3,
	RasterPager::Span | 1,
	RasterPager::Span | 3,
	RasterPager::Style | 4, RasterPager::Span | 6 | RasterPager::Last,
	// fragment instructions
	RasterPager::Lit | 1, ' ',
	RasterPager::Hex | 0,
	RasterPager::Lit | 1, ':',
	RasterPager::Hex | 1,
	RasterPager::Lit | 1, ':',
	RasterPager::Hex | 2,
	RasterPager::Dec | 3
};

class SignalPager : public RasterPager {
private:
	static const int header = 2;
	BaseDroid * droid;
	int spin_signal;
public:
	int scroll_y;

	SignalPager(BaseDroid * droid) {
		scroll_y = 0;
		this->droid = droid;
		spin_signal = 0xFF;
	}
	
	
	word click(RasterSpan * s) {
		int row = s->row;
		int col = s->col;
		if(row<2) {
		} else if(row<18) {
			int line = (row + scroll_y*16 - header);
			if(col<8) {
			} else if(col<16) {
				// spin the signal value
				spin_signal = (spin_signal==0xFF) ? line : 0xFF;
			} else if(col<20) {
			}
		}
		return RedrawLine | RedrawCursor; // post row refresh event
	}
	
	word page_increment(RasterSpan * s, int dx, int dy); // defined after RasterDroid
	
	word cursor_increment(RasterSpan * s, int dx, int dy) {
		int row = s->row;
		if(spin_signal==0xFF) {
			int line = row + scroll_y*16 - header;
			bool isline = (row >= 2) && (row < 18);
			bool iscode = isline && (line<64);
			if(s->col + dx < 0) {
				// prevent left move if at col 0 and not over a code line
				return isline ? MoveCursor | RedrawCursor | ViewSource : NoAction;
			} else if(s->col + dx > 20) {
				// prevent right move if at rightmost and not over a line
				return isline ? MoveCursor | RedrawCursor | ViewSource : NoAction;
			}
			// otherwise perform default action
			return MoveCursor | RedrawCursor;
		} else {
			// redraw the line
			int v = droid->signal_value[spin_signal];
			v -= dy;
			droid->set_signal(spin_signal,v);
			return RedrawLine | RedrawCursor; // post row refresh event
		}
	}
	
	void draw_span(RasterSpan * s) {
		int row = s->row;
		prog_uint8_t * line_format = 0;
		int line_vector[8];
		if(row==0) {
			line_format = SignalPager_title_line;
		} else if(row==1) {
			
		} else if(row<18) {
			int line = row + scroll_y*16 - header;
			line_format = SignalPager_signal_line;
			line_vector[0] = line;
			line_vector[1] = droid->signal_value[line];
			line_vector[2] = droid->fs.token_size(line&0x7F);
			line_vector[3] = line_vector[2] ? 2 : 3;
			line_vector[4] = droid->fs.token_size((line&0x3F)|0x80);
			line_vector[5] = (line<64) ? (line_vector[4] ? 2 : 3) : 7;
		} else if(row==19) {
			// filesystem statistics
			line_format = SignalPager_stats_line;
			line_vector[0] = droid->fs.head;
			line_vector[1] = droid->fs.tail;
			line_vector[2] = droid->fs.next;
			line_vector[3] = droid->fs.used;
		}
		format_span(s, line_format, line_vector);
	}
	
};

// source editor title
PROGMEM prog_uint8_t SourcePager_title_line[] = {
	// fragment metadata 
	RasterPager::Span | 14,	
	RasterPager::Span | 6 | RasterPager::Last,
	// fragment instructions
	RasterPager::Lit | 7, 'S','o','u','r','c','e',' ',
	RasterPager::Dec | 0
};

// 
PROGMEM prog_uint8_t SourcePager_math_line[] = {
	// fragment metadata 
	RasterPager::Style | 1, RasterPager::Span | 2, // delete symbol
	RasterPager::Style | 2, RasterPager::Span | 2, // add symbol
	RasterPager::Style | RasterPager::Vector | 5, RasterPager::Span | 1, // bracket symbol
	RasterPager::Style | RasterPager::Vector | 1, RasterPager::Span | 6, // accumulator value
	RasterPager::Style | RasterPager::Vector | 5, RasterPager::Span | 1, // bracket symbol
	RasterPager::Style | 6, RasterPager::Span | 1, // op lhs
	RasterPager::Span | 1,                      // equals symbol
	RasterPager::Span | 1,                      // op rhs
	RasterPager::Style | RasterPager::Vector | 6, RasterPager::Span | 1, // bracket
	RasterPager::Style | 0, RasterPager::Span | 3, // operand value
	RasterPager::Style | RasterPager::Vector | 6, RasterPager::Span | 1 | RasterPager::Last, // bracket
	// fragment instructions
	RasterPager::Lit | 2, ' ','x',
	RasterPager::Lit | 2, ' ','+',
	RasterPager::Lit | 1, '(',
	RasterPager::Dec | 0, 
	RasterPager::Lit | 1, ')',
	RasterPager::Vec | 9, 2, ' ',':','+','-','x','/','&','|','^',
	RasterPager::Lit | 1, '=',
	RasterPager::Vec | 9, 3, ' ',':','+','-','x','/','&','|','^',
	RasterPager::Lit | 1, '(',
	RasterPager::Dec | 4,
	RasterPager::Lit | 1, ')'
};

// 
PROGMEM prog_uint8_t SourcePager_skip_line[] = {
	// fragment metadata 
	RasterPager::Style | 1, RasterPager::Span | 2, // delete symbol
	RasterPager::Style | 2, RasterPager::Span | 2, // add symbol
	RasterPager::Style | 3, RasterPager::Span | 1, // bracket symbol
	RasterPager::Style | RasterPager::Vector | 1, RasterPager::Span | 6, // accumulator value
	RasterPager::Style | 3, RasterPager::Span | 1, // bracket symbol
	RasterPager::Style | 2, RasterPager::Span | 1, // skip comparison
	RasterPager::Span | 4,                         // zero symbol
	RasterPager::Span | 2,                         // operand value
	RasterPager::Span | 1 | RasterPager::Last,     // arrow
	// fragment instructions
	RasterPager::Lit | 2, ' ','x',
	RasterPager::Lit | 2, ' ','+',
	RasterPager::Lit | 1, '(',
	RasterPager::Dec | 0, 
	RasterPager::Lit | 1, ')',
	RasterPager::Vec | 4, 2, '=','!','<','>',
	RasterPager::Lit | 2, '0',' ',
	RasterPager::Dec | 4,
	RasterPager::Lit | 1, 0x19
};


class SourcePager : public RasterPager {
private:
	static const int header = 2;
	BaseDroid * droid;
	int lines;
	word source[16];
	word profile[16];
	word skipped;
	word spin_mask;
	word spin_bit;
	bool modified;
public:
	int code_index;
	
	SourcePager(BaseDroid * droid) {
		this->droid = droid;
		spin_mask = 0;
		// code_index = 0;
		// modified = false;
		load(0);
	}

	word click(RasterSpan * s) ;
	
	word cursor_increment(RasterSpan * s, int dx, int dy) {
		// source line
		int line = s->row - 2;
		// word action;
		if(spin_mask==0) {
			// return ((s->col + dx) >= 0) ? (MoveCursor | MovePage | RedrawCursor) : (NoAction);
			return (MoveCursor | MovePage | RedrawCursor);
		} else if(dy) {
			word w = source[line];
			// increment mask bits
			word wi = ( w - spin_bit*(dy&0xFF) ) & spin_mask;
			w = w &(~spin_mask) | wi;
			source[line] = w;
			return  RedrawLine | RedrawCursor;
		} else if((dx!=0)&&(spin_mask==0x0700)) {
			// we are adjusting the math operator
			if(dx<0) {
				source[line] &= 0xF7FF; // flip to lhs
			} else {
				source[line] |= 0x0800; // flip to rhs
			}
			return RedrawLine | RedrawCursor;
		}
		return RedrawCursor;
	}

	word page_increment(RasterSpan * s, int dx, int dy) {
		save();
		if(dx) {
			return (dx>0) ? (RedrawScreen | ViewSignal) : (RedrawScreen | ViewCodes);
		}
		if(dy) {
			load(code_index+dy);
			return RedrawScreen;
		}
		// return NoAction;
	}
	
	void draw_span(RasterSpan * s) {
		prog_uint8_t * line_format = 0;
		int line_vector[12]; memset(line_vector,0,24);
		int row = s->row;
		if(row==0) {
			// title row
			line_format = SourcePager_title_line;
			line_vector[0] = code_index & 0x3F;
		} else if(row==1) {
		} else if(lines>(row-2)) {
			// source line
			int index = row-2;
			word w = source[index];
			word k = skipped & (1<<index);
			byte b = w >> 8;
			// debug profile
			line_vector[0] = profile[index];
			line_vector[1] = (skipped & (1<<index)) ? 7 : 2;
			// opcode type
			if(b & 0x40) {
				// SKIP opcode
				line_format = SourcePager_skip_line;
				// comparison type
				line_vector[2] = (b>>4) & 0x03;
				// jump operand
				line_vector[4] = b & 0x0F;
			} else {
				// MATH opcode
				line_format = SourcePager_math_line;
				// assignment & math mode selection
				line_vector[(b & 0x08)?3:2] = 1 + (b & 0x07);
				// operand value
				line_vector[4] =  w & 0x7F;
				// accumulator indirection style
				line_vector[5] = (b & 0x20) ? 0 : 3;
				// operand indirection style
				line_vector[6] = (b & 0x10) ? 0 : 3;
			}
		} else if(row<18) {
			// empty lines
		}
		format_span(s, line_format, line_vector);
	}
	
	void load(int code) {
		code = (code & 0x3F) | 0x80;
		// cancel if already loaded
		code_index = code;
		// initialize editor
		skipped = 0xFFFF;
		lines = 0;
		modified = false;
		memset(source,0,32);
		// and get its token page size
		int size = droid->fs.token_size(code);
		if(size) {
			// look up the codes' memorypage
			Page * page = droid->fs.token_page(code);
			// go through the byte stream, and 'widen' into fixed source table
			byte op = 0;
			for(int i=0; i<size; i++) {
				byte b = page->read_byte(i);
				// check first bit
				if(b & 0x80) {
					if(b & 0x40) { // SKIP opcode
						source[lines++] = (word)b<<8;
					} else { // MODE opcode
						op = b;
					}
				} else { // CONST opcode
					source[lines++] = ((word)op<<8) + b;
				}
				if(lines>=16) break; // stop at 16
			}
			// free objects
			delete page;
		}
	}
	
	void save() { 
		if(!modified) return;
		// attempt to compact the source into a code stream
		byte stream[33]; // worst case, every line is a distinct math op + the token header
		stream[0] = code_index;
		byte count = 1;
		byte last_op = 0xFF;
		for(int i=0; i<lines; i++) {
			word w = source[i];
			byte b0 = w >> 8;
			// op or skip?
			if(b0 & 0x40) {
				// skip code byte
				stream[count++] = b0;
				// reset opcode
				last_op = 0xFF;
			} else {
				// op code
				if(b0!=last_op) {
					// prefix with new opcode
					stream[count++] = b0;
					last_op = b0;
				}
				// stream the operand lower byte
				stream[count++] = w;
			}
		}
		// wrap that in a memorypage and send it to the token storage
		MemoryPage page(stream);
		droid->fs.token_write(code_index, &page, count);
	}

	
};

PROGMEM prog_uint8_t CodesPager_title_line[] = {
	// fragment metadata 
	RasterPager::Span | 15,	
	RasterPager::Span | 6 | RasterPager::Last,
	// fragment instructions
	RasterPager::Lit | 7, 'S','i','g','n','a','l', ' ',
	RasterPager::Dec | 0
};

PROGMEM prog_uint8_t CodesPager_immediate_line[] = {
	// fragment metadata 
	RasterPager::Style | 3, RasterPager::Span | 2,                  // left bracket
	RasterPager::Style | 0, RasterPager::Span | 3,                  // code number
	RasterPager::Style | 6, RasterPager::Span | 7,                  // immediate mode
	RasterPager::Style | 3, RasterPager::Span | 3,                  // brackets
	RasterPager::Style | 2, RasterPager::Span | 2,                  // plus marker
	RasterPager::Style | 1, RasterPager::Span | 3,                  // delete marker
	RasterPager::Style | RasterPager::Vector | 1, RasterPager::Span | 1 | RasterPager::Last, // code arrow
	// fragment instructions
	RasterPager::Lit | 2, ' ','(',
	RasterPager::Dec | 0, 
	RasterPager::Lit | 4, ' ','<','<',' ',
	RasterPager::Lit | 3, ' ',')',' ',
	RasterPager::Lit | 2, '+',' ',
	RasterPager::Lit | 2, 'x',' ',
	RasterPager::Lit | 1, '>'
};

PROGMEM prog_uint8_t CodesPager_deferred_line[] = {
	// fragment metadata 
	RasterPager::Style | 3, RasterPager::Span | 7,                  // left bracket
	RasterPager::Style | 6, RasterPager::Span | 2,                  // deferred mode
	RasterPager::Style | 0, RasterPager::Span | 3,                  // code number
	RasterPager::Style | 3, RasterPager::Span | 3,                  // brackets
	RasterPager::Style | 2, RasterPager::Span | 2,                  // plus marker
	RasterPager::Style | 1, RasterPager::Span | 3,                  // delete marker
	RasterPager::Style | RasterPager::Vector | 1, RasterPager::Span | 1 | RasterPager::Last, // code arrow
	// fragment instructions
	RasterPager::Lit | 3, ' ','(',' ',
	RasterPager::Lit | 2, '>','>',
	RasterPager::Dec | 0, 
	RasterPager::Lit | 3, ' ',')',' ',
	RasterPager::Lit | 2, '+',' ',
	RasterPager::Lit | 2, 'x',' ',
	RasterPager::Lit | 1, '>'
};


class CodesPager : public RasterPager {
private:
	static const int header = 2;
	BaseDroid * droid;
	int signal;
	byte code[16];
	word codes;
	word spin_mask;
	word spin_bit;
	bool modified;
public:
	int code_index;
	
	CodesPager(BaseDroid * droid) {
		this->droid = droid;
		spin_mask = 0;
		// codes = 0;
		// signal = 0;
		// modified = false;
		load(0);
	}
	
	void draw_span(RasterSpan * s) {
		prog_uint8_t * line_format = 0;
		int line_vector[8];
		int row = s->row;
		if(row==0) {
			line_format = CodesPager_title_line;
			line_vector[0] = signal;
		} else if(row==1) {
		} else if(row<18) {
			int line = row-2;
			if(line<codes) {
				line_format = (code[line] & 0x80) ? CodesPager_deferred_line : CodesPager_immediate_line; // code style
				line_vector[0] = code[line] & 0x3F; // line number
				line_vector[1] = droid->fs.token_size(line_vector[0]|0x80) ? 2:3;
			}
		}
		format_span(s, line_format, line_vector);
	}
	
	void load(int sig) {
		signal = sig & 0x7F;
		// initialize editor
		// memset(code,0,16);
		modified = false;
		// and get it's token page size
		codes = droid->fs.token_size(signal);
		if(codes) {
			// look up the codes' memorypage
			Page * page = droid->fs.token_page(signal);
			// copy the token data into the editor
			page->read(0, code, codes);
			// free objects
			delete page;
		}
	}
	
	void save() { 
		if(!modified) return;
		// attempt to compact the source into a code stream
		byte stream[17]; // worst case, every line is a distinct math op + the token header
		stream[0] = signal;
		byte count = 1;
		for(int i=0; i<codes; i++) { stream[count++] = code[i]; }
		// wrap that in a memorypage and send it to the token storage
		MemoryPage page(stream);
		droid->fs.token_write(signal, &page, count);
	}

	word click(RasterSpan * s) {
		modified = true;
		if(spin_mask) {
			// turn the mask back off
			spin_mask = 0;
		} else {
			int row = s->row;
			int col = s->col;
			int line = row-2;
			if(line<0) {
				// undo all
				load(signal);
				return RedrawScreen | RedrawCursor;
			} else if(line<codes) {
				// existing line
				if(col<3) {
					// margin
				} else if(col<14) {
					// spin value
					spin_mask = 1;
				} else if(col==15) {
					if(codes<15) {
						// insert before this line
						for(int i=15; i>line; i--) code[i] = code[i-1];
						// code[line] = 0; // don't clear to copy
						codes++;
						return RedrawScreen | RedrawCursor;
					}
				} else if(col==17) {
					// delete this line
					for(int i=line; i<14; i++) code[i] = code[i+1];
					codes--;
					return RedrawScreen | RedrawCursor;
				}
			} else {
				// empty line. add one.
				if(codes<15) code[codes++] = 0;
				return RedrawScreen | RedrawCursor;
			}
	
		}
		return NoAction;
	}
	
	word cursor_increment(RasterSpan * s, int dx, int dy) {
		if(spin_mask==0) {
			// don't go past the right edge
			// return ( (s->col + dx) > 20 )  ? (NoAction) : (MoveCursor | MovePage | RedrawCursor);
			return (MoveCursor | MovePage | RedrawCursor);
		}
		int line = s->row - 2;
		if(dy) {
			// modify source line
			code[line] = ((code[line]-dy) & 0x3F) | code[line] & 0x80;
		} else if(dx) {
			// set the mode bit
			if(dx>0) {
				code[line] |= 0x80; // make deferred
			} else {
				code[line] &= 0x7F; // make immediate
			}
		}
		return RedrawLine | RedrawCursor;
	}

	word page_increment(RasterSpan * s, int dx, int dy);
	
};



//
static word RasterDroid_DefaultStyle[] = { 
	ST7735::WHITE, ST7735::RED, ST7735::GREEN, ST7735::BLUE, ST7735::CYAN, ST7735::MAGENTA, ST7735::YELLOW, ST7735::BLACK 
};


class RasterDroid : public BaseDroid {
protected:
	// screen dimensions
	static const int screen_rows = 20; 
	static const int screen_cols = 21;
	static const int pointer_speed = 0x1000;
	// joystick sample count
	int joystick_samples;
	static const int joystick_sample_count = 1024;
	// button debounce milliseconds
	static const int debounce = 50;
	// joystick pointer
	int     pointer_x;
	int     pointer_y;
	long    pointer_t;
	// joystick button
	Debounce Button;
	bool button_state;
public:
	// graphics context
	Raster16 * raster;
	RasterDraw16 Draw;
	NearProgramPage Sysfont;
	RasterFont16 Font;
	// line state flags
	static const byte LineRedraw = 0x01; 
	// joystick state 
	JoystickAxis joystick[2];
	// cursor state
	RasterSpan cursor_span;
	// display state
	int display_mode;
	int display_state;
	byte line_begin[screen_rows];
	byte line_count[screen_rows];
	// display screens
	// StatusPager Status;
	SignalPager Signals;
	CodesPager Codes;
	SourcePager Source;
	RasterPager * display;
public:
	RasterDroid(Page * storage, word size, Raster16 * raster) : BaseDroid(storage, size), Button(50), Draw(raster), Sysfont(font_5x8_128), Font(raster, &Sysfont), Signals(this), Codes(this), Source(this) /* , Status(this,16) */  {
		this->raster = raster;
		// start on the status page
		display = &Signals; // Status, Signals or Source
		// redraw();
		screen_redraw(0, screen_rows); // redraw all rows
		display_state = 0;
		// initialize the cursor to screen center
		cursor_span.draw = &Draw;
		cursor_span.font = &Font;
		cursor_span.row = screen_rows / 2;
		cursor_span.col = screen_cols / 2;
		cursor_span.width = 1;
		cursor_span.fg = RasterDroid_DefaultStyle;
		cursor_span.bg = 0x0000;
		// setup joystick
		joystick_samples = 0;
		// zero the pointer
		pointer_x = 0; 
		pointer_y = 0;
		pointer_t = 0;
		// clear button state
		button_state = false;
	}
	

	void screen_redraw(int row, int count) {
		// redraw each row entirely
		for(int i=count; i>0; i--) { 
			line_begin[row] = 0; 
			line_count[row++] = screen_cols; 
		}
	}
	
	/*
	 * 
	 */
	void line_repaint(int line, int begin, int count) {
		if(line_count[line]) {
			// we already have a repaint span, so extend it
			int nb = min(line_begin[line],begin);
			int ne = max(line_begin[line]+line_count[line],begin+count);
			int nc = ne - nb;
			line_begin[line] = nb;
			line_count[line] = nc;			
		} else {
			// no current span, so we're it.
			line_begin[line] = begin;
			line_count[line] = count;
		}
	}
	
	// update display with joystick and button information
	void update(int analog[2], bool b0, long t, int dt) {
		// shortcut if no time has passed.
		if(dt>0) {
			dt = min(15, dt); // limit it so our math doesn't go weird.
			// debounce the button
			bool pressed = Button.update(b0, t);
			bool click = false;
			if(button_state != pressed) {
				// button has changed
				button_state = pressed;
				click = pressed;
			}
			if(click) {
				int post = display->click(&cursor_span);
				do_action(&cursor_span,post);
			}
			// update the joystick axes
			// do we have a statistically significant sample?
			bool summary = (joystick_samples >= joystick_sample_count);
			int pointer[2];
			for(byte i=0; i<2; i++) {
				int v = analog[i];
				int center = joystick[i].center;
				int deadzone = joystick[i].deadzone;
				int inertia = joystick[i].inertia;
				int delta = joystick[i].delta;
				// 
				int d0 = v - center + deadzone; int d1 = v - center - deadzone;
				int d = (d0<0) ? d0 : (d1>0) ? d1 : 0; 
				// are we outside the deadzone?
				if( d ) {
					// push inertia towards a new value
					int r = d - inertia;
					inertia += (r>0) ? min(r,dt) : max(r,-dt);
				} else {
					// instant stop
					inertia = 0;
				}
				pointer[i] = inertia;
				// update stats
				delta += abs(v - joystick[i].sample);
				joystick[i].sample = v;
				if(summary) {
					// did we have almost no movement within the sample period?
					if( (deadzone * joystick_samples) > delta ) {
						// adjust the center towards the latest value
						joystick[i].center += (center < v) ? 1 : -1;
					}
					// clear old stats
					delta = 0;
				}
				joystick[i].delta = delta;
				joystick[i].inertia = inertia;
				
			}
			if(summary) joystick_samples = 0;
			joystick_samples++;
			// do we have pointer movement?
			if( pointer[0] || pointer[1] ) {
				// update pointer position
				pointer_x += (pointer[0] * dt) >> 2;
				pointer_y += (pointer[1] * dt) >> 2;
				// turn pointer overflow into cursor deltas
				int cursor_x = pointer_x / pointer_speed;
				int cursor_y = pointer_y / pointer_speed;
				bool moved = (cursor_x!=0) || (cursor_y!=0);
				// did the cursor move?
				if(moved) {
					// wrap the pointer position
					pointer_x -= cursor_x * pointer_speed;
					pointer_y -= cursor_y * pointer_speed;
					// notify the display of the pointer movement, see what it thinks should happen
					word post = display->cursor_increment(&cursor_span,cursor_x,cursor_y);
					// should we automatically move the display cursor?
					if(post & RasterPager::MoveCursor) {
						// redraw the old cursor location without it
						display->clear_span(&cursor_span);
						display->draw_span(&cursor_span);
						// update cursor position
						cursor_span.col += cursor_x;
						cursor_span.row += cursor_y;
						// has the cursor gone beyond the page boundaries?
						int page_x = cursor_span.col / screen_cols; if(cursor_span.col<0) page_x--;
						int page_y = cursor_span.row / screen_rows; if(cursor_span.row<0) page_y--;
						bool paged = (page_x!=0) || (page_y!=0);
						if(paged) {
							// page the cursor back into range
							cursor_span.col -= page_x * screen_cols;
							cursor_span.row -= page_y * screen_rows;
							// reset the pointer fractions
							pointer_x = 0; pointer_y = 0;
							// notify the display page of the update
							post |= display->page_increment(&cursor_span, page_x,page_y);
						}						
					}
					// do the post action
					do_action(&cursor_span,post);
				}				
			} else {
				// origin reset
				pointer_x = 0; pointer_y = 0;
			}
			pointer_t = t;
		}
		// update the display
		// remember the line we started from, and begin looking
		int wrap_row = display_state;
		bool more = true;
		while(more) {
			// do we have a line to render?
			int c = line_count[display_state];
			if(c) {
				// redraw the line
				RasterSpan redraw_span;
				redraw_span.draw = &Draw;
				redraw_span.font = &Font;
				redraw_span.col = line_begin[display_state];
				redraw_span.width = c;
				redraw_span.row = display_state;
				redraw_span.fg = cursor_span.fg;
				redraw_span.bg = cursor_span.bg;
				display->draw_span(&redraw_span);
				// clear the redraw span
				line_count[display_state] = 0;
				// if this was the cursor row, we should probably redraw that too
				if(display_state==cursor_span.row) do_action(&cursor_span, RasterPager::RedrawCursor);
				// we can stop looping, since we found something useful to do.
				more = false;
			} else {
				// have we completely wrapped?
				more = (display_state != wrap_row);
			}
			// next line next time
			display_state++; if(display_state>=screen_rows) display_state = 0;
		}
	}
	
	
	// perform a standard ui action
	void do_action(RasterSpan * cursor_span, word act) {
		// switch screen
		// if(act & RasterPager::ViewStatus) { display = &Status; }
		if(act & RasterPager::ViewSignal) { display = &Signals; }
		if(act & RasterPager::ViewSource) { display = &Source; }
		if(act & RasterPager::ViewCodes)  { display = &Codes; }
		// redraw commands
		if(act & RasterPager::RedrawScreen) { 
			// redraw();
			screen_redraw(0, screen_rows); // redraw all rows
			display_state = 0; // but redraw from row 0
		}
		if(act & RasterPager::RedrawLine) { 
			// redraw one row
			line_begin[cursor_span->row] = 0;
			line_count[cursor_span->row] = screen_cols;
		}
		if(act & RasterPager::RedrawCursor) { 
			// rectangle corners kept in array
			int ox[8] = { 0,2, 5,-2, 0,2,  5,-2 };
			int oy[8] = { 0,2, 0,2,  7,-2, 7,-2 };
			// minimise the number of encoded function calls
			WordPage color(0xFFFF); // color of cursor edges
			for(int i=0; i<8; i+=2) {
				int x = cursor_span->col*6 + ox[i]; int dx = ox[i+1];
				int y = cursor_span->row*8 + oy[i]; int dy = oy[i+1];
				// g->drawFastHLine(dx<0 ? x+dx+1 : x,y, abs(dx), 0xFFFF);
				raster->fragment(x,y, (dx<0) ? Raster::LEFT : Raster::RIGHT, &color,0,abs(dx) );
				// g->drawFastVLine(x,dy<0 ? y+dy+1 : y, abs(dy), 0xFFFF);
				raster->fragment(x,y, (dy<0) ? Raster::UP: Raster::DOWN, &color,0,abs(dy) );
			}
		}
	}
	
	void source_exec(int count, word * source, word * profile, word * skipped) {
		// create word page
		WordPage w(0);
		// go through the word stream
		op_mode = 0;
		op_skip = 0;
		int acc = 0;
		word k = *skipped; 
		// one at a time
		for(int i=0; i<count; i++) {
			// is it available?
			word code = source[i];
			if(code & 0x8000) {
				// update the skip mask
				word kb = 1<<i;
				word nk = op_skip ? (k|kb) : (k&~kb);
				// redraw if it changed
				bool redraw = (k!=nk);
				k = nk;
				// are we skipping?
				if(op_skip) {
					op_skip--;
				} else {
					// execute the next instruction
					// Serial.print("\n exec "); Serial.print(code,HEX);
					int n = (code & 0x4000) ? 1 : 2;
					w.value = (code<<8)|(code>>8); // put bytes in right order
					acc = code_stream(&w,n,acc);
					if(profile[i] != acc) {
						profile[i] = acc;
						// print the accumulator value
						redraw = true;
					}
				}
				// redraw the line if it changed
				if(redraw) screen_redraw(i+2,1);
			}
		}
		// all done
		*skipped = k;
	}
	
	void redraw_signal(int signal) {
		if(display==&Signals) {
			// which scrolled line does that correspond to?
			int line = 2 + signal - Signals.scroll_y * 16;
			// if that line is on the screen, then redraw the signal value
			if((line>=0) && (line<screen_rows)) line_repaint(line,9,6);
		}
	}

};



word CodesPager::page_increment(RasterSpan * s, int dx, int dy) {
	// default behavior
	if(dy) {
		save();
		load(signal+dy);
		return RedrawScreen;
	}
	if(dx) {
		save();
		if(dx<0) {
			// back to signal screen
			return RedrawScreen | ViewSignal;			
		} else {
			// show code source
			int line = s->row - 2;
			if((line>=0) && (line<codes)) {
				((RasterDroid *)droid)->Source.load(code[line]);
			}
			return RedrawScreen | ViewSource;
		}
	}
	return NoAction;
}

word SignalPager::page_increment(RasterSpan * s, int dx, int dy) {
	scroll_y = (scroll_y + dy) & 0x07;
	int line = s->row + scroll_y*16 - header;
	if(dx<0) {
		if(line<64) { // code line?
			((RasterDroid *)droid)->Source.load(line);
		}
		return RedrawScreen | ViewSource;
	} else if(dx>0) {
		// show signal codes
		((RasterDroid *)droid)->Codes.load(line);
		return RedrawScreen | ViewCodes;
	}
	return RedrawScreen;
}

word SourcePager::click(RasterSpan * s) {
	modified = true;
	if(spin_mask) {
		// turn the mask back off
		spin_mask = 0;
	} else {
		int row = s->row;
		int col = s->col;
		int line = row-2;
		if(line<0) {
			// undo all changes
			load(code_index);
			return RedrawScreen | RedrawCursor;
		} else if(line < lines) {
			if(col<4) {
				if(col==1) {
					// delete this row
					for(int i=line; i<15; i++) source[i] = source[i+1];
					lines--;
					skipped = 0xFFFF;
					return RedrawScreen | RedrawCursor;
				} else if(col==3) {
					// insert a row
					for(int i=15; i>line; i--) source[i] = source[i-1];
					lines++;
					skipped = 0xFFFF;
					return RedrawScreen | RedrawCursor;
				}
			} else if(col<11) {
				// execute the code in debug mode
				((RasterDroid *)droid)->source_exec(16,source,profile,&skipped);
				return NoAction; // no post event
			} else if(source[line] & 0x4000) {
				// skip opcode
				if(col<13) {
					// compare bits
					spin_mask = 0x3000; spin_bit = 0x1000;
				} else if(col==13) {
					// flip op mode
					source[line] ^= 0x4000;
				} else if(col<16) {
				} else if(col<19) {
					// skip operand bits
					spin_mask = 0x0F00; spin_bit = 0x0100;
				}
			} else {
				// math opcode
				if(col==11) {
					// flip accumulator indirection
					source[line] ^= 0x2000;
				} else if(col==13) {
					// flip op mode
					source[line] ^= 0x4000;
				} else if(col<15) {
					// math bits
					spin_mask = 0x0700; spin_bit = 0x0100;
				} else if(col==15) {
					// flip operand indirection
					source[line] ^= 0x1000;
				} else if(col<19) {
					// math operand bits
					spin_mask = 0x007F; spin_bit = 0x0001;
				}
			}
			return RedrawLine | RedrawCursor;
		} else if(row<18) {
			// append a row
			if(lines<16) source[lines++] = 0x8000; // [if statement probably not necessary]
			return RedrawScreen | RedrawCursor;
		} 
	}
	return NoAction; // no post event
}


#endif