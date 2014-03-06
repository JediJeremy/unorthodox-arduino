
/*
  nano operating system for the Arduino (Leonardo)  (c) Jeremy Lee 2013
*/

#ifndef UNORTHODOX_DRIVERS_I2C_H
#define UNORTHODOX_DRIVERS_I2C_H

// is the Wire library included?
#ifdef TwoWire_h

/*
 *  
 */
class I2CDevice : public Device {
private:
public:
	// constructor
	// I2CDevice() { }

	bool write_packet(byte device, int address, int data) {
		Wire.beginTransmission(device);
		Wire.write(address);
		Wire.write(data);
		Wire.endTransmission();
		return true;
	}

	bool read_packet(byte device, byte address, byte * buffer, byte length) {
		// send phase
		Wire.beginTransmission(device);
		Wire.write(address);
		Wire.endTransmission();
		// recieve phase
		Wire.beginTransmission(device);
		Wire.requestFrom(device, (uint8_t)length);
		// how many did we get?
		byte r = Wire.available();
		if(r <= length) {
			for(byte i = 0; i < r; i++) { 
				buffer[i] = Wire.read(); 
				// Serial.print("."); Serial.print(buffer[i]);
			}
		} else {
			// consume unexpected bytes
			for(byte i = 0; i < length; i++) { Wire.read(); } //  { Serial.print("!"); Serial.print(Wire.read()); }
		}
		Wire.endTransmission();
		// if(r < length) { Serial.print(" packet too short! "); Serial.print(r); }
		// if(r > length) { Serial.print(" packet too long! "); Serial.print(r); }
		return (r==length);
	}

	//
};

/*
 * HMC5883L Compass/Magnetometer
 * 
 */
class HMC5883L : public I2CDevice {
private:
	static const byte I2C_HMC5883L = 0x1E; // 0x3C;
	// public properties
public:
	void start() {
		// configuration
		write_packet(I2C_HMC5883L, 00, 0b00011000); // 1xOversample, 75Hz,
		write_packet(I2C_HMC5883L, 01, 0b00100000); // Gain=0
		write_packet(I2C_HMC5883L, 02, 0b10000000); // high-speed i2c, continuous measurement
	}
	void stop() {
	}    

	bool set_scale(word scale) {
		return write_packet(I2C_HMC5883L, 01, (scale & 0x07) << 5);
	}
	bool set_mode(word mode) {
		return write_packet(I2C_HMC5883L, 02, mode);
	}
	bool get_vector(int * v) {
		byte buffer[6];
		if(read_packet(I2C_HMC5883L, 03, buffer, 6)) {
			// [todo: bit shuffling might not be necessary - try just dumping directly into the vector]
			v[0] = (buffer[0] << 8)| buffer[1];
			v[1] = (buffer[2] << 8)| buffer[3];
			v[2] = (buffer[4] << 8)| buffer[5];
			return true;
		} else {
			return false;
		}
	}
	bool get_ident() {
		byte buffer[3];
		// [todo: might not be necessary - try just dumping directly into the vector]
		return read_packet(I2C_HMC5883L, 10, buffer, 3) 
				&& (buffer[0]=='H') && (buffer[1]=='4') && (buffer[2]=='3');
	}
	//
};


/*
 * MPU6040 Accellerometer + Gyro
 * 
 */
class MPU6050 : public I2CDevice {
private:
	static const byte I2C_MPU6050 = 0x68; // i2c device address
	// public properties
public:
	void start() {
		// configuration
		write_packet(I2C_MPU6050, 0x19, 0x01); // Sample Rate
		write_packet(I2C_MPU6050, 0x6B, 0x03); // Z-Axis gyro reference used for improved stability (X or Y is fine too)
	}
	
	void stop() {
	}    

	bool get_vector(byte reg, int * v) {
		byte buffer[6];
		if(read_packet(I2C_MPU6050, reg, buffer, 6)) {
			// [todo: bit shuffling might not be necessary - try just dumping directly into the vector]
			v[0] = (buffer[0] << 8)| buffer[1];
			v[1] = (buffer[2] << 8)| buffer[3];
			v[2] = (buffer[4] << 8)| buffer[5];
			return true;
		} else {
			return false;
		}
	}
	
	bool gyro_vector(int * v) {
		return get_vector(0x43, v);
	}
	
	bool accel_vector(int * v) {
		return get_vector(0x3B, v);
	}
	
	bool temp_vector(int * v) {
		byte buffer[2];
		if(read_packet(I2C_MPU6050, 0x41, buffer, 2)) {
			// [todo: bit shuffling might not be necessary - try just dumping directly into the vector]
			v[0] = (buffer[0] << 8)| buffer[1];
			return true;
		} else {
			return false;
		}
	}
	bool get_ident() {
		byte buffer[3];
		// [todo: might not be necessary - try just dumping directly into the vector]
		return read_packet(I2C_MPU6050, 0x75, buffer, 1) && (buffer[0]==0x68);
	}
	//
};


#endif
#endif