#include <U8g2lib.h>

#include "WatchedSerial.h"
#include "devices/GCodeDevice.h"
#include "devices/GrblDevice.h"
#include "devices/DeviceDetector.h"

#include "ui/DRO.h"
#include "ui/GrblDRO.h"
#include "ui/DetectorScreen.h"
#include "ui/FileChooser.h"

#include "Job.h"

#include <SD.h>


constexpr uint32_t PIN_LCD_CS = PA2; // not connected
constexpr uint32_t PIN_LCD_RST = PB0;
constexpr uint32_t PIN_LCD_DC = PB1;
constexpr uint32_t PIN_LCD_CLK = PA0;
constexpr uint32_t PIN_LCD_MOSI = PA1;

U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI _u8g2(U8G2_R0, PIN_LCD_CLK, PIN_LCD_MOSI, 
    PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST);
U8G2 &Display::u8g2 = _u8g2;

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

constexpr uint32_t PIN_DET  =  PC13; ///< 0V=no USB on CNC, 1=CNC connected to USB.

WatchedSerial SerialCNC{Serial1, PIN_DET};

//GrblDevice dev{&SerialCNC, PIN_DET};
uint8_t devbuf[sizeof(GrblDevice)];
GrblDevice *dev;

Job *job;

Display display;
GrblDRO dro;
FileChooser fileChooser;

GrblDevice* createGrbl(WatchedSerial *s) {
    if(dev!=nullptr) return dev;
    dev = new(devbuf) GrblDevice(s);
    delay(1000);
    dev->begin();
    dev->add_observer(*Display::getDisplay());
    dev->add_observer(*job);
    
    dro.begin();
    dro.enableRefresh();
    display.setScreen(&dro);
    return dev;
}

using Detector = GrblDetector<WatchedSerial, SerialCNC, createGrbl >;

DetectorScreen<Detector> detUI;

void setSSD1306VcomDeselect(uint8_t v) {
   _u8g2.sendF("ca", 0xdb, v<<4);
}

void setup() {
    SerialUSB.begin(115200);
    //SerialCNC.begin(115200);

    _u8g2.begin();
    setSSD1306VcomDeselect(0);
    _u8g2.setContrast(0);
    _u8g2.setFontPosTop();
    _u8g2.setFontMode(1);
    _u8g2.setDrawColor(1);

    display.begin();

    display.setScreen(&detUI); 
    //display.setScreen(&fileChooser);
    
    fileChooser.setCallback( [&](bool res, String path){
        if(res) {
            LOGF("Starting job %s\n", path.c_str() );
            job->setFile(path);            
            job->start();
        } else {
            // cancel
        }
        Display::getDisplay()->setScreen(&dro);
    } );
    fileChooser.begin();



    job = &Job::getJob();
    job->add_observer( display );
    
    
    for(auto pin: buttPins) {
        pinMode(pin, INPUT_PULLUP);
    }

    Detector::begin();

    File cDir = SD.open("/");
    File file;
    while ( file = cDir.openNextFile() ) {
        LOGF("file %s\n", file.name() );
        file.close();
    }
    cDir.close();
    
}

void loop() {

    static uint32_t nextRead;

    if( int32_t(millis() - nextRead) > 0) {
        for(int i=0; i<N_BUTT; i++) {
            bitWrite(display.buttStates, i, (digitalRead(buttPins[i])==0 ? 1:0) );
        }
        display.processInput();
        nextRead = millis() + 10;
    }    

    display.loop();

    job->loop();

    if(dev!=nullptr) dev->loop();
    else Detector::loop();

    if(SerialUSB.available()) {
        while(SerialUSB.available()) {
            SerialCNC.write(SerialUSB.read());
        }
    }

}

