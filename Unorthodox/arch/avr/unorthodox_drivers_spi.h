
/*
  nano operating system for the Arduino (Leonardo)  (c) Jeremy Lee 2013
*/

#ifndef UNORTHODOX_DRIVERS_SPI_H
#define UNORTHODOX_DRIVERS_SPI_H

// if the SPI library is inclided, then add some device drivers
#ifdef _SPI_H_INCLUDED


/*
MAX6957 Port expander with individual LED current drivers.
 */
class MAX6957 : public SPIDevice {
private:
	byte current_buffer[14];
public:
	// constructor
	MAX6957(int pin) {
		// set up the slave select line
		chip_select = pin;
	}

	// initialize the device
	void start() { 
		pinMode(chip_select, OUTPUT); digitalWrite(chip_select, HIGH);
		SPI.setBitOrder(MSBFIRST);
		SPI.setDataMode(SPI_MODE0);
		SPI.setClockDivider(SPI_CLOCK_DIV2);
		delayMicroseconds(10);
		send_reset();
	}

	void stop() { 
		pinMode(chip_select, INPUT); digitalWrite(chip_select, LOW);
	}

	void send_reset() {
		// send the startup block
		word block[16] = { 
				//0x0400, // shutdown while configuring
				0x020F, // global minimum current
				//0x0600, // interrupts off
				//0x0700, // display test off
				0x0955, 0x0A55, 0x0B00, 0x0C00, 0x0D00, 0x0E00, 0x0F00, // port configuration
				0x4400, 0x4C00, 0x5400,// lights off
				0x0441, // start normal operation
				// 0x0000 // NoOp to finish 
		};
		transfer_words(block,11);
	}

	void set_output(byte port, bool b) {
	}

	void set_global_current(byte current) {
		word block[] = { 0x0200 | (current & 0x0F) };
		transfer_words(block,1);
	}

	void set_current(byte port, byte current) {
		if(port<28) {
			int i =  port/2;
			byte b = current_buffer[i];
			if(port & 0x01) { // odd
				b = (b & 0x0F) | ((current & 0x0F)<<4);
			} else { // even
				b = (b & 0xF0) | ((current & 0x0F));
			}
			current_buffer[i] = b;
			// b = 42;
			// compose the command block
			word block[3];
			block[0] = ((0x12+i)<<8)+b;
			// word block = (a<<8)+42;
			if(current==0) {
				// turn port off entirely
				block[1] = ((0x24+port)<<8) + 0x00;
			} else {
				// turn port on
				block[1] = ((0x24+port)<<8) + 0x01;
			}
			block[2] = 0x0441; // reinstate normal operations to finish 
			transfer_words(block,3);
		}
	}

};


