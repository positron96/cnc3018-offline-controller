#include <U8g2lib.h>

#include "devices/GCodeDevice.h"

constexpr uint32_t PIN_LCD_CS = PA2; // not connected
constexpr uint32_t PIN_LCD_RST = PB0;
constexpr uint32_t PIN_LCD_DC = PB1;
constexpr uint32_t PIN_LCD_CLK = PA0;
constexpr uint32_t PIN_LCD_MOSI = PA1;

U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, PIN_LCD_CLK, PIN_LCD_MOSI, 
    PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST);


constexpr int N_BUTT = 8;
constexpr uint32_t PIN_BT_ZDOWN = PB8;
constexpr uint32_t PIN_BT_ZUP   = PB9;
constexpr uint32_t PIN_BT_R     = PB10;
constexpr uint32_t PIN_BT_L     = PB11;
constexpr uint32_t PIN_BT_CENTER= PB12;
constexpr uint32_t PIN_BT_UP    = PB13;
constexpr uint32_t PIN_BT_DOWN  = PB14;
constexpr uint32_t PIN_BT_STEP  = PB15;

constexpr uint32_t buttPins[N_BUTT] = {
    PIN_BT_ZDOWN, PIN_BT_ZUP, PIN_BT_R, PIN_BT_L, 
    PIN_BT_CENTER, PIN_BT_UP, PIN_BT_DOWN, PIN_BT_STEP
};
//bool buttStates[N_BUTT];
uint16_t buttStates = 0;


constexpr uint32_t PIN_DET  =  PC13; ///< 0V=no USB on CNC, 1=CNC connected to USB.

HardwareSerial &SerialCNC = Serial1;

GrblDevice dev{&SerialCNC};

void setup() {
    SerialUSB.begin(115200);
    SerialCNC.begin(115200);

    u8g2.begin();
    //u8g2.setBusClock(600000);
    u8g2.setFont(u8g2_font_8x13_tr);
    u8g2.setFontPosTop();
    u8g2.setFontMode(1);
    u8g2.setDrawColor(1);

    for(auto pin: buttPins) {
        pinMode(pin, INPUT_PULLUP);
    }
    
    pinMode(PIN_DET, INPUT);

    dev.begin();

}

constexpr int LCD_ROW1_HEIGHT = 16;


void onButton(uint32_t pin, bool down) {
    if(!down) return;
    switch(pin) {
        case PIN_BT_CENTER: SerialCNC.print("?\n"); break;
        case PIN_BT_STEP: dev.scheduleCommand("$X"); break;
        case PIN_BT_L: dev.jog(0, -1, 500); break;
        case PIN_BT_R: dev.jog(0, 1, 500); break;
        case PIN_BT_UP: dev.jog(1, -1, 500); break;
        case PIN_BT_DOWN: dev.jog(1, 1, 500); break;
    }
}

void loop() {

    static uint32_t nextRead;

    if( int32_t(millis() - nextRead) > 0) {
        uint16_t states = 0;
        for(int i=0; i<N_BUTT; i++) {
            bitWrite(states, i, (digitalRead(buttPins[i])==0 ? 1:0) );
        }
        uint16_t changed = buttStates ^ states;
        for(int i=0; i<N_BUTT; i++) {
            if(bitRead(changed, i) ) {
                onButton(buttPins[i], bitRead(states, i));
            }
        }
        buttStates = states;
        /*if(changed!=0)*/ nextRead = millis() + 10;
    }

    

    static uint32_t lastRedraw, lastFps;
    static int frames, lastFrames;
    if(millis()-lastRedraw > 100) {
        lastRedraw = millis();

        u8g2.clearBuffer();
        u8g2.setDrawColor(1);

        char str[100];
        snprintf(str, 100, "DET:%c", digitalRead(PIN_DET)==0 ? '0' : '1' );
        u8g2.drawStr(5, 0, str);

        //snprintf(str, 100, ");
        snprintf(str, 100, "bt:%d", buttStates);
        u8g2.drawStr(64, 0, str);

        u8g2.drawGlyph(120, 0, !dev.isConnected() ? '-' : dev.isInPanic() ? '!' : '+');

        snprintf(str, 100, "X:%3d", (int)dev.getX() );   u8g2.drawStr(1, LCD_ROW1_HEIGHT, str);
        snprintf(str, 100, "Y:%3d", (int)dev.getY() );   u8g2.drawStr(1, LCD_ROW1_HEIGHT+13, str);
        snprintf(str, 100, "Z:%3d", (int)dev.getZ() );   u8g2.drawStr(1, LCD_ROW1_HEIGHT+26, str);        

        u8g2.sendBuffer();
    }


    if(SerialUSB.available()) {
        while(SerialUSB.available()) {
            SerialCNC.write(SerialUSB.read());
        }
    }

    dev.loop();

}