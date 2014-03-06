
/*
  nano operating system for the Arduino (Leonardo)  (c) Jeremy Lee 2013
 */

#ifndef UNORTHODOX_PAGE_H
#define UNORTHODOX_PAGE_H

/*
    The abstract interface that all memory pages have.
	Some memory pages are writable as well.
 */
class Page {
public:
	// unified methods
	virtual void read(word index, void * v, int count) = 0;
	virtual void write(word index, void * v, int count) = 0;
	virtual Page * clone(int offset) = 0;
	// old methods
	byte read_byte(word index) { byte b; read(index, &b, 1); return b; }
	word read_word(word index) { word w; read(index, &w, 2);  return w; }
	void write_byte(word index, byte b) { write(index, &b, 1); }
	void write_word(word index, word w) { write(index, &w, 2); }
	// new methods
	void copy(word index, Page * source, word sindex, word scount) {
		byte buffer[16];
		word remain = scount;
		while(remain) {
			// next block
			word c = min(remain,16);
			source->read(sindex, buffer, c);
			write(index, buffer, c);
			// consume pointers
			index +=c; sindex +=c; remain -=c;
		}
	}
};


class ReadPage : public Page {
public:
	void write(word index, void * v, int count) { };
};


class MemoryPage : public Page {
private:
	byte * base;
public:
	MemoryPage(void * ptr) { base = (byte *)ptr; }
	void read(word index, void * v, int count) { 
		byte * m = base + index;
		for(int i=0; i<count; i++ ) ((byte *)v)[i] = m[i];
	};
	void write(word index, void * v, int count) { 
		byte * m = base + index;
		for(int i=0; i<count; i++ ) m[i] = ((byte *)v)[i];
	};
	Page * clone(int offset) { return new MemoryPage(base+offset); }
};


class NearProgramPage : public ReadPage {
private:
	prog_uchar * base;
public:
	NearProgramPage(prog_uchar * ptr) { base = ptr; }
	void read(word index, void * v, int count) { 
		prog_uchar * m = base + index;
		for(int i=0; i<count; i++ ) ((byte *)v)[i] = pgm_read_byte_near(m+i);
	};
	// void write(int index, void * v, int count) { };
	Page * clone(int offset) { return new NearProgramPage(base+offset); }
};

#if defined (__AVR_ATmega32U4__) // ATmega32U4 (Teensy/Leonardo).

class FarProgramPage : public ReadPage {
private:
	uint_farptr_t base;
public:
	FarProgramPage(uint_farptr_t ptr) { base = ptr; }
	void read(word index, void * v, int count) { 
		uint_farptr_t m = base + index;
		for(int i=0; i<count; i++ ) ((byte *)v)[i] = pgm_read_byte_far(m+i);
	};
	// void write(int index, void * v, int count) { };
	Page * clone(int offset) { return new FarProgramPage(base+offset); }
};

#endif

#ifdef EEPROM_h

class EEPROMPage : public Page {
private:
	word base;
public:
	EEPROMPage(word index) { this->base = index; }
	void read(word index, void * v, int count) { 
		word m = base + index;
		for(int i=0; i<count; i++ ) ((byte *)v)[i] = EEPROM.read(m+i);
	};
	void write(word index, void * v, int count) { 
		word m = base + index;
		for(int i=0; i<count; i++ ) EEPROM.write(m+i, ((byte *)v)[i]);
	};
	// void write(int index, void * v, int count) { };
	Page * clone(int offset) { return new EEPROMPage(base+offset); }
};

#endif

/*
	Because blocks of zero are so common and are so compressible, it has it's own special case.
	
	Any read from this page will return a zero.
 */
class ZeroPage : public ReadPage {
public:
	ZeroPage() { }
	void read(word index, void * v, int count) { 
		for(int i=0; i<count; i++ ) ((byte *)v)[i] = 0;
	};
	// void write(int index, void * v, int count) { };
	Page * clone(int offset) { return new ZeroPage(); }
};

/*
	This page is backed by storage for a single byte. It is mostly used to pad buffers with non-zero
	repeating values, but has other uses.
	
	Because words are two bytes long, word-level accesses return the equivalent double-byte value.
 */