/*
ENC28J60 Ethernet Interface
*/
class ENC28J60 : public SPIDevice {
  public:
    // ENC28J60 Control Registers
    // Control register definitions are a combination of address,
    // bank number, and Ethernet/MAC/PHY indicator bits.
    // - Register address        (bits 0-4)
    // - Bank number        (bits 5-6)
    // - MAC/PHY indicator        (bit 7)
    static const byte ADDR_MASK    =    0x1F;
    static const byte BANK_MASK    =    0x60;
    static const byte SPRD_MASK    =    0x80;
    // All-bank registers
    static const byte EIE          =    0x1B;
    static const byte EIR          =    0x1C;
    static const byte ESTAT        =    0x1D;
    static const byte ECON2        =    0x1E;
    static const byte ECON1        =    0x1F;
    // Bank 0 registers
    static const byte ERDPT        =    (0x00|0x00);
    static const byte EWRPT        =    (0x02|0x00);
    static const byte ETXST        =    (0x04|0x00);
    static const byte ETXND        =    (0x06|0x00);
    static const byte ERXST        =    (0x08|0x00);
    static const byte ERXND        =    (0x0A|0x00);
    static const byte ERXRDPT      =    (0x0C|0x00);
    static const byte ERXWRPT      =    (0x0E|0x00);
    static const byte EDMAST       =    (0x10|0x00);
    static const byte EDMAND       =    (0x12|0x00);
    static const byte EDMADST      =    (0x14|0x00);
    static const byte EDMACS       =    (0x16|0x00);
    // Bank 1 registers
    static const byte EHT0         =    (0x00|0x20);
    static const byte EHT1         =    (0x01|0x20);
    static const byte EHT2         =    (0x02|0x20);
    static const byte EHT3         =    (0x03|0x20);
    static const byte EHT4         =    (0x04|0x20);
    static const byte EHT5         =    (0x05|0x20);
    static const byte EHT6         =    (0x06|0x20);
    static const byte EHT7         =    (0x07|0x20);
    static const byte EPMM0        =    (0x08|0x20);
    static const byte EPMM1        =    (0x09|0x20);
    static const byte EPMM2        =    (0x0A|0x20);
    static const byte EPMM3        =    (0x0B|0x20);
    static const byte EPMM4        =    (0x0C|0x20);
    static const byte EPMM5        =    (0x0D|0x20);
    static const byte EPMM6        =    (0x0E|0x20);
    static const byte EPMM7        =    (0x0F|0x20);
    static const byte EPMCS        =    (0x10|0x20);
    static const byte EPMO         =    (0x14|0x20);
    static const byte EWOLIE       =    (0x16|0x20);
    static const byte EWOLIR       =    (0x17|0x20);
    static const byte ERXFCON      =    (0x18|0x20);
    static const byte EPKTCNT      =    (0x19|0x20);
    // Bank 2 registers
    static const byte MACON1       =    (0x00|0x40|0x80);
    static const byte MACON2       =    (0x01|0x40|0x80);
    static const byte MACON3       =    (0x02|0x40|0x80);
    static const byte MACON4       =    (0x03|0x40|0x80);
    static const byte MABBIPG      =    (0x04|0x40|0x80);
    static const byte MAIPG        =    (0x06|0x40|0x80);
    static const byte MACLCON1     =    (0x08|0x40|0x80);
    static const byte MACLCON2     =    (0x09|0x40|0x80);
    static const byte MAMXFL       =    (0x0A|0x40|0x80);
    static const byte MAPHSUP      =    (0x0D|0x40|0x80);
    static const byte MICON        =    (0x11|0x40|0x80);
    static const byte MICMD        =    (0x12|0x40|0x80);
    static const byte MIREGADR     =    (0x14|0x40|0x80);
    static const byte MIWR         =    (0x16|0x40|0x80);
    static const byte MIRD         =    (0x18|0x40|0x80);
    // Bank 3 registers
    static const byte MAADR5       =    (0x00|0x60|0x80);
    static const byte MAADR6       =    (0x01|0x60|0x80);
    static const byte MAADR3       =    (0x02|0x60|0x80);
    static const byte MAADR4       =    (0x03|0x60|0x80);
    static const byte MAADR1       =    (0x04|0x60|0x80);
    static const byte MAADR2       =    (0x05|0x60|0x80);
    static const byte EBSTSD       =    (0x06|0x60);
    static const byte EBSTCON      =    (0x07|0x60);
    static const byte EBSTCS       =    (0x08|0x60);
    static const byte MISTAT       =    (0x0A|0x60|0x80);
    static const byte EREVID       =    (0x12|0x60);
    static const byte ECOCON       =    (0x15|0x60);
    static const byte EFLOCON      =    (0x17|0x60);
    static const byte EPAUS        =    (0x18|0x60);
    // ENC28J60 ERXFCON Register Bit Definitions
    static const byte ERXFCON_UCEN  =   0x80;
    static const byte ERXFCON_ANDOR =   0x40;
    static const byte ERXFCON_CRCEN =   0x20;
    static const byte ERXFCON_PMEN  =   0x10;
    static const byte ERXFCON_MPEN  =   0x08;
    static const byte ERXFCON_HTEN  =   0x04;
    static const byte ERXFCON_MCEN  =   0x02;
    static const byte ERXFCON_BCEN  =   0x01;
    // ENC28J60 EIE Register Bit Definitions
    static const byte EIE_INTIE     =   0x80;
    static const byte EIE_PKTIE     =   0x40;
    static const byte EIE_DMAIE     =   0x20;
    static const byte EIE_LINKIE    =   0x10;
    static const byte EIE_TXIE      =   0x08;
    static const byte EIE_WOLIE     =   0x04;
    static const byte EIE_TXERIE    =   0x02;
    static const byte EIE_RXERIE    =   0x01;
    // ENC28J60 EIR Register Bit Definitions
    static const byte EIR_PKTIF     =   0x40;
    static const byte EIR_DMAIF     =   0x20;
    static const byte EIR_LINKIF    =   0x10;
    static const byte EIR_TXIF      =   0x08;
    static const byte EIR_WOLIF     =   0x04;
    static const byte EIR_TXERIF    =   0x02;
    static const byte EIR_RXERIF    =   0x01;
    // ENC28J60 ESTAT Register Bit Definitions
    static const byte ESTAT_INT     =   0x80;
    static const byte ESTAT_LATECOL =   0x10;
    static const byte ESTAT_RXBUSY  =   0x04;
    static const byte ESTAT_TXABRT  =   0x02;
    static const byte ESTAT_CLKRDY  =   0x01;
    // ENC28J60 ECON2 Register Bit Definitions
    static const byte ECON2_AUTOINC =   0x80;
    static const byte ECON2_PKTDEC  =   0x40;
    static const byte ECON2_PWRSV   =   0x20;
    static const byte ECON2_VRPS    =   0x08;
    // ENC28J60 ECON1 Register Bit Definitions
    static const byte ECON1_TXRST   =   0x80;
    static const byte ECON1_RXRST   =   0x40;
    static const byte ECON1_DMAST   =   0x20;
    static const byte ECON1_CSUMEN  =   0x10;
    static const byte ECON1_TXRTS   =   0x08;
    static const byte ECON1_RXEN    =   0x04;
    static const byte ECON1_BSEL1   =   0x02;
    static const byte ECON1_BSEL0   =   0x01;
    // ENC28J60 MACON1 Register Bit Definitions
    static const byte MACON1_LOOPBK =   0x10;
    static const byte MACON1_TXPAUS =   0x08;
    static const byte MACON1_RXPAUS =   0x04;
    static const byte MACON1_PASSALL=   0x02;
    static const byte MACON1_MARXEN =   0x01;
    // ENC28J60 MACON2 Register Bit Definitions
    static const byte MACON2_MARST  =   0x80;
    static const byte MACON2_RNDRST =   0x40;
    static const byte MACON2_MARXRST=   0x08;
    static const byte MACON2_RFUNRST=   0x04;
    static const byte MACON2_MATXRST=   0x02;
    static const byte MACON2_TFUNRST=   0x01;
    // ENC28J60 MACON3 Register Bit Definitions
    static const byte MACON3_PADCFG2=   0x80;
    static const byte MACON3_PADCFG1=   0x40;
    static const byte MACON3_PADCFG0=   0x20;
    static const byte MACON3_TXCRCEN=   0x10;
    static const byte MACON3_PHDRLEN=   0x08;
    static const byte MACON3_HFRMLEN=   0x04;
    static const byte MACON3_FRMLNEN=   0x02;
    static const byte MACON3_FULDPX =   0x01;
    // ENC28J60 MACON4 Register Bit Definitions
    static const byte MACON4_PADCFG2=   0x80;
    static const byte MACON4_DEFER  =   0x40;
    static const byte MACON4_BPEN   =   0x20;
    static const byte MACON4_NOBKOFF=   0x10;
    // ENC28J60 MICMD Register Bit Definitions
    static const byte MICMD_MIISCAN =   0x02;
    static const byte MICMD_MIIRD   =   0x01;
    // ENC28J60 MISTAT Register Bit Definitions
    static const byte MISTAT_NVALID =   0x04;
    static const byte MISTAT_SCAN   =   0x02;
    static const byte MISTAT_BUSY   =   0x01;
    // ENC28J60 EBSTCON Register Bit Definitions
    static const byte EBSTCON_PSV2  =   0x80;
    static const byte EBSTCON_PSV1  =   0x40;
    static const byte EBSTCON_PSV0  =   0x20;
    static const byte EBSTCON_PSEL  =   0x10;
    static const byte EBSTCON_TMSEL1=   0x08;
    static const byte EBSTCON_TMSEL0=   0x04;
    static const byte EBSTCON_TME   =   0x02;
    static const byte EBSTCON_BISTST=   0x01;
    // PHY registers
    static const byte PHCON1        =   0x00;
    static const byte PHSTAT1       =   0x01;
    static const byte PHHID1        =   0x02;
    static const byte PHHID2        =   0x03;
    static const byte PHCON2        =   0x10;
    static const byte PHSTAT2       =   0x11;
    static const byte PHIE          =   0x12;
    static const byte PHIR          =   0x13;
    static const byte PHLCON        =   0x14;
    // ENC28J60 PHY PHCON1 Register Bit Definitions
    static const word PHCON1_PRST     =   0x8000;
    static const word PHCON1_PLOOPBK  =   0x4000;
    static const word PHCON1_PPWRSV   =   0x0800;
    static const word PHCON1_PDPXMD   =   0x0100;
    // ENC28J60 PHY PHSTAT1 Register Bit Definitions
    static const word PHSTAT1_PFDPX   =   0x1000;
    static const word PHSTAT1_PHDPX   =   0x0800;
    static const word PHSTAT1_LLSTAT  =   0x0004;
    static const word PHSTAT1_JBSTAT  =   0x0002;
    // ENC28J60 PHY PHCON2 Register Bit Definitions
    static const word PHCON2_FRCLINK  =   0x4000;
    static const word PHCON2_TXDIS    =   0x2000;
    static const word PHCON2_JABBER   =   0x0400;
    static const word PHCON2_HDLDIS   =   0x0100;
    // ENC28J60 Packet Control Byte Bit Definitions
    static const word PKTCTRL_PHUGEEN    =  0x08;
    static const word PKTCTRL_PPADEN     =  0x04;
    static const word PKTCTRL_PCRCEN     =  0x02;
    static const word PKTCTRL_POVERRIDE  =  0x01;
    // SPI operation codes
    static const byte OP_READ_REGISTER   =  0x00;
    static const byte OP_READ_BUFFER     =  0x3A;
    static const byte OP_WRITE_REGISTER  =  0x40;
    static const byte OP_WRITE_BUFFER    =  0x7A;
    static const byte OP_BITSET_REGISTER =  0x80;
    static const byte OP_BITCLR_REGISTER =  0xA0;
    static const byte OP_SOFT_RESET      =  0xFF;
    // The RXSTART_INIT must be zero. See Rev. B4 Silicon Errata point 5.
    // Buffer boundaries applied to internal 8K ram
    // the entire available packet buffer space is allocated
    static const word TXSTART_INIT       = 0x1800;  // TX buffer,
    static const word TXSTOP_INIT        = 0x1FFF;  //    6-8K                
    static const word RXSTART_INIT       = 0x0000;  // RX buffer
    static const word RXSTOP_INIT        = 0x17FF;  //    0-6K                       
    // max frame length which the conroller will accept:
    // (note: maximum ethernet frame length would be 1518)
    static const word MAX_FRAMELEN      = 1500;        

