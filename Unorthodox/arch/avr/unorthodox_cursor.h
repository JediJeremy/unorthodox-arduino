
/*
  nano operating system for the Arduino (Leonardo)  (c) Jeremy Lee 2013
 */

#ifndef UNORTHODOX_CURSOR_H
#define UNORTHODOX_CURSOR_H

#include <Arduino.h>
// #include <unorthodox_page.h>
// #include <unorthodox_trees.h>

class Cursor  {
public:
	virtual bool apply(byte b);                  // apply the next sequence byte to the cursor
	virtual bool revert() { return false; }      // revert one sequence byte
	virtual bool valid() { return false; }       // test if cursor is in accept state
	virtual bool accept() { return false; }      // test if cursor is in accept state
	virtual int  symbol();         // the accepted symbol token
	virtual int  predict() { return 0; }         // how many bytes are predictable?
	virtual byte emit() { return 0; }            // return the next predictable byte
	// virtual byte seek(int index) { return 0; }
	// virtual byte state() { return 0; }
	virtual void reset();
};

/*
  BufferCursor will unconditionally accept a block of bytes, and store them.
 */
class BufferCursor : public Cursor  {
private:
	word count;
	byte * buffer;
	word index;
	bool cursor_valid;
public:
	// constructor
	BufferCursor(int count) {
		// create the buffer
		this->count = count;
		this->buffer = new byte[count]; 
		reset();
	}
	~BufferCursor() {
		delete this->buffer;
	}
	// reset
	void reset() {
		// clear the buffer
		for(word i=0; i<this->count; i++) this->buffer[i] = 0;
		// reset the cursor state
		cursor_valid = true;
		index = 0;
	}
	// valid state
	bool valid() { return cursor_valid; }
	// accepting state
	bool accept() { return cursor_valid && (index==count); }
	// accepting symbol
	int symbol() { return accept() ? 1 : 0; }
	// apply next serial byte
	bool apply(byte b) {
		if(cursor_valid) {
			if(index>=count) {
				// too many
				cursor_valid = false;
			} else {
				// store next
				buffer[index++] = b;
			}
		}
		return cursor_valid;
	}
};

/*
  PageCursor will accept bytes so long as they match the contents of a MemoryPage.
  This is equivalent to a "constant string".
 */
class PageCursor : public Cursor  {
private:
	word count;
	ReadPage * page;
	word index;
	bool cursor_valid;
public:
	// constructor
	PageCursor(ReadPage * page, word count) {
		// create the buffer
		this->count = count;
		this->page = page; 
		reset();
	}
	// reset
	void reset() {
		// reset the cursor state
		cursor_valid = true;
		index = 0;
	}
	// valid state
	bool valid() { return cursor_valid; }
	// accepting state
	bool accept() { return  cursor_valid && (index==count); }
	// content prediction works fine when you are really a memory block
	int predict() { return cursor_valid ? (count-index) : 0; }
	byte emit() { 
		if(!cursor_valid) return 0;
		byte b = page->read_byte(index++);
		if(index>=count) cursor_valid = 0;
		return b; 
	}
	// accepting symbol
	int symbol() { return accept() ? 1 : 0; }
	// apply next serial byte
	bool apply(byte b) {
		if(cursor_valid) {
			// test next page byte
			if(page->read_byte(index++)!=b) {
				// not the same
				cursor_valid = false;
			}
		}
		return cursor_valid;
	}
};

/*
  NumberCursor will accept bytes that form a legal ASCII number string, and will output a 'symbol' which
  is the integer value of the number.
 */
class NumberCursor : public Cursor  {
private:
	static const int SIGNED = 0x01;
	static const int UNSIGNED = 0x00;
	word state;
	word index;
	word mode;
	bool symbol_accept;
	bool symbol_negative;
	word symbol_word;
public:
	// constructor
	NumberCursor() {
		this->mode = UNSIGNED; 
		reset();
	}
	NumberCursor(word mode) {
		this->mode = mode; 
		reset();
	}
	// destructor
	~NumberCursor() {

	}
	// reset
	void reset() {
		symbol_accept = false;
		symbol_negative = false;
		symbol_word = 0;
		state = (mode & SIGNED)==0 ? 1 : 0;
	}
	// valid state
	bool valid() { return state!=2; }
	// accepting state
	bool accept() { return state==1; }
	// accepting symbol
	int symbol() { return symbol_negative ? -symbol_word : symbol_word; }
	// apply next serial byte
	bool apply(byte b) {
		int digit = b - '0'; 
		switch(state) {
		case 0: // parse next byte, expect negative or digit
			if((digit>=0) && (digit<=9)) {
				symbol_word = symbol_word*10 + digit;
				symbol_accept = true;
				state = 1;
			} else if(b=='-') {
				symbol_negative = true;
				state = 1;
			} else if(b=='+') {
				symbol_negative = false;
				state = 1;
			} else {
				state = 2;
			}
		case 1: // parse next byte, expect digit only
			if((digit>=0) && (digit<=9)) {
				symbol_word = symbol_word*10 + digit;
				symbol_accept = true;
			} else {
				state = 2;
			}
			break;
		case 2: break; // error sink
		}
		return valid();
	}
};