class BytePage : public ReadPage {
	byte _byte;
public:
	BytePage(byte value) { _byte = value; }
	void read(word index, void * v, int count) { 
		for(int i=0; i<count; i++ ) ((byte *)v)[i] = _byte;
	};
	// void write(int index, void * v, int count) { };
	Page * clone(int offset) { return new BytePage(_byte); }
};


/*
	This page is backed by storage for a single word. It is mostly used to pad buffers with non-zero
	repeating values, but has other uses.
	
	Because words are two bytes long,  index alignment still matters. Byte-level accesses will still 
	return lower or upper halves, in that order.
 */
class WordPage : public ReadPage {
private:
public:
	word value;
	WordPage(word v) { value = v; }
	void read(word index, void * v, int count) { 
		for(int i=0; i<count; i++) ((byte *)v)[i] = ((byte *)&value)[(index+i)&1];
	};
	// void write(int index, void * v, int count) { };
	Page * clone(int offset) { return new WordPage(value); }
};

/*
  Cardinals are positive integers (including zero) that can be efficiently encoded as a series of bytes.
  They are similar to UTF-* in that small 'codes' (such as the base ASCII character set) map to single
  bytes, but will also expand to accomodate larger numbers with 32-bits, or more. 

  Many stream-based algorithms can use this to take advantage of the general observation that we use a lot more
  small numbers than big ones. It is a trivial form of compression that simply removes as many leading zeroes as
  it logically can, and still use a bytestream.

  Bitstream-based algorithms can be more space-efficient, but at a serious time cost, so we stick with bytes.
*/

class Cardinal {
public:
	static int decode_size(ReadPage * page, word index) {
		byte b0 = page->read_byte(index);
		// check if single-byte cardinal
		if( (b0 & 0x80)==0 ) return 1; 
		// check if double-byte cardinal
		if( (b0 & 0x40)==0 ) return 2; 
		// check if triple-byte cardinal
		if( (b0 & 0x20)==0 ) return 3; 
		// check if quad-byte cardinal
		if( (b0 & 0x10)==0 ) return 4; 
		// too large for us
		return 0;
	}

	static word decode_word(ReadPage * page, word index) {
		byte b0 = page->read_byte(index);
		// check if single-byte cardinal
		if( (b0 & 0x80)==0 ) return b0; 
		// check if double-byte cardinal
		if( (b0 & 0x40)==0 ) return ((b0|0x7F)<<8) + page->read_byte(index+1); 
		// check if triple-byte cardinal
		if( (b0 & 0x20)==0 ) return page->read_word(index+1); 
		// too large
		return 0;
	}

	static unsigned long decode_long(ReadPage * page, word index) {
		byte b0 = page->read_byte(index);
		// check if single-byte cardinal
		if( (b0 & 0x80)==0 ) return b0; 
		// check if double-byte cardinal
		if( (b0 & 0x40)==0 ) return ((b0|0x3F)<<8) + page->read_byte(index+1); 
		// check if triple-byte cardinal
		if( (b0 & 0x20)==0 ) return ((b0|0x1F)<<16) + page->read_word(index+1); 
		// check if quad-byte cardinal
		if( (b0 & 0x10)==0 ) return ((b0|0x0F)<<24) + (page->read_byte(index+1)<<16) + page->read_word(index+2); 
		// too large
		return 0;
	}
};