  protected:
    // 
    byte _current_bank;
    word _next_packet_ptr;  

  public:
    byte device_revision;
    byte mac_address[6];
  
    // constructor
    ENC28J60(int pin) {
      // set up the slave select line
      chip_select = pin;
      // no current bank
      _current_bank = 0xFF;
      device_revision = 0;
      // partially random MAC address - made from common 'local' OUI prefix and random NIC number
      mac_address[0] = 0x42; // the bits MUST be xxxxxx10 
      mac_address[1] = 0x44;
      mac_address[2] = 0x22;
      for(int i=3; i<6; i++) mac_address[i] = random(1,255);
    }
    
    // destructor
   ~ENC28J60() {
    }

    // initialize the device
    void start() { 
      pinMode(chip_select, OUTPUT); digitalWrite(chip_select, HIGH);
      SPI.setBitOrder(MSBFIRST);
      SPI.setDataMode(SPI_MODE0);
      SPI.setClockDivider(SPI_CLOCK_DIV64);
      delayMicroseconds(100);
      send_reset();
    }
    
    void stop() { 
      pinMode(chip_select, INPUT); digitalWrite(chip_select, LOW);
    }
    
    void sleep() {
      write_op(OP_BITCLR_REGISTER, ECON1, ECON1_RXEN);
      if(!wait_ready(ESTAT, ESTAT_RXBUSY, 10)) return;
      if(!wait_ready(ECON1, ECON1_TXRTS, 10)) return;
      write_op(OP_BITSET_REGISTER, ECON2, ECON2_VRPS);
      write_op(OP_BITSET_REGISTER, ECON2, ECON2_PWRSV);
    }
    
