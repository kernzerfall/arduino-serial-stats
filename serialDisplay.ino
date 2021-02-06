#include<LiquidCrystal.h>

#define REDRAW_INTERVAL             600000
#define DISPLAY_UPDATE_INTERVAL     100
#define TEMP_UPDATE_INTERVAL        100
#define TEMP_AVERAGE_COUNT          500       
#define SERIAL_DATA_PACKET_LENGTH   6

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

byte serial[6]; // Data packet format = [hour, minute, day, month, cpu%, ram%]
float temperatureReading = 0.0f;
unsigned short temperatureN = 0;
unsigned long lastRedrawMillis = 0, lastDisplayUpdateMillis = 0, lastTemperatureReadMillis = 0, currentMillis = 0;


// Blanks num blocks starting at col, row and returns the cursor to col, row
void lBlank(uint8_t col, uint8_t row, uint8_t num){
    lcd.setCursor(col,row);
    if(col + num > 15) num = 15 - col; 
    for(uint8_t i = 0; i<num; ++i)
        lcd.write(" ");
    lcd.setCursor(col,row);
}

void drawBaseElements(){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("CPU");
    lcd.setCursor(8, 0);
    lcd.print("RAM");
}

void setup(){
    Serial.begin(9600);
    memset(&serial, 0, SERIAL_DATA_PACKET_LENGTH);
    delay(1000);
    lcd.begin(16,2);
    delay(1000);
    drawBaseElements();

    // Temperature sensor
    pinMode(A0,INPUT);
}

void loop(){
    // Redrau UI every REDRAW_INTERVAL secs
    currentMillis = millis();
    if(currentMillis - lastRedrawMillis > REDRAW_INTERVAL){
        lastRedrawMillis = currentMillis;
        drawBaseElements();
    }

    // Data packets should always be SERIAL_DATA_PACKET_LENGTH long
    if(Serial.available() == SERIAL_DATA_PACKET_LENGTH){
        Serial.readBytes(serial, SERIAL_DATA_PACKET_LENGTH);
    }

    if(currentMillis - lastDisplayUpdateMillis > DISPLAY_UPDATE_INTERVAL){
        // Reset millis since (l)ast (d)isplay (u)pdate
        lastDisplayUpdateMillis = currentMillis;

        // ==== HOURS ==== //
        uint8_t temp = (uint8_t)serial[0];
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
        temp = (uint8_t)serial[1];
        // If mins < 10, add preceding zero
        if(temp<10)
            lcd.print(0);
        lcd.print(temp);

        // ==== DAY ==== //
        temp = (uint8_t)serial[2];
        lcd.setCursor(6, 1);
        // If day < 10, add preceding zero
        if(temp<10)
            lcd.print("0");
        lcd.print(temp);
        lcd.print(".");

        // ==== MONTH ==== //
        // Cursor should already be after the separator
        temp = (uint8_t)serial[3];
        // If month < 10, add preceding zero
        if(temp<10)
            lcd.print("0");
        lcd.print(temp);

        // Blank CPU% Range
        lBlank(4, 0, 4);
        // Write CPU%
        lcd.print((uint8_t)serial[4]);
        lcd.print("%");

        // Blank RAM% Range
        lBlank(12,0,4);
        // Write RAM%
        lcd.print((uint8_t)serial[5]);
        lcd.print("%");
    }

    if(currentMillis - lastTemperatureReadMillis > TEMP_UPDATE_INTERVAL){
        lastTemperatureReadMillis = currentMillis;
        float temp = analogRead(A0);
        if ( temp > -10 && temp < 60 ) 
            temperatureReading += temp;
        else { // if we don't read a valid temp, report immediately that the sersor is kaputt
            // Blank TEMP Range
            lBlank(12,1,4);
            // Write Na
            lcd.print("N/A");
        }
        
        if (temperatureN++ >= TEMP_AVERAGE_COUNT){
            // Blank TEMP Range
            lBlank(12,1,4);
            float temp = temperatureReading * 0.437528f / temperatureN;
            // Round to 1st digit after floating point
            temp = floor(temp*10.0f + 0.5f) / 10.0f;
            // Print temp
            lcd.print(temp);
            // Reset Counter
            temperatureN = 0;
            // Reset temperatureReading
            temperatureReading = 0;
        }
    }
}