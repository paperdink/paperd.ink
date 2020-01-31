/*
 * PCF8574 GPIO Port Expand
 * https://www.mischianti.org/2019/01/02/pcf8574-i2c-digital-i-o-expander-fast-easy-usage/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Renzo Mischianti www.mischianti.org All right reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "PCF8574.h"
#include "Wire.h"

/**
 * Constructor
 * @param address: i2c address
 */
PCF8574::PCF8574(uint8_t address){
	_wire = &Wire;

	_address = address;
};

/**
 * Construcor
 * @param address: i2c address
 * @param interruptPin: pin to set interrupt
 * @param interruptFunction: function to call when interrupt raised
 */
PCF8574::PCF8574(uint8_t address, uint8_t interruptPin,  void (*interruptFunction)() ){
	_wire = &Wire;

	_address = address;
	_interruptPin = interruptPin;
	_interruptFunction = interruptFunction;
	_usingInterrupt = true;
};

#if !defined(__AVR) && !defined(__STM32F1__) && !defined(TEENSYDUINO)
	/**
	 * Constructor
	 * @param address: i2c address
	 * @param sda: sda pin
	 * @param scl: scl pin
	 */
	PCF8574::PCF8574(uint8_t address, uint8_t sda, uint8_t scl){
		_wire = &Wire;

		_address = address;
		_sda = sda;
		_scl = scl;
	};

	/**
	 * Constructor
	 * @param address: i2c address
	 * @param sda: sda pin
	 * @param scl: scl pin
	 * @param interruptPin: pin to set interrupt
 	 * @param interruptFunction: function to call when interrupt raised
	 */
	PCF8574::PCF8574(uint8_t address, uint8_t sda, uint8_t scl, uint8_t interruptPin,  void (*interruptFunction)() ){
		_wire = &Wire;

		_address = address;
		_sda = sda;
		_scl = scl;

		_interruptPin = interruptPin;
		_interruptFunction = interruptFunction;

		_usingInterrupt = true;
	};
#endif

#ifdef ESP32
	/**
	 * Constructor
	 * @param address: i2c address
	 */
	PCF8574::PCF8574(TwoWire *pWire, uint8_t address){
		_wire = pWire;

		_address = address;
	};

	/**
	 * Construcor
	 * @param address: i2c address
	 * @param interruptPin: pin to set interrupt
	 * @param interruptFunction: function to call when interrupt raised
	 */
	PCF8574::PCF8574(TwoWire *pWire, uint8_t address, uint8_t interruptPin,  void (*interruptFunction)() ){
		_wire = pWire;

		_address = address;
		_interruptPin = interruptPin;
		_interruptFunction = interruptFunction;
		_usingInterrupt = true;
	};

	/**
	 * Constructor
	 * @param address: i2c address
	 * @param sda: sda pin
	 * @param scl: scl pin
	 */
	PCF8574::PCF8574(TwoWire *pWire, uint8_t address, uint8_t sda, uint8_t scl){
		_wire = pWire;

		_address = address;
		_sda = sda;
		_scl = scl;
	};

	/**
	 * Constructor
	 * @param address: i2c address
	 * @param sda: sda pin
	 * @param scl: scl pin
	 * @param interruptPin: pin to set interrupt
	 * @param interruptFunction: function to call when interrupt raised
	 */
	PCF8574::PCF8574(TwoWire *pWire, uint8_t address, uint8_t sda, uint8_t scl, uint8_t interruptPin,  void (*interruptFunction)() ){
		_wire = pWire;

		_address = address;
		_sda = sda;
		_scl = scl;

		_interruptPin = interruptPin;
		_interruptFunction = interruptFunction;

		_usingInterrupt = true;
	};
#endif

/**
 * wake up i2c controller
 */
void PCF8574::begin(){
	#if !defined(__AVR) && !defined(__STM32F1__) && !defined(TEENSYDUINO)
		_wire->begin(_sda, _scl);
	#else
	//			Default pin for AVR some problem on software emulation
	//			#define SCL_PIN _scl
	// 			#define SDA_PIN _sda
		_wire->begin();
	#endif

	// Check if there are pins to set low
	if (writeMode>0 || readMode>0){
		DEBUG_PRINTLN("Set write mode");
		_wire->beginTransmission(_address);
		DEBUG_PRINT(" ");
		DEBUG_PRINT("usedPin pin ");


		byte usedPin = writeMode | readMode;
		DEBUG_PRINTLN( ~usedPin, BIN);

		_wire->write(~usedPin);

		DEBUG_PRINTLN("Start end trasmission if stop here check pullup resistor.");
		_wire->endTransmission();
	}

	// If using interrupt set interrupt value to pin
	if (_usingInterrupt){
		DEBUG_PRINTLN("Using interrupt pin (not all pin is interrupted)");
		::pinMode(_interruptPin, INPUT_PULLUP);
		attachInterrupt(digitalPinToInterrupt(_interruptPin), (*_interruptFunction), FALLING );
	}

	// inizialize last read
	lastReadMillis = millis();
}