/*
	A journaled filesystem has one important property - there are no special blocks in the storage
	such as catalogs, indexes, or block pointers. The storage is written to sequentially and cyclically;
	each byte in the storage is written at most once per pass. Once the journal reaches the end of
	the storage, it loops back around.
	
	This is the ideal pattern for memory which has a limited number of write cycles, such as EEPROM
	(which can survive millions of writes) or program flash. (which can take thousands of overwrites)
	Write cycles are distributed evenly over the storage. There are no hot spots that will burn out 
	first.
	
	This properly holds even between system restarts. An initial 'scan' of the storage returns
	the filesystem to the same state. (This scan mostly reads metadata, not block payload, but
	might take extended time for large volumes with thousands of entries.)
	
	The write order is arranged such that any interruption leaves an incomplete block, but does not
	corrupt the filesystem. Corrupt filesystems are detected on startup, and tend to result in the
	storage being reinitialized.
	
	The base JournalFS object does not know or care what is being stored in the blocks... so long
	as the descendant class implements a function which replies to 'block loads' and 'relevance queries',
	via block_state() the journal can perform all needed tasks. 
	
	The journal is constantly defragmenting itself as new blocks are written, and so it need to occasionally
	ask whether the block is still needed. Child classes can determine this any way they want, creating great
	flexibilty.
	
	For example, a 'syslog' FS could be implemented on top of the base journal which randomly throws old 
	entries away (literally, responds to the 'keep' request with a dice roll) meaning the storage will end up 
	containing a rough selection of recent log entries, with older entries being rarer.
	
	The TokenFS class is a little more sensible, and treats each storage block as a replacement entry for
	a token table. Since later blocks will naturally replace earlier blocks, this 'journal of table updates'
	naturally assembles into an indexed table of entries. This table is checked during the 'keep' requests,
	so obsolete blocks naturally get discarded.
	
	Internally, the journal maintains a 'head' and 'tail' and may use terminology different from what you
	expect. The 'head' is the oldest record in storage, and the 'tail' is the newest. This naming is contrary
	to a lot of memory storage algorithms, (where the 'head' is the latest) but makes more sense when talking
	about files, journals, or logs. There is also a 'next' pointer with the next free byte after the tail,
	but this isn't necessarily where the next block will actually go (especially if it's too big).
	
	When asked for block refereces, the journal will return a pointer to the first data byte in the block payload.
	(not a pointer to the beginning of the physical block) and this is a deliberate choice.
	This has three advantages: user code does not need to adjust this pointer before use, the block size byte is 
	still easily acessible by reading the byte at index -1, and requests for block indexes will only return
	zero if they don't exist. (no 'real' block will ever return a zero pointer)
	
 */
class JournalFS {
protected:
public:
	Page * page; // virtual storage accessor
	word size;   // storage size
	bool empty;  // is the filesystem entirely empty?
	word head;   // where is the storage head block
	word tail;   // where is the storage tail block
	word next;   // where is the next free byte (should be immediately after the tail)
	unsigned long updates; // rolling count of how many blocks were written to the filesystem in this session
public:
	// constructor
	JournalFS(Page * page, word size) {
		this->page = page; 
		this->size = size;
	}
	
	// initialize the filesystem
	void start() {
		head = 0;
		tail = 0;
		next = 0;
		updates = 0;
		// first pass : scan from the beginning of storage, which should inevitably lead to the current tail.
		i_p = 0;
		i_more = true;
		i_scan = true;
		i_front = true;
		i_corrupt = false;
		empty = true;
		while(i_more) {
			pass_next();
			if(i_valid) empty = false;
		}
		// second pass: load all the entires in journal order from the head to tail.
		pass_reset();
		while(i_more) {
			pass_next();
			if(i_valid) block_state(i_block, i_size, 1);
		}
		// all done. 
	}
	
