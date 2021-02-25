#include <Arduino.h>
#include <LiquidCrystal.h>
#include "./include/SerialConstant.hpp"
#include "./include/Datatypes.hpp"

#define REDRAW_INTERVAL             600000
#define TEMP_UPDATE_INTERVAL        100
#define TEMP_AVERAGE_COUNT          100       
#define TEMP_SENSOR_CALIB_VALUE     0.437528f

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

class StateKeeper {
	public:
		byte buf[SerialConstant::Data::SIZE_SERIALBUF] = {0x00};
		byte state          = SerialConstant::State::IDLE;
		byte next           = SerialConstant::State::IDLE;
		byte bIndex         = 0x00;

		struct Timekeep_S {
			u64 redraw     = 0, 
				tempRead   = 0, 
				curr       = 0;
		} timekeep;

		struct Temperature_S {
			f32 reading    = 0.0f;
			u16 divisor    = 0;
			cf32 adjust    = TEMP_SENSOR_CALIB_VALUE;
		} temp;

		void resetBuf(){
			for(u16 i = 0; i < SerialConstant::Data::SIZE_SERIALBUF; i++) this->buf[i] = 0x00;
			this->bIndex = 0x00;
			this->state  = SerialConstant::State::IDLE;
		}
} s;

// Resets the device
void (*resetDevice)(void) = 0;

// Blanks num blocks starting at col, row and returns the cursor to col, row
void lBlank(byte col, byte row, byte num){
	lcd.setCursor(col,row);
	if(col + num > 15) num = 15 - col; 
	for(byte i = 0; i<num; ++i)
		lcd.write(" ");
	lcd.setCursor(col,row);
}

// Draws some basic elements
void drawBaseElements(){
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("C");
	lcd.setCursor(6, 0);
	lcd.print("R");
}

// Date&Time handler for serial data
// Needs s.buf[0] = HOURS
//            [1] = MINS
//            [2] = DAY
//            [3] = MONTH
void handleDateTime(){
		// ==== HOURS ==== //
		byte temp = s.buf[0];
		// Blank time display
		lBlank(0,1,5);
		// If hours < 10, add preceding zero
		if(temp<10)
			lcd.print(0);
		lcd.print(temp);
		// Add separator
		lcd.print(":");

		// ==== MINS ==== //
		// Cursor should already be after the separator
		temp = s.buf[1];
		// If mins < 10, add preceding zero
		if(temp<10)
			lcd.print(0);
		lcd.print(temp);

		// ==== DAY ==== //
		temp = s.buf[2];
		lcd.setCursor(6, 1);
		// If day < 10, add preceding zero
		if(temp<10)
			lcd.print("0");
		lcd.print(temp);
		lcd.print(".");

		// ==== MONTH ==== //
		// Cursor should already be after the separator
		temp = s.buf[3];
		// If month < 10, add preceding zero
		if(temp<10)
			lcd.print("0");
		lcd.print(temp);
		
}

// CPU Utilization handler for serial data
// Needs s.buf[0] = CPU%
void handleCPUtil(){
		// Blank CPU% Range
		lBlank(2, 0, 4);
		// Write CPU%
		lcd.print(s.buf[0]);
		lcd.print("%");
}

// RAM Utilization handler for serial data
// Needs s.buf[0] = RAM%
void handleRAMUtil(){
		// Blank RAM% Range
		lBlank(8,0,4);
		// Write RAM%
		lcd.print(s.buf[0]);
		lcd.print("%");  
}


void setup(){
	Serial.begin(9600);
	delay(1000);
	lcd.begin(16,2);
	delay(1000);
	drawBaseElements();

	// Temperature sensor
	pinMode(A0,INPUT);
}

void loop(){
	// Redrau UI every REDRAW_INTERVAL secs
	s.timekeep.curr = millis();
	if(s.timekeep.curr - s.timekeep.redraw > REDRAW_INTERVAL){
		s.timekeep.redraw = s.timekeep.curr;
		drawBaseElements();
	}

	if(Serial.available() > 0){
		// If at DATA_START, don't read next byte / if done, 1 byte gets skipped
		// If next is HALT, preserve it for the switch
		if(s.next != SerialConstant::Flag::DATA_START && s.next != SerialConstant::State::HALT) 
			s.next = Serial.read();

		switch(s.next){
			case SerialConstant::Flag::DATA_START:
				// Don't mess up the state if next byte isn't available yet
				if(Serial.available() > 0)
					s.next = s.state = Serial.read();
				break;;

			case SerialConstant::Flag::DATA_END:
				// If END of DataPacket found, find the appropriate handler for the data
				switch(s.state){ 
					case SerialConstant::Data::TYPE_DATETIME: 
						handleDateTime(); break;;
					case SerialConstant::Data::TYPE_CPUTIL:
						handleCPUtil(); break;;
					case SerialConstant::Data::TYPE_RAMUTIL:
						handleRAMUtil(); break;;
				}
				s.resetBuf();
				break;;

			case SerialConstant::State::HALT:
				// Reset the device via software (send HALT byte)
				resetDevice();
				break;;

			default: {
				// If not at IDLE push back the next serial byte in the array
				if(s.state != SerialConstant::State::IDLE){
				// Overflow detection
					if(s.bIndex > SerialConstant::Data::SIZE_SERIALBUF - 1 ){
						s.bIndex = 0;
						// Reset if overflown
						s.next = SerialConstant::State::HALT;
					}
					s.buf[s.bIndex++] = s.next;
				}
			}
		}
	}

	if(s.timekeep.curr - s.timekeep.tempRead > TEMP_UPDATE_INTERVAL){
		s.timekeep.tempRead = s.timekeep.curr;
		f32 temp = analogRead(A0);
		if ( temp > 0 && temp < 140 ) 
			s.temp.reading += temp;
		else { 
			// if we don't read a valid temp, report immediately that the sersor is kaputt
			// Blank TEMP Range
			lBlank(12,1,4);
			// Write Na
			lcd.print("N/A");
		}
		
		if (s.temp.divisor++ >= TEMP_AVERAGE_COUNT){
			// Blank TEMP Range
			lBlank(12,1,4);
			f32 temp = s.temp.reading * s.temp.adjust / s.temp.divisor;
			// Round to 1st digit after floating point
			temp = floor(temp*10.0f + 0.5f) / 10.0f;
			// Print temp
			lcd.print(temp);
			// Reset Counter
			s.temp.divisor = 0;
			// Reset temperatureReading
			s.temp.reading = 0;
		}
	}
}