    void wake() {
      write_op(OP_BITCLR_REGISTER, ECON2, ECON2_PWRSV);
      if(!wait_ready(ESTAT, ESTAT_CLKRDY, 10)) return;
      write_op(OP_BITSET_REGISTER, ECON1, ECON1_RXEN);
    }
    
    bool link() {
      return (read_phy_byte(PHSTAT2) >> 2) & 1;
    }
    
    bool rx_waiting() {
      return read_reg_byte(EIR)&EIR_PKTIF;
    }
    
   void rx_accept(word ptr) {
      write_reg_word(ERXRDPT,ptr);
      write_op(OP_BITSET_REGISTER, ECON2, ECON2_PKTDEC); 
    }
    
    byte read_reg_byte(byte address) {
      switch_bank(address);
      return read_op(OP_READ_REGISTER, address);
    }
    
    word read_reg_word(byte address) {
      // return read_reg_byte(address) + (read_reg_byte(address+1) << 8);
      switch_bank(address);
      return read_op(OP_READ_REGISTER, address) | (read_op(OP_READ_REGISTER, address+1)<<8);
    }
    
    void write_reg_byte(byte address, byte data) {
      switch_bank(address);
      write_op(OP_WRITE_REGISTER, address, data);
    }
    
    void write_reg_word(byte address, word data) {
      // write_reg_byte(address, data);
      // write_reg_byte(address + 1, data >> 8);
      switch_bank(address);
      write_op(OP_WRITE_REGISTER, address, data & 0xFF);
      write_op(OP_WRITE_REGISTER, address+1, (data >> 8) & 0xFF);
    }
    