/*
  PrefixCursor will accept bytes that index into a Prefix Tree. The stored symbol token for the tree is retured as the accept symbol.
 */
class PrefixCursor : public Cursor {
private:
	ReadPage * page;
	word symbols;
	word state;
	word index;
	word span_end;
	word symbol_word;
	bool symbol_accept;
	bool head_leaf;
	bool head_span;
	bool head_radix;
	word head_count;
public:
	// constructor
	PrefixCursor(ReadPage * memory) { 
		// create the memory page
		page = memory;
		// reset the cursor
		reset();
	}
	PrefixCursor(PROGMEM prog_uchar * prefix_tree) { 
		// create the memory page
		page = new NearProgramPage(prefix_tree);
		// reset the cursor
		reset();
	}

	// destructor
	~PrefixCursor() {
		// delete page;
		delete page;
	}
	// reset 
	void reset() {
		symbol_accept = false;
		symbol_word = 0;
		// read the tree header
		symbols = Cardinal::decode_word(page,0); 
		index =  Cardinal::decode_size(page,0);
		if(symbols==0) { // stop
			state = 99; 
		} else { // first node
			next_node();
		}
	}
	// are we in an accept state?
	bool accept() { return symbol_accept; }
	bool valid() { return state!=99; }
	// what is the current accept symbol?
	int symbol() { return symbol_word; }
	// apply next sequence byte
	bool apply(byte b) {
		switch(state) {
			case 1: next_span(b); break;
			case 2: next_prefix(b); break;
			case 99: break; // error sink
			case 100: state = 99; symbol_accept = false; break; // everything was perfect, but we just had to get another byte...
		}
		return valid();
	}

	void next_node() {
		// get the node header
		word node_head = Cardinal::decode_word(page,index);
		index += Cardinal::decode_size(page,index);
		// seperate out header properties
		head_span = ( (node_head & 0x04) != 0 );
		head_leaf = ( (node_head & 0x02) != 0 );
		head_radix = ( (node_head & 0x01) != 0 );
		head_count = node_head/8;
		// not in acceot state (yet)
		symbol_accept = false;
		// if the node has a span, then that's next
		if(head_span) {
			span_end = index + head_count;
			state = 1; // next span
		} else {
			// are we currently on a leaf node?
			if(head_leaf) {
				symbol_word = Cardinal::decode_word(page,index); 
				index +=  Cardinal::decode_size(page,index);
				symbol_accept = true;
			}
			// do we have a radix block
			if(head_radix) {
				state = 2; // parse that next
			} else {
				state = symbol_accept ? 100 : 99; // we have terminated. final stop. everyone off the train.
			}
		}
	}

	int next_span(byte b) {
		byte c = page->read_byte(index++);
		symbol_accept = false;
		if(b==c) {
			if(index<span_end) {
				// good so far, remain in this state
			} else {
				// if the node has a leaf, then get the symbol
				if(head_leaf) {
					symbol_word = Cardinal::decode_word(page,index); 
					index +=  Cardinal::decode_size(page,index);
					// stop on an accepted symbol
					state = 100;
					symbol_accept = true;
				} else {
					// stop on an error
					state = 99;
				}
				// if we have a prefix table, then that's next
				if(head_radix) {
					state = 2;
				}
			}
		} else {
			// span compare has failed.
			state = 99;
		}
	}

	int next_prefix(byte b) {
		symbol_accept = false;
		// work out our radix count
		byte radix;
		if(head_span) {
			// the prefix block requires its own radix byte
			radix = page->read_byte(index++);
		} else {
			// there was no span block, so we get to use the head count
			radix = head_count;
		}
		// look for the entry in the sorted prefix table
		int order = PrefixTree::prefix_select(page, radix, index, b);
		// skip over the table
		index += radix;
		if(order==-1) {
			// not found.
			state = 99;
		} else {
			// retrieve the catalog entry
			word token = page->read_word(index + order*2);
			// is this a symbol or index?
			if(token<symbols) {
				// we are in accept state with the symbol
				symbol_word = token;
				symbol_accept = true;
				state = 100;
			} else {
				// we need to skip to another node index
				index = token - symbols;
				next_node();
			}
		}
	}

};

#endif