	/*
	  Try to append a block to the journal, while defragmenting as needed.
	  Fails if not enough space is available.
	  
	  Yes this is a stupidly long function, but it needs to be factored like this to
	  prevent recusion when defragging. This keeps it iterative, at the expense of
	  readability.
	 */
	bool journal_append(Page * source, word count) {
		Page * write_page;
		word   write_count;
		word   defrag_tail = tail; // don't defrag past the current tail
		bool   defrag = (head!=tail); // need more than one block to consider a defrag pass
		bool   retry = true;
		while(retry) {
			//Serial.print("\n try "); Serial.print(next);
			write_page = 0; 
			if(defrag) {
				// discard obsolete head blocks until at least one has been recycled
				// (or we totally defrag the fs without finding one, in which case we should stop)
				word after;
				// take a look at the head block...
				word hc = page->read_byte(head);
				byte e[2]; page->read(head+hc+3,e,2);
				byte x = e[0] ^ e[1];
				if(x==0xFF) {
					// another block follows neatly afterwards in the chain
					after = head + hc + 5;
				} else if(x==0) {
					// that was the final block in the chain, but we were clearly not the tail. 
					after = 0; // so the next block must loop to the start of storage
				} else {
					// corrupt block. let's not make it worse...
					// retry = false;
					return false;
				} 
				// can we recycle the block? (empty and obsolete blocks are discarded)
				if((hc!=0) && block_state(head+1,hc,0)) {
					// this block should now dissapear from any indexes it was in
					block_state(head+1,hc,2);
					// recycle the (still relevant) block around to the end of the journal.
					Page * sub = page->clone(head+1);
					head = after;
					write_page = sub;
					write_count = hc;
					if(head < next) { 
						// if there's not space at the end...
						if( (next + hc + 5) > size ) {
							// there must logically be space at the beginning (since we are already the first)
							next = 0;
						}
					} else {
						// the block will be copied backwards between tail and head
						// (we may just be copying the block to itself if we are completely out of space
						// but later blocks might be discardable)
					}
					// we can stop defragmenting for the moment
					defrag = false;
				} else {
					// this block should now dissapear from any indexes it was in
					block_state(head+1,hc,2);
					// effectively delete the obsolete journal entry by not recycling it
					head = after;
					// we sucessfully discarded a block, and might be able to do it again
					defrag = (head!=defrag_tail);
				}
			} else {
				// assume we will be successful
				retry = false;
				write_count = count;
				// can we find a space big enough to store the block?
				// where are the head and post-tail in relation to each other?
				if(empty) {
					// first entry! does it fit within the completely empty storage?
					if( ( count + 5 ) < size ) {
						write_page = source;
					}
				} else if(head <= tail) {
					// do we have enough room after the tail?
					if( (next + count + 5) <= size ) {
						// no worries, it will fit right in at the current next
						write_page = source; 
					} else if( (count + 5) <= head ) {
						// there's enough room to fit it at the start, so loop back around
						next = 0; // this means the first storage entry will be the tail.
						write_page = source; 
					} else {
						// not currently enough room at either end. more defragmentation is our only hope.
						defrag = true;
					}
				} else {
					// the tail has wrapped around. do we have enough room between them?
					if( (head - next) >= (count + 5) ) {
						// no worries.
						write_page = source;
					} else {
						// not enough room. defragmentation is our only hope.
						defrag = true;
					}
				}
				if(defrag) {
					// try to continue defragmenting. if we cannot then we fail.
					// retry = (head!=defrag_tail);
					if(head==defrag_tail) return false;
					retry = true;
				}
			}
			// do we need to write a new block on this pass?
			if(write_page) {
				// Serial.print(" write "); Serial.print(next);
				updates++;
				// write our block size
				word p = next;
				page->write_byte(p++, write_count);
				// copy our block data from source
				for(word i=0; i<write_count; i++) { page->write_byte(p++, write_page->read_byte(i)); }
				// page->copy(p, source, 0,write_count);
				// write the latest head pointer
				page->write_word(p, head);
				// mark our stutter byte as the current end (copy the next byte verbatim, but avoid writing it)
				page->write_byte(p+2, page->read_byte(p+3) );
				// page->copy(p+2, page, p+3,1);
				// were we the very first?
				if(empty) {
					// we are now the head (and tail)
					head = next;
					empty = false;
				// } else if(next!=0) {
				} else if(tail < next) { // hmmm....
					// update the old tail stutter byte (which marks us into the chain. checkpoint!)
					byte tc = page->read_byte(tail);
					byte te = page->read_byte(tail+tc+3);
					page->write_byte(tail+tc+4, ~te);
				}
				// notify the fs of the new block
				block_state(next+1, write_count, 1);
				// we are now the tail
				tail = next;
				next = tail + write_count + 5;
				// if we recycled a block, then free the page
				if(write_page!=source) delete write_page;
			} 
			// finished the loop
			//Serial.print("\n head:"); Serial.print(head); Serial.print(" tail:"); Serial.print(tail); Serial.print(" next:"); Serial.print(next);
		}
		return true;
	}

	// block iterator properties
	word i_block;
	word i_size;
	byte i_meta[4];
	bool i_valid;
	bool i_more;
	bool i_front;
	bool i_corrupt;
	bool i_scan;
	word i_p;