    word read_phy_byte(byte address) {
      write_reg_byte(MIREGADR, address);
      write_reg_byte(MICMD, MICMD_MIIRD);
      // while (read_reg_byte(MISTAT) & MISTAT_BUSY) { }
      if(!wait_ready(MISTAT, MISTAT_BUSY, 10)) return 0;
      write_reg_byte(MICMD, 0x00);
      return read_reg_byte(MIRD+1);
    }
    
    void write_phy_word(byte address, word data) {
        write_reg_byte(MIREGADR, address);
        write_reg_word(MIWR, data);
        // while (read_reg_byte(MISTAT) & MISTAT_BUSY) { }
        if(!wait_ready(MISTAT, MISTAT_BUSY, 10)) return;
    }
    
    void read_buffer(byte * data, word count) {
      digitalWrite(chip_select, HIGH); // should have been this way to start with...
      delayMicroseconds(10); // not sure how long since the last one, so wait a moment
      digitalWrite(chip_select, LOW); // select the chip, let it know data is coming.
      delayMicroseconds(4); // give it a moment to prepare
      SPI.transfer(OP_READ_BUFFER);
      // SPI.transfer(0);
      while(count-->0) { *data = SPI.transfer(0); data++; } // transfer the data
      digitalWrite(chip_select, HIGH); // and de-select the chip
    }

    void write_buffer(byte * data, word count) {
      digitalWrite(chip_select, HIGH); // should have been this way to start with...
      delayMicroseconds(10); // not sure how long since the last one, so wait a moment
      digitalWrite(chip_select, LOW); // select the chip, let it know data is coming.
      delayMicroseconds(4); // give it a moment to prepare
      SPI.transfer(OP_WRITE_BUFFER);
      while(count-->0) { SPI.transfer(*data); data++; } // transfer the data
      digitalWrite(chip_select, HIGH); // and de-select the chip
    }
    
    
    // Functions to enable/disable broadcast filter bits
    // With the bit set, broadcast packets are filtered.
    void enable_broadcast () {
      write_reg_byte(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN|ERXFCON_BCEN);
    }
    