/**
 * Set if fin is OUTPUT or INPUT
 * @param pin: pin to set
 * @param mode: mode, supported only INPUT or OUTPUT (to semplify)
 */
void PCF8574::pinMode(uint8_t pin, uint8_t mode){
	DEBUG_PRINT("Set pin ");
	DEBUG_PRINT(pin);
	DEBUG_PRINT(" as ");
	DEBUG_PRINTLN(mode);

	if (mode == OUTPUT){
		writeMode = writeMode | bit(pin);
		readMode =  readMode & ~bit(pin);
		DEBUG_PRINT("writeMode: ");
		DEBUG_PRINT(writeMode, BIN);
		DEBUG_PRINT("readMode: ");
		DEBUG_PRINTLN(readMode, BIN);

	}else if (mode == INPUT){
		writeMode = writeMode & ~bit(pin);
		readMode =   readMode | bit(pin);
		DEBUG_PRINT("writeMode: ");
		DEBUG_PRINT(writeMode, BIN);
		DEBUG_PRINT("readMode: ");
		DEBUG_PRINTLN(readMode, BIN);
	}
	else{
		DEBUG_PRINTLN("Mode non supported by PCF8574")
	}
	DEBUG_PRINT("Write mode: ");
	DEBUG_PRINTLN(writeMode, BIN);

};

/**
 * Read value from i2c and bufferize it
 * @param force
 */
void PCF8574::readBuffer(bool force){
	if (millis() > PCF8574::lastReadMillis+READ_ELAPSED_TIME || _usingInterrupt || force){
		_wire->requestFrom(_address,(uint8_t)1);// Begin transmission to PCF8574 with the buttons
		lastReadMillis = millis();
		if(_wire->available())   // If bytes are available to be recieved
		{
			byte iInput = _wire->read();// Read a byte
			if ((iInput & readMode)>0){
				byteBuffered = byteBuffered | (byte)iInput;
			}
		}
	}
}

#ifndef PCF8574_LOW_MEMORY
	/**
	 * Read value of all INPUT pin
	 * Debounce read more fast than 10millis, non managed for interrupt mode
	 * @return
	 */
	PCF8574::DigitalInput PCF8574::digitalReadAll(void){
		DEBUG_PRINTLN("Read from buffer");
		_wire->requestFrom(_address,(uint8_t)1);// Begin transmission to PCF8574 with the buttons
		lastReadMillis = millis();
		if(_wire->available())   // If bytes are available to be recieved
		{
			  DEBUG_PRINTLN("Data ready");
			  byte iInput = _wire->read();// Read a byte

			  if ((iInput & readMode)>0){
				  DEBUG_PRINT("Input ");
				  DEBUG_PRINTLN((byte)iInput, BIN);

				  byteBuffered = byteBuffered | (byte)iInput;
				  DEBUG_PRINT("byteBuffered ");
				  DEBUG_PRINTLN(byteBuffered, BIN);
			  }
		}

		DEBUG_PRINT("Buffer value ");
		DEBUG_PRINTLN(byteBuffered, BIN);

		if ((bit(0) & readMode)>0) digitalInput.p0 = ((byteBuffered & bit(0))>0)?HIGH:LOW;
		if ((bit(1) & readMode)>0) digitalInput.p1 = ((byteBuffered & bit(1))>0)?HIGH:LOW;
		if ((bit(2) & readMode)>0) digitalInput.p2 = ((byteBuffered & bit(2))>0)?HIGH:LOW;
		if ((bit(3) & readMode)>0) digitalInput.p3 = ((byteBuffered & bit(3))>0)?HIGH:LOW;
		if ((bit(4) & readMode)>0) digitalInput.p4 = ((byteBuffered & bit(4))>0)?HIGH:LOW;
		if ((bit(5) & readMode)>0) digitalInput.p5 = ((byteBuffered & bit(5))>0)?HIGH:LOW;
		if ((bit(6) & readMode)>0) digitalInput.p6 = ((byteBuffered & bit(6))>0)?HIGH:LOW;
		if ((bit(7) & readMode)>0) digitalInput.p7 = ((byteBuffered & bit(7))>0)?HIGH:LOW;

		if ((readMode & byteBuffered)>0){
			byteBuffered = ~readMode & byteBuffered;
			DEBUG_PRINT("Buffer hight value readed set readed ");
			DEBUG_PRINTLN(byteBuffered, BIN);
		}
		DEBUG_PRINT("Return value ");
		return digitalInput;
	};