	// block iterator reset
	void pass_reset() {
		i_p = head;
		i_more = !empty;
		i_scan = false;
		i_front = (tail < head);
		// i_corrupt = false;
	}

	// block iterator
	void pass_next() {
		if(i_more) {
			i_valid = false; 
			i_more = false; 
			// read block metrics
			byte bc = page->read_byte(i_p); // get the block count 
			word bs = bc + 5; // (the block size is five bytes larger, of course)
			word pn = i_p + bs; // so where will the next block start?
			// store the block metrics
			i_block = i_p+1; 
			i_size = bc; 
			// read block metadata
			page->read(i_p+bc+1, i_meta,4);
			// extract the latest head pointer
			word block_head = *((word *)i_meta);
			// Serial.print("\n iterate "); Serial.print(i_p); Serial.print(" "); Serial.print(block_head); 
			// if bigger than the storage, then we clearly have a corrupt block.
			if(block_head<size) {
				// are the stutter bytes equal, or inverse?
				byte xb = i_meta[2] ^ i_meta[3];
				if(xb==0xFF) {
					// we have a following block in the chain
					i_p = pn; 
					i_valid = true; 
					i_more = true;
				} else if(xb==0) {
					// this is the final block in the chain
					i_valid = true;
					if(i_scan) {
						// since we were scanning, our job is done
						tail = i_p; // we've found the latest tail, 
						head = block_head; // which means we also know the latest head pointer.			
						next = tail + page->read_byte(tail) + 5; // and the first free byte pointer
					} else {
						// do we skip back to the front chain?
						if(i_front) {
							// loop around to the beginning of storage for the next one
							i_p = 0;
							i_front = false; 
							i_more = true;
						}
					}
				} else {
					// block corruption
					i_corrupt = true; 
				}
			} else {
				// the pointer was invalid, indicating the block is bad
				i_corrupt = true;
			}
		}

	}

	/*
  	  overload this method to answer the filesystem's request if the block is still relevant and to be notified of new blocks
	 */
	virtual int block_state(word index, word count, int mode) = 0;

};

class TokenFS : public JournalFS {
public:
	word * token;
	word tokens;
	word used;
	// constructor
	TokenFS(Page * page, word size, word tokens) : JournalFS(page,size) {
		this->tokens = tokens;
		// create our pointer storage
		token = new word[tokens];
		memset(token,0,tokens<<1);
		used = 0;
	}

	~TokenFS() {
		delete token;
	}

	/*
	 * respond to block notifications and validity requests
	 */
	int block_state(word index, word count, int mode) {
		// get the token id of the block
		byte id = page->read_byte(index);
		if(id<tokens) {
			// Serial.print("\n token "); Serial.print(id); 
			word current = token[id];
			if(mode==0) {
				// block verification
				return (current == index) && (count>1);
			} else if(mode==1) {
				// block notification
				if(current) { used -= token_size(id) + 6; } // un-account for the old block
				if(count>1) {
					token[id] = index; // before we replace it
					used += token_size(id) + 6; // and then account for the new
				} else {
					token[id] = 0; // before we discard it because it's empty
				}
			} else if(mode==2) {
				// block reclaim
				if(current==index) {
					// the current block was reclaimed
					used -= token_size(id) + 6;
					token[id] = 0;
				}
			}
			// Serial.print("\n used "); Serial.print(used); 
		}
	}

	/*
	 * Create a new Page accessor
	 */
	Page * token_page(word index) {
		// do we have a pointer for this index?
		word i = token[index];
		if(i==0) return 0;
		return page->clone(i + 1);
	}

	word token_size(word index) {
		// do we have a pointer for this index?
		word i = token[index];
		if(i==0) return 0;
		// the storage immediately prior to the token index is actually the block size byte
		// so read that and subtract one.
		return page->read_byte(i - 1) - 1;
	}

	bool token_write(word index, Page * source, int count) {
		// discard old entry before defragmentation
		if(token[index]) {
			used -= token_size(index) + 6;
			token[index] = 0;
		} else {
			// don't bother if the token is empty (apart from the id header)
			if(count==1) return true;
		}
		// write new token page to the journal. we will be notified if successful.
		return journal_append(source, count);
	}

};

	 
#endif