    void disable_broadcast () {
      write_reg_byte(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
    }
    
    void disable_multicast () { // disable multicast filter , enable multicast reception
      write_reg_byte(ERXFCON, ERXFCON_CRCEN);
    }
    
  private:
    bool wait_ready(byte reg, byte mask, word time) {
      unsigned long timeout = millis()+time;
      while (!read_op(OP_READ_REGISTER, reg) & mask) { if(millis()>timeout) return false; }
      return true;
    }
    
    void send_reset() {
      write_op(OP_SOFT_RESET, 0, OP_SOFT_RESET);
      delayMicroseconds(2000);
      if(!wait_ready(ESTAT, ESTAT_CLKRDY, 10)) return;
      // clear the buffer memory (for debug reasons)
      write_reg_word(EWRPT, 0);   // Buffer Serial Write Pointer
      byte zeroes[32]; for(int i=0; i<32; i++) zeroes[i]=0;
      for(int i=0; i<256; i++) { write_buffer(zeroes,32); }
      // _next_packet_ptr = RXSTART_INIT;
      write_reg_word(ERDPT, RXSTART_INIT);   // Buffer Serial Read Pointer
      write_reg_word(EWRPT, TXSTART_INIT);   // Buffer Serial Write Pointer
      write_reg_word(ETXST, TXSTART_INIT);   // TX Buffer Start
      write_reg_word(ETXND, TXSTOP_INIT);    // TX Buffer Stop
      write_reg_word(ERXST, RXSTART_INIT);   // RX Buffer Start
      write_reg_word(ERXND, RXSTOP_INIT);    // RX Buffer Stop
      write_reg_word(ERXRDPT, RXSTART_INIT);  // RX Read (Lock) Pointer
      // write_reg_word(ERXWRPT, RXSTART_INIT); // RX Write Pointer - should be reset to RXSTART_INIT
      enable_broadcast(); // change to add ERXFCON_BCEN recommended by epam
      write_reg_word(EPMM0, 0x303f);
      write_reg_word(EPMCS, 0xf7f9);
      write_reg_byte(MACON1, MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS);
      write_reg_byte(MACON2, 0x00);
      write_op(OP_BITSET_REGISTER, MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN | MACON3_FULDPX); 
      write_reg_word(MAIPG, 0x0C12);
      write_reg_byte(MABBIPG, 0x12);
      write_reg_word(MAMXFL, MAX_FRAMELEN);  
      write_reg_byte(MAADR1, mac_address[0]);
      write_reg_byte(MAADR2, mac_address[1]);
      write_reg_byte(MAADR3, mac_address[2]);
      write_reg_byte(MAADR4, mac_address[3]);
      write_reg_byte(MAADR5, mac_address[4]);
      write_reg_byte(MAADR6, mac_address[5]);
      write_phy_word(PHCON1, PHCON1_PDPXMD);
      write_phy_word(PHCON2, PHCON2_HDLDIS);
      // write_phy_word(PHLCON, 0b0011 0100 0010 0010 ); // default 
      write_phy_word(PHLCON, 0b0011111011010010 );
      switch_bank(ECON1);
      write_op(OP_BITSET_REGISTER, EIE, EIE_INTIE | EIE_PKTIE);
      write_op(OP_BITSET_REGISTER, ECON1, ECON1_RXEN);
      // get the device revision, which should be 0x06.
      device_revision = read_reg_byte(EREVID);
    }
    
        
    // 
    byte read_op(byte op, byte address) {
        byte buffer[] = { op|(address & ADDR_MASK), 0, 0 };
        byte n = (address & 0x80)?3:2; // is it an extended address?
        transfer_bytes( buffer, n );
        return buffer[n-1];
    }
    /*
    byte read_eth(byte op, byte address) {
        byte buffer[] = { op|(address & ADDR_MASK), 0, };
        transfer_bytes( buffer, 2 );
        return buffer[1];
    }
    
    byte read_mac(byte op, byte address) {
        byte buffer[] = { op|(address & ADDR_MASK), 0, 0 };
        transfer_bytes( buffer, 3 );
        return buffer[2];
    }
    */
    
    void write_op(byte op, byte address, byte data) {
      byte buffer[] = { op|(address & ADDR_MASK), data };
      transfer_bytes( buffer,2 );
    }
    

    void switch_bank(byte address) {
      if ((address & BANK_MASK) != _current_bank) {
        write_op(OP_BITCLR_REGISTER, ECON1, ECON1_BSEL1|ECON1_BSEL0);
        _current_bank = address & BANK_MASK;
        write_op(OP_BITSET_REGISTER, ECON1, _current_bank>>5);
      }
    }
    

   
};



#endif
#endif