#else
	/**
	 * Read value of all INPUT pin in byte format for low memory usage
	 * Debounce read more fast than 10millis, non managed for interrupt mode
	 * @return
	 */
	byte PCF8574::digitalReadAll(void){
		DEBUG_PRINTLN("Read from buffer");
		_wire->requestFrom(_address,(uint8_t)1);// Begin transmission to PCF8574 with the buttons
		lastReadMillis = millis();
		if(_wire->available())   // If bytes are available to be recieved
		{
			  DEBUG_PRINTLN("Data ready");
			  byte iInput = _wire->read();// Read a byte

			  if ((iInput & readMode)>0){
				  DEBUG_PRINT("Input ");
				  DEBUG_PRINTLN((byte)iInput, BIN);

				  byteBuffered = byteBuffered | (byte)iInput;
				  DEBUG_PRINT("byteBuffered ");
				  DEBUG_PRINTLN(byteBuffered, BIN);
			  }
		}

		DEBUG_PRINT("Buffer value ");
		DEBUG_PRINTLN(byteBuffered, BIN);

		byte byteRead = byteBuffered;

		if ((readMode & byteBuffered)>0){
			byteBuffered = ~readMode & byteBuffered;
			DEBUG_PRINT("Buffer hight value readed set readed ");
			DEBUG_PRINTLN(byteBuffered, BIN);
		}
		DEBUG_PRINT("Return value ");
		return byteRead;
	};
#endif

/**
 * Read value of specified pin
 * Debounce read more fast than 10millis, non managed for interrupt mode
 * @param pin
 * @return
 */
uint8_t PCF8574::digitalRead(uint8_t pin){
	uint8_t value = LOW;
	DEBUG_PRINT("Read pin ");
	DEBUG_PRINTLN(pin);
	// Check if pin already HIGH than read and prevent reread of i2c
	if ((bit(pin) & byteBuffered)>0){
		DEBUG_PRINTLN("Pin already up");
		value = HIGH;
	 }else if ((/*(bit(pin) & byteBuffered)<=0 && */millis() > PCF8574::lastReadMillis+READ_ELAPSED_TIME) /*|| _usingInterrupt*/){
		 DEBUG_PRINTLN("Read from buffer");
		  _wire->requestFrom(_address,(uint8_t)1);// Begin transmission to PCF8574 with the buttons
		  lastReadMillis = millis();
		  if(_wire->available())   // If bytes are available to be recieved
		  {
			  DEBUG_PRINTLN("Data ready");
			  byte iInput = _wire->read();// Read a byte

			  if ((iInput & readMode)>0){
				  DEBUG_PRINT("Input ");
				  DEBUG_PRINTLN((byte)iInput, BIN);

				  byteBuffered = byteBuffered | (byte)iInput;
				  DEBUG_PRINT("byteBuffered ");
				  DEBUG_PRINTLN(byteBuffered, BIN);

				  if ((bit(pin) & byteBuffered)>0){
					  value = HIGH;
				  }
			  }
		  }
	}
	DEBUG_PRINT("Buffer value ");
	DEBUG_PRINTLN(byteBuffered, BIN);
	// If HIGH set to low to read buffer only one time
	if (value==HIGH){
		byteBuffered = ~bit(pin) & byteBuffered;
		DEBUG_PRINT("Buffer hight value readed set readed ");
		DEBUG_PRINTLN(byteBuffered, BIN);
	}
	DEBUG_PRINT("Return value ");
	DEBUG_PRINTLN(value);
	return value;
};

/**
 * Write on pin
 * @param pin
 * @param value
 */
void PCF8574::digitalWrite(uint8_t pin, uint8_t value){
	DEBUG_PRINTLN("Begin trasmission");
	_wire->beginTransmission(_address);     //Begin the transmission to PCF8574
	if (value==HIGH){
		writeByteBuffered = writeByteBuffered | bit(pin);
	}else{
		writeByteBuffered = writeByteBuffered & ~bit(pin);
	}
	DEBUG_PRINT("Write data ");
	DEBUG_PRINT(writeByteBuffered, BIN);
	DEBUG_PRINT(" for pin ");
	DEBUG_PRINT(pin);
	DEBUG_PRINT(" bin value ");
	DEBUG_PRINT(bit(pin), BIN);
	DEBUG_PRINT(" value ");
	DEBUG_PRINTLN(value);

	writeByteBuffered = writeByteBuffered & writeMode;
	_wire->write(writeByteBuffered);
	DEBUG_PRINTLN("Start end trasmission if stop here check pullup resistor.");

	_wire->endTransmission();
};


